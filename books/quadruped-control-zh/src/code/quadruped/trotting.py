"""Walking controller states: ``FreeStand`` and ``TrottingController``.

Both states consume only :meth:`~quadruped.sim.A1Sim.read_sensors`: IMU
attitude/rate/acceleration, joint encoders and foot contacts.  The base
position and velocity come from the Chapter 7 estimator, never from the
simulator's ground truth.

``FreeStand`` holds the body at a commanded pose with all four feet on the
ground.  The ground forces come from :class:`~quadruped.balance.BalanceCtrl`;
the joints receive the feed-forward torque plus light damping.

``TrottingController`` is the state of Chapter 10.  It combines the estimator,
the gait generator, the balance QP for the stance legs and a Cartesian PD for
the swing legs.  The public command is ``(vx, vy, yaw_rate)`` in the body
frame plus body height and step height; ``run`` executes one 2 ms cycle and
returns the joint command.
"""

from __future__ import annotations

import numpy as np

from .balance import BalanceCtrl
from .estimator import Estimator
from .fsm import State, StateID
from .gait import GaitGenerator, GaitParams, WaveStatus
from .rotation import rpy_to_R
from .types import JointCmd, MotorMode


class FreeStand(State):
    """Balanced stand with roll/pitch/yaw/height commands (QP force control)."""

    def __init__(self, context, robot=None, params=None, balance=None,
                 stand_height: float = 0.27,
                 estimator: Estimator | None = None) -> None:
        super().__init__(context, StateID.FREE_STAND)
        self.robot = robot
        self.params = params
        self.balance = balance or BalanceCtrl(
            mass=(params.mass if params is not None else 12.45),
            inertia=(params.body_equivalent_inertia(params.home_q)
                     if params is not None else None))
        if robot is not None:
            self.balance.set_robot(robot)
        self.estimator = estimator or Estimator(robot=robot, dt=context.dt)
        self.stand_height = float(stand_height)
        self.p0 = np.zeros(3)
        self.yaw0 = 0.0
        self.p_des = np.zeros(3)
        self.roll = 0.0
        self.pitch = 0.0
        self.yaw = 0.0
        self.dx = 0.0
        self.dy = 0.0
        self.height = self.stand_height
        self.feet_world = np.zeros((4, 3))
        self.forces = np.zeros((4, 3))
        self.tau = np.zeros(12)
        self.last_q = np.zeros(12)
        # Joint tracking uses the nominal stance gains.  A slow integral trim
        # on the commanded height removes the few millimetres of droop caused
        # by leg/contact compliance.  It is driven only by the estimated
        # height, so it is available on the real robot as well.
        self.kp_leg = np.array([60.0, 150.0, 200.0] * 4)
        self.kd_leg = np.array([2.0, 3.0, 3.0] * 4)
        self.z_trim_ki = 2.0
        self._z_trim = 0.0
        # Slow horizontal integral trim, bounded to 8 cm per axis.  It
        # removes the small standing offset left by foot slip without turning
        # the pose loop into a position servo against the simulator truth.
        self.xy_trim_ki = 2.0
        self._xy_trim = np.zeros(2)
        self._xy_trim_max = 0.08

    # -- command interface ----------------------------------------------
    def set_pose(self, roll: float | None = None, pitch: float | None = None,
                 yaw: float | None = None, height: float | None = None,
                 dx: float | None = None, dy: float | None = None) -> None:
        if roll is not None:
            self.roll = float(roll)
        if pitch is not None:
            self.pitch = float(pitch)
        if yaw is not None:
            self.yaw = float(yaw)
        if height is not None:
            self.height = float(height)
        if dx is not None:
            self.dx = float(dx)
        if dy is not None:
            self.dy = float(dy)

    # -- state interface -------------------------------------------------
    def _feet_world(self, est, q: np.ndarray) -> np.ndarray:
        """Feet in the estimator's world frame (FK + estimated base pose)."""
        if self.robot is None:
            return np.zeros((4, 3))
        R = est.rotation()
        return est.position + (R @ self.robot.fk_all(q).T).T

    def enter(self) -> None:
        super().enter()
        sensors = self.sim.read_sensors()
        joints = sensors.joints
        self.estimator.reset(sensors.base_state(), joints, sensors.contact)
        est = self.estimator.estimated_base(sensors.base_state())
        # The estimator defines the horizontal origin; on hardware this is
        # the declared start point.  No simulated base position is read.
        self.p0 = est.position.copy()
        self.p_des = est.position.copy()
        self.yaw0 = float(est.rpy[2])
        self.roll = self.pitch = 0.0
        self.yaw = self.yaw0
        self.height = self.stand_height
        self.dx = self.dy = 0.0
        self._z_trim = 0.0
        self._xy_trim[:] = 0.0
        self.last_q = joints.q.copy()
        self.feet_world = self._feet_world(est, joints.q)
        self.forces[:] = 0.0
        self.balance.x_prev[:] = 0.0

    def run(self) -> JointCmd:
        sensors = self.sim.read_sensors()
        joints = sensors.joints
        self.last_q = joints.q.copy()
        meas = sensors.base_state()
        est = self.estimator.step(meas, joints, sensors.contact)
        R = est.rotation()                 # IMU frame quaternion
        omega_b = sensors.omega_b          # IMU gyro
        com = est.position + R @ (self.params.com_body_home
                                  if self.params is not None else np.zeros(3))
        self.p_des = self.p0 + np.array([self.dx, self.dy, 0.0])
        self.p_des[2] = self.height + self._z_trim
        R_des = rpy_to_R([self.roll, self.pitch, self.yaw])
        a, alpha = self.balance.desired_acceleration(
            self.p_des, np.zeros(3), est.position, est.velocity,
            R_des, R, np.zeros(3), omega_b)
        self._xy_trim = np.clip(
            self._xy_trim + (self.p_des[:2] - est.position[:2])
            * self.context.dt, -self._xy_trim_max, self._xy_trim_max)
        a = a + np.array([self.balance.Kp[0, 0] * self._xy_trim[0],
                          self.balance.Kp[1, 1] * self._xy_trim[1], 0.0])
        # Integral trim on height (anti-windup: at most 2 cm down, 5 mm up).
        self._z_trim = float(np.clip(
            self._z_trim - self.z_trim_ki
            * (est.position[2] - self.height) * self.context.dt,
            -0.02, 0.005))
        feet_rel = self.feet_world - com
        self.forces = self.balance.solve_force(
            a, alpha, R, feet_rel, np.ones(4, dtype=bool))
        self.tau = self.balance.to_joint_torque(
            joints.q, self.forces, R, self.robot)
        # Joint targets from the desired body pose and the fixed footholds.
        # The QP provides the force feed-forward; the joint PD makes the pose
        # tracking stiff enough to reject contact/leg-dynamics effects.
        q_des = joints.q.copy()
        for i in range(4):
            sl = slice(3 * i, 3 * i + 3)
            try:
                p_body = R_des.T @ (self.feet_world[i] - self.p_des)
                q_leg = self.robot.legs[i].ik(
                    p_body - self.params.hip_offset[i], knee=-1,
                    q_ref=joints.q[sl])
                q_des[sl] = q_leg
            except ValueError:
                q_des[sl] = joints.q[sl]
        kp = self.kp_leg.copy()
        kd = self.kd_leg.copy()
        cmd = JointCmd(q_des, np.zeros(12), kp, kd, self.tau,
                       mode=np.full(12, int(MotorMode.SERVO)))
        return self._send(cmd)


