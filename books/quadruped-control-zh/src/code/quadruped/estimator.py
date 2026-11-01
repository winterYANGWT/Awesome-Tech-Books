"""State estimator for the A1: IMU model, 18-state Kalman filter and covariance
statistics.

The filter follows the formulation of Chapter 7 of the reference book.  The
state is

``x = [p_b, v_b, p_f0, p_f1, p_f2, p_f3]``  (18 values),

the input is the world-frame base acceleration obtained from the IMU,

``u = R_sb a_b + g``  (3 values),

and the measurement is the 28-vector made of the four foot positions relative
to the base (12), the four foot velocities relative to the base (12) and the
four foot heights (4).  All foot quantities are expressed in the world frame.

The implementation does not use MuJoCo; the caller supplies the base state
(IMU readings and attitude) and the joint state.  The kinematics come from the
shared :class:`~quadruped.robot.Quadruped` model.

The covariance ``R`` is adapted per foot: a swinging foot cannot use the
"world-fixed foot" assumption, so its position/velocity/height blocks are
inflated to a large value (the book's 1e6 trick).  This keeps the filter from
being dragged by a foot that is in the air.
"""

from __future__ import annotations

import numpy as np

from .types import BaseState, JointState

GRAVITY = np.array([0.0, 0.0, -9.81])


# ---------------------------------------------------------------------------
# IMU model
# ---------------------------------------------------------------------------
class IMU:
    """Simple MEMS IMU model: constant bias plus white Gaussian noise.

    ``measure`` takes the ideal body-frame proper acceleration and angular
    rate and returns the sensor readings.  The accelerometer convention is the
    one of Eq. (7.6): at rest the vertical reading is ``+g``.
    """

    def __init__(self, acc_bias: np.ndarray | None = None,
                 gyro_bias: np.ndarray | None = None,
                 acc_std: float = 0.0, gyro_std: float = 0.0,
                 seed: int | None = 0) -> None:
        self.acc_bias = (np.zeros(3) if acc_bias is None
                         else np.asarray(acc_bias, dtype=float).reshape(3))
        self.gyro_bias = (np.zeros(3) if gyro_bias is None
                          else np.asarray(gyro_bias, dtype=float).reshape(3))
        self.acc_std = float(acc_std)
        self.gyro_std = float(gyro_std)
        self._rng = np.random.default_rng(seed)

    def measure(self, acc_b: np.ndarray, omega_b: np.ndarray
                ) -> tuple[np.ndarray, np.ndarray]:
        acc = np.asarray(acc_b, dtype=float).reshape(3)
        gyro = np.asarray(omega_b, dtype=float).reshape(3)
        if self.acc_std > 0.0:
            acc = acc + self._rng.normal(0.0, self.acc_std, 3)
        if self.gyro_std > 0.0:
            gyro = gyro + self._rng.normal(0.0, self.gyro_std, 3)
        return acc + self.acc_bias, gyro + self.gyro_bias

    def world_acceleration(self, acc_meas: np.ndarray,
                           R_sb: np.ndarray) -> np.ndarray:
        """Eq. (7.7): rotate the reading and add gravity."""
        return np.asarray(R_sb, dtype=float).reshape(3, 3) @ np.asarray(
            acc_meas, dtype=float).reshape(3) + GRAVITY


# ---------------------------------------------------------------------------
# Recursive mean and covariance
# ---------------------------------------------------------------------------
class AvgCov:
    """Recursive mean/covariance of a vector signal (Section 7.8.1).

    Only the current mean and covariance are stored, so the memory cost is
    O(dim^2) and the filter can run on an embedded computer.  The parameters
    mirror the reference implementation: ``start_count`` samples are skipped,
    results are printed every ``print_period`` updates when ``print_min`` is
    true, and ``scale`` multiplies the printed values.
    """

    def __init__(self, dim: int, name: str = "", print_min: bool = True,
                 print_period: int = 1000, start_count: int = 0,
                 scale: float = 1.0) -> None:
        self.dim = int(dim)
        self.name = str(name)
        self.print_min = bool(print_min)
        self.print_period = int(print_period)
        self.start_count = int(start_count)
        self.scale = float(scale)
        self.count = 0
        self.mean = np.zeros(self.dim)
        self.cov = np.zeros((self.dim, self.dim))
        self._m2 = np.zeros((self.dim, self.dim))

    def reset(self) -> None:
        self.count = 0
        self.mean[:] = 0.0
        self.cov[:] = 0.0
        self._m2[:] = 0.0

    def update(self, value: np.ndarray) -> None:
        x = np.asarray(value, dtype=float).reshape(self.dim)
        self.count += 1
        if self.count <= self.start_count:
            return
        n = self.count - self.start_count
        delta = x - self.mean
        self.mean += delta / n
        self._m2 += np.outer(delta, x - self.mean)
        if n > 1:
            self.cov = self._m2 / (n - 1)
        if self.print_min and self.print_period > 0 and n % self.print_period == 0:
            self.report()

    measure = update

    def report(self) -> None:
        print(f"[{self.name}] n={self.count} "
              f"mean={np.array2string(self.mean * self.scale, precision=4)} "
              f"cov_diag={np.array2string(np.diag(self.cov) * self.scale ** 2, precision=4)}")

    @property
    def std(self) -> np.ndarray:
        return np.sqrt(np.maximum(np.diag(self.cov), 0.0))


