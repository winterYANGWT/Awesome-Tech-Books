"""Foot forces to base acceleration: the 6x12 contact map (Section 6.4).

Four world-frame foot forces act on a free rigid body with the A1 mass and the
equivalent inertia of Eq. (6.32).  The resultant force, resultant moment,
linear acceleration and angular acceleration are compared with a MuJoCo free
body carrying the same four point forces.  The second case keeps only two
stance feet and starts with a non-zero body rate, so the gyroscopic term of
Eq. (6.35) is exercised as well.

expect: 四个足端力与接触集
expect: 合力与合力矩
expect: MuJoCo 对照
expect: 两足支撑并带初始角速度
"""

from __future__ import annotations

import logging

import numpy as np
import mujoco

from quadruped import A1Params, Quadruped
from quadruped.rotation import rpy_to_R
from quadruped.plotting import savefig
import matplotlib.pyplot as plt

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

MASS = 12.453
INERTIA = np.diag([0.1320, 0.3475, 0.3775])
GRAVITY = np.array([0.0, 0.0, -9.81])
PARAMS = A1Params.from_menagerie()
ROBOT = Quadruped(PARAMS)
FORCES = np.array([[14.0, -4.0, 38.0],
                   [8.0, 3.0, 35.0],
                   [-2.0, 5.0, 34.0],
                   [3.0, -2.0, 36.0]])        # N, ground on foot
CONTACT = np.array([True, True, True, True])
CONTACT2 = np.array([True, False, True, False])


def free_body_model() -> tuple[mujoco.MjModel, int]:
    spec = mujoco.MjSpec()
    body = spec.worldbody.add_body(name="base")
    body.add_freejoint()
    body.mass = MASS
    body.inertia = np.diag(INERTIA).copy()
    body.iquat = np.array([1.0, 0.0, 0.0, 0.0])
    body.ipos = np.zeros(3)
    body.add_geom(type=mujoco.mjtGeom.mjGEOM_BOX,
                  size=[0.15, 0.09, 0.035], rgba=[0.4, 0.6, 0.9, 0.4],
                  contype=0, conaffinity=0, mass=0.0)
    model = spec.compile()
    return model, body.id


def base_feet(R: np.ndarray, p_com: np.ndarray) -> np.ndarray:
    """Foot positions in world for the home stance, relative to the COM."""
    return (R @ (ROBOT.fk_all(PARAMS.home_q) - PARAMS.com_body_home).T).T + p_com


# ANCHOR: srb
def srb(F: np.ndarray, feet: np.ndarray, p_com: np.ndarray,
        R: np.ndarray, omega_b: np.ndarray):
    """Eqs. (6.38)-(6.40): resultant force, moment, and accelerations."""
    f_sum = F.sum(axis=0)
    r = feet - p_com
    tau = np.zeros(3)
    for i in range(4):
        tau += np.cross(r[i], F[i])
    a = f_sum / MASS + GRAVITY
    RIR = R @ INERTIA @ R.T
    omega_s = R @ omega_b
    alpha_s = np.linalg.solve(RIR, tau - np.cross(omega_s, RIR @ omega_s))
    return f_sum, tau, a, R.T @ alpha_s
# ANCHOR_END: srb


# ANCHOR: compare
def mujoco_acc(F: np.ndarray, feet: np.ndarray, p_com: np.ndarray,
               R: np.ndarray, omega_b: np.ndarray,
               contact: np.ndarray):
    """Apply the four forces at the foot points and return qacc of the base."""
    model, body_id = free_body_model()
    data = mujoco.MjData(model)
    quat = np.zeros(4)
    mujoco.mju_mat2Quat(quat, R.reshape(-1))
    data.qpos[0:3] = p_com
    data.qpos[3:7] = quat
    data.qvel[0:3] = 0.0
    data.qvel[3:6] = omega_b
    mujoco.mj_forward(model, data)
    data.qfrc_applied[:] = 0.0
    for i in range(4):
        if contact[i]:
            mujoco.mj_applyFT(model, data, F[i], np.zeros(3), feet[i],
                              body_id, data.qfrc_applied)
    mujoco.mj_forward(model, data)
    return data.qacc[0:3].copy(), data.qacc[3:6].copy()
# ANCHOR_END: compare


