"""Forward kinematics of one A1 leg (section 5.1).

The same foot position is computed twice: by projecting the three links onto
the axes of the leg base frame, and by multiplying four homogeneous
transforms.  The two results are independent implementations of Eq. (5.11).
The script also draws the planar two-link arm of Fig. 5-1 and the frame chain
plus simplified leg geometry of Fig. 5-2.

expect: 5 组最大分量差
expect: 齐次矩阵与几何公式
expect: 与 Leg.fk
"""

from __future__ import annotations

import logging
import math

import numpy as np

from quadruped.leg import Leg
from quadruped.plotting import label, savefig
from quadruped.types import LegId
import matplotlib.pyplot as plt

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

# A1 geometry, from the Menagerie model used throughout the book (section 0.3.5).
L_THIGH = 0.20       # thigh length, m
L_CALF = 0.20        # calf length, m
FOOT_RADIUS = 0.02   # foot-sphere radius, m
D_RIGHT = -0.08505   # signed lateral offset of the thigh joint, right leg, m
LEG_Q_MIN = np.array([-0.802851, -1.0472, -2.69653])
LEG_Q_MAX = np.array([0.802851, 4.18879, -0.916298])


# ANCHOR: fk_formulas
def fk_geometric(q: np.ndarray, d: float = D_RIGHT) -> np.ndarray:
    """Project the three links onto {0}; this is Eq. (5-3) written out."""
    q1, q2, q3 = np.asarray(q, dtype=float)
    s1, c1 = math.sin(q1), math.cos(q1)
    s2, c2 = math.sin(q2), math.cos(q2)
    s23, c23 = math.sin(q2 + q3), math.cos(q2 + q3)
    x = -L_CALF * s23 - L_THIGH * s2
    y = d * c1 + s1 * (L_THIGH * c2 + L_CALF * c23)
    z = d * s1 - c1 * (L_THIGH * c2 + L_CALF * c23)
    return np.array([x, y, z])


def _rx4(a: float) -> np.ndarray:
    T = np.eye(4)
    T[1:3, 1:3] = [[math.cos(a), -math.sin(a)],
                   [math.sin(a), math.cos(a)]]
    return T


def _ry4(a: float) -> np.ndarray:
    c, s = math.cos(a), math.sin(a)
    return np.array([[c, 0.0, s, 0.0],
                     [0.0, 1.0, 0.0, 0.0],
                     [-s, 0.0, c, 0.0],
                     [0.0, 0.0, 0.0, 1.0]])


def fk_homogeneous(q: np.ndarray, d: float = D_RIGHT) -> np.ndarray:
    """Multiply T01, T12, T23 and map p3; this is Eq. (5-11) in matrix form."""
    q1, q2, q3 = np.asarray(q, dtype=float)
    T01 = _rx4(q1)
    T12 = _ry4(q2)
    T23 = _ry4(q3)
    T23[:3, 3] = [0.0, d, -L_THIGH]
    p3 = np.array([0.0, 0.0, -L_CALF, 1.0])
    return (T01 @ T12 @ T23 @ p3)[:3]
# ANCHOR_END: fk_formulas


def draw_frame_2d(ax, origin, angle, color, tag, scale=0.045):
    """Two unit axes of a planar frame."""
    for k, name in enumerate(("x", "y")):
        a = angle + 0.5 * math.pi * k
        tip = np.asarray(origin) + scale * np.array([math.cos(a), math.sin(a)])
        ax.annotate("", xy=tip, xytext=origin,
                    arrowprops=dict(arrowstyle="-|>", color=color, lw=1.2))
        ax.text(*(np.asarray(origin) + 1.25 * scale *
                  np.array([math.cos(a), math.sin(a)])),
                rf"$\hat{{{name}}}_{tag}$", color=color, fontsize=8,
                ha="center", va="center")


