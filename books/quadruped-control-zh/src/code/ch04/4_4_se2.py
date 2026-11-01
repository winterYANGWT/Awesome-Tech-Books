"""Planar rigid-body motion and homogeneous coordinates (section 4.4).

A 3x3 homogeneous matrix carries the rotation and the translation of a planar
body in one multiplication.  Points are padded with 1 (the translation acts),
vectors with 0 (it does not).  The script checks the two cases, composes two
transforms, compares the analytic inverse with a general one, and draws the
initial and final triangle.

expect: 点带平移
expect: 向量不带平移
expect: 复合
expect: 解析逆
"""

from __future__ import annotations

import logging
import math

import numpy as np

from quadruped.plotting import savefig
from quadruped.rotation import R2
import matplotlib.pyplot as plt

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

TRIANGLE = np.array([[0.6, 0.1], [1.2, 0.1], [0.6, 0.6]])


def T2(R: np.ndarray, p: np.ndarray) -> np.ndarray:
    """Assemble a 3x3 homogeneous transform."""
    out = np.eye(3)
    out[:2, :2] = R
    out[:2, 2] = p
    return out


def invT2(T: np.ndarray) -> np.ndarray:
    """Analytic inverse: [[R^T, -R^T p], [0, 1]]."""
    R = T[:2, :2]
    p = T[:2, 2]
    out = np.eye(3)
    out[:2, :2] = R.T
    out[:2, 2] = -R.T @ p
    return out


def report(T: np.ndarray, name: str) -> None:
    print(f"\n {name} =")
    for row in T:
        print("   [" + "  ".join(f"{x: .4f}" for x in row) + "]")


def main() -> None:
    print("=== 4.4 二维刚体运动与齐次坐标 ===")
    th, p_sb = math.radians(30.0), np.array([0.8, 0.5])
    T_sb = T2(R2(th), p_sb)
    report(T_sb, "T_sb (theta=30 deg, p=(0.8, 0.5))")
    print(f" 旋转角 = {math.degrees(th):.4f} deg，平移范数 = {np.linalg.norm(p_sb):.4f} m")

    # ANCHOR: pointvec
    point_b = np.array([1.0, 0.2])
    vec_b = np.array([1.0, 0.0])
    point_s = T_sb @ np.append(point_b, 1.0)
    vec_s = T_sb @ np.append(vec_b, 0.0)
    print(f"\n 点 p_b = ({point_b[0]:.4f}, {point_b[1]:.4f})")
    print(f" 点带平移：T[p_b;1] = ({point_s[0]:.4f}, {point_s[1]:.4f}, {point_s[2]:.4f})")
    print(f" 与 R p_b + t 的差 = "
          f"{np.max(np.abs(point_s[:2] - (R2(th) @ point_b + p_sb))):.2e}")
    print(f" 向量不带平移：T[v_b;0] = ({vec_s[0]:.4f}, {vec_s[1]:.4f}, {vec_s[2]:.4f})")
    print(f" 与 R v_b 的差 = {np.max(np.abs(vec_s[:2] - R2(th) @ vec_b)):.2e}")
    print(f" 向量末尾补 0 后第三分量 = {vec_s[2]:.1f}")

    # ANCHOR_END: pointvec
    th_bc, p_bc = math.radians(-20.0), np.array([0.4, -0.2])
    T_bc = T2(R2(th_bc), p_bc)
    T_sc = T_sb @ T_bc
    direct = T2(R2(th) @ R2(th_bc), p_sb + R2(th) @ p_bc)
    report(T_bc, "T_bc (theta=-20 deg, p=(0.4, -0.2))")
    report(T_sc, "T_sc = T_sb T_bc")
    print(f"\n 复合与直接构造的最大元素差 = {np.max(np.abs(T_sc - direct)):.2e}")
    print(f" 先 b->c 再 c->s 与一次 b->s 的点变换差 = "
          f"{np.max(np.abs((T_sb @ T_bc @ np.append(point_b, 1.0)) - (T_sc @ np.append(point_b, 1.0)))):.2e}")

    # ANCHOR: compose
    inv = invT2(T_sb)
    report(inv, "T_sb^-1（解析式）")
    print(f" 解析逆与通用求逆的最大差 = "
          f"{np.max(np.abs(inv - np.linalg.inv(T_sb))):.2e}")
    print(f" T T^-1 与 I 的最大差 = {np.max(np.abs(T_sb @ inv - np.eye(3))):.2e}")
    print(f" T^-1 的旋转块与 R^T 的最大差 = {np.max(np.abs(inv[:2, :2] - R2(th).T)):.2e}")

    # ANCHOR_END: compose
    # Figure 4-8: initial triangle at {s}, final triangle at {b}.
    moved = (R2(th) @ TRIANGLE.T).T + p_sb
    fig, ax = plt.subplots(figsize=(6.6, 6.0))
    ax.add_patch(plt.Polygon(TRIANGLE, closed=True, fill=False, ls="--", lw=1.4,
                             edgecolor="0.55"))
    ax.add_patch(plt.Polygon(moved, closed=True, fill=True, alpha=0.18,
                             facecolor="tab:green", edgecolor="tab:green", lw=1.8))
    for origin, angle, color, name in ((np.zeros(2), 0.0, "0.35", "s"),
                                       (p_sb, th, "tab:green", "b")):
        for k, axis_name in enumerate(("x", "y")):
            a = angle + 0.5 * math.pi * k
            d = np.array([math.cos(a), math.sin(a)]) * 0.45
            ax.annotate("", xy=origin + d, xytext=origin,
                        arrowprops=dict(arrowstyle="-|>", color=color, lw=1.6))
            ax.text(*(origin + 1.22 * d), f"$\\hat{{{axis_name}}}_{name}$",
                    color=color, fontsize=10, ha="center")
    ax.annotate("", xy=p_sb, xytext=(0, 0),
                arrowprops=dict(arrowstyle="-|>", color="tab:red", lw=1.6,
                                ls="--"))
    ax.text(*(0.5 * p_sb + np.array([-0.05, 0.12])),
            r"$\overrightarrow{OO'}$", color="tab:red", fontsize=11)
    ax.annotate("", xy=(0.36 * math.cos(th), 0.36 * math.sin(th)),
                xytext=(0.36, 0.0),
                arrowprops=dict(arrowstyle="-|>", color="tab:orange", lw=1.3,
                                connectionstyle="arc3,rad=0.3"))
    ax.text(0.50 * math.cos(0.5 * th), 0.50 * math.sin(0.5 * th) + 0.03,
            r"$\theta$", color="tab:orange", fontsize=12)
    ax.text(-0.16, -0.16, r"$O$", fontsize=11)
    ax.text(p_sb[0] - 0.02, p_sb[1] - 0.2, r"$O'$", fontsize=11)
    ax.set_xlim(-0.45, 2.15)
    ax.set_ylim(-0.45, 1.95)
    ax.set_aspect("equal")
    ax.grid(True, alpha=0.25)
    ax.set_xlabel(r"$x_s$ / m")
    ax.set_ylabel(r"$y_s$ / m")
    ax.set_title("平面刚体运动 = 平移 + 旋转，用 3×3 齐次矩阵一次完成")
    savefig(fig, "4_4_se2")


if __name__ == "__main__":
    main()
