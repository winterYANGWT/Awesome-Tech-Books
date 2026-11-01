"""Unit tests for the Chapter 11 navigation primitives."""

from __future__ import annotations

import math
from pathlib import Path
from types import SimpleNamespace

import numpy as np
import pytest

from quadruped.nav import AStarPlanner, GridMap, Lidar2D, NavStack, PurePursuit

ROOT = Path(__file__).resolve().parents[3]
NAV_XML = ROOT / "src" / "code" / "assets" / "nav_scene.xml"


# ---------------------------------------------------------------------------
# Grid mapping
# ---------------------------------------------------------------------------
def test_grid_ray_update_marks_free_then_occupied():
    grid = GridMap(x_min=0.0, y_min=0.0, x_max=4.0, y_max=4.0,
                   resolution=0.05)
    origin = (0.5, 0.5)
    grid.update_ray(origin, angle=0.0, distance=1.0, hit=True)

    # Cells strictly before the endpoint are free (p < 0.5), the endpoint is
    # occupied (p > 0.5), and cells beyond the measured range stay unknown.
    for x in (0.6, 0.9, 1.2, 1.4):
        ix, iy = grid.world_to_cell(x, 0.5)
        assert grid.probability[iy, ix] < 0.45
        assert grid.observed[iy, ix]
    ix, iy = grid.world_to_cell(1.5, 0.5)
    assert grid.probability[iy, ix] > 0.65
    ix, iy = grid.world_to_cell(2.0, 0.5)
    assert not grid.observed[iy, ix]
    assert abs(grid.probability[iy, ix] - 0.5) < 1e-12


def test_grid_no_return_clears_the_whole_ray():
    grid = GridMap(x_min=0.0, y_min=0.0, x_max=3.0, y_max=3.0,
                   resolution=0.05)
    grid.update_ray((0.2, 0.2), angle=math.pi / 2, distance=1.0, hit=False)
    for y in (0.4, 0.8, 1.1):
        ix, iy = grid.world_to_cell(0.2, y)
        assert grid.observed[iy, ix]
        assert grid.probability[iy, ix] < 0.45
        assert not grid.occupied[iy, ix]


def test_grid_inflation_is_clearance_aware():
    grid = GridMap(x_min=0.0, y_min=0.0, x_max=3.0, y_max=3.0,
                   resolution=0.05, robot_half_width=0.16,
                   safety_margin=0.04)
    grid.update_ray((0.5, 1.5), 0.0, 1.0, hit=True)
    free = grid.traversable(allow_unknown=True)
    ix, iy = grid.world_to_cell(1.5, 1.5)
    assert not free[iy, ix]                      # obstacle itself
    # 0.12 m to the side is inside the 0.20 m inflation radius.
    ix, iy = grid.world_to_cell(1.5, 1.62)
    assert not free[iy, ix]
    # 0.35 m to the side is outside it.
    ix, iy = grid.world_to_cell(1.5, 1.85)
    assert free[iy, ix]


def test_grid_from_occupancy_builds_known_inflated_map():
    occupancy = np.zeros((40, 60), dtype=bool)
    occupancy[20, 30] = True
    grid = GridMap.from_occupancy(occupancy, 0.05, origin=(-0.5, -0.5),
                                  robot_half_width=0.10,
                                  safety_margin=0.0)

    assert grid.shape == (40, 60)
    assert grid.x_min == pytest.approx(-0.5)
    assert grid.y_min == pytest.approx(-0.5)
    assert grid.x_max == pytest.approx(60 * 0.05 - 0.5)
    assert grid.y_max == pytest.approx(40 * 0.05 - 0.5)
    assert np.all(grid.known)
    assert grid.occupied[20, 30]
    assert grid.probability[0, 0] < 0.2         # free cell, p = sigmoid(-2.2)
    # Inflation radius 0.10 m = 2 cells: 0.10 m is blocked, 0.15 m is free.
    assert not grid.traversable()[20, 32]
    assert grid.traversable()[20, 33]


