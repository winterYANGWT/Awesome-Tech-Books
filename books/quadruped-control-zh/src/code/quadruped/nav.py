"""2D perception and navigation for the quadruped.

This module is the Chapter 11 stack.  It is deliberately split into small,
independently testable pieces:

``Lidar2D``
    A horizontal MuJoCo ray-cast sensor.  It is given a *pose estimate* and a
    yaw angle, never the simulator's ground truth.
``GridMap``
    Log-odds occupancy grid with free-space carving, obstacle inflation and a
    traversability mask.
``AStarPlanner``
    Eight-connected A* with octile heuristic, no corner cutting, line-of-sight
    shortcutting and a short smoothing pass.
``PurePursuit``
    Geometric path tracker returning a body-frame ``(vx, wz)`` command.
``NavStack``
    Periodic scan -> map -> plan -> track loop that writes commands into a
    :class:`~quadruped.trotting.TrottingController`.

Nothing here copies code from the reference book or from ROS navigation.  The
algorithms are written from their standard definitions; the MuJoCo ray API and
the coordinate conventions are documented in the code.

Control-period budget
---------------------
``NavStack.step`` is called from the 2 ms control loop.  Only the pure-pursuit
update and a command write happen every cycle.  Ray casting runs at
``scan_hz`` (default 10 Hz), and A* runs at ``plan_hz`` (default 2 Hz).  The
map update uses integer Bresenham traversal, so it allocates very little per
cycle.
"""

from __future__ import annotations

import heapq
import math
import time
from dataclasses import dataclass, field
from typing import Iterable, Sequence

import numpy as np

try:  # MuJoCo is an optional dependency for the pure-grid unit tests.
    import mujoco
except ImportError:  # pragma: no cover - exercised only without MuJoCo
    mujoco = None


# ---------------------------------------------------------------------------
# Small helpers
# ---------------------------------------------------------------------------
def wrap_angle(angle: float) -> float:
    """Wrap an angle to ``[-pi, pi)``."""
    return (float(angle) + math.pi) % (2.0 * math.pi) - math.pi


def _bresenham(x0: int, y0: int, x1: int, y1: int) -> list[tuple[int, int]]:
    """Integer line from ``(x0, y0)`` to ``(x1, y1)``, both included."""
    cells: list[tuple[int, int]] = []
    dx = abs(x1 - x0)
    dy = abs(y1 - y0)
    sx = 1 if x0 < x1 else -1
    sy = 1 if y0 < y1 else -1
    err = dx - dy
    x, y = x0, y0
    while True:
        cells.append((x, y))
        if x == x1 and y == y1:
            return cells
        e2 = 2 * err
        if e2 > -dy:
            err -= dy
            x += sx
        if e2 < dx:
            err += dx
            y += sy


# ---------------------------------------------------------------------------
# Lidar
# ---------------------------------------------------------------------------
class Lidar2D:
    """Horizontal multi-ray lidar backed by :func:`mujoco.mj_multiRay`.

    Parameters
    ----------
    model, data:
        MuJoCo model and data.  A :class:`~quadruped.sim.A1Sim` may be passed
        as the first argument instead; its ``model`` and ``data`` are used.
    n_beams, angle_min, angle_max:
        Scan geometry.  The default is 360 beams covering one revolution at
        1 degree spacing.  ``angle_max`` is exclusive.
    max_range, range_min:
        Returns outside this interval are reported as ``inf`` (no return).
    noise_std:
        Optional Gaussian range noise (metres).  Zero by default, matching an
        ideal simulator; the acceptance script uses a small value to exercise
        the mapping code.
    mount:
        Lidar origin in the body frame.  It is rotated by the yaw estimate.
    geomgroup:
        MuJoCo geom-group mask.  The default includes groups 0/1/4/5 and
        excludes groups 2/3, which the A1 model uses for visual and collision
        geoms.  This prevents self-hits without needing a per-ray body filter.
    """

    def __init__(self, model, data=None, n_beams: int = 360,
                 angle_min: float = -math.pi, angle_max: float = math.pi,
                 max_range: float = 6.0, range_min: float = 0.05,
                 noise_std: float = 0.0,
                 mount: Sequence[float] = (0.0, 0.0, 0.08),
                 geomgroup: Sequence[int] | None = (1, 1, 0, 0, 1, 1),
                 bodyexclude: int = -1, seed: int = 0) -> None:
        if data is None:
            if not hasattr(model, "model") or not hasattr(model, "data"):
                raise TypeError("pass either (model, data) or an A1Sim-like object")
            data = model.data
            model = model.model
        if mujoco is None:  # pragma: no cover
            raise ImportError("Lidar2D requires the 'mujoco' package")
        self.model = model
        self.data = data
        self.n_beams = int(n_beams)
        self.angle_min = float(angle_min)
        self.angle_max = float(angle_max)
        self.max_range = float(max_range)
        self.range_min = float(range_min)
        self.noise_std = float(noise_std)
        self.mount = np.asarray(mount, dtype=float).reshape(3)
        self.geomgroup = (None if geomgroup is None else
                          np.asarray(geomgroup, dtype=np.uint8).reshape(6))
        self.bodyexclude = int(bodyexclude)
        self.angles = (self.angle_min
                       + np.arange(self.n_beams, dtype=float)
                       * (self.angle_max - self.angle_min) / self.n_beams)
        self._rng = np.random.default_rng(seed)
        self.last_origin = np.zeros(3)
        self.last_directions = np.zeros((self.n_beams, 3))
        self.last_hit_points = np.zeros((self.n_beams, 3))
        self.last_geom_ids = np.full(self.n_beams, -1, dtype=int)

    # -- pose -----------------------------------------------------------
    @staticmethod
    def yaw_rotation(yaw: float) -> np.ndarray:
        c, s = math.cos(yaw), math.sin(yaw)
        return np.array([[c, -s, 0.0], [s, c, 0.0], [0.0, 0.0, 1.0]])

    def scan(self, base_position: Sequence[float], yaw: float) -> np.ndarray:
        """Cast one scan from ``base_position`` (estimated) and ``yaw`` (IMU).

        Returns a ``(n_beams,)`` float array of ranges.  ``inf`` marks a beam
        with no return inside ``max_range``.
        """
        base_position = np.asarray(base_position, dtype=float).reshape(3)
        Rz = self.yaw_rotation(float(yaw))
        origin = base_position + Rz @ self.mount
        c = np.cos(self.angles + float(yaw))
        s = np.sin(self.angles + float(yaw))
        directions = np.stack([c, s, np.zeros_like(c)], axis=1)
        vec = np.ascontiguousarray(directions.reshape(-1), dtype=np.float64)
        geomid = np.full(self.n_beams, -1, dtype=np.int32)
        dist = np.full(self.n_beams, -1.0, dtype=np.float64)
        mujoco.mj_multiRay(
            self.model, self.data, np.ascontiguousarray(origin),
            vec, self.geomgroup, True, self.bodyexclude,
            geomid, dist, None, self.n_beams, self.max_range)

        ranges = np.asarray(dist, dtype=float).copy()
        hit = ranges >= 0.0
        ranges[~hit] = np.inf
        if self.noise_std > 0.0:
            noisy = ranges.copy()
            good = np.isfinite(noisy)
            noisy[good] += self._rng.normal(0.0, self.noise_std, good.sum())
            ranges = noisy
        ranges[(ranges < self.range_min) | (ranges > self.max_range)] = np.inf
        self.last_origin = origin
        self.last_directions = directions
        self.last_geom_ids = geomid
        hit_points = origin[None, :] + np.where(
            np.isfinite(ranges), ranges, 0.0)[:, None] * directions
        self.last_hit_points = hit_points
        return ranges


