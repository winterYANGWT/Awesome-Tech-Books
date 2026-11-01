# expect: 摆线
# expect: 五次多项式
# expect: 最大高度
# expect: 起落速度
# expect: 加速度峰值
"""Swing-leg trajectory: the cycloid and a piecewise quintic (9.3).

Step length L = 0.12 m, lift height h = 0.06 m, swing time T = 0.2 s.  The
cycloid is Eq. (9-14); the alternative is a quintic that rises 0 -> h in the
first half and falls back in the second half, with zero velocity and zero
acceleration at both ends and at the apex.  Both have the same endpoints;
the script compares their peak speed and acceleration.

"""

from __future__ import annotations

import logging

import numpy as np

from quadruped.plotting import label, savefig
import matplotlib.pyplot as plt
from matplotlib.patches import Arc, Circle, FancyArrowPatch

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

L = 0.12
H = 0.06
T = 0.20
N = 4001


# ANCHOR: cycloid
def cycloid(p: np.ndarray) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    """Position, velocity and acceleration of Eq. (9-14)/(9-15)."""
    th = 2.0 * np.pi * p
    x = L * (th - np.sin(th)) / (2.0 * np.pi)
    z = 0.5 * H * (1.0 - np.cos(th))
    vx = L / T * (1.0 - np.cos(th))
    vz = np.pi * H / T * np.sin(th)
    ax = 2.0 * np.pi * L / T ** 2 * np.sin(th)
    az = 2.0 * np.pi ** 2 * H / T ** 2 * np.cos(th)
    return np.column_stack([x, z]), np.column_stack([vx, vz]), np.column_stack([ax, az])


# ANCHOR_END: cycloid


def q5(s: np.ndarray) -> np.ndarray:
    return 10.0 * s ** 3 - 15.0 * s ** 4 + 6.0 * s ** 5


def q5d(s: np.ndarray) -> np.ndarray:
    return 30.0 * s ** 2 * (1.0 - s) ** 2


def q5dd(s: np.ndarray) -> np.ndarray:
    return 60.0 * s - 180.0 * s ** 2 + 120.0 * s ** 3


# ANCHOR: quintic
def quintic(p: np.ndarray) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    """Piecewise quintic: x smoothstep over T, z rises to h at T/2."""
    first = p <= 0.5
    s = np.where(first, 2.0 * p, 2.0 * (1.0 - p))
    x = L * q5(p)
    z = H * q5(s)
    vx = L / T * q5d(p)
    vz = np.where(first, 1.0, -1.0) * 2.0 * H / T * q5d(s)
    ax = L / T ** 2 * q5dd(p)
    az = 4.0 * H / T ** 2 * q5dd(s)
    return np.column_stack([x, z]), np.column_stack([vx, vz]), np.column_stack([ax, az])


# ANCHOR_END: quintic


def report(name: str, t: np.ndarray, pos: np.ndarray, vel: np.ndarray,
           acc: np.ndarray) -> None:
    speed = np.linalg.norm(vel, axis=1)
    anorm = np.linalg.norm(acc, axis=1)
    print(f"{name}：最大高度 {pos[:, 1].max() * 1000:.2f} mm"
          f"（t = {t[np.argmax(pos[:, 1])] * 1000:.1f} ms），"
          f"起落速度 {speed[0]:.4f} / {speed[-1]:.4f} m/s")
    print(f"  最大速度 {speed.max():.4f} m/s（t = {t[np.argmax(speed)] * 1000:.1f} ms），"
          f"最大加速度 {anorm.max():.2f} m/s²")
    print(f"  加速度峰值 |ax| = {np.abs(acc[:, 0]).max():.2f}，"
          f"|az| = {np.abs(acc[:, 1]).max():.2f} m/s²；"
          f"端点加速度 ({acc[0, 0]:.2f}, {acc[0, 1]:.2f}) m/s²")


