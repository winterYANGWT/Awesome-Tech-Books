"""Spatial rotation matrices: elementary rotations, invariants, order (4.2).

Three elementary rotations are built, checked against the six orthonormality
constraints and det = 1, and composed in two different orders.  The two
compositions are compared element by element and by the geodesic angle of the
relative rotation.

expect: 正交性
expect: det
expect: 不可交换
"""

from __future__ import annotations

import logging
import math

import numpy as np

from quadruped.plotting import savefig
from quadruped.rotation import Rx, Ry, Rz, so3_log, so3_rotation_error
import matplotlib.pyplot as plt

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

D = math.radians


def show(name: str, R: np.ndarray) -> None:
    print(f"\n {name} =")
    for row in R:
        print("   [" + "  ".join(f"{x: .4f}" for x in row) + "]")


def draw_frame(ax, R, origin=(0.0, 0.0, 0.0), color="k", ls="-", length=1.0,
               lw=1.8, text=True):
    """Draw the three axes of a frame whose columns are the axes in {s}."""
    o = np.asarray(origin, dtype=float)
    for k, name in enumerate(("x", "y", "z")):
        d = R[:, k] * length
        ax.quiver(*o, *d, color=color, linewidth=lw, linestyle=ls,
                  arrow_length_ratio=0.14)
        if text:
            ax.text(*(o + 1.10 * d), f"${name}$", color=color, fontsize=8.5)


def legend_handle(color, ls, label):
    from matplotlib.lines import Line2D
    return Line2D([0], [0], color=color, ls=ls, lw=2, label=label)


def setup3d(ax, lim=1.25):
    ax.set_xlim(-lim, lim)
    ax.set_ylim(-lim, lim)
    ax.set_zlim(-lim, lim)
    ax.set_box_aspect((1, 1, 1))
    ax.view_init(elev=20, azim=-58)
    ax.grid(False)
    for pane in (ax.xaxis.pane, ax.yaxis.pane, ax.zaxis.pane):
        pane.set_alpha(0.0)
    ax.tick_params(labelsize=7)
    ax.set_xlabel("x", fontsize=8)
    ax.set_ylabel("y", fontsize=8)
    ax.set_zlabel("z", fontsize=8)


def draw_elementary() -> None:
    """Figure 4-3: the three elementary rotations, axis out of the page."""
    fig, axes = plt.subplots(1, 3, figsize=(12.0, 4.4))
    cases = (
        ("x", "y", "z", 0, "tab:red"),
        ("y", "z", "x", 1, "tab:green"),
        ("z", "x", "y", 2, "tab:blue"),
    )
    for ax, (out, h, v, _, color) in zip(axes, cases):
        lim = 1.25
        ax.set_xlim(-lim, lim)
        ax.set_ylim(-lim, lim)
        ax.set_aspect("equal")
        ax.grid(True, alpha=0.22)
        ax.axhline(0.0, color="0.75", lw=0.8)
        ax.axvline(0.0, color="0.75", lw=0.8)
        for d, name in (((1, 0), h), ((0, 1), v)):
            ax.annotate("", xy=d, xytext=(0, 0),
                        arrowprops=dict(arrowstyle="-|>", color="0.35", lw=1.5))
            ax.text(1.18 * d[0], 1.18 * d[1], f"$\\hat{{\\boldsymbol{{{name}}}}}$",
                    color="0.3", fontsize=12, ha="center", va="center")
        # The rotation axis points out of the page: dot in a circle.
        ax.add_patch(plt.Circle((0, 0), 0.07, color="0.25", zorder=5))
        ax.add_patch(plt.Circle((0, 0), 0.11, fill=False, color="0.25", lw=1.2, zorder=5))
        ax.text(-0.20, 0.10, f"$\\hat{{\\boldsymbol{{{out}}}}}$", color="0.25",
                fontsize=12, ha="right")

        # Positive direction: from the first axis to the second, CCW.
        th = np.linspace(0.0, 0.78 * math.pi / 2, 50)
        r = 0.72
        ax.plot(r * np.cos(th), r * np.sin(th), color=color, lw=2.0)
        ax.annotate("", xy=(r * math.cos(th[-1]), r * math.sin(th[-1])),
                    xytext=(r * math.cos(th[-4]), r * math.sin(th[-4])),
                    arrowprops=dict(arrowstyle="-|>", color=color, lw=2.0,
                                    connectionstyle="arc3,rad=-0.2"))
        ax.text(0.45 * math.cos(0.55 * th[-1]), 0.45 * math.sin(0.55 * th[-1]),
                r"$\theta$", color=color, fontsize=13, ha="center")

        # The rotated pair at 30 deg with projection lines.
        a = math.radians(30.0)
        ax.annotate("", xy=(math.cos(a), math.sin(a)), xytext=(0, 0),
                    arrowprops=dict(arrowstyle="-|>", color=color, lw=1.4, ls="--"))
        ax.plot([math.cos(a), math.cos(a)], [0.0, math.sin(a)], ls=":", color=color, lw=0.9)
        ax.plot([0.0, math.cos(a)], [math.sin(a), math.sin(a)], ls=":", color=color, lw=0.9)
        ax.text(math.cos(a) + 0.06, math.sin(a) + 0.07,
                f"${h}'$", color=color, fontsize=11)
        ax.set_title(f"绕 {out} 轴：{h} 转向 {v} 为正", fontsize=10)
    fig.suptitle("右手定则：大拇指指向轴的正向（垂直纸面向外），四指弯曲方向就是正转角",
                 fontsize=11)
    fig.tight_layout()
    savefig(fig, "4_2_elementary")