def test_navstack_reset_can_keep_the_static_map():
    grid = GridMap(x_min=0.0, y_min=0.0, x_max=2.0, y_max=2.0,
                   resolution=0.05)
    grid.update_ray((0.5, 0.5), 0.0, 0.5, hit=True)
    updates = grid.updates

    class Trot:
        est_base = SimpleNamespace(position=np.zeros(3), rpy=np.zeros(3))

        def set_command(self, *args):
            pass

    nav = NavStack(object(), Trot(), (1.0, 1.0), grid=grid, lidar=object())
    nav.reset(keep_map=True)
    assert grid.updates == updates
    assert grid.observed.any()
    assert np.any(grid.log_odds != 0.0)

    nav.reset()
    assert grid.updates == 0
    assert not grid.observed.any()
    assert not np.any(grid.log_odds != 0.0)


# ---------------------------------------------------------------------------
# A*
# ---------------------------------------------------------------------------
def _make_wall_map():
    grid = GridMap(x_min=0.0, y_min=0.0, x_max=2.0, y_max=2.0,
                   resolution=0.05, robot_half_width=0.0, safety_margin=0.0)
    # A solid wall at x ~= 1.0 with a single-cell gap at y ~= 1.5.  Both
    # coordinates are the *centres* of the cells, so the gap is one cell wide.
    gx, _ = grid.world_to_cell(1.0, 0.0)
    for gy in range(grid.height):
        if abs(grid.cell_to_world(gx, gy)[1] - 1.5) > 0.03:
            grid.log_odds[gy, gx] = 5.0
            grid.observed[gy, gx] = True
    grid._inflated = None
    return grid


def _dijkstra_length(grid, start, goal):
    """Independent 8-neighbour shortest-path length used as a reference."""
    free = grid.traversable()
    w, h = grid.width, grid.height
    sx, sy = grid.world_to_cell(*start)
    gx, gy = grid.world_to_cell(*goal)
    dist = np.full((h, w), np.inf)
    done = np.zeros((h, w), dtype=bool)
    dist[sy, sx] = 0.0
    moves = [(-1, 0, 1.0), (1, 0, 1.0), (0, -1, 1.0), (0, 1, 1.0),
             (-1, -1, math.sqrt(2)), (-1, 1, math.sqrt(2)),
             (1, -1, math.sqrt(2)), (1, 1, math.sqrt(2))]
    for _ in range(h * w):
        best = np.inf
        cell = None
        for y in range(h):
            for x in range(w):
                if not done[y, x] and dist[y, x] < best:
                    best, cell = dist[y, x], (x, y)
        if cell is None:
            break
        x, y = cell
        done[y, x] = True
        if cell == (gx, gy):
            break
        for dx, dy, cost in moves:
            nx, ny = x + dx, y + dy
            if not (0 <= nx < w and 0 <= ny < h) or not free[ny, nx]:
                continue
            if dx and dy and not (free[y, nx] and free[ny, x]):
                continue
            nd = best + cost
            if nd < dist[ny, nx]:
                dist[ny, nx] = nd
    return float(dist[gy, gx] * grid.resolution)


@pytest.mark.parametrize("start,goal", [((0.225, 0.225), (1.775, 1.775)),
                                        ((0.225, 1.775), (1.775, 0.225)),
                                        ((0.325, 0.425), (1.675, 1.575))])
def test_astar_shortest_path_matches_dijkstra(start, goal):
    grid = _make_wall_map()
    planner = AStarPlanner(grid)
    path = planner.plan(start, goal, smooth=False)
    assert len(path) >= 2
    length = planner.path_length(path)
    reference = _dijkstra_length(grid, start, goal)
    assert length == pytest.approx(reference, abs=1e-8)
    assert np.allclose(path[0], start)
    assert np.allclose(path[-1], goal)
    # Every vertex must be traversable and every segment collision free.
    for x, y in path:
        ix, iy = grid.world_to_cell(x, y)
        assert grid.traversable()[iy, ix]
    for a, b in zip(path[:-1], path[1:]):
        assert planner.line_clear(a, b)


