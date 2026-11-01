"""FreeStand by fixed feet + inverse kinematics (Section 6.5).

The robot stands on four fixed footholds.  A target body pose (height, roll,
pitch, yaw) is converted back to four body-frame foot targets, then to twelve
joint angles by inverse kinematics.  Only joint position control is used: no
force feed-forward, no QP.  The four poses are rendered and the attitude
history is plotted.

expect: 站立高度
expect: 升高 3 cm
expect: roll
expect: pitch
expect: yaw
expect: 四足接触
"""

from __future__ import annotations

import logging
import math
import os

os.environ.setdefault("MESA_SHADER_CACHE_DISABLE", "true")

import numpy as np

from quadruped import A1Sim, A1Params, Quadruped
from quadruped.rotation import rpy_to_R, R_to_rpy, wrap_angle
from quadruped.types import JointCmd
from quadruped.plotting import savefig
import matplotlib.pyplot as plt

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

DT = 0.002
KP = np.array([100.0, 800.0, 1000.0] * 4)
KD = np.array([4.0, 10.0, 10.0] * 4)


# ANCHOR: controller
class KinematicStand:
    """Fixed-feet pose control with IK and joint PD only."""

    def __init__(self, sim, robot, params):
        self.sim = sim
        self.robot = robot
        self.params = params
        self.p0 = np.zeros(3)
        self.rpy0 = np.zeros(3)
        self.feet = np.zeros((4, 3))
        self.q_ref = np.zeros(12)
        self.target = np.zeros(4)
        self.goal = np.zeros(4)
        self.rate = np.array([0.12, 0.35, 0.35, 0.35])

    def enter(self, base, q):
        # In simulation the pose at entry is read from ground truth.  On a
        # real robot this is the declared starting pose (or the estimator's
        # initial pose); the controller itself only uses q thereafter.
        self.p0 = np.asarray(base.position, dtype=float).copy()
        self.rpy0 = R_to_rpy(base.rotation())
        self.feet = (base.rotation() @ self.robot.fk_all(q).T).T + self.p0
        self.q_ref = np.asarray(q, dtype=float).copy()
        self.set_pose()

    def set_pose(self, dh=0.0, droll=0.0, dpitch=0.0, dyaw=0.0):
        self.goal = np.array([dh, droll, dpitch, dyaw], dtype=float)

    def advance(self, dt: float) -> None:
        step = self.rate * dt
        self.target = self.target + np.clip(self.goal - self.target,
                                           -step, step)

    def command(self, q_now, dt: float = DT) -> JointCmd:
        self.advance(dt)
        dh, droll, dpitch, dyaw = self.target
        height = self.p0[2] + dh
        rpy = np.array([self.rpy0[0] + droll,
                        self.rpy0[1] + dpitch,
                        self.rpy0[2] + dyaw])
        R_des = rpy_to_R(rpy)
        p_des = np.array([self.p0[0], self.p0[1], height])
        q_des = np.asarray(q_now, dtype=float).copy()
        for i in range(4):
            p_body = R_des.T @ (self.feet[i] - p_des)
            try:
                q_leg = self.robot.legs[i].ik(
                    p_body - self.params.hip_offset[i], knee=-1,
                    q_ref=self.q_ref[3 * i:3 * i + 3])
            except ValueError:
                q_leg = q_des[3 * i:3 * i + 3]
            q_des[3 * i:3 * i + 3] = q_leg
        self.q_ref = q_des.copy()
        return JointCmd(q_des, np.zeros(12), KP, KD, np.zeros(12))
# ANCHOR_END: controller


def rot_deg(x):
    return math.degrees(float(x))