class TrottingController(State):
    """Trot state with velocity commands (Section 10.4)."""

    def __init__(self, context, robot=None, params=None,
                 gait_params: GaitParams | None = None,
                 balance: BalanceCtrl | None = None,
                 estimator: Estimator | None = None,
                 stand_height: float = 0.27) -> None:
        super().__init__(context, StateID.TROTTING)
        self.robot = robot
        self.params = params
        self.stand_height = float(stand_height)
        self.gait = GaitGenerator(robot, params, gait_params)
        self.balance = balance or BalanceCtrl(
            mass=(params.mass if params is not None else 12.45),
            inertia=(params.body_equivalent_inertia(params.home_q)
                     if params is not None else None))
        if robot is not None:
            self.balance.set_robot(robot)
        self.estimator = estimator or Estimator(robot=robot, dt=context.dt)

        # Inner-loop gains.  Stance legs are mostly torque controlled; swing
        # legs track the IK solution with a joint PD plus a Cartesian PD
        # feed-forward term.
        self.kp_stance = np.array([60.0, 150.0, 200.0] * 4)
        self.kd_stance = np.array([2.0, 3.0, 3.0] * 4)
        self.kp_swing = np.array([60.0, 150.0, 200.0] * 4)
        self.kd_swing = np.array([2.0, 3.0, 3.0] * 4)
        self.kp_cart = np.diag([400.0, 400.0, 400.0])
        self.kd_cart = np.diag([12.0, 12.0, 12.0])

        # Command and integrated reference.
        self.v_cmd = np.zeros(3)
        self.v_cmd_smooth = np.zeros(3)
        self.body_height = self.stand_height
        self.step_height = self.gait.step_height
        self.p_des = np.zeros(3)
        self.yaw_des = 0.0
        self.feet_des = np.zeros((4, 3))
        self.feet_vel = np.zeros((4, 3))
        self.forces = np.zeros((4, 3))
        self.tau = np.zeros(12)
        self.q_des = np.zeros(12)
        self.dq_des = np.zeros(12)
        self.est_base = None
        self._time = 0.0
        self._gait_started = False
        self._stand_phase = 0.2
        self._restart_yaw = False

    # -- command interface ----------------------------------------------
    def set_command(self, vx: float = 0.0, vy: float = 0.0,
                    vyaw: float = 0.0, body_height: float | None = None,
                    step_height: float | None = None) -> None:
        old_wz = self.v_cmd[2]
        self.v_cmd[:] = [float(vx), float(vy), float(vyaw)]
        if abs(old_wz) < 1e-9 and abs(float(vyaw)) > 1e-9:
            self._restart_yaw = True
        if body_height is not None:
            self.body_height = float(body_height)
        if step_height is not None:
            self.step_height = float(step_height)

    def stop(self) -> None:
        self.v_cmd[:] = 0.0

    # -- state interface -------------------------------------------------
    def enter(self) -> None:
        super().enter()
        sensors = self.sim.read_sensors()
        joints = sensors.joints
        self._time = 0.0
        self._gait_started = False
        self._stand_phase = 0.2
        self._restart_yaw = False
        self.v_cmd[:] = 0.0
        self.v_cmd_smooth[:] = 0.0
        self.body_height = self.stand_height
        meas = sensors.base_state()
        self.estimator.reset(meas, joints, sensors.contact)
        est = self.estimator.estimated_base(meas)
        self.p_des = est.position.copy()
        self.yaw_des = float(est.rpy[2])
        self.q_des = joints.q.copy()
        self.dq_des = np.zeros(12)
        self.forces[:] = 0.0
        self.balance.x_prev[:] = 0.0
        feet = est.position + (est.rotation() @ self.robot.fk_all(joints.q).T).T
        self.gait.reset()
        self.gait.set_status(WaveStatus.STANCE_ALL, 0.0)
        self.gait.update(0.0, est.position, self.yaw_des, np.zeros(3), 0.0,
                         feet_current=feet, status=WaveStatus.STANCE_ALL)
        self.feet_des = self.gait.all_targets()
        self.feet_vel = np.zeros((4, 3))

    def exit(self) -> None:
        self.stop()
        super().exit()

    def _safe_ik(self, leg: int, p_body: np.ndarray, q_ref: np.ndarray,
                 fallback: np.ndarray) -> np.ndarray:
        try:
            return self.robot.legs[leg].ik(
                np.asarray(p_body, dtype=float).reshape(3)
                - self.params.hip_offset[leg], knee=-1, q_ref=q_ref)
        except ValueError:
            return np.asarray(fallback, dtype=float).copy()

    def run(self) -> JointCmd:
        sensors = self.sim.read_sensors()
        joints = sensors.joints
        contact_meas = sensors.contact
        meas = sensors.base_state()
        self.est_base = self.estimator.step(meas, joints, contact_meas)
        est = self.est_base

        dt = self.context.dt
        self._time += dt
        # First-order command smoothing (80 ms time constant).
        alpha = dt / (dt + 0.08)
        self.v_cmd_smooth += alpha * (self.v_cmd - self.v_cmd_smooth)
        vx, vy, wz = self.v_cmd_smooth

        yaw = float(sensors.rpy[2])        # IMU frame quaternion -> yaw
        if self._restart_yaw:
            self.yaw_des = yaw
            self._restart_yaw = False
        R = meas.rotation()                # IMU frame quaternion
        omega_b = sensors.omega_b          # IMU gyro
        omega_world = R @ omega_b
        w_cur = float(omega_world[2])
        v_world = R @ np.array([vx, vy, 0.0])
        self.yaw_des += wz * dt
        # Position reference integrates the commanded body-frame velocity.
        self.p_des[0] += v_world[0] * dt
        self.p_des[1] += v_world[1] * dt
        self.p_des[2] = self.body_height

        # Start walking only after a short all-stance transition.
        if not self._gait_started and self._time >= self._stand_phase:
            self._gait_started = True
            self.gait.set_status(WaveStatus.WAVE_ALL, self._time)
        com_body = (self.params.com_body_home if self.params is not None
                    else np.zeros(3))
        feet_body = self.robot.fk_all(joints.q)
        # Plan and control in a frame anchored at the commanded body
        # position.  The absolute x/y estimate is unobservable and drifts,
        # so mixing it with world-frame foot kinematics would corrupt the
        # QP moment arms.  Relative geometry is unaffected by the anchor.
        feet_frame = (self.p_des[None, :] + (R @ feet_body.T).T)
        self.gait.set_gait(v_world[:2], wz, self.step_height)
        self.feet_des, self.feet_vel = self.gait.update(
            self._time, self.p_des, self.yaw_des, v_world, wz,
            feet_current=feet_frame)
        contact = self.gait.contact

        # --- stance: balance QP
        R_des = rpy_to_R([0.0, 0.0, self.yaw_des])
        v_des = np.array([v_world[0], v_world[1], 0.0])
        # Velocity-level balance: x/y position is not observable, so only
        # the height and the attitude use absolute references.
        p_ref = est.position.copy()
        p_ref[2] = self.body_height
        a, alpha_des = self.balance.desired_acceleration(
            p_ref, v_des, est.position, est.velocity,
            R_des, R, np.zeros(3), omega_b)
        feet_rel_com = (R @ (feet_body - com_body).T).T
        self.forces = self.balance.solve_force(
            a, alpha_des, R, feet_rel_com, contact)
        tau_stance = self.balance.to_joint_torque(
            joints.q, self.forces, R, self.robot)

        # --- joint targets
        self.q_des = joints.q.copy()
        self.dq_des = joints.dq.copy()
        kp = self.kp_stance.copy()
        kd = self.kd_stance.copy()
        tau_ff = tau_stance.copy()

        for i in range(4):
            sl = slice(3 * i, 3 * i + 3)
            if contact[i]:
                # Hold the stance foot with a stiff-ish IK target; this also
                # keeps the leg from collapsing if the QP saturates.
                p_world = self.feet_des[i]
                p_body = R_des.T @ (p_world - self.p_des)
                self.q_des[sl] = self._safe_ik(
                    i, p_body, joints.q[sl], joints.q[sl])
                self.dq_des[sl] = 0.0
            else:
                p_world = self.feet_des[i]
                p_body = R.T @ (p_world - est.position)
                self.q_des[sl] = self._safe_ik(
                    i, p_body, joints.q[sl], joints.q[sl])
                J = self.robot.leg_jacobian(joints.q, i)
                v_body = R.T @ (self.feet_vel[i] - est.velocity)
                dq_leg = np.linalg.lstsq(J, v_body, rcond=None)[0]
                self.dq_des[sl] = dq_leg
                kp[sl] = self.kp_swing[sl]
                kd[sl] = self.kd_swing[sl]
                # Cartesian PD feed-forward force, mapped by J^T R^T.
                p_act = feet_frame[i]
                Jw = R @ J
                v_act = (v_des + R @ (np.cross(omega_b, feet_body[i]))
                         + Jw @ joints.dq[sl])
                f_virt = (self.kp_cart @ (p_world - p_act)
                          + self.kd_cart @ (self.feet_vel[i] - v_act))
                tau_ff[sl] += J.T @ (R.T @ f_virt)

        self.tau = tau_ff
        cmd = JointCmd(self.q_des.copy(), self.dq_des.copy(), kp, kd,
                       tau_ff, mode=np.full(12, int(MotorMode.SERVO)))
        return self._send(cmd)


__all__ = ["FreeStand", "TrottingController"]
