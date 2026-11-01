"""Pose PD on the discrete second-order model (Section 8.1).

The position loop is simulated with the same update used in the book:

    p[k+1] = p[k] + dt v[k] + dt^2/2 a[k]
    v[k+1] = v[k] + dt a[k]

Three gain sets are compared: a slow critically damped loop, a fast
critically damped loop, and an underdamped loop.  The attitude loop applies
the SO(3) error ``log(R_des R^T)`` and reports the axis-angle error.

expect: 位置环
expect: kp=1600
expect: 欠阻尼
expect: 姿态环
expect: 轴角
"""

from __future__ import annotations

import logging
import math
import os

os.environ.setdefault("MESA_SHADER_CACHE_DISABLE", "true")

import numpy as np
from quadruped.plotting import savefig, label
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D  # noqa: F401  (registers 3d projection)

from quadruped.rotation import so3_exp, so3_log

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

DT = 0.002
TARGET = 0.100
GAINS = [
    ("慢临界阻尼", 100.0, 2.0 * math.sqrt(100.0) - 0.5 * DT * 100.0),
    ("快临界阻尼", 1600.0, 2.0 * math.sqrt(1600.0) - 0.5 * DT * 1600.0),
    ("欠阻尼", 1600.0, 20.0),
]


# ANCHOR: position_loop
def position_loop(kp: float, kd: float, duration: float = 1.5
                  ) -> tuple[np.ndarray, np.ndarray]:
    """Integrate the one-axis loop and return time and position."""
    n = int(duration / DT)
    t = np.arange(n) * DT
    p = np.zeros(n)
    v = 0.0
    for k in range(n):
        a = kp * (TARGET - p[k]) - kd * v
        if k + 1 < n:
            p[k + 1] = p[k] + DT * v + 0.5 * DT * DT * a
        v = v + DT * a
    return t, p
# ANCHOR_END: position_loop


def step_metrics(t: np.ndarray, p: np.ndarray) -> tuple[float, float, float]:
    """Overshoot (%), settling time to 2 % (s) and steady-state error (m)."""
    overshoot = max(0.0, (p.max() - TARGET) / TARGET * 100.0)
    band = 0.02 * TARGET
    outside = np.flatnonzero(np.abs(p - TARGET) > band)
    settling = (outside[-1] + 1) * DT if outside.size else 0.0
    steady = float(np.mean(p[-int(0.2 / DT):]) - TARGET)
    return overshoot, settling, steady


def sine_tracking(kp: float, kd: float, duration: float = 2.0
                  ) -> tuple[float, float]:
    """RMS and peak tracking error for a 0.5 Hz sine target (mm)."""
    n = int(duration / DT)
    p = 0.0
    v = 0.0
    errors = []
    for k in range(n):
        t = k * DT
        w = 2.0 * math.pi * 0.5
        r = 0.05 * math.sin(w * t)
        dr = 0.05 * w * math.cos(w * t)
        a = kp * (r - p) + kd * (dr - v)
        p = p + DT * v + 0.5 * DT * DT * a
        v = v + DT * a
        if t > 1.0:
            errors.append(r - p)
    errors = np.asarray(errors)
    return float(np.sqrt(np.mean(errors ** 2)) * 1000.0), float(np.abs(errors).max() * 1000.0)


# ANCHOR: attitude_loop
def attitude_loop(kp: float, theta0: float = 0.8, duration: float = 2.0
                  ) -> dict:
    """Integrate Rdot = [omega]x R with the PD law of Eq. (8-22)."""
    kd = 2.0 * math.sqrt(kp)
    axis = np.array([0.6, 0.4, -0.5])
    axis = axis / np.linalg.norm(axis)
    R = so3_exp(axis * theta0)
    omega = np.zeros(3)
    n = int(duration / DT)
    t = np.arange(n) * DT
    angle = np.zeros(n)
    speed = np.zeros(n)
    for k in range(n):
        e_R = so3_log(R.T)              # R_des = I, so R_err = R^T
        angle[k] = np.linalg.norm(e_R)
        alpha = kp * e_R - kd * omega
        omega = omega + DT * alpha
        speed[k] = np.linalg.norm(omega)
        R = so3_exp(omega * DT) @ R
    final = float(np.linalg.norm(so3_log(R.T)))
    band = 0.01 * theta0
    outside = np.flatnonzero(angle > band)
    settling = (outside[-1] + 1) * DT if outside.size else 0.0
    return {"t": t, "angle": angle, "speed": speed, "final": final,
            "settling": settling, "max_speed": float(speed.max()),
            "R": R}
# ANCHOR_END: attitude_loop


def draw_triad(ax, R, origin, color, name):
    for i, (c, axis_name) in enumerate(zip(("r", "g", "b"), ("x", "y", "z"))):
        v = R[:, i] * 0.09
        ax.quiver(*origin, *v, color=c, arrow_length_ratio=0.15, lw=1.4)
        ax.text(*(origin + v * 1.08), f"${axis_name}_{name}$", color=c, fontsize=9)