def draw_frame(ax, R, p, color, tag, length=0.12):
    for k, name in enumerate(("x", "y", "z")):
        d = R[:, k] * length
        ax.quiver(*p, *d, color=color, linewidth=1.4, arrow_length_ratio=0.18)
        ax.text(*(p + 1.2 * d), f"${name}_{tag}$", color=color, fontsize=8.5)


def draw_box(ax, R, p, color="tab:blue", dx=0.17, dy=0.09, dz=0.035):
    corners = np.array([[sx * dx, sy * dy, sz * dz]
                        for sx in (-1, 1) for sy in (-1, 1) for sz in (-1, 1)])
    c = (R @ corners.T).T + p
    edges = [(0, 1), (0, 2), (0, 4), (1, 3), (1, 5), (2, 3),
             (2, 6), (3, 7), (4, 5), (4, 6), (5, 7), (6, 7)]
    for a, b in edges:
        ax.plot(*zip(c[a], c[b]), color=color, lw=1.4)


def figure(F, feet, p_com, R):
    fig = plt.figure(figsize=(8.0, 6.4))
    ax = fig.add_subplot(projection="3d")
    p_body = p_com - R @ PARAMS.com_body_home
    draw_box(ax, R, p_body)
    for i in range(4):
        ax.plot(*feet[i], "o", color="tab:red", ms=5)
        ax.text(*(feet[i] + np.array([0.012, 0.012, 0.008])), f"$P_{i}$",
                color="tab:red", fontsize=9)
        ax.quiver(*feet[i], *(F[i] * 0.003), color="tab:green",
                  arrow_length_ratio=0.20, lw=1.8)
        ax.text(*(feet[i] + F[i] * 0.003 + np.array([0.0, 0.0, 0.018])),
                f"$f_{i}$", color="tab:green", fontsize=8.5)
        # Moment arm from the COM.
        ax.plot(*zip(p_com, feet[i]), color="0.55", ls="--", lw=0.9)
    f_sum = F.sum(axis=0)
    ax.quiver(*p_com, *(f_sum * 0.0015), color="k", arrow_length_ratio=0.16,
              lw=2.2)
    ax.text(*(p_com + f_sum * 0.0015 + np.array([0.02, 0.02, 0.015])),
            r"$\sum f_i$", fontsize=10)
    ax.quiver(*p_com, 0, 0, -MASS * 9.81 * 0.002, color="tab:red",
              arrow_length_ratio=0.18, lw=1.6)
    ax.text(*(p_com + np.array([0.01, 0.01, -MASS * 9.81 * 0.002 - 0.02])),
            "$m g$", color="tab:red", fontsize=9)
    ax.plot(*p_com, "o", color="k", ms=5)
    ax.text(*(p_com + np.array([0.015, 0.09, 0.05])), "$P_g$", fontsize=10)
    ax.text(*(p_body + np.array([0.0, -0.14, 0.02])), "$P_b$", fontsize=10)
    draw_frame(ax, R, p_body, "tab:blue", "b", 0.12)
    from matplotlib.lines import Line2D
    handles = [Line2D([0], [0], color="tab:green", lw=2, label="足端力 $f_i$"),
               Line2D([0], [0], color="k", lw=2, label=r"合力 $\sum f_i$"),
               Line2D([0], [0], color="tab:red", lw=2, label="重力 $mg$"),
               Line2D([0], [0], color="0.55", ls="--", lw=1,
                      label="力臂 $p_i-p_g$")]
    ax.legend(handles=handles, fontsize=7.5, loc="upper left")
    ax.set_xlim(-0.36, 0.36)
    ax.set_ylim(-0.30, 0.30)
    ax.set_zlim(0.0, 0.62)
    ax.set_xlabel("x (m)")
    ax.set_ylabel("y (m)")
    ax.set_zlabel("z (m)")
    ax.set_box_aspect((1.0, 0.8, 0.8))
    ax.view_init(elev=22, azim=-60)
    ax.grid(False)
    for pane in (ax.xaxis.pane, ax.yaxis.pane, ax.zaxis.pane):
        pane.set_alpha(0.0)
    ax.set_title("四个足端力与重心至足端的力臂")
    fig.tight_layout()
    savefig(fig, "6_4_dynamics")


