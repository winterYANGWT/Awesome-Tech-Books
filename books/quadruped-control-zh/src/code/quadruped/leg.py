"""Single-leg kinematics and statics for the Unitree A1.

The leg is modelled as a three-degree-of-freedom serial chain:

* joint 1 (abduction) rotates about the hip x axis;
* joint 2 (thigh) rotates about the y axis of the hip frame;
* joint 3 (knee) rotates about the y axis of the thigh frame.

All positions returned by :meth:`Leg.fk` are expressed in the leg base frame
``{0}``, whose origin is the abduction axis and whose axes are parallel to the
body frame when all joints are zero.  The returned point is the *geometric
centre of the foot sphere*, not the contact point; add ``foot_radius`` along
the local foot normal if a contact point is needed.

The implementation follows the geometric derivation of Chapter 5.  It never
calls MuJoCo: the same formulas are used by the controller and by the tests
that compare against the simulator.
"""

from __future__ import annotations

import math

import numpy as np

from .types import LegId


class Leg:
    """A three-joint leg with analytic FK, IK, Jacobian and statics."""

    def __init__(self, leg_id: LegId, hip_offset: np.ndarray,
                 thigh_offset: np.ndarray, l_thigh: float = 0.2,
                 l_calf: float = 0.2, foot_radius: float = 0.02,
                 q_min: np.ndarray | None = None,
                 q_max: np.ndarray | None = None) -> None:
        self.leg_id = LegId(leg_id)
        self.hip_offset = np.asarray(hip_offset, dtype=float).reshape(3)
        self.thigh_offset = np.asarray(thigh_offset, dtype=float).reshape(3)
        if abs(self.thigh_offset[0]) > 1e-12 or abs(self.thigh_offset[2]) > 1e-12:
            raise ValueError("thigh_offset must be a lateral (y) offset")
        self.l_thigh = float(l_thigh)
        self.l_calf = float(l_calf)
        self.foot_radius = float(foot_radius)
        self.q_min = (np.full(3, -np.inf) if q_min is None
                      else np.asarray(q_min, dtype=float).reshape(3))
        self.q_max = (np.full(3, np.inf) if q_max is None
                      else np.asarray(q_max, dtype=float).reshape(3))
        self._a = np.array([0.0, 0.0, -self.l_thigh])
        self._b = np.array([0.0, 0.0, -self.l_calf])

    # -- basic transforms -------------------------------------------------
    @staticmethod
    def _rx(theta: float) -> np.ndarray:
        c, s = math.cos(theta), math.sin(theta)
        return np.array([[1.0, 0.0, 0.0],
                         [0.0, c, -s],
                         [0.0, s, c]], dtype=float)

    @staticmethod
    def _ry(theta: float) -> np.ndarray:
        c, s = math.cos(theta), math.sin(theta)
        return np.array([[c, 0.0, s],
                         [0.0, 1.0, 0.0],
                         [-s, 0.0, c]], dtype=float)

    # -- forward kinematics ----------------------------------------------
    def fk(self, q: np.ndarray) -> np.ndarray:
        """Foot-sphere centre in the leg base frame ``{0}``."""
        q1, q2, q3 = np.asarray(q, dtype=float).reshape(3)
        w = self._ry(q2) @ (self._a + self._ry(q3) @ self._b)
        return self._rx(q1) @ (self.thigh_offset + w)

    # -- inverse kinematics ----------------------------------------------
    def ik(self, p: np.ndarray, knee: int = -1,
           q_ref: np.ndarray | None = None) -> np.ndarray:
        """Solve inverse kinematics for a foot-sphere centre.

        ``knee=-1`` selects the A1's valid branch (knee flexed backwards);
        ``knee=+1`` selects the other geometric branch.  A ``ValueError`` is
        raised for a point outside the workspace rather than returning NaNs.

        Near the workspace boundary the abduction angle can have two valid
        solutions (the leg plane can tilt to either side).  By default the
        solution closest to zero abduction is returned.  Passing ``q_ref``
        (for example the previous joint vector) selects the branch closest to
        that reference, which is what a trajectory controller should do.
        """
        if knee not in (-1, 1):
            raise ValueError("knee must be -1 or +1")
        p = np.asarray(p, dtype=float).reshape(3)
        d = float(self.thigh_offset[1])
        y, z = float(p[1]), float(p[2])
        rho = math.hypot(y, z)
        if rho < abs(d) - 1e-12:
            raise ValueError("foot target lies inside the hip lateral offset")
        if rho < 1e-15:
            raise ValueError("foot target is at the hip axis")

        beta = math.acos(max(-1.0, min(1.0, d / rho)))
        alpha = math.atan2(z, y)
        q1_candidates = [self._wrap(alpha + beta), self._wrap(alpha - beta)]

        solutions = []
        for q1 in q1_candidates:
            sol = self._solve_planar(p, q1, knee)
            if sol is not None:
                solutions.append(sol)
        if not solutions:
            raise ValueError("foot target is outside the leg workspace")

        if q_ref is not None:
            ref = np.asarray(q_ref, dtype=float).reshape(3)
            def cost(q):
                return float(np.linalg.norm(self._wrap(q - ref)))
            return min(solutions, key=cost)

        def default_cost(q):
            in_range = all(self._range_violation(q[i], i) <= 1e-9
                           for i in range(3))
            return (0 if in_range else 1, abs(q[0]), np.linalg.norm(q))

        return min(solutions, key=default_cost)

    def _solve_planar(self, p: np.ndarray, q1: float,
                      knee: int) -> np.ndarray | None:
        """Solve q2, q3 once an abduction branch ``q1`` is chosen."""
        w = self._rx(-q1) @ p
        vx, vz = float(w[0]), float(w[2])
        r2 = vx * vx + vz * vz
        c3 = (r2 - self.l_thigh ** 2 - self.l_calf ** 2) / (
            2.0 * self.l_thigh * self.l_calf)
        if c3 < -1.0 - 1e-9 or c3 > 1.0 + 1e-9:
            return None
        q3 = knee * math.acos(max(-1.0, min(1.0, c3)))
        gamma = math.atan2(-vx, -vz)
        q2 = gamma - math.atan2(self.l_calf * math.sin(q3),
                                self.l_thigh + self.l_calf * math.cos(q3))
        out = np.array([q1, self._canonical(q2, 1),
                        self._canonical(q3, 2)], dtype=float)
        out[0] = self._canonical(out[0], 0)
        return out

    # -- differential kinematics -----------------------------------------
    def jacobian(self, q: np.ndarray) -> np.ndarray:
        """Analytic Jacobian ``d p / d q`` in the leg base frame."""
        q1, q2, q3 = np.asarray(q, dtype=float).reshape(3)
        R1 = self._rx(q1)
        dR1 = self._skew_x() @ R1
        w = self._ry(q2) @ (self._a + self._ry(q3) @ self._b)
        v = self.thigh_offset + w
        ey = np.array([0.0, 1.0, 0.0])
        v2 = np.cross(ey, w)
        v3 = self._ry(q2) @ np.cross(ey, self._ry(q3) @ self._b)
        return np.column_stack((dR1 @ v, R1 @ v2, R1 @ v3))

    def jacobian_dot(self, q: np.ndarray, dq: np.ndarray) -> np.ndarray:
        """Time derivative of the analytic Jacobian."""
        q1, q2, q3 = np.asarray(q, dtype=float).reshape(3)
        dq1, dq2, dq3 = np.asarray(dq, dtype=float).reshape(3)
        R1 = self._rx(q1)
        Sx = self._skew_x()
        dR1 = Sx @ R1
        ddR1 = Sx @ dR1
        w = self._ry(q2) @ (self._a + self._ry(q3) @ self._b)
        v = self.thigh_offset + w
        ey = np.array([0.0, 1.0, 0.0])
        v2 = np.cross(ey, w)
        w3 = self._ry(q3) @ self._b
        v3 = self._ry(q2) @ np.cross(ey, w3)
        vdot = v2 * dq2 + v3 * dq3
        v2dot = np.cross(ey, vdot)
        v3dot = np.cross(ey, v3) * dq2 + self._ry(q2) @ np.cross(
            ey, np.cross(ey, w3)) * dq3
        Jd = np.column_stack((
            ddR1 @ v * dq1 + dR1 @ vdot,
            dR1 @ v2 * dq1 + R1 @ v2dot,
            dR1 @ v3 * dq1 + R1 @ v3dot,
        ))
        return Jd

    def foot_velocity(self, q: np.ndarray, dq: np.ndarray) -> np.ndarray:
        return self.jacobian(q) @ np.asarray(dq, dtype=float).reshape(3)

    # -- statics ----------------------------------------------------------
    def tau_from_force(self, q: np.ndarray, f: np.ndarray) -> np.ndarray:
        """Joint torques that produce a foot force: ``tau = J^T f``."""
        J = self.jacobian(q)
        return J.T @ np.asarray(f, dtype=float).reshape(3)

    def force_from_tau(self, q: np.ndarray, tau: np.ndarray) -> np.ndarray:
        """Foot force produced by joint torques: solve ``J^T f = tau``."""
        J = self.jacobian(q)
        tau = np.asarray(tau, dtype=float).reshape(3)
        try:
            return np.linalg.solve(J.T, tau)
        except np.linalg.LinAlgError:
            return np.linalg.lstsq(J.T, tau, rcond=None)[0]

    # -- helpers ----------------------------------------------------------
    @staticmethod
    def _wrap(a: float) -> float:
        return (a + math.pi) % (2.0 * math.pi) - math.pi

    @staticmethod
    def _skew_x() -> np.ndarray:
        return np.array([[0.0, 0.0, 0.0],
                         [0.0, 0.0, -1.0],
                         [0.0, 1.0, 0.0]], dtype=float)

    def _canonical(self, value: float, index: int) -> float:
        """Pick the 2*pi-equivalent angle inside the joint range."""
        best = value
        best_cost = (self._range_violation(value, index), abs(value))
        for k in (-2, -1, 1, 2):
            candidate = value + 2.0 * math.pi * k
            cost = (self._range_violation(candidate, index), abs(candidate))
            if cost < best_cost:
                best, best_cost = candidate, cost
        return best

    def _range_violation(self, value: float, index: int) -> float:
        lo, hi = self.q_min[index], self.q_max[index]
        if value < lo:
            return lo - value
        if value > hi:
            return value - hi
        return 0.0

    def __repr__(self) -> str:
        return (f"Leg({self.leg_id.name}, l_thigh={self.l_thigh:.3f}, "
                f"l_calf={self.l_calf:.3f})")


__all__ = ["Leg"]
