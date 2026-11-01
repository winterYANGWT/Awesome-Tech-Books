"""Swing-leg foot trajectories.

The default is the cycloid of Chapter 9.  With ``p in [0, 1]`` the position is

.. math::

   x(p) &= x_0 + (x_1-x_0)\\frac{2\\pi p-\\sin 2\\pi p}{2\\pi},\\\\
   y(p) &= y_0 + (y_1-y_0)\\frac{2\\pi p-\\sin 2\\pi p}{2\\pi},\\\\
   z(p) &= z_0 + (z_1-z_0)p + \\frac{h}{2}(1-\\cos 2\\pi p),

and the velocity follows by differentiating with respect to time
(``\\dot p = 1/T``).  The linear ``z`` term is the correction for unequal
take-off and landing heights which the reference formula omits; it is zero in
flat-ground trotting.  A quintic Bezier alternative is provided for obstacle
or irregular-terrain use.
"""

from __future__ import annotations

import math

import numpy as np


class SwingTrajectory:
    """A single swing phase from ``start`` to ``end``.

    Parameters
    ----------
    start, end:
        World-frame foot-sphere centres at lift-off and touch-down.
    height:
        Peak height above the straight line joining start and end (m).
    duration:
        Swing duration ``T`` in seconds.
    kind:
        ``"cycloid"`` (default) or ``"bezier"``.
    """

    def __init__(self, start: np.ndarray | None = None,
                 end: np.ndarray | None = None, height: float = 0.06,
                 duration: float = 0.15, kind: str = "cycloid") -> None:
        self.start = np.zeros(3) if start is None else np.asarray(
            start, dtype=float).reshape(3).copy()
        self.end = np.zeros(3) if end is None else np.asarray(
            end, dtype=float).reshape(3).copy()
        self.height = float(height)
        self.duration = float(duration)
        self.kind = str(kind)
        self._bezier = self._bezier_control()

    def set(self, start: np.ndarray, end: np.ndarray, height: float | None = None,
            duration: float | None = None, kind: str | None = None) -> None:
        self.start = np.asarray(start, dtype=float).reshape(3).copy()
        self.end = np.asarray(end, dtype=float).reshape(3).copy()
        if height is not None:
            self.height = float(height)
        if duration is not None:
            self.duration = float(duration)
        if kind is not None:
            self.kind = str(kind)
        self._bezier = self._bezier_control()

    def _bezier_control(self) -> np.ndarray:
        """Quintic Bezier control points with zero end velocities."""
        s, e = self.start, self.end
        # Two control points near each end, lifted by the swing height.
        return np.array([
            s,
            s + (e - s) / 5.0 + np.array([0.0, 0.0, 0.4 * self.height]),
            s + 2.0 * (e - s) / 5.0 + np.array([0.0, 0.0, self.height]),
            s + 3.0 * (e - s) / 5.0 + np.array([0.0, 0.0, self.height]),
            s + 4.0 * (e - s) / 5.0 + np.array([0.0, 0.0, 0.4 * self.height]),
            e,
        ])

    # -- cycloid ---------------------------------------------------------
    def cycloid_position(self, p: float) -> np.ndarray:
        p = min(1.0, max(0.0, float(p)))
        th = 2.0 * math.pi * p
        s = (th - math.sin(th)) / (2.0 * math.pi)
        z = 0.5 * self.height * (1.0 - math.cos(th))
        out = self.start + (self.end - self.start) * s
        out[2] = self.start[2] + (self.end[2] - self.start[2]) * p + z
        return out

    def cycloid_velocity(self, p: float) -> np.ndarray:
        p = min(1.0, max(0.0, float(p)))
        th = 2.0 * math.pi * p
        if self.duration <= 0.0:
            return np.zeros(3)
        ds = (1.0 - math.cos(th)) / self.duration
        dz = (math.pi * self.height / self.duration) * math.sin(th)
        out = (self.end - self.start) * ds
        out[2] = (self.end[2] - self.start[2]) / self.duration + dz
        return out

    # -- Bezier ----------------------------------------------------------
    def bezier_position(self, p: float) -> np.ndarray:
        p = min(1.0, max(0.0, float(p)))
        u = 1.0 - p
        b = np.array([u ** 5, 5 * u ** 4 * p, 10 * u ** 3 * p ** 2,
                      10 * u ** 2 * p ** 3, 5 * u * p ** 4, p ** 5])
        return b @ self._bezier

    def bezier_velocity(self, p: float) -> np.ndarray:
        p = min(1.0, max(0.0, float(p)))
        u = 1.0 - p
        db = np.array([-5 * u ** 4, 5 * u ** 4 - 20 * u ** 3 * p,
                       20 * u ** 3 * p - 30 * u ** 2 * p ** 2,
                       30 * u ** 2 * p ** 2 - 20 * u * p ** 3,
                       20 * u * p ** 3 - 5 * p ** 4,
                       5 * p ** 4])
        if self.duration <= 0.0:
            return np.zeros(3)
        return (db @ self._bezier) / self.duration

    # -- public API ------------------------------------------------------
    def position(self, p: float) -> np.ndarray:
        if self.kind == "bezier":
            return self.bezier_position(p)
        return self.cycloid_position(p)

    def velocity(self, p: float) -> np.ndarray:
        if self.kind == "bezier":
            return self.bezier_velocity(p)
        return self.cycloid_velocity(p)

    def evaluate(self, p: float) -> tuple[np.ndarray, np.ndarray]:
        return self.position(p), self.velocity(p)


__all__ = ["SwingTrajectory"]