def draw_arm2d() -> None:
    """Fig. 5-1: two links, three frames and the projections of Eq. (5.1)."""
    l1 = l2 = 0.2
    q1, q2 = math.radians(48.0), math.radians(58.0)
    O = np.zeros(2)
    A = l1 * np.array([math.cos(q1), math.sin(q1)])
    P = A + l2 * np.array([math.cos(q1 + q2), math.sin(q1 + q2)])

    fig, ax = plt.subplots(figsize=(6.8, 5.4))
    ax.plot([O[0], A[0]], [O[1], A[1]], lw=5, color="tab:blue",
            solid_capstyle="round", alpha=0.85)
    ax.plot([A[0], P[0]], [A[1], P[1]], lw=5, color="tab:orange",
            solid_capstyle="round", alpha=0.85)
    ax.plot(*O, "o", color="0.2", ms=6)
    ax.plot(*A, "o", color="0.2", ms=6)
    ax.plot(*P, "o", color="tab:red", ms=7)
    ax.text(O[0] - 0.025, O[1] - 0.03, r"$O$", fontsize=11)
    ax.text(A[0] - 0.03, A[1] + 0.018, r"$A$", fontsize=11)
    ax.text(P[0] + 0.01, P[1] + 0.01, r"$P$", fontsize=12, color="tab:red")

    draw_frame_2d(ax, O, 0.0, "0.35", "0")
    draw_frame_2d(ax, A, q1 + q2, "0.35", "2")
    ax.text(O[0] - 0.025, O[1] + 0.018, r"$\{0\},\{1\}$", fontsize=9,
            color="0.35")

    # Projections onto x and y, drawn separately so the two terms are visible.
    ax.plot([A[0], A[0]], [0.0, A[1]], ls=":", color="tab:blue", lw=1.1)
    ax.plot([0.0, A[0]], [A[1], A[1]], ls=":", color="tab:blue", lw=1.1)
    ax.plot([P[0], P[0]], [0.0, P[1]], ls=":", color="tab:orange", lw=1.1)
    ax.plot([0.0, P[0]], [P[1], P[1]], ls=":", color="tab:orange", lw=1.1)

    ax.annotate("", xy=(A[0], -0.028), xytext=(0.0, -0.028),
                arrowprops=dict(arrowstyle="<->", color="tab:blue", lw=1.0))
    ax.text(A[0] / 2, -0.052, r"$l_1\cos\theta_1$", ha="center",
            color="tab:blue", fontsize=10)
    ax.annotate("", xy=(P[0], -0.068), xytext=(A[0], -0.068),
                arrowprops=dict(arrowstyle="<->", color="tab:orange", lw=1.0))
    ax.text((A[0] + P[0]) / 2, -0.092,
            r"$l_2\cos(\theta_1+\theta_2)$", ha="center",
            color="tab:orange", fontsize=10)
    ax.annotate("", xy=(-0.035, A[1]), xytext=(-0.035, 0.0),
                arrowprops=dict(arrowstyle="<->", color="tab:blue", lw=1.0))
    ax.text(-0.048, A[1] / 2, r"$l_1\sin\theta_1$", rotation=90,
            va="center", ha="right", color="tab:blue", fontsize=10)
    ax.annotate("", xy=(-0.065, P[1]), xytext=(-0.065, A[1]),
                arrowprops=dict(arrowstyle="<->", color="tab:orange", lw=1.0))
    ax.text(-0.078, (A[1] + P[1]) / 2,
            r"$l_2\sin(\theta_1+\theta_2)$", rotation=90,
            va="center", ha="right", color="tab:orange", fontsize=10)

    # Angle arcs.
    ax.add_patch(plt.matplotlib.patches.Arc(O, 0.13, 0.13, theta1=0,
                                             theta2=math.degrees(q1),
                                             color="tab:red", lw=1.3))
    ax.text(0.078, 0.028, r"$\theta_1$", color="tab:red", fontsize=12)
    ax.add_patch(plt.matplotlib.patches.Arc(
        A, 0.10, 0.10, theta1=math.degrees(q1),
        theta2=math.degrees(q1 + q2), color="tab:red", lw=1.3))
    ax.text(A[0] - 0.075, A[1] + 0.035, r"$\theta_2$", color="tab:red",
            fontsize=12)

    ax.set_xlim(-0.16, 0.42)
    ax.set_ylim(-0.14, 0.39)
    ax.set_aspect("equal")
    ax.grid(True, alpha=0.25)
    ax.set_xlabel(r"$x_0$ / m")
    ax.set_ylabel(r"$y_0$ / m")
    ax.set_title(label("二维串联臂：两段连杆在两个坐标轴上的投影",
                       "planar arm: link projections on the two axes"))
    savefig(fig, "5_1_arm2d")
    plt.close(fig)


