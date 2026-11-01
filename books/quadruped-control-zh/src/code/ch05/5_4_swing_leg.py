"""Swing-leg tracking: joint position only versus Cartesian PD (section 5.4).

A single A1 leg is mounted on a fixed base (the base pose is pinned every
2 ms, which stands in for a test stand).  The foot follows two cycloid swings
with a small lateral bump for 2 s.  The first controller sends only joint
position commands; the second adds joint velocity feedforward and a Cartesian
PD force mapped by tau = J^T f.

expect: 纯关节位置
expect: 笛卡尔 PD
expect: RMS
expect: 最大误差
"""

from __future__ import annotations

import logging
import math

import numpy as np

from quadruped import A1Params, A1Sim, JointCmd, Quadruped
from quadruped.plotting import label, savefig
from quadruped.swing import SwingTrajectory
import matplotlib.pyplot as plt

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

BASE_Z = 0.62          # high enough that all four feet are in the air
T_SWING = 1.0          # one swing takes 1 s
X_SWING = 0.12         # forward displacement of one swing, m
Z_SWING = 0.05         # swing height, m
Y_BUMP = 0.02          # lateral bump, m
DT = 0.002


def pin_base(sim: A1Sim, base: np.ndarray) -> None:
    """Pin the floating base: a stand/hanger constraint for a single-leg test."""
    sim.data.qpos[0:3] = base
    sim.data.qpos[3:7] = [1.0, 0.0, 0.0, 0.0]
    sim.data.qvel[0:6] = 0.0


def make_trajectory(p0: np.ndarray):
    a = p0 + np.array([X_SWING, 0.0, 0.0])
    forward = SwingTrajectory(p0, a, height=Z_SWING, duration=T_SWING)
    backward = SwingTrajectory(a, p0, height=Z_SWING, duration=T_SWING)

    def desired(t: float):
        if t <= 0.0 or t >= 2.0 * T_SWING:
            return p0.copy(), np.zeros(3)
        if t < T_SWING:
            tr, s = forward, t / T_SWING
        else:
            tr, s = backward, (t - T_SWING) / T_SWING
        p = tr.position(s).copy()
        v = tr.velocity(s).copy()
        # A smooth lateral bump, zero position and velocity at both ends.
        p[1] += 0.5 * Y_BUMP * (1.0 - math.cos(2.0 * math.pi * s))
        v[1] += 0.5 * Y_BUMP * (2.0 * math.pi / T_SWING) * math.sin(
            2.0 * math.pi * s)
        return p, v

    return desired


def run(mode: str, params: A1Params, leg, trajectory, base: np.ndarray):
    sim = A1Sim()
    sim.reset(q0=params.home_q, base_pos=base)
    q_home = params.home_q.copy()
    kp = np.tile([40.0, 80.0, 100.0], 4)
    kd = np.tile([1.5, 2.0, 2.0], 4)
    Kp = np.diag([1500.0, 1500.0, 1500.0])
    Kd = np.diag([30.0, 30.0, 30.0])
    n = int(round(2.0 / DT))
    times = np.arange(n) * DT
    err = np.zeros((n, 3))
    p_des = np.zeros((n, 3))
    p_act = np.zeros((n, 3))
    q_cmd_hist = np.zeros((n, 3))
    q_act_hist = np.zeros((n, 3))

    for i in range(n):
        t = times[i]
        pd, vd = trajectory(t)
        qd = leg.ik(pd)
        Jd = leg.jacobian(qd)
        dqd = np.linalg.solve(Jd, vd)
        q = sim.joint_positions
        dq = sim.joint_velocities
        p = leg.fk(q[:3])
        v = leg.jacobian(q[:3]) @ dq[:3]
        # ANCHOR: cartesian_pd
        f = Kp @ (pd - p) + Kd @ (vd - v)
        tau_ff = leg.tau_from_force(q[:3], f)
        # ANCHOR_END: cartesian_pd

        q_cmd = q_home.copy()
        q_cmd[:3] = qd
        dq_cmd = np.zeros(12)
        dq_cmd[:3] = dqd
        tau_cmd = np.zeros(12)
        tau_cmd[:3] = tau_ff
        if mode == "joint":
            dq_cmd[:3] = 0.0
            tau_cmd[:3] = 0.0
        cmd = JointCmd(q_cmd, dq_cmd, kp, kd, tau_cmd)
        pin_base(sim, base)
        sim.step(cmd)

        err[i] = pd - p
        p_des[i] = pd
        p_act[i] = p
        q_cmd_hist[i] = qd
        q_act_hist[i] = q[:3]

    sim.close()
    return times, err, p_des, p_act, q_cmd_hist, q_act_hist