# ---------------------------------------------------------------------------
# Occupancy grid
# ---------------------------------------------------------------------------
class GridMap:
    """Log-odds 2D occupancy grid.

    The array layout is ``grid[iy, ix]``; ``ix=0`` is the left edge
    ``x_min``.  Cell centres are at
    ``(x_min + (ix + 0.5) * resolution, y_min + (iy + 0.5) * resolution)``.

    Occupied cells are inflated by ``robot_half_width + safety_margin`` before
    planning.  Unknown cells are treated as traversable by default (the
    chapter's lidar sees the goal area from the start, but a real costmap also
    has to move into unknown space before it can clear it).
    """

    def __init__(self, x_min: float = -1.0, y_min: float = -3.0,
                 x_max: float = 5.0, y_max: float = 3.0,
                 resolution: float = 0.05,
                 log_odds_occupied: float = 0.85,
                 log_odds_free: float = -0.40,
                 log_odds_min: float = -3.5, log_odds_max: float = 3.5,
                 occupied_threshold: float = 0.60,
                 free_threshold: float = 0.40,
                 robot_half_width: float = 0.16,
                 safety_margin: float = 0.06,
                 allow_unknown: bool = True) -> None:
        if resolution <= 0.0:
            raise ValueError("resolution must be positive")
        if x_max <= x_min or y_max <= y_min:
            raise ValueError("x_max/y_max must be larger than x_min/y_min")
        self.resolution = float(resolution)
        self.x_min = float(x_min)
        self.y_min = float(y_min)
        self.x_max = float(x_max)
        self.y_max = float(y_max)
        self.width = int(math.ceil((self.x_max - self.x_min) / self.resolution))
        self.height = int(math.ceil((self.y_max - self.y_min) / self.resolution))
        self.log_odds_occupied = float(log_odds_occupied)
        self.log_odds_free = float(log_odds_free)
        self.log_odds_min = float(log_odds_min)
        self.log_odds_max = float(log_odds_max)
        self.occupied_threshold = float(occupied_threshold)
        self.free_threshold = float(free_threshold)
        self.robot_half_width = float(robot_half_width)
        self.safety_margin = float(safety_margin)
        self.allow_unknown = bool(allow_unknown)
        self.log_odds = np.zeros((self.height, self.width), dtype=np.float32)
        self.observed = np.zeros((self.height, self.width), dtype=bool)
        self._inflated: np.ndarray | None = None
        self._version = 0
        self.updates = 0

    @classmethod
    def from_occupancy(cls, occupancy: np.ndarray, resolution: float,
                       origin: Sequence[float] = (0.0, 0.0), *,
                       occupied_log_odds: float = 2.2,
                       free_log_odds: float = -2.2,
                       **kwargs) -> "GridMap":
        """Build a fully known map from a surveyed occupancy array.

        Parameters
        ----------
        occupancy:
            ``(height, width)`` array with the same ``[iy, ix]`` layout as
            :attr:`log_odds`.  Boolean arrays are used directly; numeric
            arrays are thresholded at ``0.5`` (so a probability or 0/254
            costmap works too).
        resolution:
            Cell size in metres.
        origin:
            World coordinates of the lower-left corner of cell ``(0, 0)``,
            i.e. ``(x_min, y_min)``.  Cell centres follow the usual
            ``x_min + (ix + 0.5) * resolution`` convention.
        occupied_log_odds, free_log_odds:
            Log-odds assigned to occupied and free cells.  The defaults give
            probabilities 0.90 and 0.10, both firmly past the thresholds.
        **kwargs:
            Forwarded to :class:`GridMap` (``robot_half_width``,
            ``safety_margin``, thresholds, ...).

        Every cell is marked as observed, and the inflation layer is built
        immediately so the result is ready for :class:`AStarPlanner`.  This
        is the public alternative to writing ``log_odds``/``observed`` and
        clearing ``_inflated`` by hand.
        """
        occ = np.asarray(occupancy)
        if occ.ndim != 2:
            raise ValueError("occupancy must be a 2D (height, width) array")
        if occ.shape[0] == 0 or occ.shape[1] == 0:
            raise ValueError("occupancy must not be empty")
        if resolution <= 0.0:
            raise ValueError("resolution must be positive")
        occupied = (occ if occ.dtype == bool else occ >= 0.5)
        origin = np.asarray(origin, dtype=float).reshape(2)
        height, width = occupied.shape
        grid = cls(x_min=float(origin[0]), y_min=float(origin[1]),
                   x_max=float(origin[0]) + width * float(resolution),
                   y_max=float(origin[1]) + height * float(resolution),
                   resolution=float(resolution), **kwargs)
        grid.log_odds[:, :] = np.where(
            occupied, float(occupied_log_odds), float(free_log_odds)
        ).astype(np.float32)
        grid.observed[:, :] = True
        grid._inflated = None
        grid._version += 1
        grid.inflated_obstacles()   # complete the inflation step eagerly
        return grid

    # -- geometry -------------------------------------------------------
    @property
    def shape(self) -> tuple[int, int]:
        return self.height, self.width

    @property
    def inflation_radius(self) -> float:
        return self.robot_half_width + self.safety_margin

    def reset(self) -> None:
        self.log_odds.fill(0.0)
        self.observed.fill(False)
        self._inflated = None
        self._version += 1
        self.updates = 0

    def contains(self, x: float, y: float) -> bool:
        return (self.x_min <= x < self.x_max
                and self.y_min <= y < self.y_max)

    def world_to_cell(self, x: float, y: float) -> tuple[int, int]:
        ix = int(math.floor((float(x) - self.x_min) / self.resolution))
        iy = int(math.floor((float(y) - self.y_min) / self.resolution))
        return ix, iy

    def cell_to_world(self, ix: int, iy: int) -> tuple[float, float]:
        return (self.x_min + (float(ix) + 0.5) * self.resolution,
                self.y_min + (float(iy) + 0.5) * self.resolution)

    def cell_center(self, cell: tuple[int, int]) -> np.ndarray:
        return np.array(self.cell_to_world(*cell), dtype=float)

    def _in_bounds(self, ix: int, iy: int) -> bool:
        return 0 <= ix < self.width and 0 <= iy < self.height

    def clip_cell(self, ix: int, iy: int) -> tuple[int, int]:
        return (min(max(int(ix), 0), self.width - 1),
                min(max(int(iy), 0), self.height - 1))

    # -- probabilities --------------------------------------------------
    @property
    def probability(self) -> np.ndarray:
        """Occupancy probability of every cell (unknown = 0.5)."""
        return 1.0 / (1.0 + np.exp(-self.log_odds.astype(float)))

    @property
    def occupied(self) -> np.ndarray:
        return self.probability >= self.occupied_threshold

    @property
    def free(self) -> np.ndarray:
        p = self.probability
        return self.observed & (p <= self.free_threshold)

    @property
    def known(self) -> np.ndarray:
        return self.observed

    def _add(self, ix: int, iy: int, delta: float) -> None:
        if self._in_bounds(ix, iy):
            value = float(self.log_odds[iy, ix]) + delta
            self.log_odds[iy, ix] = min(self.log_odds_max,
                                        max(self.log_odds_min, value))
            self.observed[iy, ix] = True
            self._inflated = None

    # -- ray update -----------------------------------------------------
    def update_ray(self, origin: Sequence[float], angle: float,
                   distance: float, hit: bool = True,
                   free_stride: int = 1) -> None:
        """Apply one beam's free/occupied evidence to the grid.

        ``distance`` is the measured range.  When ``hit`` is false the beam is
        traced to that distance and all cells, including the last, are marked
        free; this is how a no-return beam clears space.  When ``hit`` is true
        the last cell is marked occupied and the others free.
        """
        ox, oy = float(origin[0]), float(origin[1])
        r = float(distance)
        if not np.isfinite(r) or r <= 0.0:
            return
        ex = ox + r * math.cos(float(angle))
        ey = oy + r * math.sin(float(angle))
        c0 = self.world_to_cell(ox, oy)
        c1 = self.world_to_cell(ex, ey)
        if not self._in_bounds(*c0):
            return
        # Clip the far end to the grid along the ray direction.  If the
        # measured endpoint is outside the map, the in-map part is still
        # useful as free-space evidence, but the boundary cell must not be
        # promoted to an obstacle: the hit happened outside the map.
        endpoint_outside = not self._in_bounds(*c1)
        if endpoint_outside:
            c1 = self._clip_to_grid(c0, origin=(ox, oy), end=(ex, ey))
        cells = _bresenham(c0[0], c0[1], c1[0], c1[1])
        stride = max(1, int(free_stride))
        if hit and not endpoint_outside:
            for cell in cells[:-1:stride]:
                self._add(cell[0], cell[1], self.log_odds_free)
            if len(cells) > 1 and (len(cells) - 2) % stride:
                cell = cells[-2]
                self._add(cell[0], cell[1], self.log_odds_free)
            if cells:
                self._add(cells[-1][0], cells[-1][1], self.log_odds_occupied)
        else:
            for cell in cells[::stride]:
                self._add(cell[0], cell[1], self.log_odds_free)
            if (len(cells) - 1) % stride:
                cell = cells[-1]
                self._add(cell[0], cell[1], self.log_odds_free)
        self.updates += 1
        self._version += 1

    def _clip_to_grid(self, c0: tuple[int, int],
                      origin: tuple[float, float] | None = None,
                      end: tuple[float, float] | None = None) -> tuple[int, int]:
        """Return the last grid cell on the segment to the map boundary."""
        if origin is None or end is None:  # pragma: no cover - legacy call
            origin = self.cell_center(c0)
            end = origin
        ex, ey = end
        x0, y0 = origin
        c1 = self.world_to_cell(ex, ey)
        if self._in_bounds(*c1):
            return c1
        # Compute intersections with the four borders and keep the nearest
        # point on the segment that lies inside the grid.
        dx, dy = ex - x0, ey - y0
        candidates: list[float] = []
        eps = 1e-9
        if abs(dx) > eps:
            for xb in (self.x_min + eps, self.x_max - eps):
                t = (xb - x0) / dx
                y = y0 + t * dy
                if 0.0 <= t <= 1.0 and self.y_min - eps <= y <= self.y_max + eps:
                    candidates.append(t)
        if abs(dy) > eps:
            for yb in (self.y_min + eps, self.y_max - eps):
                t = (yb - y0) / dy
                x = x0 + t * dx
                if 0.0 <= t <= 1.0 and self.x_min - eps <= x <= self.x_max + eps:
                    candidates.append(t)
        t = min(candidates) if candidates else 0.0
        return self.clip_cell(*self.world_to_cell(x0 + t * dx, y0 + t * dy))

    def update_scan(self, ranges: Sequence[float], origin: Sequence[float],
                    yaw: float, angles: Sequence[float] | None = None,
                    max_range: float | None = None,
                    free_stride: int = 1,
                    vectorized: bool = True) -> int:
        """Integrate a complete scan; return the number of usable beams.

        The default implementation samples every ray at ``free_stride`` grid
        cells and applies all free/occupied evidence in one vectorised update.
        This keeps a 360-beam map update below a millisecond on the reference
        machine.  ``vectorized=False`` falls back to the exact per-ray
        Bresenham traversal (used by tests and small maps).
        """
        ranges = np.asarray(ranges, dtype=float).reshape(-1)
        if angles is None:
            angles = np.linspace(-math.pi, math.pi, ranges.size, endpoint=False)
        angles = np.asarray(angles, dtype=float).reshape(-1)
        if max_range is None:
            finite = ranges[np.isfinite(ranges)]
            max_range = 1.2 * float(finite.max()) if finite.size else 6.0
        origin = np.asarray(origin, dtype=float).reshape(3)
        stride = max(1, int(free_stride))
        if not vectorized:
            used = 0
            for r, a in zip(ranges, angles):
                if np.isfinite(r):
                    self.update_ray(origin, yaw + a, float(r), hit=True,
                                    free_stride=stride)
                    used += 1
                else:
                    self.update_ray(origin, yaw + a, float(max_range),
                                    hit=False, free_stride=stride)
            return used

        dist = np.where(np.isfinite(ranges),
                        np.clip(ranges, 0.0, float(max_range)),
                        float(max_range))
        step = self.resolution * stride
        ts = np.arange(0.0, float(max_range) + 0.5 * step, step)
        c = np.cos(float(yaw) + angles)
        s = np.sin(float(yaw) + angles)
        xs = float(origin[0]) + c[:, None] * ts[None, :]
        ys = float(origin[1]) + s[:, None] * ts[None, :]
        mask = ((ts[None, :] < dist[:, None])
                & (xs >= self.x_min) & (xs < self.x_max)
                & (ys >= self.y_min) & (ys < self.y_max))
        ix = np.floor((xs - self.x_min) / self.resolution).astype(np.int64)
        iy = np.floor((ys - self.y_min) / self.resolution).astype(np.int64)
        if np.any(mask):
            np.add.at(self.log_odds, (iy[mask], ix[mask]),
                      self.log_odds_free)
            self.observed[iy[mask], ix[mask]] = True

        # Occupied endpoints: only beams that actually returned and whose
        # endpoint is inside the map.  A hit outside the map contributes free
        # space up to the boundary but must not create a false obstacle there.
        hit = np.isfinite(ranges) & (ranges <= float(max_range))
        ex = float(origin[0]) + c * dist
        ey = float(origin[1]) + s * dist
        eix = np.floor((ex - self.x_min) / self.resolution).astype(np.int64)
        eiy = np.floor((ey - self.y_min) / self.resolution).astype(np.int64)
        inside = (hit & (eix >= 0) & (eix < self.width)
                  & (eiy >= 0) & (eiy < self.height))
        if np.any(inside):
            np.add.at(self.log_odds, (eiy[inside], eix[inside]),
                      self.log_odds_occupied)
            self.observed[eiy[inside], eix[inside]] = True

        np.clip(self.log_odds, self.log_odds_min, self.log_odds_max,
                out=self.log_odds)
        self._inflated = None
        self.updates += int(ranges.size)
        self._version += 1
        return int(np.count_nonzero(np.isfinite(ranges)))

    # -- planning layers ------------------------------------------------
    def inflated_obstacles(self) -> np.ndarray:
        """Occupied mask dilated by the robot half-width plus margin."""
        if self._inflated is not None:
            return self._inflated
        obstacles = self.occupied
        radius = int(math.ceil(self.inflation_radius / self.resolution))
        out = obstacles.copy()
        # A disk-shaped structuring element, applied by array shifts.  This is
        # a Euclidean dilation up to the rasterisation of the disk.
        for dy in range(-radius, radius + 1):
            for dx in range(-radius, radius + 1):
                if dx * dx + dy * dy > radius * radius:
                    continue
                if dx == 0 and dy == 0:
                    continue
                ys = slice(max(0, dy), self.height + min(0, dy))
                xs = slice(max(0, dx), self.width + min(0, dx))
                yd = slice(max(0, -dy), self.height - max(0, dy))
                xd = slice(max(0, -dx), self.width - max(0, dx))
                out[yd, xd] |= obstacles[ys, xs]
        self._inflated = out
        return out

    def traversable(self, allow_unknown: bool | None = None) -> np.ndarray:
        """Boolean planning mask (``True`` means the cell may be entered)."""
        if allow_unknown is None:
            allow_unknown = self.allow_unknown
        blocked = self.inflated_obstacles()
        if allow_unknown:
            return ~blocked
        return (~blocked) & self.known & (~self.occupied)

    def nearest_free_cell(self, cell: tuple[int, int],
                          max_radius: int = 25) -> tuple[int, int] | None:
        """Nearest traversable cell to ``cell`` (searching square rings)."""
        free = self.traversable()
        ix, iy = self.clip_cell(*cell)
        if free[iy, ix]:
            return ix, iy
        best = None
        best_d2 = None
        for r in range(1, int(max_radius) + 1):
            for dy in range(-r, r + 1):
                for dx in (-r, r) if abs(dy) != r else range(-r, r + 1):
                    x, y = ix + dx, iy + dy
                    if not self._in_bounds(x, y) or not free[y, x]:
                        continue
                    d2 = dx * dx + dy * dy
                    if best is None or d2 < best_d2:
                        best, best_d2 = (x, y), d2
            if best is not None:
                return best
        return best


