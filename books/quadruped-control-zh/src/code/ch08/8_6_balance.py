"""BalanceTest: pose PD, QP foot forces, joint torques, 30 N push (8.6).

The controller follows the five steps of ``State_BalanceTest``:

1. read the state and run the estimator;
2. compute the desired linear acceleration with Eq. (8-17);
3. compute the desired angular acceleration with Eq. (8-22);
4. solve the QP for four world-frame foot forces;
5. map the forces to twelve joint torques with Eq. (8-39).

The QP torque is the feed-forward; a soft joint PD keeps the leg shape.  A
30 N lateral force acts for 0.1 s at t = 1.0 s.  Six rendered frames, the
force history and the recovery metrics are saved.

expect: BalanceTest
expect: 最大 roll
expect: 恢复时间
expect: 足端力峰值
expect: 最大关节力矩
expect: 安全检查
"""

from __future__ import annotations

import logging
import os

os.environ.setdefault("MESA_SHADER_CACHE_DISABLE", "true")

import numpy as np
from quadruped.plotting import savefig, label
import matplotlib.pyplot as plt

from quadruped import A1Sim, A1Params, Quadruped
from quadruped.balance import BalanceCtrl
from quadruped.estimator import Estimator
from quadruped.rotation import rpy_to_R
from quadruped.types import JointCmd, MotorMode

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

DT = 0.002
TARGET_H = 0.270
PUSH_T0 = 1.0
PUSH_T1 = 1.1
PUSH_N = 30.0
KP_LEG = np.array([30.0, 60.0, 80.0] * 4)
KD_LEG = np.array([1.5, 2.0, 2.0] * 4)
TAU_MAX = 33.5

# ANCHOR: controller
class BalanceTest:
    """Pose PD -> QP force -> joint torque, with the safety checks of 8.6."""

    def __init__(self, sim, robot, params, balance, estimator):
        self.sim = sim
        self.robot = robot
        self.params = params
        self.balance = balance
        self.estimator = estimator
        self.p_des = np.zeros(3)
        self.R_des = np.eye(3)
        self.height = TARGET_H
        self.roll = 0.0
        self.pitch = 0.0
        self.yaw = 0.0
        self.feet_world = np.zeros((4, 3))
        self.forces = np.zeros((4, 3))
        self.tau = np.zeros(12)
        self.a_des = np.zeros(3)
        self.alpha_des = np.zeros(3)
        self.fail_count = 0
        self.safety_count = 0
        self.last_cmd = None

    def enter(self) -> None:
        sensors = self.sim.read_sensors()
        self.estimator.reset(sensors.base_state(), sensors.joints,
                             sensors.contact)
        est = self.estimator.estimated_base(sensors.base_state())
        self.p_des = np.array([est.position[0], est.position[1], TARGET_H])
        self.height = TARGET_H
        self.roll = self.pitch = self.yaw = 0.0
        self.R_des = rpy_to_R([0.0, 0.0, 0.0])
        self.feet_world = est.position + (
            est.rotation() @ self.robot.fk_all(sensors.joints.q).T).T
        self.balance.x_prev[:] = 0.0
        self.fail_count = 0
        self.safety_count = 0

    def set_pose(self, height: float | None = None,
                 roll: float | None = None, pitch: float | None = None,
                 yaw: float | None = None) -> None:
        """Four operator targets: height plus roll, pitch and yaw."""
        if height is not None:
            self.height = float(height)
            self.p_des[2] = self.height
        if roll is not None:
            self.roll = float(roll)
        if pitch is not None:
            self.pitch = float(pitch)
        if yaw is not None:
            self.yaw = float(yaw)
        self.R_des = rpy_to_R([self.roll, self.pitch, self.yaw])

    def calc_tau(self) -> JointCmd:
        sensors = self.sim.read_sensors()
        joints = sensors.joints
        est = self.estimator.step(sensors.base_state(), joints,
                                  sensors.contact)
        R = est.rotation()
        com = est.position + R @ self.params.com_body_home
        self.feet_world = est.position + (R @ self.robot.fk_all(joints.q).T).T

        # Steps 1-2: pose PD -> desired linear and angular acceleration.
        self.a_des, self.alpha_des = self.balance.desired_acceleration(
            self.p_des, np.zeros(3), est.position, est.velocity,
            self.R_des, R, np.zeros(3), sensors.omega_b)

        # Step 3: four world-frame foot forces.
        self.forces = self.balance.solve_force(
            self.a_des, self.alpha_des, R, self.feet_world - com,
            sensors.contact.astype(bool))
        if self.balance.last_status not in (1, 2):
            self.fail_count += 1

        # Step 4: J^T R^T f -> twelve joint torques.
        self.tau = self.balance.to_joint_torque(
            joints.q, self.forces, R, self.robot)

        # Step 5: leg targets from the desired body pose and fixed footholds.
        q_des = joints.q.copy()
        for i in range(4):
            sl = slice(3 * i, 3 * i + 3)
            try:
                p_body = self.R_des.T @ (self.feet_world[i] - self.p_des)
                q_des[sl] = self.robot.legs[i].ik(
                    p_body - self.params.hip_offset[i], knee=-1,
                    q_ref=joints.q[sl])
            except ValueError:
                q_des[sl] = joints.q[sl]

        if not self.safety_ok(sensors, est):
            self.safety_count += 1
            self.last_cmd = JointCmd.zero()
            return self.last_cmd

        self.last_cmd = JointCmd(q_des, np.zeros(12), KP_LEG, KD_LEG,
                                 self.tau, mode=np.full(12, int(MotorMode.SERVO)))
        return self.last_cmd

    def safety_ok(self, sensors, est) -> bool:
        if self.balance.last_status not in (1, 2):
            return False
        if int(np.sum(sensors.contact)) < 2:
            return False
        rpy = est.rpy
        if abs(rpy[0]) > np.radians(30.0) or abs(rpy[1]) > np.radians(30.0):
            return False
        if np.max(np.abs(self.tau)) > TAU_MAX:
            return False
        return True