# ANCHOR: main
def main() -> None:
    print("=== 6.4 四足机器人的动力学简化方程 ===")
    R = rpy_to_R([0.08, -0.05, 0.17])
    p_com = np.array([0.08, -0.04, 0.30])
    feet = base_feet(R, p_com)
    print("机身姿态 roll/pitch/yaw = (0.080, -0.050, 0.170) rad")
    print(f"重心 p_g = ({p_com[0]:.4f}, {p_com[1]:.4f}, {p_com[2]:.4f}) m")
    print("四个足端的世界坐标：")
    for i in range(4):
        print(f"  P{i} = ({feet[i, 0]: .4f}, {feet[i, 1]: .4f}, "
              f"{feet[i, 2]: .4f}) m")

    print("\n四个足端力与接触集（地面对足端，向上为正）：")
    for i in range(4):
        print(f"  f{i} = ({FORCES[i, 0]: .2f}, {FORCES[i, 1]: .2f}, "
              f"{FORCES[i, 2]: .2f}) N, c{i} = {int(CONTACT[i])}")

    f_sum, tau, a, alpha_b = srb(FORCES, feet, p_com, R, np.zeros(3))
    print("\n合力与合力矩：")
    print(f"  Σf_i = ({f_sum[0]: .4f}, {f_sum[1]: .4f}, {f_sum[2]: .4f}) N")
    print(f"  Σ(p_i - p_g) × f_i = ({tau[0]: .4f}, {tau[1]: .4f}, "
          f"{tau[2]: .4f}) N·m")
    print(f"  单刚体加速度 a = ({a[0]: .4f}, {a[1]: .4f}, {a[2]: .4f}) m/s^2")
    print(f"  单刚体角加速度 α_b = ({alpha_b[0]: .4f}, {alpha_b[1]: .4f}, "
          f"{alpha_b[2]: .4f}) rad/s^2")

    a_mj, alpha_mj = mujoco_acc(FORCES, feet, p_com, R, np.zeros(3), CONTACT)
    print("\nMuJoCo 对照（同一组点力，自由刚体）：")
    print(f"  a_mj = ({a_mj[0]: .4f}, {a_mj[1]: .4f}, {a_mj[2]: .4f}) m/s^2")
    print(f"  α_mj = ({alpha_mj[0]: .4f}, {alpha_mj[1]: .4f}, "
          f"{alpha_mj[2]: .4f}) rad/s^2")
    print(f"  加速度误差 = {np.max(np.abs(a - a_mj)):.3e} m/s^2")
    print(f"  角加速度误差 = {np.max(np.abs(alpha_b - alpha_mj)):.3e} rad/s^2")

    # Two stance feet, non-zero initial body rate.
    F2 = FORCES.copy()
    F2[~CONTACT2] = 0.0
    omega_b = np.array([0.80, -0.50, 0.40])
    f_sum2, tau2, a2, alpha_b2 = srb(F2, feet, p_com, R, omega_b)
    a_mj2, alpha_mj2 = mujoco_acc(F2, feet, p_com, R, omega_b, CONTACT2)
    print("\n两足支撑并带初始角速度 ω_b = (0.80, -0.50, 0.40) rad/s：")
    print(f"  接触集 c = ({int(CONTACT2[0])}, {int(CONTACT2[1])}, "
          f"{int(CONTACT2[2])}, {int(CONTACT2[3])})，摆动足力置零")
    print(f"  Σf_i = ({f_sum2[0]: .4f}, {f_sum2[1]: .4f}, {f_sum2[2]: .4f}) N")
    print(f"  Σ(p_i - p_g) × f_i = ({tau2[0]: .4f}, {tau2[1]: .4f}, "
          f"{tau2[2]: .4f}) N·m")
    print(f"  α_b（含陀螺项） = ({alpha_b2[0]: .4f}, {alpha_b2[1]: .4f}, "
          f"{alpha_b2[2]: .4f}) rad/s^2")
    print(f"  MuJoCo α_b = ({alpha_mj2[0]: .4f}, {alpha_mj2[1]: .4f}, "
          f"{alpha_mj2[2]: .4f}) rad/s^2")
    print(f"  加速度误差 = {np.max(np.abs(a2 - a_mj2)):.3e} m/s^2")
    print(f"  角加速度误差 = {np.max(np.abs(alpha_b2 - alpha_mj2)):.3e} rad/s^2")

    figure(FORCES, feet, p_com, R)


# ANCHOR_END: main


if __name__ == "__main__":
    main()