def test_astar_open_map_is_straight_and_smoothed():
    grid = GridMap(x_min=0.0, y_min=0.0, x_max=2.0, y_max=2.0,
                   resolution=0.05)
    planner = AStarPlanner(grid)
    path = planner.plan((0.2, 0.2), (1.8, 1.8))
    assert len(path) == 2
    assert planner.path_length(path) == pytest.approx(
        math.hypot(1.6, 1.6), abs=1e-9)


def test_astar_returns_empty_when_goal_is_sealed():
    grid = GridMap(x_min=0.0, y_min=0.0, x_max=2.0, y_max=2.0,
                   resolution=0.05, robot_half_width=0.0, safety_margin=0.0)
    # A closed square wall around the goal.
    for x in range(10, 15):
        for y in range(10, 15):
            edge = x in (10, 14) or y in (10, 14)
            if edge:
                grid.log_odds[y, x] = 5.0
                grid.observed[y, x] = True
    grid._inflated = None
    planner = AStarPlanner(grid)
    assert len(planner.plan((0.2, 0.2), (0.6, 0.6))) == 0


# ---------------------------------------------------------------------------
# Pure pursuit
# ---------------------------------------------------------------------------
def test_pure_pursuit_straight_line_commands_forward():
    pp = PurePursuit(lookahead=0.5, v_max=0.35, w_max=1.0)
    vx, wz, arrived = pp.compute((0.0, 0.0), 0.0, [(0.0, 0.0), (3.0, 0.0)])
    assert 0.0 < vx <= 0.35
    assert abs(wz) < 1e-12
    assert not arrived


def test_pure_pursuit_arc_signs_and_limits():
    pp = PurePursuit(lookahead=0.4, v_max=0.3, w_max=0.7)
    left = [(0.0, 0.0), (0.4, 0.02), (0.8, 0.08), (1.2, 0.2), (1.6, 0.4)]
    right = [(x, -y) for x, y in left]
    vx_l, wz_l, _ = pp.compute((0.0, 0.0), 0.0, left)
    vx_r, wz_r, _ = pp.compute((0.0, 0.0), 0.0, right)
    assert vx_l > 0.0 and vx_r > 0.0
    assert wz_l > 0.0 and wz_r < 0.0
    assert abs(wz_l) <= 0.7 and abs(wz_r) <= 0.7
    assert abs(wz_l) == pytest.approx(abs(wz_r), rel=1e-9)


def test_pure_pursuit_arrival_and_turning_in_place():
    pp = PurePursuit(lookahead=0.4, goal_tolerance=0.15, w_max=0.8)
    vx, wz, arrived = pp.compute((1.0, 0.0), 0.0, [(0.0, 0.0), (1.05, 0.0)])
    assert arrived and vx == 0.0 and wz == 0.0
    # Target behind the robot: rotate in place with the correct sign.
    vx, wz, arrived = pp.compute((0.0, 0.0), 0.0, [(0.0, 0.0), (0.0, 1.0)])
    assert vx == 0.0 and wz > 0.0 and not arrived


# ---------------------------------------------------------------------------
# MuJoCo scene and lidar
# ---------------------------------------------------------------------------
def test_nav_scene_loads_and_front_ray_hits_the_middle_crate():
    pytest.importorskip("mujoco")
    from quadruped.sim import A1Sim

    sim = A1Sim(model_path=str(NAV_XML))
    try:
        lidar = Lidar2D(sim, noise_std=0.0)
        ranges = lidar.scan((0.0, 0.0, 0.27), 0.0)
        # The crate face is at x=1.05; the lidar origin is 1.35-0.30 away.
        front = float(ranges[np.argmin(np.abs(lidar.angles))])
        assert front == pytest.approx(1.05, abs=0.03)
        # The robot's own visual/collision geoms are excluded by group mask.
        finite = ranges[np.isfinite(ranges)]
        assert finite.size > 0 and finite.min() > 1.0
        assert lidar.last_origin[2] == pytest.approx(0.35, abs=1e-9)
    finally:
        sim.close()


def test_nav_scene_alias_via_environment(monkeypatch):
    pytest.importorskip("mujoco")
    from quadruped.robot import default_model_path

    monkeypatch.setenv("QUADRUPED_A1_XML", str(NAV_XML))
    assert Path(default_model_path()) == NAV_XML