# ANCHOR_END: controller


def main() -> None:
    params = A1Params.from_menagerie()
    robot = Quadruped(params)
    sim = A1Sim()
    sim.reset(q0=params.home_q, base_pos=(0.0, 0.0, TARGET_H))
    cam = sim.model.camera("track").id
    sim.model.cam_pos[cam] = [0.55, -0.55, 0.42]
    sim.model.cam_fovy[cam] = 40.0

    stand = JointCmd(params.home_q, np.zeros(12),
                     np.array([100.0, 300.0, 400.0] * 4),
                     np.array([4.0, 8.0, 8.0] * 4), np.zeros(12))
    for _ in range(500):
        sim.step(stand)

    ctrl = BalanceCtrl(mass=params.mass, inertia=params.trunk_inertia,
                       w_dynamics=1000.0, w_force=0.01, w_delta=0.1)
    ctrl.set_robot(robot)
    ctrl.Kp = np.diag([100.0, 100.0, 200.0])
    ctrl.Kd = np.diag([20.0, 20.0, 28.0])
    ctrl.kp_rot = 120.0
    ctrl.Kd_rot = np.diag([16.0, 16.0, 10.0])
    estimator = Estimator(robot=robot, dt=DT)
    state = BalanceTest(sim, robot, params, ctrl, estimator)
    state.enter()

    print("=== 8.6 BalanceTest：站立 + 30 N 侧向推扰 ===")
    print(f"目标高度 = {TARGET_H:.4f} m，推扰 = {PUSH_N:.1f} N，"
          f"{PUSH_T0:.1f}–{PUSH_T1:.1f} s，侧向（$y$）")
    body_id = sim.model.body("trunk").id
    n_steps = int(2.6 / DT)
    hist = {"t": [], "rpy": [], "z": [], "xy": [], "forces": [], "tau": []}
    frames = {}
    frame_steps = {450: "推前", 505: "推中", 545: "推后 0.09 s",
                   575: "推后 0.15 s", 650: "推后 0.30 s",
                   1150: "推后 1.30 s"}

    for k in range(n_steps):
        t = k * DT
        cmd = state.calc_tau()
        if PUSH_T0 <= t < PUSH_T1:
            sim.data.xfrc_applied[body_id, :3] = [0.0, PUSH_N, 0.0]
        else:
            sim.data.xfrc_applied[body_id, :3] = 0.0
        base = sim.step(cmd)
        hist["t"].append(t)
        hist["rpy"].append(base.rpy.copy())
        hist["z"].append(base.position[2])
        hist["xy"].append(base.position[:2].copy())
        hist["forces"].append(state.forces.copy())
        hist["tau"].append(state.tau.copy())
        if k in frame_steps:
            frames[k] = sim.render(width=520, height=390)

    t = np.asarray(hist["t"])
    rpy = np.asarray(hist["rpy"])
    z = np.asarray(hist["z"])
    xy = np.asarray(hist["xy"])
    forces = np.asarray(hist["forces"])
    taus = np.asarray(hist["tau"])

    roll = np.degrees(rpy[:, 0])
    after = t >= PUSH_T1
    post_t = t[after]
    post_roll = np.degrees(rpy[after, 0])
    steady_roll = float(np.mean(post_roll[-150:]))
    outside = np.flatnonzero(np.abs(post_roll - steady_roll) > 0.1)
    if outside.size and outside[-1] + 1 < len(post_t):
        recovery = float(post_t[outside[-1] + 1] - PUSH_T1)
    elif outside.size:
        recovery = float(post_t[-1] - PUSH_T1)
    else:
        recovery = 0.0
    print(f"QP 求解失败 = {state.fail_count} 次，"
          f"安全检查触发 = {state.safety_count} 次")
    print(f"最大 roll = {np.abs(roll).max():.4f} deg，"
          f"稳态 roll = {steady_roll:+.4f} deg")
    print(f"恢复时间（回到稳态 ±0.1 deg）= {recovery:.3f} s")
    print(f"最大 pitch = {np.degrees(np.abs(rpy[:, 1]).max()):.4f} deg，"
          f"最低高度 = {z.min():.4f} m")
    print("足端力峰值（范数）= [" + ", ".join(
        f"{v:.2f}" for v in np.linalg.norm(forces, axis=2).max(axis=0)) + "] N")
    print(f"最大关节力矩 = {np.abs(taus).max():.4f} N·m，"
          f"占 ±{TAU_MAX:.1f} N·m 的 "
          f"{100.0 * np.abs(taus).max() / TAU_MAX:.2f} %")
    print(f"结束位置偏移 = [{xy[-1, 0] * 1000.0:+.2f} "
          f"{xy[-1, 1] * 1000.0:+.2f}] mm")

    # Figure 8-7: six frames plus the force/roll history.
    fig = plt.figure(figsize=(12.6, 8.2))
    grid = fig.add_gridspec(3, 3, height_ratios=[1.0, 1.0, 1.25],
                            hspace=0.28, wspace=0.08)
    for n, (k, tag) in enumerate(frame_steps.items()):
        ax = fig.add_subplot(grid[n // 3, n % 3])
        ax.imshow(frames[k])
        ax.set_title(f"{tag}（t = {k * DT:.2f} s）", fontsize=9)
        ax.axis("off")

    ax = fig.add_subplot(grid[2, :])
    for i, name in enumerate(("FR", "FL", "RR", "RL")):
        ax.plot(t, forces[:, i, 2], lw=1.2, label=f"$f_{{z,{name}}}$")
    ax.axvspan(PUSH_T0, PUSH_T1, color="#ffcdd2", alpha=0.7)
    ax.set_xlabel(label("时间 (s)", "time (s)"))
    ax.set_ylabel(label("法向力 (N)", "normal force (N)"))
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=8, ncol=4, loc="upper left")
    ax2 = ax.twinx()
    ax2.plot(t, forces[:, :, 1].sum(axis=1), "k--", lw=1.0,
             label="总侧向力")
    ax2.set_ylabel(label("总侧向力 (N)", "total lateral force (N)"),
                   color="k")
    ax2.tick_params(axis="y", colors="k")
    ax.set_title(label("推扰期间的足端法向力与总侧向力",
                       "normal forces and total lateral force"))
    fig.subplots_adjust(left=0.05, right=0.95, top=0.95, bottom=0.06)
    savefig(fig, "8_7_balance")
    plt.close(fig)
    sim.close()


if __name__ == "__main__":
    main()