# ---------------------------------------------------------------------------
# 18-state Kalman filter
# ---------------------------------------------------------------------------
class Estimator:
    """Discrete Kalman filter of Section 7.7, specialised to the A1.

    Parameters
    ----------
    robot:
        A :class:`~quadruped.robot.Quadruped`; used for the foot kinematics.
    dt:
        Nominal control period in seconds (2 ms in the book).
    R0, Cu, Qdig:
        Initial measurement covariance, input covariance and the extra
        diagonal process noise.  The defaults are the values used by the
        package; see ``README.md`` for the tuning notes.
    """

    N_STATE = 18
    N_MEAS = 28
    N_INPUT = 3

    def __init__(self, robot=None, dt: float = 0.002,
                 R0: np.ndarray | None = None,
                 Cu: np.ndarray | None = None,
                 Qdig: np.ndarray | None = None,
                 foot_radius: float | None = None,
                 contact_height: float | None = None) -> None:
        self.robot = robot
        self.dt = float(dt)
        params = getattr(robot, "params", None)
        if foot_radius is None:
            foot_radius = float(getattr(params, "foot_radius", 0.02))
        self.foot_radius = float(foot_radius)
        if contact_height is None:
            # Centre of a foot sphere resting on the ground.  In MuJoCo the
            # geom's contact margin adds to the radius; on hardware the margin
            # is zero and only the rubber radius remains.
            margin = float(getattr(params, "foot_margin", 0.0))
            contact_height = self.foot_radius + margin
        self.contact_height = float(contact_height)

        # The process noise input covariance: IMU noise mapped through B.
        self.Cu = (np.diag([0.02, 0.02, 0.02]) if Cu is None
                   else np.asarray(Cu, dtype=float).reshape(3, 3))
        # Extra diagonal process noise (book's _Qdig): small for the base,
        # larger for the four foot positions.
        if Qdig is None:
            Qdig = np.array([1e-8] * 3 + [1e-5] * 3 + [2e-5] * 12)
        self.Qdig = np.asarray(Qdig, dtype=float).reshape(18)

        # Measurement covariance (diagonal).  Foot position is very reliable;
        # the joint-velocity difference that produces the relative velocity is
        # noisier; the artificial zero-height measurement gets a large sigma.
        if R0 is None:
            R0 = np.concatenate([
                np.full(12, 2.0e-6),      # foot relative position
                np.full(12, 5.0e-4),      # foot relative velocity
                np.full(4, 5.0e-3),       # foot height
            ])
        self.R0 = np.asarray(R0, dtype=float).reshape(28)
        self.R_nominal = self.R0.copy()

        self.x = np.zeros(18)
        self.P = np.eye(18) * 1e-4
        self.initialized = False
        self.last_contact = np.zeros(4, dtype=bool)
        # Optional covariance measurement of Section 7.10 (_RCheck/_uCheck).
        self.R_check = AvgCov(28, "estimator R", print_min=False)
        self.u_check = AvgCov(3, "estimator u", print_min=False)
        self.collect_covariance = False
        self.H = np.zeros((28, 18))
        self._build_H()
        self._build_F()

    # -- model matrices ---------------------------------------------------
    def _build_F(self) -> None:
        A = np.zeros((18, 18))
        A[0:3, 3:6] = np.eye(3)
        B = np.zeros((18, 3))
        B[3:6, :] = np.eye(3)
        self.A = A
        self.B = B
        self.F = np.eye(18) + self.dt * A
        self.G = self.dt * B

    def _build_H(self) -> None:
        H = np.zeros((28, 18))
        eye = np.eye(3)
        # Four foot positions relative to the base: p_i - p_b.
        for i in range(4):
            H[3 * i:3 * i + 3, 0:3] = -eye
            H[3 * i:3 * i + 3, 6 + 3 * i:9 + 3 * i] = eye
        # Four foot relative velocities: -v_b under the contact assumption.
        for i in range(4):
            H[12 + 3 * i:15 + 3 * i, 3:6] = -eye
        # Four foot heights: the z component of p_i.
        for i in range(4):
            H[24 + i, 6 + 3 * i + 2] = 1.0
        self.H = H

    # -- initialization ---------------------------------------------------
    def reset(self, base: BaseState, joints: JointState,
              contact: np.ndarray | None = None) -> None:
        """Initialize the filter from sensor readings, not from truth.

        The horizontal origin is arbitrary (the filter cannot observe an
        absolute x/y position without an external reference), so it is set to
        zero.  On hardware this corresponds to declaring the start point the
        origin; a surveyed start pose could be substituted without changing
        the filter.  The initial height uses the leg kinematics plus the
        assumption that a foot reporting contact sits on the ground: this is
        the same information the measurement model uses, only at ``t=0``.
        Velocity starts at zero (the robot is standing still at entry).

        ``base.position`` and ``base.velocity`` are intentionally **not**
        read.  For the controllers these fields are zeros or stale placeholders;
        in the simulator they would be ground truth, which the estimator must
        not use.
        """
        R = base.rotation()
        feet_b = self._feet_body(joints.q)
        contact = (np.zeros(4, dtype=bool) if contact is None
                   else np.asarray(contact, dtype=bool).reshape(4))
        p0 = np.zeros(3)
        if np.any(contact):
            # Foot-sphere centre height under the contact assumption.
            p0[2] = float(np.mean(
                self.contact_height - (R @ feet_b[contact].T)[2]))
        feet_s = (R @ feet_b.T).T + p0
        self.x[0:3] = p0
        self.x[3:6] = 0.0
        self.x[6:18] = feet_s.reshape(12)
        self.P = np.eye(18)
        self.P[0:3, 0:3] *= 1e-4
        self.P[3:6, 3:6] *= 1e-2
        self.P[6:18, 6:18] *= 1e-4
        self.initialized = True
        self.last_contact = contact.copy()

    def _feet_body(self, q: np.ndarray) -> np.ndarray:
        if self.robot is not None:
            return np.asarray(self.robot.fk_all(q), dtype=float)
        q = np.asarray(q, dtype=float).reshape(12)
        return np.tile(np.array([0.183, 0.132, -0.249]), (4, 1))

    # -- measurement construction ----------------------------------------
    def measurement(self, base: BaseState, joints: JointState,
                    contact: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
        """Build ``y`` (28,) and the diagonal ``R`` from the sensors."""
        q = np.asarray(joints.q, dtype=float).reshape(12)
        dq = np.asarray(joints.dq, dtype=float).reshape(12)
        R_sb = base.rotation()
        omega = np.asarray(base.omega_b, dtype=float).reshape(3)
        feet_b = self._feet_body(q)
        feet_rel = (R_sb @ feet_b.T).T
        y = np.zeros(28)
        y[0:12] = feet_rel.reshape(12)

        if self.robot is not None:
            for i in range(4):
                J = self.robot.leg_jacobian(q, i)
                v_leg = np.cross(omega, feet_b[i]) + J @ dq[3 * i:3 * i + 3]
                y[12 + 3 * i:15 + 3 * i] = R_sb @ v_leg
        else:
            # No leg model: fall back to the static-foot assumption.  The
            # relative foot velocity is then zero, not -base.velocity (the
            # base velocity is not measured and must never enter the filter).
            for i in range(4):
                y[12 + 3 * i:15 + 3 * i] = 0.0

        # The four feet are assumed to touch the ground; the foot-sphere
        # centre then sits at z = contact_height (radius plus the model's
        # contact margin, which is zero on hardware).  This is the constant
        # "zero height" measurement of the book, expressed for the sphere.
        y[24:28] = self.contact_height

        contact = np.asarray(contact, dtype=bool).reshape(4)
        R = self.R_nominal.copy()
        for i in range(4):
            if not contact[i]:
                # The foot is in the air: do not believe its "fixed" position,
                # relative velocity or zero height.
                R[3 * i:3 * i + 3] = 1e6
                R[12 + 3 * i:15 + 3 * i] = 1e6
                R[24 + i] = 1e6
        return y, R

    # -- filter step ------------------------------------------------------
    def step(self, base: BaseState, joints: JointState,
             contact: np.ndarray | None = None, dt: float | None = None) -> BaseState:
        """Run one predict/update cycle and return the estimated base state."""
        if not self.initialized:
            self.reset(base, joints, contact)
        contact = (np.asarray(contact, dtype=bool).reshape(4)
                   if contact is not None else self.last_contact)
        dd = self.dt if dt is None else float(dt)
        if dd != self.dt:
            # Rebuild the discretization for a non-nominal period.
            self.dt = dd
            self._build_F()

        R_sb = base.rotation()
        u = R_sb @ np.asarray(base.acc_b, dtype=float).reshape(3) + GRAVITY

        # --- time update
        self.x = self.F @ self.x + self.G @ u
        Q = self.G @ self.Cu @ self.G.T + np.diag(self.Qdig)
        self.P = self.F @ self.P @ self.F.T + Q
        self.P = 0.5 * (self.P + self.P.T)

        # --- measurement update
        y, Rdiag = self.measurement(base, joints, contact)
        if self.collect_covariance:
            self.R_check.update(y)
            self.u_check.update(u)
        H = self.H
        yhat = H @ self.x
        S = H @ self.P @ H.T + np.diag(Rdiag)
        # Solve S K^T = P H^T instead of forming an inverse.
        PHt = self.P @ H.T
        try:
            Kt = np.linalg.solve(S, PHt.T)
        except np.linalg.LinAlgError:
            Kt = np.linalg.lstsq(S, PHt.T, rcond=None)[0]
        K = Kt.T
        self.x = self.x + K @ (y - yhat)
        IKH = np.eye(18) - K @ H
        # Joseph form keeps P symmetric positive semi-definite.
        self.P = IKH @ self.P @ IKH.T + K @ np.diag(Rdiag) @ K.T
        self.P = 0.5 * (self.P + self.P.T)
        self.last_contact = contact.copy()

        return self.estimated_base(base)

    def estimated_base(self, base: BaseState) -> BaseState:
        out = base.copy()
        out.position = self.x[0:3].copy()
        out.velocity = self.x[3:6].copy()
        return out

    # -- getters used by the controller ----------------------------------
    @property
    def position(self) -> np.ndarray:
        return self.x[0:3].copy()

    @property
    def velocity(self) -> np.ndarray:
        return self.x[3:6].copy()

    def foot_position(self, leg: int, base: BaseState | None = None,
                      joints: JointState | None = None) -> np.ndarray:
        """World foot position (Eq. 7.88).

        When base and joint measurements are supplied, forward kinematics is
        used, which is valid for both stance and swing feet.  ``base`` must be
        an **estimated** state (or an IMU-only placeholder); passing the
        simulator truth here would leak it into the controller.  Otherwise the
        filter state is returned.
        """
        if base is not None and joints is not None:
            R = base.rotation()
            p_body = self._feet_body(joints.q)[int(leg)]
            return base.position + R @ p_body
        i = int(leg)
        return self.x[6 + 3 * i:9 + 3 * i].copy()

    def feet_positions(self, base: BaseState | None = None,
                       joints: JointState | None = None) -> np.ndarray:
        return np.vstack([self.foot_position(i, base, joints) for i in range(4)])

    def feet_velocities(self, base: BaseState, joints: JointState) -> np.ndarray:
        """World foot velocities from kinematics (Eq. 7.89).

        ``base.velocity`` must be the estimator output, not simulator truth:
        the controller has no direct base-velocity measurement.
        """
        R = base.rotation()
        omega = np.asarray(base.omega_b, dtype=float).reshape(3)
        v_base = np.asarray(base.velocity, dtype=float).reshape(3)
        feet_b = self._feet_body(joints.q)
        out = np.zeros((4, 3))
        for i in range(4):
            J = self.robot.leg_jacobian(joints.q, i) if self.robot is not None else np.zeros((3, 3))
            v_rel = np.cross(omega, feet_b[i]) + J @ joints.dq[3 * i:3 * i + 3]
            out[i] = v_base + R @ v_rel
        return out

    # -- covariance measurement (Section 7.10) ---------------------------
    def start_covariance(self, start_count: int = 5000,
                         print_period: int = 1000) -> None:
        self.R_check.reset()
        self.u_check.reset()
        self.R_check.start_count = int(start_count)
        self.u_check.start_count = int(start_count)
        self.R_check.print_period = int(print_period)
        self.u_check.print_period = int(print_period)
        self.collect_covariance = True

    def stop_covariance(self) -> tuple[np.ndarray, np.ndarray]:
        self.collect_covariance = False
        return self.measured_R(), self.measured_Cu()

    def measured_R(self) -> np.ndarray:
        return self.R_check.cov.copy()

    def measured_Cu(self) -> np.ndarray:
        return self.u_check.cov.copy()

    # -- names used by the reference implementation ----------------------
    def getPosition(self) -> np.ndarray:
        return self.position

    def getVelocity(self) -> np.ndarray:
        return self.velocity

    def getFootPos(self, leg: int) -> np.ndarray:
        return self.foot_position(leg)

    def getFeetPos(self) -> np.ndarray:
        return self.x[6:18].reshape(4, 3).copy()

    def getFeetVel(self, base: BaseState, joints: JointState) -> np.ndarray:
        return self.feet_velocities(base, joints)


__all__ = ["IMU", "AvgCov", "Estimator", "GRAVITY"]
