"""Single rigid-body dynamics: Newton + Euler + quaternion (Section 6.2).

A 12.453 kg body with the A1 equivalent inertia diag(0.1320, 0.3475, 0.3775)
is acted on by a constant world force and a constant body torque for 1 s.  The
explicit-Euler / quaternion solution is compared with a fine RK4 reference.
The gyroscopic term is then switched off to show what Eq. (6.36) discards.

expect: 单刚体参数
expect: 1 s 末态
expect: Euler 与 RK4
expect: 陀螺项
"""

from __future__ import annotations

import logging
import numpy as np

from quadruped.plotting import savefig
from quadruped.rotation import quat_mul, quat_to_R, so3_log
import matplotlib.pyplot as plt

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

MASS = 12.453
INERTIA = np.diag([0.1320, 0.3475, 0.3775])
GRAVITY = np.array([0.0, 0.0, -9.81])
FORCE = np.array([5.0, -3.0, 128.0])          # N, world frame
TORQUE = np.array([0.40, -0.25, 0.15])        # N m, body frame
T_END = 1.0


# ANCHOR: deriv
def deriv(y: np.ndarray, use_gyro: bool = True) -> np.ndarray:
    """State derivative for y = [p, v, quat(w,x,y,z), omega_b]."""
    p, v, quat, omega = y[0:3], y[3:6], y[6:10], y[10:13]
    R = quat_to_R(quat)
    d_v = FORCE / MASS + GRAVITY
    gyro = np.cross(omega, INERTIA @ omega) if use_gyro else np.zeros(3)
    d_omega = np.linalg.solve(INERTIA, TORQUE - gyro)
    d_quat = 0.5 * quat_mul(quat, np.array([0.0, *omega]))
    return np.concatenate([v, d_v, d_quat, d_omega])
# ANCHOR_END: deriv


# ANCHOR: integrators
def rk4(y: np.ndarray, dt: float, n: int, use_gyro: bool = True) -> np.ndarray:
    out = [y.copy()]
    for _ in range(n):
        k1 = deriv(y, use_gyro)
        k2 = deriv(y + 0.5 * dt * k1, use_gyro)
        k3 = deriv(y + 0.5 * dt * k2, use_gyro)
        k4 = deriv(y + dt * k3, use_gyro)
        y = y + dt * (k1 + 2 * k2 + 2 * k3 + k4) / 6.0
        y[6:10] /= np.linalg.norm(y[6:10])
        out.append(y.copy())
    return np.asarray(out)
# ANCHOR_END: integrators


def euler(y: np.ndarray, dt: float, n: int, use_gyro: bool = True) -> np.ndarray:
    out = [y.copy()]
    for _ in range(n):
        y = y + dt * deriv(y, use_gyro)
        y[6:10] /= np.linalg.norm(y[6:10])
        out.append(y.copy())
    return np.asarray(out)


def att_error(qa: np.ndarray, qb: np.ndarray) -> float:
    Ra, Rb = quat_to_R(qa), quat_to_R(qb)
    return float(np.linalg.norm(so3_log(Ra.T @ Rb)))


def draw_frame(ax, R, p, color, tag, length=0.12):
    for k, name in enumerate(("x", "y", "z")):
        d = R[:, k] * length
        ax.quiver(*p, *d, color=color, linewidth=1.5, arrow_length_ratio=0.18)
        ax.text(*(p + 1.18 * d), f"${name}_{tag}$", color=color, fontsize=9)
    ax.plot(*p, "o", color=color, ms=4)


def draw_body(ax, R, p, color="tab:blue", dx=0.17, dy=0.09, dz=0.035):
    corners = np.array([[sx * dx, sy * dy, sz * dz]
                        for sx in (-1, 1) for sy in (-1, 1) for sz in (-1, 1)])
    c = (R @ corners.T).T + p
    edges = [(0, 1), (0, 2), (0, 4), (1, 3), (1, 5), (2, 3),
             (2, 6), (3, 7), (4, 5), (4, 6), (5, 7), (6, 7)]
    for a, b in edges:
        ax.plot(*zip(c[a], c[b]), color=color, lw=1.4)
    return c