# ---------------------------------------------------------------------------
# A*
# ---------------------------------------------------------------------------
class AStarPlanner:
    """Eight-neighbour A* on a :class:`GridMap`."""

    _MOVES = ((-1, 0, 1.0), (1, 0, 1.0), (0, -1, 1.0), (0, 1, 1.0),
              (-1, -1, math.sqrt(2.0)), (-1, 1, math.sqrt(2.0)),
              (1, -1, math.sqrt(2.0)), (1, 1, math.sqrt(2.0)))

    def __init__(self, grid: GridMap, allow_unknown: bool | None = None,
                 max_expand: int = 200_000, smooth: bool = True) -> None:
        self.grid = grid
        self.allow_unknown = allow_unknown
        self.max_expand = int(max_expand)
        self.smooth = bool(smooth)
        self.last_path = np.zeros((0, 2))
        self.last_expanded = 0
        self.last_time = 0.0
        self.last_raw_length = 0.0
        self.last_length = 0.0

    # -- heuristic ------------------------------------------------------
    @staticmethod
    def _octile(dx: int, dy: int) -> float:
        dx, dy = abs(dx), abs(dy)
        return (max(dx, dy) + (math.sqrt(2.0) - 1.0) * min(dx, dy))

    def plan(self, start: Sequence[float], goal: Sequence[float],
             smooth: bool | None = None) -> np.ndarray:
        """Plan from ``start`` to ``goal`` in world coordinates.

        Returns an ``(N, 2)`` array (empty on failure).  With smoothing
        enabled the raw cell path is shortcut by line-of-sight and lightly
        averaged; the returned first/last points are the requested endpoints.
        """
        t0 = time.perf_counter()
        smooth = self.smooth if smooth is None else bool(smooth)
        free = self.grid.traversable(self.allow_unknown)
        start = np.asarray(start, dtype=float).reshape(2)
        goal = np.asarray(goal, dtype=float).reshape(2)
        start_cell = self.grid.nearest_free_cell(self.grid.world_to_cell(*start))
        goal_cell = self.grid.nearest_free_cell(self.grid.world_to_cell(*goal))
        self.last_path = np.zeros((0, 2))
        self.last_expanded = 0
        self.last_length = 0.0
        self.last_raw_length = 0.0
        if start_cell is None or goal_cell is None:
            self.last_time = time.perf_counter() - t0
            return self.last_path
        if not free[goal_cell[1], goal_cell[0]]:
            self.last_time = time.perf_counter() - t0
            return self.last_path

        w = self.grid.width
        n = w * self.grid.height
        g = np.full(n, np.inf, dtype=float)
        parent = np.full(n, -1, dtype=np.int64)
        closed = np.zeros(n, dtype=bool)
        sidx = start_cell[1] * w + start_cell[0]
        gidx = goal_cell[1] * w + goal_cell[0]
        g[sidx] = 0.0
        gx, gy = goal_cell
        heap: list[tuple[float, float, int]] = [
            (self._octile(start_cell[0] - gx, start_cell[1] - gy), 0.0, sidx)]
        expanded = 0
        found = False
        while heap:
            _, _, current = heapq.heappop(heap)
            if closed[current]:
                continue
            if current == gidx:
                found = True
                break
            closed[current] = True
            expanded += 1
            if expanded > self.max_expand:
                break
            cy, cx = divmod(current, w)
            gc = g[current]
            for dx, dy, step in self._MOVES:
                nx, ny = cx + dx, cy + dy
                if not (0 <= nx < self.grid.width and 0 <= ny < self.grid.height):
                    continue
                if not free[ny, nx]:
                    continue
                if dx != 0 and dy != 0:
                    # No corner cutting: both cardinal neighbours must be free.
                    if not (free[cy, nx] and free[ny, cx]):
                        continue
                nidx = ny * w + nx
                if closed[nidx]:
                    continue
                tentative = gc + step
                if tentative + 1e-12 < g[nidx]:
                    g[nidx] = tentative
                    parent[nidx] = current
                    h = self._octile(nx - gx, ny - gy)
                    heapq.heappush(heap, (tentative + h, h, nidx))
        self.last_expanded = expanded
        self.last_time = time.perf_counter() - t0
        if not found:
            return self.last_path
        # Reconstruct and smooth.
        cells: list[tuple[int, int]] = []
        node = gidx
        while node != -1:
            cy, cx = divmod(int(node), w)
            cells.append((cx, cy))
            if node == sidx:
                break
            node = parent[node]
        cells.reverse()
        raw = np.array([self.grid.cell_center(c) for c in cells], dtype=float)
        raw[0] = start
        raw[-1] = goal
        self.last_raw_length = self.path_length(raw)
        if smooth and len(raw) > 2:
            path = self.simplify(raw)
        else:
            path = raw
        self.last_path = path
        self.last_length = self.path_length(path)
        return self.last_path

    def line_clear(self, a: Sequence[float], b: Sequence[float]) -> bool:
        """True when every cell on the segment ``a-b`` is traversable."""
        free = self.grid.traversable(self.allow_unknown)
        ca = self.grid.world_to_cell(*np.asarray(a, dtype=float).reshape(2))
        cb = self.grid.world_to_cell(*np.asarray(b, dtype=float).reshape(2))
        for x, y in _bresenham(int(ca[0]), int(ca[1]), int(cb[0]), int(cb[1])):
            if not self.grid._in_bounds(x, y) or not free[y, x]:
                return False
        return True

    def simplify(self, path: np.ndarray) -> np.ndarray:
        """Greedy line-of-sight shortcut plus a conservative smoothing pass."""
        pts = np.asarray(path, dtype=float).reshape(-1, 2)
        if len(pts) <= 2:
            return pts.copy()
        keep = [pts[0]]
        i = 0
        while i < len(pts) - 1:
            j = len(pts) - 1
            while j > i + 1 and not self.line_clear(pts[i], pts[j]):
                j -= 1
            keep.append(pts[j])
            i = j
        out = np.array(keep, dtype=float)
        if len(out) <= 2:
            return out
        # One moving-average pass; reject any point whose two new segments are
        # not collision free, so smoothing can never push the path into a box.
        for _ in range(2):
            for k in range(1, len(out) - 1):
                cand = 0.25 * out[k - 1] + 0.5 * out[k] + 0.25 * out[k + 1]
                if (self.line_clear(out[k - 1], cand)
                        and self.line_clear(cand, out[k + 1])):
                    out[k] = cand
        return out

    @staticmethod
    def path_length(path: np.ndarray) -> float:
        pts = np.asarray(path, dtype=float).reshape(-1, 2)
        if len(pts) < 2:
            return 0.0
        return float(np.sum(np.linalg.norm(np.diff(pts, axis=0), axis=1)))


