"""Whole-robot kinematics: body motion with fixed feet (Section 6.1).

A prescribed 1 s trajectory combines a constant world-frame translation with a
constant body angular velocity, while the four legs swing sinusoidally.  The
analytic foot velocity of Eq. (6.13) is checked against a central finite
difference of the foot position p_s(t).  One leg is then decomposed into the
convective part (body translation + rotation, joints frozen) and the relative
part (joint rotation).

expect: 解析式与中心差分
expect: 最大误差
expect: FR 足端速度分解
"""

from __future__ import annotations

import logging
import math

import numpy as np

from quadruped import A1Params, Quadruped
import matplotlib.pyplot as plt

from quadruped.plotting import savefig
from quadruped.rotation import rpy_to_R, so3_exp

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

# ---------------------------------------------------------------------------
# Trajectory definition
# ---------------------------------------------------------------------------
PARAMS = A1Params.from_menagerie()
ROBOT = Quadruped(PARAMS)
T_END = 1.0
N_SAMPLE = 1001
V_WORLD = np.array([0.18, -0.06, 0.02])          # m/s, constant
OMEGA_B = np.array([0.12, -0.08, 0.25])          # rad/s, constant in {b}
P0 = np.array([0.02, -0.01, 0.30])               # m, initial {b} origin
R0 = rpy_to_R([0.03, -0.04, 0.10])
AMP = np.array([0.05, 0.10, -0.08])              # rad, per joint
FREQ = 1.0                                       # Hz
PHASE = np.array([0.0, 0.5 * math.pi, math.pi, 1.5 * math.pi])
A = np.tile(AMP, 4)
PH = np.repeat(PHASE, 3)


# ANCHOR: trajectory
def pose(t: float) -> tuple[np.ndarray, np.ndarray]:
    """Return R_sb(t), p_sb(t)."""
    return R0 @ so3_exp(OMEGA_B * t), P0 + V_WORLD * t


def joints(t: float) -> tuple[np.ndarray, np.ndarray]:
    """Return q(t), qdot(t) with a sinusoidal swing on every joint."""
    q = PARAMS.home_q + A * np.sin(2 * math.pi * FREQ * t + PH)
    qd = A * (2 * math.pi * FREQ) * np.cos(2 * math.pi * FREQ * t + PH)
    return q, qd


def feet_world(t: float) -> np.ndarray:
    """Foot-sphere centres in {s} from forward kinematics and the body pose."""
    R, p = pose(t)
    q, _ = joints(t)
    return (R @ ROBOT.fk_all(q).T).T + p
# ANCHOR_END: trajectory


# ANCHOR: velocity
def feet_velocity_analytic(t: float) -> tuple[np.ndarray, np.ndarray]:
    """Eq. (6.13): return absolute and body-relative foot velocities."""
    R, _ = pose(t)
    q, qd = joints(t)
    pb = ROBOT.fk_all(q)
    omega = OMEGA_B
    v_s = V_WORLD
    v_abs = np.zeros((4, 3))
    v_conv = np.zeros((4, 3))
    v_rel = np.zeros((4, 3))
    for i in range(4):
        J = ROBOT.body_jacobian(q, i)
        v_conv[i] = v_s + R @ np.cross(omega, pb[i])
        v_rel[i] = R @ (J @ qd[3 * i:3 * i + 3])
        v_abs[i] = v_conv[i] + v_rel[i]
    return v_abs, v_rel
# ANCHOR_END: velocity


# ---------------------------------------------------------------------------
# Numerical check
# ---------------------------------------------------------------------------
# ANCHOR: check
def fd_check() -> tuple[float, float, np.ndarray, np.ndarray]:
    h = 1e-5
    ts = np.linspace(0.0, T_END, N_SAMPLE)
    max_err = 0.0
    max_speed = 0.0
    sample_t = 0.35
    sample = None
    for t in ts:
        v_a, _ = feet_velocity_analytic(float(t))
        v_fd = (feet_world(float(t) + h) - feet_world(float(t) - h)) / (2.0 * h)
        max_err = max(max_err, float(np.max(np.abs(v_a - v_fd))))
        max_speed = max(max_speed, float(np.max(np.linalg.norm(v_a, axis=1))))
        if abs(t - sample_t) < 1e-9:
            sample = (v_a, v_fd)
    assert sample is not None
    return max_err, max_speed, sample[0], sample[1]
# ANCHOR_END: check


def draw_frame(ax, R, p, color, tag, length=0.11):
    for k, name in enumerate(("x", "y", "z")):
        d = R[:, k] * length
        ax.quiver(*p, *d, color=color, linewidth=1.6, arrow_length_ratio=0.18)
        ax.text(*(p + 1.18 * d), f"${name}_{tag}$", color=color, fontsize=9)
    ax.plot(*p, "o", color=color, ms=4)