def draw(times, results, p0) -> None:
    fig = plt.figure(figsize=(10.8, 4.7))
    ax = fig.add_subplot(1, 2, 1, projection="3d")
    ax.plot(*results["cart"][2].T, color="0.25", lw=2.0, ls="--",
            label="目标 $p_d$")
    ax.plot(*results["joint"][3].T, color="tab:blue", lw=1.1,
            label="纯关节位置")
    ax.plot(*results["cart"][3].T, color="tab:orange", lw=1.3,
            label="关节 + 笛卡尔 PD")
    ax.set_xlabel(r"$x_0$ / m", fontsize=8)
    ax.set_ylabel(r"$y_0$ / m", fontsize=8)
    ax.set_zlabel(r"$z_0$ / m", fontsize=8)
    ax.tick_params(labelsize=7)
    ax.view_init(elev=24, azim=-58)
    ax.legend(fontsize=8, loc="upper left")
    ax.set_title(label("足端三维轨迹", "3-D foot path"), fontsize=10)

    ax2 = fig.add_subplot(1, 2, 2)
    for key, color, name in (("joint", "tab:blue", "纯关节位置"),
                             ("cart", "tab:orange", "关节 + 笛卡尔 PD")):
        e = np.linalg.norm(results[key][1], axis=1)
        ax2.plot(times, 1000 * e, color=color, lw=1.3,
                 label=f"{name}: RMS {1000 * np.sqrt(np.mean(e ** 2)):.2f} mm")
        imax = int(np.argmax(e))
        ax2.plot(times[imax], 1000 * e[imax], "o", color=color, ms=4)
    ax2.set_xlabel(label("时间 (s)", "time (s)"))
    ax2.set_ylabel(label("足端误差 $\\|p_d-p\\|$ (mm)", "foot error (mm)"))
    ax2.grid(True, alpha=0.3)
    ax2.legend(fontsize=8)
    ax2.set_title(label("跟踪误差", "tracking error"), fontsize=10)
    fig.tight_layout()
    savefig(fig, "5_4_tracking")
    plt.close(fig)


def main() -> None:
    params = A1Params.from_menagerie()
    robot = Quadruped(params)
    leg = robot.legs[0]
    q_home = params.home_q
    p0 = leg.fk(q_home[:3])
    base = np.array([0.0, 0.0, BASE_Z])
    trajectory = make_trajectory(p0)

    print("=== 5.4 右前腿摆动跟踪 ===")
    p0s = np.where(np.abs(p0) < 5e-5, 0.0, p0)
    print(f"腿基座系中的起点 P0 = {p0s.round(4)} m")
    print(f"轨迹: 2 段各 {T_SWING:.1f} s 的摆线，x 行程 {X_SWING:.2f} m，"
          f"抬高 {Z_SWING:.2f} m，侧向 {Y_BUMP:.2f} m")
    print(f"关节增益 kp=[40,80,100], kd=[1.5,2,2]；"
          f"笛卡尔增益 Kp=1500, Kd=30；周期 {DT * 1000:.0f} ms")

    results = {}
    for mode, name in (("joint", "纯关节位置"), ("cart", "关节位置 + 笛卡尔 PD")):
        results[mode] = run(mode, params, leg, trajectory, base)
        err = np.linalg.norm(results[mode][1], axis=1)
        rms, mx = np.sqrt(np.mean(err ** 2)), np.max(err)
        print(f"{name}: RMS = {1000 * rms:.2f} mm, "
              f"最大误差 = {1000 * mx:.2f} mm (t = {results[mode][0][np.argmax(err)]:.2f} s)")

    e1 = np.linalg.norm(results["joint"][1], axis=1)
    e2 = np.linalg.norm(results["cart"][1], axis=1)
    print(f"RMS 降低到 {100 * np.sqrt(np.mean(e2 ** 2)) / np.sqrt(np.mean(e1 ** 2)):.1f}%，"
          f"最大误差降低到 {100 * np.max(e2) / np.max(e1):.1f}%")
    print("残差主要来自腿的惯性、关节摩擦和 2 ms 采样；"
          "单腿静力学近似在高速段变差。")

    draw(results["joint"][0], results, p0)


if __name__ == "__main__":
    main()