# ---------------------------------------------------------------------------
# Pure pursuit
# ---------------------------------------------------------------------------
class PurePursuit:
    """Geometric pure-pursuit tracker for a differential-drive-like base.

    The robot is commanded with a forward speed and a yaw rate; lateral speed
    is left at zero because the A1 trot controller used in this book tracks
    ``(vx, vy, wz)`` and pure pursuit only needs the first and third.
    """

    def __init__(self, lookahead: float = 0.45, v_max: float = 0.32,
                 w_max: float = 1.0, goal_tolerance: float = 0.15,
                 slow_radius: float = 0.9, heading_gain: float = 1.5,
                 min_turning_speed: float = 0.06) -> None:
        self.lookahead = float(lookahead)
        self.v_max = float(v_max)
        self.w_max = float(w_max)
        self.goal_tolerance = float(goal_tolerance)
        self.slow_radius = float(slow_radius)
        self.heading_gain = float(heading_gain)
        self.min_turning_speed = float(min_turning_speed)

    @staticmethod
    def _project(points: np.ndarray, position: np.ndarray):
        """Project ``position`` onto a polyline.

        Returns ``(distance, segment_index, fraction, s, cumulative)``.
        """
        seg = points[1:] - points[:-1]
        lengths = np.linalg.norm(seg, axis=1)
        cumulative = np.concatenate([[0.0], np.cumsum(lengths)])
        best = (np.inf, 0, 0.0)
        for i in range(len(seg)):
            length2 = float(seg[i] @ seg[i])
            if length2 < 1e-18:
                t = 0.0
            else:
                t = float((position - points[i]) @ seg[i] / length2)
                t = min(1.0, max(0.0, t))
            proj = points[i] + t * seg[i]
            d = float(np.linalg.norm(position - proj))
            if d < best[0]:
                best = (d, i, t)
        d, i, t = best
        s = float(cumulative[i] + t * lengths[i]) if len(lengths) else 0.0
        return d, i, t, s, cumulative

    @staticmethod
    def _point_at(points: np.ndarray, cumulative: np.ndarray,
                  s: float) -> np.ndarray:
        if s <= 0.0:
            return points[0].copy()
        total = cumulative[-1]
        if s >= total:
            return points[-1].copy()
        i = int(np.searchsorted(cumulative, s, side="right") - 1)
        i = min(max(i, 0), len(points) - 2)
        seg = cumulative[i + 1] - cumulative[i]
        t = 0.0 if seg < 1e-12 else (s - cumulative[i]) / seg
        return points[i] + t * (points[i + 1] - points[i])

    def compute(self, position: Sequence[float], yaw: float,
                path: Sequence[Sequence[float]],
                dt: float | None = None) -> tuple[float, float, bool]:
        """Return ``(vx, wz, arrived)`` for one control cycle.

        ``dt`` is accepted for API symmetry with the 2 ms loop; the command is
        a pure pursuit law and does not integrate state, so the result is
        independent of it.  Speed and yaw-rate are clipped to the limits.
        """
        points = np.asarray(path, dtype=float).reshape(-1, 2)
        if len(points) == 0:
            return 0.0, 0.0, False
        position = np.asarray(position, dtype=float).reshape(2)
        goal = points[-1]
        goal_dist = float(np.linalg.norm(goal - position))
        if goal_dist <= self.goal_tolerance:
            return 0.0, 0.0, True
        if len(points) == 1:
            target = goal
        else:
            _, _, _, s, cumulative = self._project(points, position)
            target = self._point_at(points, cumulative,
                                    s + self.lookahead)
        delta = target - position
        ld = float(np.linalg.norm(delta))
        if ld < 1e-6:
            return 0.0, 0.0, False
        alpha = wrap_angle(math.atan2(delta[1], delta[0]) - float(yaw))
        # Slow near the goal and while the heading error is large.  If the
        # target is far to the side, rotate in place instead of driving a wide
        # arc; the trot controller supports zero-linear-speed yaw commands.
        if abs(alpha) > 0.45 * math.pi:
            vx = 0.0
            wz = float(np.clip(self.heading_gain * alpha,
                               -self.w_max, self.w_max))
            return vx, wz, False
        slow = min(1.0, goal_dist / max(self.slow_radius, 1e-6))
        align = max(0.25, math.cos(alpha))
        vx = float(np.clip(self.v_max * slow * align, 0.0, self.v_max))
        if vx < self.min_turning_speed and abs(alpha) > 0.15:
            vx = 0.0
            wz = float(np.clip(self.heading_gain * alpha,
                               -self.w_max, self.w_max))
            return vx, wz, False
        curvature = 2.0 * math.sin(alpha) / max(ld, 1e-6)
        wz = float(np.clip(vx * curvature, -self.w_max, self.w_max))
        return vx, wz, False