def draw_body(ax, R, p, color="tab:blue"):
    dx, dy, dz = 0.17, 0.09, 0.035
    corners = np.array([[sx * dx, sy * dy, sz * dz]
                        for sx in (-1, 1) for sy in (-1, 1) for sz in (-1, 1)])
    c = (R @ corners.T).T + p
    edges = [(0, 1), (0, 2), (0, 4), (1, 3), (1, 5), (2, 3),
             (2, 6), (3, 7), (4, 5), (4, 6), (5, 7), (6, 7)]
    for a, b in edges:
        ax.plot(*zip(c[a], c[b]), color=color, lw=1.5)
    return c


def fig_robot():
    """Draw a tilted standing pose with the four feet on the ground."""
    R = R0
    p = np.array([0.02, -0.01, 0.29])
    # Nominal foot prints on the ground, obtained from the home stance.
    feet = ROBOT.fk_all(PARAMS.home_q) + np.array([0.0, 0.0, 0.27])
    feet[:, 2] = 0.021
    p_body = (R.T @ (feet - p).T).T
    qfig = ROBOT.ik_all(p_body)
    feet = (R @ ROBOT.fk_all(qfig).T).T + p

    fig = plt.figure(figsize=(7.4, 6.2))
    ax = fig.add_subplot(projection="3d")
    gx, gy = np.meshgrid([-0.35, 0.42], [-0.32, 0.35])
    ax.plot_surface(gx, gy, np.zeros_like(gx), color="0.9", alpha=0.45,
                    linewidth=0, zorder=0)
    draw_body(ax, R, p)
    hips = (R @ PARAMS.hip_offset.T).T + p
    for i in range(4):
        hip, foot = hips[i], feet[i]
        mid = 0.5 * (hip + foot) + np.array([0.0, 0.0, 0.045])
        knee = 0.55 * mid + 0.45 * foot
        ax.plot(*zip(hip, mid, knee, foot), color="0.25", lw=2.0)
        ax.plot(*foot, "o", color="tab:red", ms=6)
        ax.text(*(foot + np.array([0.015, 0.015, 0.012])), f"$P_{i}$",
                color="tab:red", fontsize=10)
    draw_frame(ax, R, p, "tab:blue", "b", 0.13)
    draw_frame(ax, np.eye(3), np.zeros(3), "0.35", "s", 0.13)
    ax.text(*(p + np.array([-0.04, 0.115, 0.055])), r"$\{b\}$", color="tab:blue",
            fontsize=11)
    ax.text(*(np.array([0.025, 0.02, 0.035])), r"$\{s\}$", color="0.35",
            fontsize=11)
    ax.set_xlim(-0.3, 0.42)
    ax.set_ylim(-0.32, 0.35)
    ax.set_zlim(-0.02, 0.55)
    ax.set_xlabel("x (m)")
    ax.set_ylabel("y (m)")
    ax.set_zlabel("z (m)")
    ax.view_init(elev=24, azim=-58)
    ax.set_box_aspect((1.0, 0.95, 0.9))
    ax.grid(False)
    for pane in (ax.xaxis.pane, ax.yaxis.pane, ax.zaxis.pane):
        pane.set_alpha(0.0)
    ax.set_title("四足机器人简化图：机身系 $\\{b\\}$ 与世界系 $\\{s\\}$")
    fig.tight_layout()
    savefig(fig, "6_1_kinematics")