def figure_geometry() -> None:
    R = 0.5
    theta = 3.6
    fig, ax = plt.subplots(figsize=(7.4, 4.2))
    ax.plot([-0.1, 2 * np.pi * R + 0.15], [0, 0], color="0.2", lw=1.6)
    for x in np.arange(-0.08, 2 * np.pi * R + 0.14, 0.11):
        ax.plot([x, x - 0.06], [0, -0.06], color="0.5", lw=0.7)
    th = np.linspace(0.0, 2.0 * np.pi, 600)
    cx = R * (th - np.sin(th))
    cz = R * (1.0 - np.cos(th))
    ax.plot(cx, cz, ls="--", color="C0", lw=1.5)
    ax.add_patch(Circle((0, R), R, fill=False, ls=":", ec="0.6", lw=0.9))
    ax.add_patch(Circle((R * theta, R), R, fill=False, ec="0.35", lw=1.2))
    ax.add_patch(Circle((2 * np.pi * R, R), R, fill=False, ls=":", ec="0.6", lw=0.9))
    A2 = np.array([R * theta - R * np.sin(theta), R - R * np.cos(theta)])
    P = np.array([R * theta, 0.0])
    O2 = np.array([R * theta, R])
    ax.plot([0, 2 * np.pi * R], [2 * R, 2 * R], ls=":", color="0.65", lw=0.8)
    ax.text(0.08, 2 * R + 0.04, r"$z_{\max}=2R$", color="0.35", fontsize=9)
    ax.annotate("", xy=(2 * np.pi * R, -0.13), xytext=(0, -0.13),
                arrowprops=dict(arrowstyle="<->", color="0.4", lw=0.9))
    ax.text(np.pi * R, -0.19, r"$2\pi R$", ha="center", color="0.3")
    ax.annotate("", xy=(O2[0], O2[1]), xytext=(P[0], P[1]),
                arrowprops=dict(arrowstyle="<->", color="C3", lw=1.0))
    ax.text(O2[0] + 0.04, R / 2, r"$R$", color="C3")
    ax.plot([O2[0], A2[0]], [O2[1], A2[1]], color="C3", lw=1.0)
    ax.add_patch(Arc(tuple(O2), 0.55, 0.55, theta1=-90 - np.degrees(theta),
                     theta2=-90, color="C3", lw=1.0))
    mid = np.deg2rad(-90 - 0.5 * np.degrees(theta))
    ax.text(O2[0] + 0.31 * np.cos(mid), O2[1] + 0.31 * np.sin(mid),
            r"$\theta$", color="C3")

    ax.plot(0, 0, "o", color="0.1", ms=5)
    ax.plot(*A2, "o", color="C0", ms=5)
    ax.plot(2 * np.pi * R, 0, "o", color="0.1", ms=5)
    ax.plot(0, R, "o", color="0.4", ms=4)
    ax.plot(*O2, "o", color="0.4", ms=4)
    ax.plot(*P, "o", color="C3", ms=4)
    ax.text(-0.05, 0.05, r"$A_1$", ha="right")
    ax.text(A2[0] + 0.04, A2[1] + 0.04, r"$A_2$")
    ax.text(2 * np.pi * R + 0.04, 0.03, r"$A_3$")
    ax.text(-0.06, R, r"$O$", ha="right")
    ax.text(O2[0] + 0.03, O2[1] + 0.05, r"$O_2$")
    ax.text(P[0] - 0.02, -0.10, r"$P$", ha="center", color="C3")
    ax.annotate("", xy=(0.55, 0.92), xytext=(0.35, 0.92),
                arrowprops=dict(arrowstyle="-|>", color="0.2", lw=1.2))
    ax.text(0.45, 0.97, label("纯滚动", "rolling"), ha="center", fontsize=9)

    ax.set_xlim(-0.22, 2 * np.pi * R + 0.25)
    ax.set_ylim(-0.26, 2 * R + 0.22)
    ax.set_aspect("equal")
    ax.set_xlabel(r"$x$ / m")
    ax.set_ylabel(r"$z$ / m")
    ax.set_title(label("半径 R 的圆纯滚动一周，圆上一点划出一条摆线",
                       "a cycloid traced by a rolling circle"), fontsize=10)
    fig.tight_layout()
    savefig(fig, "9_3_geometry")
    plt.close(fig)