def draw_leg_schematic() -> None:
    """Fig. 5-2: frame chain (a) and an axonometric leg sketch (b)."""
    fig = plt.figure(figsize=(10.6, 5.2))
    gs = fig.add_gridspec(1, 2, width_ratios=[1.0, 1.12])

    # (a) connection diagram: frames, joints and links, not a bare flow chart.
    ax = fig.add_subplot(gs[0, 0])
    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1)
    ax.axis("off")
    boxes = [
        (0.74, r"$\{0\}$  固定于机身"),
        (0.57, r"$\{1\}$  随髋连杆"),
        (0.40, r"$\{2\}$  随大腿"),
        (0.23, r"$\{3\}$  随小腿"),
    ]
    joints = [
        (0.675, r"关节 1  $\theta_1$，绕 $x$ 轴"),
        (0.505, r"关节 2  $\theta_2$，绕 $y$ 轴"),
        (0.335, r"关节 3  $\theta_3$，绕 $y$ 轴"),
    ]
    for y, text in boxes:
        ax.add_patch(plt.Rectangle((0.12, y - 0.038), 0.70, 0.076,
                                   facecolor="#eef4fb", edgecolor="#4c78a8",
                                   lw=1.2, zorder=2))
        ax.text(0.47, y, text, ha="center", va="center", fontsize=10, zorder=3)
    for y, text in joints:
        ax.add_patch(plt.Circle((0.47, y), 0.012, color="0.25", zorder=4))
        ax.plot([0.47, 0.47], [y - 0.038, y + 0.038], color="0.25", lw=2,
                zorder=3)
        ax.text(0.50, y, text, ha="left", va="center", fontsize=9.5)
    ax.annotate("", xy=(0.47, 0.105), xytext=(0.47, 0.192),
                arrowprops=dict(arrowstyle="-|>", color="0.2", lw=1.5))
    ax.text(0.47, 0.085, r"足端 $P$", ha="center", va="center", fontsize=11)
    ax.text(0.47, 0.885, label("坐标系链：旋转轴与坐标轴重合",
                               "frame chain: joint axes along coordinate axes"),
            ha="center", fontsize=10.5)
    ax.text(0.03, 0.02, "髋连杆沿 $y$ 偏置 $d$；大腿、小腿在 $xz$ 平面内",
            fontsize=8.5, color="0.35")

    # (b) axonometric sketch in a 2-D axes: y to the right, z up, x oblique.
    ax2 = fig.add_subplot(gs[0, 1])
    q = np.array([0.25, 0.72, -1.55])
    R1 = np.array([[1, 0, 0],
                   [0, math.cos(q[0]), -math.sin(q[0])],
                   [0, math.sin(q[0]), math.cos(q[0])]])
    R2 = np.array([[math.cos(q[1]), 0, math.sin(q[1])],
                   [0, 1, 0],
                   [-math.sin(q[1]), 0, math.cos(q[1])]])
    R3 = np.array([[math.cos(q[2]), 0, math.sin(q[2])],
                   [0, 1, 0],
                   [-math.sin(q[2]), 0, math.cos(q[2])]])
    O = np.zeros(3)
    A = R1 @ np.array([0.0, D_RIGHT, 0.0])
    O3 = A + R1 @ R2 @ np.array([0.0, 0.0, -L_THIGH])
    P = O3 + R1 @ R2 @ R3 @ np.array([0.0, 0.0, -L_CALF])

    def proj(p):
        # Oblique x, horizontal y, vertical z.  The factors are chosen so the
        # three links do not overlap in this particular pose.
        return np.array([p[1] - 0.55 * p[0], p[2] - 0.32 * p[0]])

    for a, b, color, name, off in (
            (O, A, "tab:green", r"$d$", -0.030),
            (A, O3, "tab:blue", r"大腿 $l_{\rm hip}$", 0.024),
            (O3, P, "tab:orange", r"小腿 $l_{\rm knee}$", 0.024)):
        pa, pb = proj(a), proj(b)
        ax2.plot([pa[0], pb[0]], [pa[1], pb[1]], color=color, lw=4.5,
                 solid_capstyle="round", zorder=3)
        mid = 0.5 * (pa + pb)
        dvec = pb - pa
        perp = np.array([-dvec[1], dvec[0]])
        perp = perp / max(np.linalg.norm(perp), 1e-9)
        ax2.text(*(mid + off * perp), name, fontsize=9, color=color,
                 ha="center", va="center", zorder=5)

    for p3, text, off in ((O, "O", (0.015, -0.018)),
                          (A, "A", (-0.018, 0.022)),
                          (O3, r"$O_3$", (0.018, 0.020)),
                          (P, "P", (0.018, -0.020))):
        pp = proj(p3)
        ax2.plot(*pp, "o", color="0.2", ms=6, zorder=6)
        ax2.text(pp[0] + off[0], pp[1] + off[1], text, fontsize=11, zorder=7)
    ax2.plot([proj(O)[0], proj(P)[0]], [proj(O)[1], proj(P)[1]],
             ls=":", color="0.55", lw=1.1, zorder=1)

    # Frame axes: {0}, {1}, {2} share O; {3} is at O3.
    origin = proj(O)
    axis_scale = 0.075
    for vec, name, color in (
            (np.array([-0.55, -0.32]), r"$x_0$", "tab:red"),
            (np.array([1.0, 0.0]), r"$y_0$", "tab:green"),
            (np.array([0.0, 1.0]), r"$z_0$", "tab:blue")):
        tip = origin + axis_scale * vec / np.linalg.norm(vec)
        ax2.annotate("", xy=tip, xytext=origin,
                     arrowprops=dict(arrowstyle="-|>", color=color, lw=1.4))
        ax2.text(*(origin + 1.22 * (tip - origin)), name, color=color,
                 fontsize=9, ha="center", va="center")
    ax2.text(origin[0] + 0.022, origin[1] + 0.030,
             r"$\{0\},\{1\},\{2\}$", fontsize=8.5, color="0.35")
    ax2.text(proj(O3)[0] + 0.020, proj(O3)[1] - 0.005, r"$\{3\}$",
             fontsize=9, color="0.35")
    ax2.set_xlim(-0.17, 0.12)
    ax2.set_ylim(-0.36, 0.105)
    ax2.set_aspect("equal")
    ax2.set_xticks([])
    ax2.set_yticks([])
    ax2.set_title(label("单腿简化图：O、A、$O_3$、P",
                        "axonometric leg: O, A, O3, P"), fontsize=10.5,
                  pad=12)
    fig.tight_layout()
    savefig(fig, "5_1_leg")
    plt.close(fig)