def schematic(ax):
    R = np.eye(3)
    p = np.array([0.0, 0.0, 0.32])
    feet = np.array([[0.20, -0.12, 0.02], [0.20, 0.12, 0.02],
                     [-0.20, -0.12, 0.02], [-0.20, 0.12, 0.02]])
    draw_body(ax, R, p)
    hips = feet.copy()
    hips[:, 0] *= 0.85
    hips[:, 2] = p[2] - 0.03
    for i in range(4):
        hip, foot = hips[i], feet[i]
        mid = 0.5 * (hip + foot) + np.array([0.0, 0.0, 0.04])
        ax.plot(*zip(hip, mid, foot), color="0.25", lw=1.8)
        ax.plot(*foot, "o", color="tab:red", ms=5)
    # Four foot forces (1 N = 2.5 mm in the drawing).
    for i, f in enumerate((35.0, 30.0, 28.0, 30.0)):
        ax.quiver(*feet[i], 0, 0, f / 400.0, color="tab:green",
                  arrow_length_ratio=0.22, lw=1.8)
        ax.text(*(feet[i] + np.array([0.01, 0.01, f / 400.0 + 0.02])),
                f"$f_{i}$", color="tab:green", fontsize=9)
    # Weight at the COM.
    ax.quiver(*p, 0, 0, -MASS * 9.81 / 400.0, color="tab:red",
              arrow_length_ratio=0.20, lw=1.8)
    ax.text(*(p + np.array([0.01, -0.02, -MASS * 9.81 / 400.0 - 0.03])),
            "$m g$", color="tab:red", fontsize=9)
    draw_frame(ax, R, p, "k", "b", 0.14)
    ax.plot(*p, "o", color="k", ms=4)
    ax.text(*(p + np.array([-0.03, 0.07, 0.05])), "重心 $P_g$", fontsize=9)
    ax.text(-0.25, 0.10, 0.48, "四个足端力", color="tab:green",
            fontsize=9)
    ax.set_xlim(-0.28, 0.28)
    ax.set_ylim(-0.20, 0.20)
    ax.set_zlim(-0.35, 0.58)
    ax.set_xlabel("x (m)")
    ax.set_ylabel("y (m)")
    ax.set_zlabel("z (m)")
    ax.set_box_aspect((1.0, 0.7, 1.0))
    ax.view_init(elev=20, azim=-62)
    ax.grid(False)
    for pane in (ax.xaxis.pane, ax.yaxis.pane, ax.zaxis.pane):
        pane.set_alpha(0.0)
    ax.set_title("单刚体模型：足端力与重力")