def figure_compare(t: np.ndarray, pc: np.ndarray, vc: np.ndarray, ac: np.ndarray,
                   pq: np.ndarray, vq: np.ndarray, aq: np.ndarray) -> None:
    fig, axes = plt.subplots(2, 2, figsize=(9.8, 5.8), sharex=True)
    ax = axes[0, 0]
    ax.plot(t * 1000, pc[:, 0] * 1000, color="C0", lw=2, label="cycloid")
    ax.plot(t * 1000, pq[:, 0] * 1000, color="C3", lw=1.6, ls="--", label="quintic")
    ax.set_ylabel(label("x / mm", "x / mm"))
    ax.set_title(label("水平位置", "horizontal position"), fontsize=9)

    ax = axes[0, 1]
    ax.plot(t * 1000, pc[:, 1] * 1000, color="C0", lw=2)
    ax.plot(t * 1000, pq[:, 1] * 1000, color="C3", lw=1.6, ls="--")
    ax.set_ylabel(label("z / mm", "z / mm"))
    ax.set_title(label("竖直位置", "vertical position"), fontsize=9)

    ax = axes[1, 0]
    ax.plot(t * 1000, vc[:, 1], color="C0", lw=2, label=label(r"摆线 $\dot z$", r"cycloid $\dot z$"))
    ax.plot(t * 1000, vq[:, 1], color="C3", lw=1.6, ls="--", label=label(r"多项式 $\dot z$", r"quintic $\dot z$"))
    ax.plot(t * 1000, vc[:, 0], color="C0", lw=1.2, ls=":")
    ax.plot(t * 1000, vq[:, 0], color="C3", lw=1.0, ls="-.")
    ax.set_ylabel(label("速度 / (m/s)", "velocity / (m/s)"))
    ax.set_xlabel(label("时间 t / ms", "t / ms"))
    ax.set_title(label(r"速度分量（点线为 $\dot x$）", r"velocity components (dotted: $\dot x$)"), fontsize=9)
    ax.legend(fontsize=7)

    ax = axes[1, 1]
    ax.plot(t * 1000, ac[:, 1], color="C0", lw=2)
    ax.plot(t * 1000, aq[:, 1], color="C3", lw=1.6, ls="--")
    ax.plot(t * 1000, ac[:, 0], color="C0", lw=1.2, ls=":")
    ax.plot(t * 1000, aq[:, 0], color="C3", lw=1.0, ls="-.")
    ax.axhline(ac[0, 1], color="C0", lw=0.6, ls=":")
    ax.text(T * 1000 * 0.98, ac[0, 1] - 2.0,
            label("摆线端点 29.61", "cycloid end 29.61"), ha="right",
            fontsize=7, color="C0")
    ax.set_ylabel(label("加速度 / (m/s²)", "acceleration / (m/s²)"))
    ax.set_xlabel(label("时间 t / ms", "t / ms"))
    ax.set_title(label(r"加速度分量（点线为 $\ddot x$）", r"acceleration (dotted: $\ddot x$)"), fontsize=9)

    for ax in axes.ravel():
        ax.axvline(T * 500, color="0.75", lw=0.8)
        ax.grid(alpha=0.22)
    fig.suptitle(label("摆线与五次多项式：位置、速度、加速度",
                       "cycloid vs quintic polynomial"), fontsize=11)
    fig.tight_layout()
    savefig(fig, "9_3_compare")
    plt.close(fig)


def main() -> None:
    t = np.linspace(0.0, T, N)
    p = t / T
    pc, vc, ac = cycloid(p)
    pq, vq, aq = quintic(p)

    print(f"步长 L = {L:.3f} m，抬腿高度 h = {H:.3f} m，摆动时长 T = {T:.3f} s")
    report("摆线", t, pc, vc, ac)
    report("五次多项式", t, pq, vq, aq)
    sc = np.linalg.norm(vc, axis=1)
    sq = np.linalg.norm(vq, axis=1)
    print(f"加速度峰值（范数）：摆线 {np.linalg.norm(ac, axis=1).max():.2f} m/s²，"
          f"多项式 {np.linalg.norm(aq, axis=1).max():.2f} m/s²")
    print(f"最大速度：摆线 {sc.max():.4f} m/s，多项式 {sq.max():.4f} m/s；"
          "两者的起落速度都为 0")
    print("摆线两端速度为零但加速度不为零；多项式把端点加速度也压到零，"
          "代价是峰值更高。")

    figure_geometry()
    figure_compare(t, pc, vc, ac, pq, vq, aq)


if __name__ == "__main__":
    main()
