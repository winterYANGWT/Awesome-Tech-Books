"""Balance controller: pose PD, friction pyramid and QP foot-force allocation.

The chain implemented here is the one of Chapter 8:

``pose error -> desired base acceleration -> QP foot forces -> joint torque``

The QP solves

.. math::

   \\min_f \\; \\tfrac12 f^T (A^T S A + \\alpha W + \\beta U) f
   - (b^T S A + f_{prev}^T \\beta U) f

subject to the friction pyramid for every stance foot and ``f_i = 0`` for
every swing foot.  The dynamics matrix is

.. math::

   A = [I I I I; [p_0]_x ... [p_3]_x], \\quad
   b = [m (a_des - g); R I_b R^T \\alpha_des].

The solver is OSQP.  A small regularization and a fallback "gravity split"
keep the controller alive when the solver fails.
"""

from __future__ import annotations

import numpy as np

try:
    import osqp
    import scipy.sparse as sp
    _HAS_OSQP = True
except ImportError:  # pragma: no cover - environment error path
    _HAS_OSQP = False

from .rotation import skew, so3_log
from .types import JointCmd

GRAVITY = np.array([0.0, 0.0, -9.81])


class BalanceCtrl:
    """QP-based ground reaction force allocation.

    Parameters
    ----------
    mass:
        Equivalent single-rigid-body mass (kg), not necessarily the total
        mass of the robot (Section 8.5).
    inertia:
        Equivalent body inertia in the body frame (kg m^2).
    gravity:
        Gravity vector in the world frame.
    fric_mu:
        Friction coefficient of the linearised (pyramid) cone.
    f_max:
        Upper bound on the normal force of one foot.
    w_dynamics, w_force, w_delta:
        Weights of the three cost terms.  ``w_force`` and ``w_delta`` scale
        ``W`` and ``U`` directly (``alpha`` and ``beta`` in the book).
        ``w_dynamics`` builds the 6x6 dynamics weight when ``S`` is not
        supplied: ``S = w_dynamics * I6``.  A caller-supplied ``S`` takes
        precedence, so ``w_dynamics`` is ignored in that case.
    S:
        Optional full 6x6 dynamics weight matrix.  When both ``S`` and
        ``w_dynamics`` are given, ``S`` wins; ``w_dynamics`` is only the
        shorthand for ``S = w_dynamics * I6``.
    W, U:
        Optional 12x12 force-magnitude and force-change weight matrices;
        ``w_force`` and ``w_delta`` multiply them.
    """

    def __init__(self, mass: float = 12.0,
                 inertia: np.ndarray | None = None,
                 gravity: np.ndarray | None = None,
                 fric_mu: float = 0.6, f_max: float = 90.0,
                 w_dynamics: float = 1.0, w_force: float = 0.02,
                 w_delta: float = 0.01,
                 S: np.ndarray | None = None,
                 W: np.ndarray | None = None,
                 U: np.ndarray | None = None,
                 solver_eps: float = 1e-5) -> None:
        self.mass = float(mass)
        self.inertia = (np.diag([0.1320, 0.3475, 0.3775]) if inertia is None
                        else np.asarray(inertia, dtype=float).reshape(3, 3))
        self.gravity = (GRAVITY.copy() if gravity is None
                        else np.asarray(gravity, dtype=float).reshape(3))
        self.fric_mu = float(fric_mu)
        self.f_max = float(f_max)
        self.w_dynamics = float(w_dynamics)
        self.w_force = float(w_force)
        self.w_delta = float(w_delta)

        if S is None:
            # w_dynamics is the scalar shorthand for S = w_dynamics * I6.
            # An explicit S overrides it, so existing callers that pass the
            # full matrix keep exactly the same P as before.
            S = self.w_dynamics * np.eye(6)
        if W is None:
            W = np.eye(12)
        if U is None:
            U = np.eye(12)
        self.S = np.asarray(S, dtype=float).reshape(6, 6)
        self.W = np.asarray(W, dtype=float).reshape(12, 12)
        self.U = np.asarray(U, dtype=float).reshape(12, 12)
        self.solver_eps = float(solver_eps)

        # 5x3 friction pyramid of Eq. (8.24): F f >= 0 with
        # F = [[1,0,mu],[-1,0,mu],[0,1,mu],[0,-1,mu],[0,0,1]].
        mu = self.fric_mu
        self.fric_mat = np.array([[1.0, 0.0, mu],
                                  [-1.0, 0.0, mu],
                                  [0.0, 1.0, mu],
                                  [0.0, -1.0, mu],
                                  [0.0, 0.0, 1.0]])
        self.x_prev = np.zeros(12)
        self.last_forces = np.zeros((4, 3))
        self.last_status = 0
        self._solver = None
        self._last_contact = None
        self._P_key = None

        # Pose gains.  Position: critically damped around 10 rad/s.
        self.Kp = np.diag([100.0, 100.0, 400.0])
        self.Kd = np.diag([20.0, 20.0, 40.0])
        self.kp_rot = np.array([150.0, 150.0, 50.0])
        self.Kd_rot = np.diag([18.0, 18.0, 12.0])

    # ------------------------------------------------------------------
    # Pose feedback
    # ------------------------------------------------------------------
    def desired_acceleration(self, p_des: np.ndarray, v_des: np.ndarray,
                             p: np.ndarray, v: np.ndarray,
                             R_des: np.ndarray, R: np.ndarray,
                             omega_des: np.ndarray | None = None,
                             omega: np.ndarray | None = None,
                             a_ff: np.ndarray | None = None,
                             alpha_ff: np.ndarray | None = None
                             ) -> tuple[np.ndarray, np.ndarray]:
        """Eqs. (8.17) and (8.22): pose PD -> desired accelerations."""
        p_des = np.asarray(p_des, dtype=float).reshape(3)
        v_des = np.asarray(v_des, dtype=float).reshape(3)
        p = np.asarray(p, dtype=float).reshape(3)
        v = np.asarray(v, dtype=float).reshape(3)
        omega_des = (np.zeros(3) if omega_des is None
                     else np.asarray(omega_des, dtype=float).reshape(3))
        omega = (np.zeros(3) if omega is None
                 else np.asarray(omega, dtype=float).reshape(3))
        a = self.Kp @ (p_des - p) + self.Kd @ (v_des - v)
        if a_ff is not None:
            a = a + np.asarray(a_ff, dtype=float).reshape(3)

        R_des = np.asarray(R_des, dtype=float).reshape(3, 3)
        R = np.asarray(R, dtype=float).reshape(3, 3)
        e_R = so3_log(R_des @ R.T)
        # ``kp_rot`` may be a scalar or a per-axis vector; the yaw axis is
        # deliberately softer because foot contact can only supply a yaw
        # moment through tangential friction (see README).
        kp_rot = np.asarray(self.kp_rot, dtype=float)
        rot_term = kp_rot @ e_R if kp_rot.ndim == 2 else kp_rot * e_R
        alpha = rot_term + self.Kd_rot @ (omega_des - omega)
        if alpha_ff is not None:
            alpha = alpha + np.asarray(alpha_ff, dtype=float).reshape(3)
        return a, alpha

    # ------------------------------------------------------------------
    # QP assembly and solve
    # ------------------------------------------------------------------
    def dynamics_matrix(self, feet_rel_com: np.ndarray) -> np.ndarray:
        """6x12 matrix ``A`` of Eq. (8.34)."""
        feet = np.asarray(feet_rel_com, dtype=float).reshape(4, 3)
        A = np.zeros((6, 12))
        A[0:3, :] = np.tile(np.eye(3), (1, 4))
        for i in range(4):
            A[3:6, 3 * i:3 * i + 3] = skew(feet[i])
        return A

    def dynamics_rhs(self, a_des: np.ndarray, alpha_des: np.ndarray,
                     R: np.ndarray) -> np.ndarray:
        """6-vector ``b`` of Eq. (8.34)."""
        a = np.asarray(a_des, dtype=float).reshape(3)
        alpha = np.asarray(alpha_des, dtype=float).reshape(3)
        R = np.asarray(R, dtype=float).reshape(3, 3)
        b = np.zeros(6)
        b[0:3] = self.mass * (a - self.gravity)
        b[3:6] = R @ self.inertia @ R.T @ alpha
        return b

    def _assemble(self, A: np.ndarray, b: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
        P = A.T @ self.S @ A + self.w_force * self.W + self.w_delta * self.U
        q = -A.T @ self.S @ b - self.w_delta * self.U @ self.x_prev
        # Numerical regularization; does not change the solution materially.
        P = P + np.eye(12) * 1e-8
        return 0.5 * (P + P.T), q

    def solve_force(self, a_des: np.ndarray, alpha_des: np.ndarray,
                    R: np.ndarray, feet_rel_com: np.ndarray,
                    contact: np.ndarray) -> np.ndarray:
        """Solve the QP and return the four world-frame ground forces (4,3).

        ``feet_rel_com`` is ``p_foot - p_com`` in the world frame.  The sign
        convention is "force of the ground on the foot": a standing robot has
        four positive vertical components summing to ``m g``.
        """
        feet = np.asarray(feet_rel_com, dtype=float).reshape(4, 3)
        contact = np.asarray(contact, dtype=bool).reshape(4)
        n_stance = int(contact.sum())
        if n_stance == 0:
            self.last_forces = np.zeros((4, 3))
            self.last_status = -1
            return self.last_forces.copy()

        A = self.dynamics_matrix(feet)
        b = self.dynamics_rhs(a_des, alpha_des, R)
        P, q = self._assemble(A, b)

        # Constraints: l <= C x <= u.
        rows = []
        lows = []
        highs = []
        for i in range(4):
            sl = slice(3 * i, 3 * i + 3)
            if contact[i]:
                F = self.fric_mat.copy()
                # f_z <= f_max (add an explicit upper bound row)
                F = np.vstack([F, np.array([0.0, 0.0, 1.0])])
                block = np.zeros((6, 12))
                block[:, sl] = F
                rows.append(block)
                lows.extend([0.0] * 5 + [-np.inf])
                highs.extend([np.inf] * 5 + [self.f_max])
            else:
                block = np.zeros((3, 12))
                block[:, sl] = np.eye(3)
                rows.append(block)
                lows.extend([0.0] * 3)
                highs.extend([0.0] * 3)
        C = np.vstack(rows)
        l = np.asarray(lows)
        u = np.asarray(highs)

        if not _HAS_OSQP:
            forces = self._fallback(a_des, contact)
            self.last_forces = forces
            self.last_status = -2
            return forces.copy()

        # OSQP expects the upper-triangular part of P.  Build it explicitly so
        # the sparsity pattern (and hence the data length) is invariant.
        P_up = sp.triu(sp.csc_matrix(P)).tocsc()
        C_sp = sp.csc_matrix(C)
        try:
            if self._solver is None or self._P_key != (n_stance, contact.tobytes()):
                self._solver = osqp.OSQP()
                self._solver.setup(P_up, q, C_sp, l, u, verbose=False,
                                   eps_abs=self.solver_eps,
                                   eps_rel=self.solver_eps,
                                   max_iter=4000, polishing=False,
                                   warm_starting=True)
                self._P_key = (n_stance, contact.tobytes())
            else:
                self._solver.update(Px=P_up.data, q=q,
                                    Ax=C_sp.data, l=l, u=u,
                                    raise_error=False)
            res = self._solver.solve(raise_error=False)
            status = int(res.info.status_val)
            if status in (1, 2) and res.x is not None:
                f = np.asarray(res.x, dtype=float)
                # Clip tiny constraint violations caused by the solver
                # tolerance (negative normal force or f_z above f_max).
                for i in range(4):
                    if contact[i]:
                        f[3 * i + 2] = min(self.f_max,
                                           max(0.0, f[3 * i + 2]))
                        # Project the tangential force onto the pyramid
                        # boundary.  OSQP may return a force a few 1e-4 N
                        # outside the cone because of its tolerance; clipping
                        # makes the commanded force exactly feasible.
                        tangent = f[3 * i:3 * i + 2]
                        limit = self.fric_mu * f[3 * i + 2]
                        norm = float(np.linalg.norm(tangent))
                        if norm > limit and norm > 0.0:
                            f[3 * i:3 * i + 2] = tangent * (limit / norm)
                forces = f.reshape(4, 3)
                # The QP equalities are satisfied only to solver tolerance;
                # make the swing-foot force exactly zero for the controller.
                forces[~contact] = 0.0
            else:
                forces = self._fallback(a_des, contact)
        except Exception:
            forces = self._fallback(a_des, contact)
            status = -1

        self.x_prev = forces.reshape(12).copy()
        self.last_forces = forces.copy()
        self.last_status = status if 'status' in locals() else -1
        return forces.copy()

    def _fallback(self, a_des: np.ndarray, contact: np.ndarray) -> np.ndarray:
        """Keep the robot upright if the QP fails: split gravity + m a_z."""
        forces = np.zeros((4, 3))
        stance = np.flatnonzero(contact)
        if stance.size == 0:
            return forces
        fz = self.mass * (-self.gravity[2] + float(np.asarray(a_des).reshape(3)[2]))
        fz = max(0.0, fz) / stance.size
        forces[stance, 2] = fz
        return forces

    # ------------------------------------------------------------------
    # Torque mapping
    # ------------------------------------------------------------------
    def to_joint_torque(self, q: np.ndarray, forces_world: np.ndarray,
                        R: np.ndarray, robot) -> np.ndarray:
        """Eq. (8.39): ``tau_i = -J_i^T R^T f_i`` for each leg."""
        q = np.asarray(q, dtype=float).reshape(12)
        f = np.asarray(forces_world, dtype=float).reshape(4, 3)
        R = np.asarray(R, dtype=float).reshape(3, 3)
        tau = np.zeros(12)
        for i in range(4):
            J = robot.leg_jacobian(q, i)
            tau[3 * i:3 * i + 3] = -J.T @ (R.T @ f[i])
        return tau

    # ------------------------------------------------------------------
    # One-call helper used by the states
    # ------------------------------------------------------------------
    def step(self, q, p_des, v_des, R_des, p, v, R, omega,
             feet_world, com_world, contact,
             omega_des=None, a_ff=None, alpha_ff=None, robot=None):
        """Full pose-PD + QP + torque chain.

        ``feet_world`` is (4,3) world foot positions; ``com_world`` is the
        world centre of mass used for the moment arms.  Returns
        ``(forces, tau, a_des, alpha_des)``.
        """
        a_des, alpha_des = self.desired_acceleration(
            p_des, v_des, p, v, R_des, R, omega_des, omega, a_ff, alpha_ff)
        feet_rel = np.asarray(feet_world, dtype=float).reshape(4, 3) - np.asarray(
            com_world, dtype=float).reshape(3)
        forces = self.solve_force(a_des, alpha_des, R, feet_rel, contact)
        model = robot if robot is not None else getattr(self, "_robot", None)
        if model is None:
            raise ValueError("BalanceCtrl.step needs a robot model")
        tau = self.to_joint_torque(np.asarray(q, dtype=float).reshape(12),
                                   forces, R, model)
        return forces, tau, a_des, alpha_des

    def set_robot(self, robot) -> None:
        self._robot = robot


__all__ = ["BalanceCtrl", "GRAVITY"]
