"""Spatial rigid-body motion: SE(3), composition and analytic inverse (4.5).

The example follows the numeric frame chain of section 4.5.2: {a} coincides
with {s}, {b} sits at (-2,-2,0), {c} at (0,2,1).  It rebuilds T_bc from the
pose of {c} in {s}, verifies T_sc = T_sb T_bc, transforms a point and a
vector, and checks the analytic inverse against a general one on 100 random
transforms.

expect: 位姿链
expect: 点与向量
expect: 解析逆
expect: 100 组随机
"""

from __future__ import annotations

import logging
import math

import numpy as np

from quadruped.plotting import savefig
from quadruped.rotation import make_T, inv_T, so3_exp, so3_log
import matplotlib.pyplot as plt

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

RNG = np.random.default_rng(5)

R_SB = np.array([[0.0, 0.0, 1.0], [0.0, -1.0, 0.0], [1.0, 0.0, 0.0]])
R_SC = np.array([[-1.0, 0.0, 0.0], [0.0, 0.0, 1.0], [0.0, 1.0, 0.0]])
P_SB = np.array([-2.0, -2.0, 0.0])
P_SC = np.array([0.0, 2.0, 1.0])


def show(T: np.ndarray, name: str) -> None:
    print(f"\n {name} =")
    for row in T:
        print("   [" + "  ".join(f"{x: .4f}" for x in row) + "]")


def draw_frame(ax, R, p, color, label, length=0.8, lw=1.6):
    for k, name in enumerate(("x", "y", "z")):
        d = R[:, k] * length
        ax.quiver(*p, *d, color=color, linewidth=lw, arrow_length_ratio=0.15)
        ax.text(*(p + 1.12 * d), f"${name}_{label}$", color=color, fontsize=8.5)
    ax.plot(*p, "o", color=color, ms=4)
    ax.text(*(p + np.array([0.1, -0.1, -0.15])), f"$O_{label}$", color=color,
            fontsize=10)