# ANCHOR: main
def main() -> None:
    print("=== 6.2 单刚体动力学：牛顿 + 欧拉 + 动能 ===")
    print(f"质量 m = {MASS:.3f} kg")
    print(f"惯性张量 I_b = diag({INERTIA[0, 0]:.4f}, {INERTIA[1, 1]:.4f}, "
          f"{INERTIA[2, 2]:.4f}) kg·m^2")
    print(f"世界系常力 F = ({FORCE[0]:.2f}, {FORCE[1]:.2f}, {FORCE[2]:.2f}) N")
    print(f"机体系常力矩 τ_b = ({TORQUE[0]:.2f}, {TORQUE[1]:.2f}, "
          f"{TORQUE[2]:.2f}) N·m")
    print(f"初始速度 v(0) = (0.45, -0.25, 0.10) m/s，"
          f"ω_b(0) = (0.6, -0.4, 0.3) rad/s")
    print(f"初始加速度 a(0) = F/m + g = "
          f"({(FORCE / MASS + GRAVITY)[0]:.4f}, "
          f"{(FORCE / MASS + GRAVITY)[1]:.4f}, "
          f"{(FORCE / MASS + GRAVITY)[2]:.4f}) m/s^2")

    y0 = np.zeros(13)
    y0[3:6] = [0.45, -0.25, 0.10]
    y0[6] = 1.0
    y0[10:13] = [0.6, -0.4, 0.3]
    dt_ref = 1e-4
    n_ref = int(round(T_END / dt_ref))
    ref = rk4(y0, dt_ref, n_ref, True)
    dt = 1e-3
    n = int(round(T_END / dt))
    eu = euler(y0, dt, n, True)
    no_gyro = euler(y0, dt, n, False)

    yf = ref[-1]
    ye = eu[-1]
    t = np.linspace(0.0, T_END, n_ref + 1)
    t_eu = np.linspace(0.0, T_END, n + 1)
    print("\n1 s 末态（Euler dt=1 ms，参考 RK4 dt=0.1 ms）：")
    print(f"  参考：p = ({yf[0]: .4f}, {yf[1]: .4f}, {yf[2]: .4f}) m")
    print(f"        v = ({yf[3]: .4f}, {yf[4]: .4f}, {yf[5]: .4f}) m/s")
    print(f"        ω_b = ({yf[10]: .4f}, {yf[11]: .4f}, {yf[12]: .4f}) rad/s")
    print(f"  Euler：p = ({ye[0]: .4f}, {ye[1]: .4f}, {ye[2]: .4f}) m")
    print(f"         v = ({ye[3]: .4f}, {ye[4]: .4f}, {ye[5]: .4f}) m/s")
    print(f"         ω_b = ({ye[10]: .4f}, {ye[11]: .4f}, {ye[12]: .4f}) rad/s")
    print(f"\nEuler 与 RK4 的差：")
    print(f"  位置差 |Δp| = {np.linalg.norm(ye[0:3] - yf[0:3]):.3e} m")
    print(f"  速度差 |Δv| = {np.linalg.norm(ye[3:6] - yf[3:6]):.3e} m/s")
    print(f"  角速度差 |Δω| = {np.linalg.norm(ye[10:13] - yf[10:13]):.3e} rad/s")
    print(f"  姿态误差 = {att_error(ye[6:10], yf[6:10]):.3e} rad")

    # Gyroscopic torque over the reference trajectory.
    gyro = np.array([np.cross(ref[k, 10:13], INERTIA @ ref[k, 10:13])
                     for k in range(len(ref))])
    print(f"\n陀螺项 ω×I_bω 的最大模长 = "
          f"{np.max(np.linalg.norm(gyro, axis=1)):.4f} N·m")
    print(f"常力矩模长 |τ_b| = {np.linalg.norm(TORQUE):.4f} N·m")
    print(f"删去陀螺项后，1 s 姿态差 = "
          f"{att_error(no_gyro[-1, 6:10], yf[6:10]):.4f} rad")

    fig = plt.figure(figsize=(13.2, 4.2))
    ax0 = fig.add_subplot(1, 3, 1, projection="3d")
    schematic(ax0)

    ax1 = fig.add_subplot(1, 3, 2, projection="3d")
    ax1.plot(ref[:, 0], ref[:, 1], ref[:, 2], color="tab:blue", lw=1.8,
             label="RK4")
    ax1.plot(eu[:, 0], eu[:, 1], eu[:, 2], color="tab:orange", lw=1.0,
             ls="--", label="Euler")
    ax1.plot(*ref[0, 0:3], "o", color="k", ms=5)
    ax1.plot(*ref[-1, 0:3], "s", color="tab:red", ms=5)
    ax1.text(*(ref[0, 0:3] + 0.02), "起点", fontsize=8)
    ax1.text(*(ref[-1, 0:3] + 0.02), "终点", fontsize=8, color="tab:red")
    ax1.set_xlabel("x (m)")
    ax1.set_ylabel("y (m)")
    ax1.set_zlabel("z (m)")
    ax1.set_box_aspect((1.0, 0.9, 0.8))
    ax1.set_title("重心三维轨迹")
    ax1.legend(fontsize=8, loc="upper left")
    ax1.grid(True, alpha=0.3)

    ax2 = fig.add_subplot(1, 3, 3)
    ax2.plot(t, ref[:, 10], color="tab:red", lw=1.4, label="$\\omega_x$")
    ax2.plot(t, ref[:, 11], color="tab:green", lw=1.4, label="$\\omega_y$")
    ax2.plot(t, ref[:, 12], color="tab:blue", lw=1.4, label="$\\omega_z$")
    ax2.plot(t_eu, no_gyro[:, 10], color="tab:red", lw=0.8, ls=":")
    ax2.plot(t_eu, no_gyro[:, 11], color="tab:green", lw=0.8, ls=":")
    ax2.plot(t_eu, no_gyro[:, 12], color="tab:blue", lw=0.8, ls=":")
    ax2.set_xlabel("时间 (s)")
    ax2.set_ylabel("角速度 (rad/s)")
    ax2.set_title("机体系角速度（实线含陀螺项，点线不含）")
    ax2.grid(True, alpha=0.3)
    ax2.legend(fontsize=8, ncol=3)
    fig.tight_layout()
    savefig(fig, "6_2_srb_dynamics")
# ANCHOR_END: main


if __name__ == "__main__":
    main()