def fig_velocity(t_sample, v_abs, v_conv, v_rel):
    R, p = pose(t_sample)
    q, qd = joints(t_sample)
    pb = ROBOT.fk_all(q)
    foot = (R @ pb[0]) + p
    ts = np.linspace(0.0, T_END, 201)
    curve = []
    for t in ts:
        va, vr = feet_velocity_analytic(float(t))
        R2, _ = pose(float(t))
        vc = V_WORLD + R2 @ np.cross(OMEGA_B, feet_world(float(t))[0] - pose(float(t))[1])
        curve.append([np.linalg.norm(vc), np.linalg.norm(vr[0]), np.linalg.norm(va[0])])
    curve = np.asarray(curve)

    fig = plt.figure(figsize=(9.6, 4.3))
    ax = fig.add_subplot(1, 2, 1)
    v_rot = v_conv - V_WORLD
    scale = 2.8
    # Proper vector chain: translation, then rotation, then joint motion.
    chain = ((V_WORLD, "机身平移 $v_s$", "tab:blue"),
             (v_rot, "机身旋转", "tab:cyan"),
             (v_rel[0], "关节相对", "tab:green"))
    start = foot.copy()
    for v, name, color in chain:
        ax.quiver(start[0], start[1], v[0], v[1], color=color, angles="xy",
                  scale_units="xy", scale=scale, width=0.010,
                  label=f"{name} {np.linalg.norm(v):.3f} m/s")
        start = start + v / scale
    vtot = v_abs[0]
    ax.quiver(foot[0], foot[1], vtot[0], vtot[1], color="k", angles="xy",
              scale_units="xy", scale=scale, width=0.006,
              label=f"合成 {np.linalg.norm(vtot):.3f} m/s")
    ax.plot(*foot[:2], "o", color="tab:red", ms=7)
    ax.text(foot[0] + 0.008, foot[1] - 0.024, "FR 足端", color="tab:red",
            fontsize=9)
    ax.text(0.98, 0.02, "箭头比例：1 m/s = 0.36 m", transform=ax.transAxes,
            ha="right", va="bottom", fontsize=7.5, color="0.35")
    ax.set_aspect("equal", adjustable="box")
    ax.set_xlim(foot[0] - 0.15, foot[0] + 0.18)
    ax.set_ylim(foot[1] - 0.16, foot[1] + 0.14)
    ax.set_xlabel("x (m)")
    ax.set_ylabel("y (m)")
    ax.set_title(f"t={t_sample:.2f} s：速度矢量在 x-y 平面的投影")
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=7.2, loc="upper left")

    ax2 = fig.add_subplot(1, 2, 2)
    ax2.plot(ts, curve[:, 0], color="tab:blue", lw=1.4, label="牵连")
    ax2.plot(ts, curve[:, 1], color="tab:green", lw=1.4, label="相对")
    ax2.plot(ts, curve[:, 2], color="k", lw=1.8, label="合成")
    ax2.axvline(t_sample, color="0.6", ls="--", lw=1.0)
    ax2.set_xlabel("时间 (s)")
    ax2.set_ylabel("速度模长 (m/s)")
    ax2.set_title("FR 足端速度模长随时间的变化")
    ax2.grid(True, alpha=0.3)
    ax2.legend(fontsize=8)
    fig.tight_layout()
    savefig(fig, "6_1_velocity")


def main() -> None:
    print("=== 6.1 整机运动学：机身动、足端不动 ===")
    q, qd = joints(0.0)
    R, p = pose(0.0)
    print(f"初始机身原点 p_sb(0) = ({p[0]:.4f}, {p[1]:.4f}, {p[2]:.4f}) m")
    print(f"初始足端 P0 在 {{b}} 中 p_b0(0) = "
          f"({ROBOT.fk_all(q)[0, 0]:.4f}, {ROBOT.fk_all(q)[0, 1]:.4f}, "
          f"{ROBOT.fk_all(q)[0, 2]:.4f}) m")
    feet0 = (R @ ROBOT.fk_all(q).T).T + p
    print("初始足端世界坐标：")
    for i in range(4):
        print(f"  P{i} = ({feet0[i, 0]: .4f}, {feet0[i, 1]: .4f}, "
              f"{feet0[i, 2]: .4f}) m")
    print(f"机身速度 v_s = ({V_WORLD[0]:.4f}, {V_WORLD[1]:.4f}, "
          f"{V_WORLD[2]:.4f}) m/s，角速度 ω_b = ({OMEGA_B[0]:.4f}, "
          f"{OMEGA_B[1]:.4f}, {OMEGA_B[2]:.4f}) rad/s")

    err, vmax, va, vfd = fd_check()
    print("\n解析式与中心差分（1001 个时刻、4 条腿、h=1e-5 s）：")
    print(f"  最大误差 max |v_analytic - v_fd| = {err:.3e} m/s")
    print(f"  足端最大速度 |v|max = {vmax:.4f} m/s，相对误差 {err / vmax:.2e}")

    t_sample = 0.35
    v_abs, v_rel = feet_velocity_analytic(t_sample)
    R, p = pose(t_sample)
    q, qd = joints(t_sample)
    pb = ROBOT.fk_all(q)
    v_conv = V_WORLD + R @ np.cross(OMEGA_B, pb[0])
    print(f"\nFR 足端速度分解 @ t={t_sample} s：")
    print(f"  牵连（机身平移 + 旋转） = ({v_conv[0]: .4f}, {v_conv[1]: .4f}, "
          f"{v_conv[2]: .4f}) m/s")
    print(f"  相对（三个关节转动）     = ({v_rel[0, 0]: .4f}, {v_rel[0, 1]: .4f}, "
          f"{v_rel[0, 2]: .4f}) m/s")
    print(f"  合成                     = ({v_abs[0, 0]: .4f}, {v_abs[0, 1]: .4f}, "
          f"{v_abs[0, 2]: .4f}) m/s")
    print(f"  合成模长 |v| = {np.linalg.norm(v_abs[0]):.4f} m/s")
    print(f"  世界系下足端相对机身速度 = ({v_rel[0, 0]: .4f}, "
          f"{v_rel[0, 1]: .4f}, {v_rel[0, 2]: .4f}) m/s")

    fig_robot()
    fig_velocity(t_sample, v_abs, v_conv, v_rel)


if __name__ == "__main__":
    main()