def main() -> None:
    print("=== 8.1 位姿 PD：离散二阶模型 ===")
    print(f"位置环 dt = {DT:.3f} s，目标阶跃 = {TARGET:.3f} m")
    curves = []
    for name, kp, kd in GAINS:
        t, p = position_loop(kp, kd)
        over, settle, steady = step_metrics(t, p)
        lam = 1.0 - DT * math.sqrt(kp) if kd >= 2.0 * math.sqrt(kp) - 0.5 * DT * kp else None
        curves.append((name, kp, kd, t, p, over, settle, steady, lam))
        lam_text = f"{lam:.4f}" if lam is not None else "—"
        print(f"{name}: kp = {kp:.1f}, kd = {kd:.3f}, lambda = {lam_text}, "
              f"超调 = {over:.2f} %, 调节时间 = {settle:.3f} s, "
              f"稳态误差 = {steady * 1000.0:+.4f} mm")

    rms, peak = sine_tracking(1600.0, 2.0 * math.sqrt(1600.0) - DT * 800.0)
    print(f"正弦目标 0.5 Hz、幅值 0.05 m：RMS 误差 = {rms:.4f} mm，"
          f"最大误差 = {peak:.4f} mm")

    att = {}
    for kp in (40.0, 100.0):
        att[kp] = attitude_loop(kp)
    a100 = att[100.0]
    print(f"姿态环 kp = 100.0, kd = {2.0 * math.sqrt(100.0):.1f}："
          f"初始轴角 = 0.8000 rad，终止误差 = {a100['final']:.2e} rad，"
          f"1 % 调节时间 = {a100['settling']:.3f} s，"
          f"最大角速度 = {a100['max_speed']:.4f} rad/s")
    print("轴角误差按 log(R_des R^T) 计算，方向为旋转轴，模长为旋转角。")

    # Figure 8-1: eigenvalue powers and the three step responses.
    fig, axes = plt.subplots(1, 2, figsize=(10.4, 3.8))
    ax = axes[0]
    k = np.arange(0, 11)
    for lam, marker, name in ((0.3, "^", r"$\lambda=0.3$"),
                              (0.8, "s", r"$\lambda=0.8$")):
        ax.plot(k, lam ** k, marker=marker, ms=5, lw=1.0, label=name)
    ax.set_xlabel(label("步数 $k$", "step $k$"))
    ax.set_ylabel(r"$\lambda^k$")
    ax.set_title(label("特征值的幂决定收敛速度", "eigenvalue powers"))
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=8)

    ax = axes[1]
    for name, kp, kd, t, p, over, settle, steady, lam in curves:
        ax.plot(t, p * 1000.0, lw=1.4, label=f"{name} ($k_p$={kp:.0f})")
    ax.axhline(TARGET * 1000.0, color="0.5", ls="--", lw=0.9)
    ax.axhspan((TARGET - 0.02 * TARGET) * 1000.0,
               (TARGET + 0.02 * TARGET) * 1000.0, color="0.8", alpha=0.35)
    ax.set_xlim(0.0, 0.8)
    ax.set_xlabel(label("时间 (s)", "time (s)"))
    ax.set_ylabel(label("位置 (mm)", "position (mm)"))
    ax.set_title(label("位置阶跃：临界阻尼与欠阻尼", "step response"))
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=8)
    fig.tight_layout()
    savefig(fig, "8_1_convergence")
    plt.close(fig)

    # Figure 8-2: axis-angle picture and the attitude error norm.
    fig = plt.figure(figsize=(10.6, 4.2))
    ax = fig.add_subplot(1, 2, 1, projection="3d")
    origin = np.zeros(3)
    draw_triad(ax, np.eye(3), origin, "k", "d")
    draw_triad(ax, so3_exp(np.array([0.6, 0.4, -0.5]) / np.linalg.norm([0.6, 0.4, -0.5]) * 0.8),
               origin, "0.45", "0")
    axis = np.array([0.6, 0.4, -0.5])
    axis = axis / np.linalg.norm(axis)
    ax.quiver(0, 0, 0, *(axis * 0.13), color="m", lw=2.0,
              arrow_length_ratio=0.12)
    ax.text(*(axis * 0.14), r"$\hat{\omega}$", color="m", fontsize=11)
    arc = np.array([so3_exp(axis * s) @ np.array([0.0, 0.0, 0.12])
                    for s in np.linspace(0.0, 0.8, 40)])
    ax.plot(arc[:, 0], arc[:, 1], arc[:, 2], color="m", lw=1.2)
    ax.text(*(arc[20] * 1.08), r"$\theta$", color="m", fontsize=11)
    ax.set_xlim(-0.05, 0.14)
    ax.set_ylim(-0.05, 0.14)
    ax.set_zlim(-0.05, 0.14)
    ax.set_xlabel("$x_s$")
    ax.set_ylabel("$y_s$")
    ax.set_zlabel("$z_s$")
    ax.set_title(label("姿态误差的轴—角表示", "axis-angle error"))
    ax.view_init(elev=24, azim=-58)

    ax = fig.add_subplot(1, 2, 2)
    for kp in (40.0, 100.0):
        ax.plot(att[kp]["t"], np.degrees(att[kp]["angle"]), lw=1.4,
                label=f"$k_p$={kp:.0f}")
    ax.set_xlabel(label("时间 (s)", "time (s)"))
    ax.set_ylabel(label("姿态误差角 (deg)", "attitude error (deg)"))
    ax.set_title(label("误差范数：$k_p$ 越大收敛越快", "error norm"))
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=8)
    fig.tight_layout()
    savefig(fig, "8_2_attitude")
    plt.close(fig)


if __name__ == "__main__":
    main()
