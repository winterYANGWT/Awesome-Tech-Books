"""Foot force to joint torque by single-leg statics (Section 8.4).

For a fixed pose and four world-frame foot forces, the example computes

    tau_i = -J_i^T R^T f_i

and compares it with MuJoCo's ``mj_inverse``.  The external force is applied
at the foot-sphere centre with ``mj_applyFT``; the inverse-dynamics joint
torque with and without that force differs by exactly ``-J_i^T R^T f_i``.

expect: 简化逆向动力学
expect: 解析
expect: MuJoCo
expect: 最大绝对误差
expect: 最大力矩
"""

from __future__ import annotations

import logging
import os

os.environ.setdefault("MESA_SHADER_CACHE_DISABLE", "true")

import numpy as np
from quadruped.plotting import savefig, label
import matplotlib.pyplot as plt
import mujoco

from quadruped import A1Sim, A1Params, Quadruped
from quadruped.rotation import quat_to_R

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

Q_LEG = np.array([0.0, 0.6, -1.2])
FORCES = np.array([[6.0, -3.0, 32.0],
                   [5.0, -2.0, 30.0],
                   [8.0, -4.0, 34.0],
                   [7.0, -3.0, 33.0]])
TAU_MAX = 33.5


def main() -> None:
    params = A1Params.from_menagerie()
    robot = Quadruped(params)
    sim = A1Sim()
    q = np.tile(Q_LEG, 4)
    sim.reset(q0=q, base_pos=(0.0, 0.0, 0.35))
    model, data = sim.model, sim.data
    data.qvel[:] = 0.0
    data.qacc[:] = 0.0
    data.qfrc_applied[:] = 0.0
    data.xfrc_applied[:] = 0.0
    mujoco.mj_forward(model, data)
    mujoco.mj_inverse(model, data)
    base = data.qfrc_inverse.copy()          # gravity + bias at this pose
    R = quat_to_R(data.qpos[3:7])

    # ANCHOR: mapping
    tau = np.zeros(12)
    tau_mj = np.zeros(12)
    for i in range(4):
        f_body = R.T @ FORCES[i]
        tau[3 * i:3 * i + 3] = -robot.leg_jacobian(q, i).T @ f_body

        data.qfrc_applied[:] = 0.0
        point = data.geom_xpos[sim.foot_geom_ids[i]].copy()
        mujoco.mj_applyFT(model, data, FORCES[i], np.zeros(3), point,
                          sim.foot_body_ids[i], data.qfrc_applied)
        mujoco.mj_inverse(model, data)
        # qfrc_inverse is the generalized force needed before applied
        # forces; the actuator must supply qfrc_inverse - qfrc_applied.
        tau_mj[3 * i:3 * i + 3] = (
            data.qfrc_inverse - data.qfrc_applied - base
        )[sim.dof_adr[3 * i:3 * i + 3]]

    # ANCHOR_END: mapping
    diff = np.abs(tau - tau_mj)
    denom = max(np.abs(tau).max(), 1e-12)
    print("=== 8.4 简化逆向动力学：足端力 → 关节力矩 ===")
    print(f"位形 q(FR) = [{Q_LEG[0]:.2f} {Q_LEG[1]:.2f} {Q_LEG[2]:.2f}] rad，"
          f"四条腿相同；机身姿态取单位阵")
    print("世界系足端力 (N) =")
    for i in range(4):
        print(f"  f_{i} = [{FORCES[i, 0]:+.2f} {FORCES[i, 1]:+.2f} "
              f"{FORCES[i, 2]:+.2f}]")
    print("解析 τ = [" + ", ".join(f"{v:+.4f}" for v in tau) + "] N·m")
    print("MuJoCo τ = [" + ", ".join(f"{v:+.4f}" for v in tau_mj) + "] N·m")
    print(f"最大绝对误差 = {diff.max():.3e} N·m，"
          f"最大相对误差 = {diff.max() / denom:.3e}")
    print(f"最大力矩 = {np.abs(tau).max():.4f} N·m，"
          f"占 ±{TAU_MAX:.1f} N·m 的 "
          f"{100.0 * np.abs(tau).max() / TAU_MAX:.2f} %")

    # Figure 8-5: one-leg lever arms and the twelve joint torques.
    fig = plt.figure(figsize=(11.0, 4.4))
    ax = fig.add_subplot(1, 2, 1)
    l1, l2 = params.l_thigh, params.l_calf
    q2, q3 = Q_LEG[1], Q_LEG[2]
    hip = np.array([0.0, 0.0])
    knee = np.array([-l1 * np.sin(q2), -l1 * np.cos(q2)])
    foot = knee + np.array([-l2 * np.sin(q2 + q3),
                            -l2 * np.cos(q2 + q3)])
    ax.plot([hip[0], knee[0]], [hip[1], knee[1]], "-o", color="#37474f",
            lw=3.0, ms=6)
    ax.plot([knee[0], foot[0]], [knee[1], foot[1]], "-o", color="#546e7a",
            lw=3.0, ms=6)
    ax.plot([hip[0], hip[0]], [hip[1], -0.36], ls=":", color="0.6", lw=0.9)
    f = FORCES[0]
    fs = 0.006
    ax.quiver(foot[0], foot[1], f[0] * fs, f[2] * fs, color="#1565c0",
              angles="xy", scale_units="xy", scale=1.0, width=0.006)
    ax.text(foot[0] + f[0] * fs, foot[1] + f[2] * fs,
            "  $f_s=[f_x,f_z]$", color="#1565c0", fontsize=9)
    ax.annotate("", xy=foot, xytext=(foot[0], foot[1] + 0.12),
                arrowprops=dict(arrowstyle="<->", color="#c62828", lw=1.0))
    ax.text(foot[0] + 0.006, foot[1] + 0.06, "力臂", color="#c62828",
            fontsize=9, rotation=90, va="center")
    ax.text(hip[0] - 0.01, hip[1] + 0.012, "关节 1", ha="right", fontsize=8)
    ax.text(knee[0] - 0.01, knee[1] + 0.012, "关节 2", ha="right", fontsize=8)
    ax.text(foot[0] - 0.01, foot[1] - 0.012, "关节 3", ha="right", fontsize=8)
    ax.set_aspect("equal")
    ax.set_xlim(-0.24, 0.14)
    ax.set_ylim(-0.38, 0.08)
    ax.set_xlabel("$x_b$ (m)")
    ax.set_ylabel("$z_b$ (m)")
    ax.set_title(label("单腿力臂与雅可比转置", "lever arm and $J^T$"))
    ax.grid(True, alpha=0.3)
    ax.text(-0.23, 0.045, r"$\tau_i=-J_i^{T}R^{T}f_i$", fontsize=11)

    ax = fig.add_subplot(1, 2, 2)
    idx = np.arange(12)
    ax.bar(idx, tau, width=0.62, color="#90caf9", edgecolor="#1565c0",
           label="解析 $\\tau=-J^TR^Tf$")
    ax.plot(idx, tau_mj, "ko", ms=3.0, label="MuJoCo $mj\\_inverse$")
    ax.axhline(TAU_MAX, color="#c62828", ls="--", lw=0.9)
    ax.axhline(-TAU_MAX, color="#c62828", ls="--", lw=0.9)
    ax.text(0.2, TAU_MAX + 0.8, "±33.5 N·m", color="#c62828", fontsize=8)
    ax.set_xticks(idx)
    ax.set_xticklabels([f"{leg}{j + 1}" for leg in ("FR", "FL", "RR", "RL")
                        for j in range(3)], fontsize=7, rotation=45)
    ax.set_ylabel(label("关节力矩 (N·m)", "joint torque (N·m)"))
    ax.set_title(label("12 个关节力矩的解析值与 MuJoCo 对照",
                       "analytic and MuJoCo torques"))
    ax.grid(True, axis="y", alpha=0.3)
    ax.legend(fontsize=8)
    fig.tight_layout()
    savefig(fig, "8_5_torque")
    plt.close(fig)
    sim.close()


if __name__ == "__main__":
    main()