def draw_order() -> None:
    """Figure 4-4: the same two rotations applied in the two orders."""
    a, b = D(30.0), D(60.0)
    first = Rx(a) @ Ry(b)
    second = Ry(b) @ Rx(a)
    fig = plt.figure(figsize=(11.6, 5.4))

    ax = fig.add_subplot(1, 2, 1, projection="3d")
    setup3d(ax)
    draw_frame(ax, np.eye(3), color="0.55", lw=1.2)
    draw_frame(ax, Rx(a), color="0.30", ls="--", lw=1.4)
    draw_frame(ax, first, color="tab:red", lw=2.0)
    ax.legend(handles=[legend_handle("0.55", "-", r"起点 $\{s\}$"),
                       legend_handle("0.30", "--", "转完 x：中间姿态"),
                       legend_handle("tab:red", "-", "再转 y：终点")],
              loc="lower left", fontsize=8, framealpha=0.9)
    ax.set_title("(a) 先绕 x 转 30°，再绕 y 转 60°", fontsize=10)

    ax = fig.add_subplot(1, 2, 2, projection="3d")
    setup3d(ax)
    draw_frame(ax, np.eye(3), color="0.55", lw=1.2)
    draw_frame(ax, first, color="tab:red", lw=2.0)
    draw_frame(ax, second, color="tab:blue", lw=2.0)
    ax.legend(handles=[legend_handle("0.55", "-", r"起点 $\{s\}$"),
                       legend_handle("tab:red", "-", "x→y 的终点"),
                       legend_handle("tab:blue", "-", "y→x 的终点")],
              loc="lower left", fontsize=8, framealpha=0.9)
    ax.set_title("(b) 两个终点叠在一起：不是同一个姿态", fontsize=10)

    fig.suptitle("同样的两个角度、不同的顺序：终点姿态相差 29.74°", fontsize=11)
    fig.tight_layout()
    savefig(fig, "4_2_order")


def main() -> None:
    print("=== 4.2 三维旋转矩阵 ===")
    show("Rx(90 deg)", Rx(D(90.0)))
    show("Ry(90 deg)", Ry(D(90.0)))
    show("Rz(90 deg)", Rz(D(90.0)))

    # ANCHOR: properties
    R = Rx(D(30.0)) @ Ry(D(60.0)) @ Rz(D(45.0))
    show("R = Rx(30) Ry(60) Rz(45)", R)
    I3 = np.eye(3)
    print("\n 旋转矩阵的性质：")
    print(f" 正交性 ||R^T R - I||_max = {np.max(np.abs(R.T @ R - I3)):.2e}")
    print(f" 正交性 ||R R^T - I||_max = {np.max(np.abs(R @ R.T - I3)):.2e}")
    print(f" det R = {np.linalg.det(R):.6f}")
    print(f" det(-R) = {np.linalg.det(-R):.6f}  (镜像，不是旋转)")
    print(f" ||R^-1 - R^T||_max = {np.max(np.abs(np.linalg.inv(R) - R.T)):.2e}")
    # R_sb^{-1} = R_bs: invert by transposing and by rebuilding from -theta.
    print(f" R_sb^-1 = R_bs：转置与反向旋转的最大差 = "
          f"{np.max(np.abs(R.T - (Rz(D(-45)) @ Ry(D(-60)) @ Rx(D(-30))))):.2e}")

    # ANCHOR_END: properties
    # Composition: {b} -> {a} -> {s} equals {b} -> {s} directly.
    R_sa, R_ab = Rz(D(20)), Ry(D(35))
    R_sb = R_sa @ R_ab
    test_v = np.array([0.3, -0.7, 1.1])
    print(f"\n 复合：R_sb v 与 R_sa (R_ab v) 的最大差 = "
          f"{np.max(np.abs(R_sb @ test_v - R_sa @ (R_ab @ test_v))):.2e}")

    # ANCHOR: order
    print("\n 不可交换：先 x 后 y，与先 y 后 x")
    R_xy = Rx(D(30.0)) @ Ry(D(60.0))
    R_yx = Ry(D(60.0)) @ Rx(D(30.0))
    show("Rx(30) Ry(60)", R_xy)
    show("Ry(60) Rx(30)", R_yx)
    print(f" 两组姿态的最大元素差 = {np.max(np.abs(R_xy - R_yx)):.4f}")
    rel = so3_log(R_xy.T @ R_yx)
    print(f" 相对旋转角 = {math.degrees(np.linalg.norm(rel)):.4f} deg，"
          f"转轴 = ({rel[0] / np.linalg.norm(rel): .4f}, "
          f"{rel[1] / np.linalg.norm(rel): .4f}, {rel[2] / np.linalg.norm(rel): .4f})")
    print(f" 测地线夹角（库函数）= {math.degrees(so3_rotation_error(R_xy, R_yx)):.4f} deg")

    # ANCHOR_END: order
    draw_elementary()
    draw_order()


if __name__ == "__main__":
    main()