# ---------------------------------------------------------------------------
# Navigation stack
# ---------------------------------------------------------------------------
@dataclass
class NavStats:
    scans: int = 0
    plans: int = 0
    failed_plans: int = 0
    last_scan_time: float = 0.0
    last_plan_time: float = 0.0
    last_path_length: float = 0.0
    expanded: int = 0


class NavStack:
    """Scan -> map -> plan -> track loop for a trotting A1.

    The pose fed to the lidar is the estimator's base position; the yaw is the
    IMU yaw from ``read_sensors``.  The simulator's ground-truth base pose is
    never read here.  ``step`` writes ``vx``/``wz`` into the controller with
    :meth:`TrottingController.set_command`; the caller still runs the trot
    controller and advances physics.

    Typical use (inside the 2 ms loop)::

        nav.step()
        cmd = trot.run()
        sim.step(cmd)

    The controller must have been entered and run at least once so that
    ``controller.est_base`` exists.
    """

    def __init__(self, sim, controller, goal: Sequence[float],
                 grid: GridMap | None = None, lidar: Lidar2D | None = None,
                 planner: AStarPlanner | None = None,
                 pursuit: PurePursuit | None = None,
                 scan_hz: float = 10.0, plan_hz: float = 2.0,
                 control_dt: float = 0.002,
                 arrival_tolerance: float = 0.15,
                 goal_margin: float = 1.5,
                 robot_half_width: float = 0.16,
                 safety_margin: float = 0.06,
                 lidar_height: float = 0.08,
                 noise_std: float = 0.0,
                 seed: int = 0) -> None:
        self.sim = sim
        self.controller = controller
        self.goal = np.asarray(goal, dtype=float).reshape(2)
        self.control_dt = float(control_dt)
        self.scan_period = 1.0 / max(float(scan_hz), 1e-6)
        self.plan_period = 1.0 / max(float(plan_hz), 1e-6)
        self.arrival_tolerance = float(arrival_tolerance)
        self.goal_margin = float(goal_margin)
        self.robot_half_width = float(robot_half_width)
        self.safety_margin = float(safety_margin)
        self.grid = grid
        self.lidar = lidar or Lidar2D(
            sim, max_range=6.0, n_beams=360, noise_std=noise_std,
            mount=(0.0, 0.0, float(lidar_height)), seed=seed)
        self.planner = planner
        self.pursuit = pursuit or PurePursuit()
        self.stats = NavStats()
        self.path = np.zeros((0, 2))
        self.path_history: list[np.ndarray] = []
        self.trajectory: list[np.ndarray] = []
        self.scan_history: list[tuple[np.ndarray, np.ndarray, np.ndarray]] = []
        self.arrived = False
        self.elapsed = 0.0
        self._initialized = False
        self._next_scan = 0.0
        self._next_plan = 0.0
        self._traj_period = 0.1
        self._next_traj = 0.0
        self._last_est = None
        self._last_command = (0.0, 0.0)

    # -- setup ----------------------------------------------------------
    def reset(self, keep_map: bool = False) -> None:
        """(Re)initialise timers, map and path from the current estimate.

        ``keep_map=True`` keeps the existing grid (for example one built with
        :meth:`GridMap.from_occupancy`) and only resets timers, paths and
        statistics.  The default ``False`` preserves the original behaviour:
        an existing grid is cleared before a new run.
        """
        self.elapsed = 0.0
        self._next_scan = 0.0
        self._next_plan = 0.0
        self._next_traj = 0.0
        self.arrived = False
        self.path = np.zeros((0, 2))
        self.path_history = []
        self.trajectory = []
        self.scan_history = []
        self.stats = NavStats()
        self._last_command = (0.0, 0.0)
        self._last_est = None
        if self.grid is None:
            start = self._estimated_position()
            x0 = min(start[0], self.goal[0]) - self.goal_margin
            x1 = max(start[0], self.goal[0]) + self.goal_margin
            y0 = min(start[1], self.goal[1]) - 2.0 * self.goal_margin
            y1 = max(start[1], self.goal[1]) + 2.0 * self.goal_margin
            self.grid = GridMap(x_min=x0, y_min=y0, x_max=x1, y_max=y1,
                                robot_half_width=self.robot_half_width,
                                safety_margin=self.safety_margin)
        elif not keep_map:
            self.grid.reset()
        if self.planner is None or self.planner.grid is not self.grid:
            self.planner = AStarPlanner(self.grid)
        self._initialized = True

    def _estimated_position(self) -> np.ndarray:
        est = getattr(self.controller, "est_base", None)
        if est is None:
            return np.zeros(3)
        return np.asarray(est.position, dtype=float).reshape(3).copy()

    def _imu_yaw(self) -> float:
        est = getattr(self.controller, "est_base", None)
        if est is not None:
            return float(np.asarray(est.rpy, dtype=float).reshape(3)[2])
        return 0.0

    # -- loop -----------------------------------------------------------
    def step(self) -> tuple[float, float]:
        """Run one control cycle; return the command sent to the controller."""
        if not self._initialized:
            self.reset()
        est = getattr(self.controller, "est_base", None)
        if est is None:
            # The caller has not run the trot controller yet.  Do nothing
            # rather than inventing a pose; the next cycles will start.
            return 0.0, 0.0
        self._last_est = est
        pos = np.asarray(est.position, dtype=float).reshape(3)
        yaw = self._imu_yaw()
        self.elapsed += self.control_dt

        # --- low-rate sensing and mapping -----------------------------
        if self.elapsed + 1e-12 >= self._next_scan:
            self._next_scan = self.elapsed + self.scan_period
            sensors = self.sim.read_sensors()
            yaw_imu = float(np.asarray(sensors.rpy, dtype=float).reshape(3)[2])
            t0 = time.perf_counter()
            ranges = self.lidar.scan(pos, yaw_imu)
            self.grid.update_scan(
                ranges, self.lidar.last_origin, yaw_imu,
                angles=self.lidar.angles, max_range=self.lidar.max_range,
                free_stride=2)
            self.stats.scans += 1
            self.stats.last_scan_time = time.perf_counter() - t0
            self.scan_history.append((self.lidar.last_origin.copy(),
                                      self.lidar.last_directions.copy(),
                                      ranges.copy()))
            if len(self.scan_history) > 500:
                self.scan_history.pop(0)

        # --- low-rate planning ----------------------------------------
        if self.elapsed + 1e-12 >= self._next_plan:
            self._next_plan = self.elapsed + self.plan_period
            t0 = time.perf_counter()
            path = self.planner.plan(pos[:2], self.goal)
            self.stats.plans += 1
            self.stats.last_plan_time = time.perf_counter() - t0
            self.stats.last_path_length = self.planner.last_length
            self.stats.expanded = self.planner.last_expanded
            if len(path) >= 2:
                self.path = path
                self.path_history.append(path.copy())
                if len(self.path_history) > 20:
                    self.path_history.pop(0)
            elif len(self.path) < 2:
                self.stats.failed_plans += 1
                self.controller.set_command(0.0, 0.0, 0.0)
                return 0.0, 0.0

        # --- every cycle: track ---------------------------------------
        if len(self.path) < 2:
            self.controller.set_command(0.0, 0.0, 0.0)
            return 0.0, 0.0
        goal_dist = float(np.linalg.norm(pos[:2] - self.goal))
        vx, wz, arrived = self.pursuit.compute(pos[:2], yaw, self.path,
                                               dt=self.control_dt)
        if arrived or goal_dist <= self.arrival_tolerance:
            self.arrived = True
            vx = wz = 0.0
        self._last_command = (float(vx), float(wz))
        self.controller.set_command(float(vx), 0.0, float(wz))
        if self.elapsed + 1e-12 >= self._next_traj:
            self._next_traj = self.elapsed + self._traj_period
            self.trajectory.append(np.array([pos[0], pos[1], yaw, vx, wz],
                                            dtype=float))
        return self._last_command

    # -- inspection -----------------------------------------------------
    @property
    def last_command(self) -> tuple[float, float]:
        return self._last_command

    def distance_to_goal(self) -> float:
        est = getattr(self.controller, "est_base", None)
        if est is None:
            return float("inf")
        return float(np.linalg.norm(
            np.asarray(est.position, dtype=float).reshape(3)[:2] - self.goal))

    def map_array(self) -> np.ndarray:
        """Occupancy probability image (row 0 is ``y_min``)."""
        return self.grid.probability

    def set_goal(self, goal: Sequence[float]) -> None:
        """Change the goal and force a replan on the next cycle."""
        self.goal = np.asarray(goal, dtype=float).reshape(2)
        self.arrived = False
        self._next_plan = self.elapsed


__all__ = ["Lidar2D", "GridMap", "AStarPlanner", "PurePursuit", "NavStack",
           "NavStats", "wrap_angle"]