def main() -> None:
    print("=== 4.5 三维刚体运动与齐次变换 ===")
    T_sb = make_T(R_SB, P_SB)
    T_sc = make_T(R_SC, P_SC)
    show(T_sb, "T_sb")
    show(T_sc, "T_sc")
    ang = math.degrees(math.acos(max(-1.0, min(1.0, 0.5 * (np.trace(R_SB) - 1.0)))))
    print(f"\n T_sb 的旋转角 = {ang:.4f} deg，平移范数 = {np.linalg.norm(P_SB):.4f} m")
    print(f" 验证 det(R_sb) = {np.linalg.det(R_SB):.4f}，"
          f"||R_sb^T R_sb - I|| = {np.max(np.abs(R_SB.T @ R_SB - np.eye(3))):.2e}")

    # ANCHOR: chain
    # Build T_bc from the poses of {b} and {c} in {s}.
    R_bc = R_SB.T @ R_SC
    p_bc = R_SB.T @ (P_SC - P_SB)
    T_bc = make_T(R_bc, p_bc)
    show(T_bc, "T_bc = [R_sb^T R_sc, R_sb^T (p_sc - p_sb)]")
    print(f"\n p_bc = ({p_bc[0]:.4f}, {p_bc[1]:.4f}, {p_bc[2]:.4f})")
    print(f" 位姿链 ||T_sb T_bc - T_sc||_max = "
          f"{np.max(np.abs(T_sb @ T_bc - T_sc)):.2e}")

    # A point and a vector, padded with 1 and 0.
    p_b = np.array([1.0, -4.0, 2.0])
    v_s = np.array([1.0, 0.0, 0.0])
    p_s = T_sb @ np.append(p_b, 1.0)
    v_out = T_sb @ np.append(v_s, 0.0)
    print(f"\n 点与向量：p_b = ({p_b[0]:.4f}, {p_b[1]:.4f}, {p_b[2]:.4f})")
    print(f" 点 T_sb[p_b;1] 前三维 = ({p_s[0]:.4f}, {p_s[1]:.4f}, {p_s[2]:.4f})")
    print(f" 与 R_sb p_b + p_sb 的差 = "
          f"{np.max(np.abs(p_s[:3] - (R_SB @ p_b + P_SB))):.2e}")
    print(f" 向量 T_sb[v_s;0] 前三维 = ({v_out[0]:.4f}, {v_out[1]:.4f}, {v_out[2]:.4f})")
    print(f" 与 R_sb v_s 的差 = {np.max(np.abs(v_out[:3] - R_SB @ v_s)):.2e}")
    print(f" 向量结果的第四分量 = {v_out[3]:.1f}")

    # The inverse of T_sb and its interpretation.
    inv = inv_T(T_sb)
    print(f"\n 解析逆 ||T^-1 - inv(T)||_max = "
          f"{np.max(np.abs(inv - np.linalg.inv(T_sb))):.2e}")
    print(f" T T^-1 与 I 的最大差 = {np.max(np.abs(T_sb @ inv - np.eye(4))):.2e}")
    print(f" T^-1 的旋转块与 R^T 的差 = {np.max(np.abs(inv[:3, :3] - R_SB.T)):.2e}")
    # ANCHOR_END: chain

    # ANCHOR: random
    n = 100
    worst = 0.0
    worst_inv = 0.0
    worst_pose = 0.0
    for _ in range(n):
        axis = RNG.normal(size=3)
        axis /= np.linalg.norm(axis)
        R = so3_exp(axis * RNG.uniform(0.0, math.pi))
        T = make_T(R, RNG.normal(size=3))
        Ti = inv_T(T)
        worst = max(worst, float(np.max(np.abs(T @ Ti - np.eye(4)))))
        worst_inv = max(worst_inv, float(np.max(np.abs(Ti - np.linalg.inv(T)))))
        worst_pose = max(worst_pose, float(np.linalg.norm(so3_log(Ti[:3, :3] @ T[:3, :3]))))
    print(f"\n {n} 组随机齐次变换：")
    print(f" 解析逆 ||T T^-1 - I||_max = {worst:.2e}")
    print(f" 解析逆与通用求逆的最大元素差 = {worst_inv:.2e}")
    print(f" 逆变换的旋转角最大误差 = {worst_pose:.2e} rad")
    # ANCHOR_END: random

    # Figure 4-9: three frames and the position vectors.
    fig = plt.figure(figsize=(7.0, 6.4))
    ax = fig.add_subplot(projection="3d")
    draw_frame(ax, np.eye(3), np.zeros(3), "0.45", "s")
    draw_frame(ax, R_SB, P_SB, "tab:blue", "b")
    draw_frame(ax, R_SC, P_SC, "tab:green", "c")
    for p0, p1, color, name, off in ((np.zeros(3), P_SB, "tab:blue", r"$p_{ab}$",
                                      [-0.9, 0.15, 0.15]),
                                     (P_SB, P_SC, "tab:red", r"$p_{bc}$", [0.15, 0.0, 0.0]),
                                     (np.zeros(3), P_SC, "0.2", r"$p_{ac}$", [0.1, 0.1, 0.0])):
        ax.plot([p0[0], p1[0]], [p0[1], p1[1]], [p0[2], p1[2]], color=color,
                ls="--", lw=1.3)
        m = 0.5 * (p0 + p1)
        ax.text(*(m + np.array(off)), name, color=color, fontsize=11)
    ax.quiver(0, 0, 0, *P_SB, color="tab:blue", lw=1.4, arrow_length_ratio=0.1)
    # A point P and its coordinates in two frames.
    P = np.array([1.2, 0.6, 1.4])
    ax.plot(*P, "o", color="k", ms=5)
    ax.text(*(P + np.array([0.1, 0.0, 0.1])), r"$P$", fontsize=11)
    ax.set_xlim(-2.6, 1.8)
    ax.set_ylim(-2.6, 2.6)
    ax.set_zlim(-0.3, 2.2)
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.set_zlabel("z")
    ax.view_init(elev=20, azim=-58)
    ax.grid(False)
    for pane in (ax.xaxis.pane, ax.yaxis.pane, ax.zaxis.pane):
        pane.set_alpha(0.0)
    ax.set_title("三个坐标系与位姿链：$T_{sc}=T_{sb}T_{bc}$（$\\{s\\}$ 与 $\\{a\\}$ 重合）")
    fig.tight_layout()
    savefig(fig, "4_5_se3")


if __name__ == "__main__":
    main()