# ANCHOR: main
def main() -> None:
    params = A1Params.from_menagerie()
    robot = Quadruped(params)
    sim = A1Sim()
    sim.reset(q0=params.home_q, base_pos=(0.0, 0.0, 0.27))
    # Move the tracking camera closer than the default 0.75 m so that the
    # body pose fills the four rendered panels.
    cam = sim.model.camera("track").id
    sim.model.cam_pos[cam] = [0.55, -0.55, 0.45]
    sim.model.cam_fovy[cam] = 40.0

    # Settle in the nominal stand pose before entering FreeStand.
    stand = JointCmd(params.home_q, np.zeros(12), KP, KD, np.zeros(12))
    for _ in range(int(1.5 / DT)):
        sim.step(stand)
    base, joints = sim.get_state()
    print("=== 6.5 FreeStand：固定足端 + 逆运动学 ===")
    print(f"站立高度 = {base.position[2]:.4f} m，"
          f"roll/pitch/yaw = ({rot_deg(base.rpy[0]): .3f}, "
          f"{rot_deg(base.rpy[1]): .3f}, {rot_deg(base.rpy[2]): .3f}) deg")

    ctrl = KinematicStand(sim, robot, params)
    ctrl.enter(base, joints.q)
    print(f"进入 FreeStand：记录四个足端世界坐标，p0 = "
          f"({ctrl.p0[0]:.4f}, {ctrl.p0[1]:.4f}, {ctrl.p0[2]:.4f}) m")

    phases = [
        ("升高 3 cm", dict(dh=0.03)),
        ("roll +5°", dict(droll=math.radians(5.0))),
        ("pitch -5°", dict(dpitch=math.radians(-5.0))),
        ("yaw +10°", dict(dyaw=math.radians(10.0))),
    ]
    hold = 1.0
    rest = 0.5
    t = 0.0
    hist = {"t": [], "h": [], "rpy": [], "h_des": [], "rpy_des": []}
    captures = []

    # A short nominal hold, then the four poses with a return to nominal
    # between them.  The return makes each panel an isolated single-axis move.
    ctrl.set_pose()
    for _ in range(int(0.3 / DT)):
        cmd = ctrl.command(sim.joint_positions, DT)
        sim.step(cmd)
        base, joints = sim.get_state()
        t += DT
        hist["t"].append(t)
        hist["h"].append(base.position[2])
        hist["rpy"].append(base.rpy.copy())
        hist["h_des"].append(ctrl.p0[2] + ctrl.target[0])
        hist["rpy_des"].append(np.array(ctrl.rpy0) + np.array(ctrl.target[1:]))

    for idx, (name, kw) in enumerate(phases):
        ctrl.set_pose(**kw)
        n_hold = int(hold / DT)
        h_samples, rpy_samples, f_samples, qerr, slip = [], [], [], [], []
        img = None
        for k in range(n_hold):
            cmd = ctrl.command(sim.joint_positions, DT)
            sim.step(cmd)
            base, joints = sim.get_state()
            t += DT
            hist["t"].append(t)
            hist["h"].append(base.position[2])
            hist["rpy"].append(base.rpy.copy())
            hist["h_des"].append(ctrl.p0[2] + ctrl.target[0])
            hist["rpy_des"].append(np.array(ctrl.rpy0) + np.array(ctrl.target[1:]))
            if k >= n_hold - int(0.2 / DT):
                h_samples.append(base.position[2])
                rpy_samples.append(base.rpy.copy())
                f_samples.append(sim.foot_forces().copy())
                qerr.append(np.max(np.abs(joints.q - cmd.q_des)))
                slip.append(float(np.max(np.linalg.norm(
                    sim.foot_positions_world() - ctrl.feet, axis=1))))
            if k == n_hold - 1:
                frame = sim.render(width=480, height=360)
                img = frame[50:350, 20:460]
        base, joints = sim.get_state()
        target_rpy = np.array(ctrl.rpy0) + np.array(ctrl.target[1:])
        target_h = ctrl.p0[2] + ctrl.target[0]
        h = float(np.mean(h_samples))
        rpy = np.mean(np.asarray(rpy_samples), axis=0)
        fz = np.mean(np.asarray(f_samples), axis=0)[:, 2]
        captures.append((name, img))
        print(f"\n[{name}] 目标 h={target_h:.4f} m, "
              f"roll/pitch/yaw=({rot_deg(target_rpy[0]): .2f}, "
              f"{rot_deg(target_rpy[1]): .2f}, "
              f"{rot_deg(target_rpy[2]): .2f}) deg")
        print(f"  实测 h={h:.4f} m，误差 {(h - target_h) * 1000:+.2f} mm；"
              f"姿态误差 = ({rot_deg(wrap_angle(rpy[0] - target_rpy[0])):+.3f}, "
              f"{rot_deg(wrap_angle(rpy[1] - target_rpy[1])):+.3f}, "
              f"{rot_deg(wrap_angle(rpy[2] - target_rpy[2])):+.3f}) deg")
        print(f"  四足接触力 fz = ({fz[0]:.2f}, {fz[1]:.2f}, "
              f"{fz[2]:.2f}, {fz[3]:.2f}) N，总竖直力 {fz.sum():.2f} N")
        print(f"  关节跟踪最大误差 = {np.max(qerr):.4f} rad，"
              f"足端最大滑移 = {np.max(slip) * 1000:.2f} mm")

        # Return to nominal before the next isolated pose.  The last pose is
        # left at its target so the history ends on the measured plateau.
        if idx < len(phases) - 1:
            ctrl.set_pose()
            for _ in range(int(rest / DT)):
                cmd = ctrl.command(sim.joint_positions, DT)
                sim.step(cmd)
                base, joints = sim.get_state()
                t += DT
                hist["t"].append(t)
                hist["h"].append(base.position[2])
                hist["rpy"].append(base.rpy.copy())
                hist["h_des"].append(ctrl.p0[2] + ctrl.target[0])
                hist["rpy_des"].append(np.array(ctrl.rpy0) + np.array(ctrl.target[1:]))

    # Does the body move horizontally?  The target keeps x/y fixed; any
    # deviation is leg compliance / foot slip.
    print("\n结束时机身水平位置 = "
          f"({base.position[0] - ctrl.p0[0]:+.4f}, "
          f"{base.position[1] - ctrl.p0[1]:+.4f}) m（相对进入点）")

    # Figure 6-6: four rendered poses and the pose history.
    fig = plt.figure(figsize=(14.0, 7.4))
    for k, (name, img) in enumerate(captures):
        ax = fig.add_subplot(2, 4, k + 1)
        ax.imshow(img)
        ax.set_xticks([])
        ax.set_yticks([])
        ax.set_title(name, fontsize=10)
    t_arr = np.asarray(hist["t"])
    h_arr = np.asarray(hist["h"])
    rpy_arr = np.asarray(hist["rpy"])
    h_des = np.asarray(hist["h_des"])
    rpy_des = np.asarray(hist["rpy_des"])

    ax = fig.add_subplot(2, 4, 5)
    ax.plot(t_arr, (h_arr - ctrl.p0[2]) * 100, color="tab:blue", lw=1.3,
            label="实测")
    ax.plot(t_arr, (h_des - ctrl.p0[2]) * 100, color="0.5", ls="--", lw=1.0,
            label="目标")
    ax.set_xlabel("时间 (s)")
    ax.set_ylabel("高度增量 (cm)")
    ax.set_title("机身高度")
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=7)

    for k, (name, color) in enumerate((("roll", "tab:red"),
                                       ("pitch", "tab:green"),
                                       ("yaw", "tab:blue"))):
        ax = fig.add_subplot(2, 4, 6 + k)
        ax.plot(t_arr, np.degrees(rpy_arr[:, k]), color=color, lw=1.2,
                label="实测")
        ax.plot(t_arr, np.degrees(rpy_des[:, k]), color="0.5", ls="--",
                lw=1.0, label="目标")
        ax.set_xlabel("时间 (s)")
        ax.set_ylabel("角度 (deg)")
        ax.set_title(name)
        ax.grid(True, alpha=0.3)
        ax.legend(fontsize=7)
    fig.suptitle("FreeStand：升高 3 cm、roll +5°、pitch -5°、yaw +10°",
                 fontsize=12)
    fig.tight_layout()
    savefig(fig, "6_5_free_stand")

    sim.close()
# ANCHOR_END: main


if __name__ == "__main__":
    main()