def main() -> None:
    print("=== 5.1 A1 右前腿的正向运动学 ===")
    print(f"几何参数: l_thigh = {L_THIGH:.3f} m, l_calf = {L_CALF:.3f} m, "
          f"d = {D_RIGHT:.5f} m, 足端球半径 = {FOOT_RADIUS:.3f} m")
    print(f"关节限位: {LEG_Q_MIN.round(4)} ... {LEG_Q_MAX.round(4)} rad")

    q_sets = np.array([
        [0.000, 0.900, -1.800],
        [0.250, 0.600, -1.400],
        [-0.400, 1.100, -2.100],
        [0.150, 1.500, -2.500],
        [-0.600, 0.300, -1.000],
    ])
    print("\n  θ1      θ2      θ3   |        p_geo (m)        |"
          "        p_T (m)          |  max|diff|")
    diffs = []

    def shown(p):
        return np.where(np.abs(p) < 5e-5, 0.0, p)

    for q in q_sets:
        pg = fk_geometric(q)
        pm = fk_homogeneous(q)
        diff = float(np.max(np.abs(pg - pm)))
        diffs.append(diff)
        g, m = shown(pg), shown(pm)
        print(f" {q[0]:6.3f} {q[1]:6.3f} {q[2]:6.3f} | "
              f"{g[0]:8.4f} {g[1]:8.4f} {g[2]:8.4f} | "
              f"{m[0]:8.4f} {m[1]:8.4f} {m[2]:8.4f} | {diff:.2e}")
    print(f"\n5 组最大分量差 = {max(diffs):.2e} m")
    print(f"齐次矩阵与几何公式在双精度下一致（最大 {max(diffs):.2e} m）")

    leg = Leg(LegId.FR, np.array([0.183, -0.047, 0.0]),
              np.array([0.0, D_RIGHT, 0.0]), L_THIGH, L_CALF,
              FOOT_RADIUS, q_min=LEG_Q_MIN, q_max=LEG_Q_MAX)
    err_leg = max(float(np.max(np.abs(leg.fk(q) - fk_homogeneous(q))))
                  for q in q_sets)
    print(f"与 Leg.fk 的最大差 = {err_leg:.2e} m")

    p_home = fk_homogeneous(np.array([0.0, 0.900, -1.800]))
    ph = np.where(np.abs(p_home) < 5e-5, 0.0, p_home)
    print(f"\nMenagerie 的 home 关节角 [0, 0.9, -1.8]: "
          f"P = ({ph[0]:.4f}, {ph[1]:.4f}, {ph[2]:.4f}) m")
    print(f"该点距腿基座原点 {np.linalg.norm(p_home):.4f} m；"
          f"足端球最低点约 {p_home[2] - FOOT_RADIUS:.4f} m")

    draw_arm2d()
    draw_leg_schematic()


if __name__ == "__main__":
    main()
