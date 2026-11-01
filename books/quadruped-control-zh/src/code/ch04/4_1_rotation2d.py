"""Planar rotation: orthogonal basis, coordinates and R(theta) (section 4.1).

A triangle is a rigid planar body.  Its vertices are known in the body frame
{b}; at t=0 {b} coincides with the world frame {s}.  Rotating the body about
the origin by theta leaves the body-frame coordinates unchanged and changes
only the basis used to express them, so p_s = R_sb(theta) p_b.

The script prints the coordinates before and after a 30 deg and a 90 deg
rotation, the length error and the two invariants of R, and draws the two
figures of section 4.1.

expect: 长度最大误差
expect: 正交性
expect: det R
"""

from __future__ import annotations

import logging
import math

import numpy as np

from quadruped.plotting import savefig
from quadruped.rotation import R2
import matplotlib.pyplot as plt

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

# Vertices of the triangle in the body frame {b}, metres.
TRIANGLE = np.array([[1.0, 0.2], [2.0, 0.2], [1.0, 1.2]])


def deg(x: float) -> float:
    return math.degrees(x)


def rotate(theta: float) -> np.ndarray:
    """Body-frame coordinates -> world-frame coordinates."""
    return (R2(theta) @ TRIANGLE.T).T


# ANCHOR: report
def report(theta_deg: float) -> np.ndarray:
    th = math.radians(theta_deg)
    R = R2(th)
    out = rotate(th)
    print(f"\n theta = {theta_deg:6.1f} deg = {th:.4f} rad")
    print(f" R = [[{R[0, 0]: .4f}, {R[0, 1]: .4f}], [{R[1, 0]: .4f}, {R[1, 1]: .4f}]]")
    for name, p0, p1 in zip("ABC", TRIANGLE, out):
        print(f" {name}: {p0[0]: .4f},{p0[1]: .4f}  ->  {p1[0]: .4f},{p1[1]: .4f}")
    len0 = np.linalg.norm(TRIANGLE, axis=1)
    len1 = np.linalg.norm(out, axis=1)
    print(f" 长度最大误差 = {np.max(np.abs(len1 - len0)):.2e} m")
    print(f" 旋转后边长 A-B = {np.linalg.norm(out[1] - out[0]):.4f} m "
          f"(旋转前 {np.linalg.norm(TRIANGLE[1] - TRIANGLE[0]):.4f} m)")
    return out


# ANCHOR_END: report

def draw_frame(ax, origin, angle, color, label, length=1.0, ls="-"):
    """Draw the two axes of a planar frame whose x axis is at ``angle``."""
    for k, name in enumerate(("x", "y")):
        a = angle + 0.5 * math.pi * k
        d = np.array([math.cos(a), math.sin(a)])
        ax.annotate("", xy=origin + length * d, xytext=origin,
                    arrowprops=dict(arrowstyle="-|>", color=color, lw=1.7, ls=ls))
        ax.text(*(origin + (length + 0.12) * d), f"$\\hat{{{name}}}_{label}$",
                color=color, fontsize=11, ha="center", va="center")


def draw_rotation() -> None:
    """Figure 4-1: one body, two bases, one point in two coordinate pairs."""
    th = math.radians(30.0)
    moved = rotate(th)
    fig, ax = plt.subplots(figsize=(6.6, 6.2))
    draw_frame(ax, np.zeros(2), 0.0, "0.35", "s", 1.25)
    draw_frame(ax, np.zeros(2), th, "tab:blue", "b", 1.25)

    # Initial (dashed) and rotated (solid) triangle; {b} turns with the body.
    ax.add_patch(plt.Polygon(TRIANGLE, closed=True, fill=False, ls="--",
                             lw=1.4, edgecolor="0.55"))
    ax.add_patch(plt.Polygon(moved, closed=True, fill=True, alpha=0.18,
                             facecolor="tab:blue", edgecolor="tab:blue", lw=1.8))
    for name, q in zip("ABC", moved):
        ax.plot(*q, "o", color="tab:blue", ms=5)
        ax.text(*(q + np.array([0.07, 0.07])), f"{name}'", color="tab:blue", fontsize=9)

    # The same material point before and after the rotation.
    p0, p1 = TRIANGLE[0], moved[0]
    ax.plot(*p0, "o", color="0.45", ms=5)
    ax.plot(*p1, "o", color="tab:blue", ms=5)
    for p, c in ((p0, "0.45"), (p1, "tab:blue")):
        ax.plot([p[0], p[0]], [0.0, p[1]], ls=":", color=c, lw=1.0)
        ax.plot([0.0, p[0]], [p[1], p[1]], ls=":", color=c, lw=1.0)
    ax.text(p0[0] + 0.07, p0[1] - 0.16, r"$P_0$", color="0.4", fontsize=11)
    ax.text(p1[0] - 0.10, p1[1] + 0.12, r"$P$", color="tab:blue", fontsize=11)
    ax.text(0.60, 1.92,
            "$P_0=(x_b,y_b)=(1.0000,\\,0.2000)$\n"
            "$P=(x_s,y_s)=(0.7660,\\,0.6732)$",
            fontsize=9.5, color="0.2",
            bbox=dict(boxstyle="round,pad=0.3", fc="white", ec="0.8", alpha=0.9))

    ax.annotate("", xy=(0.62 * math.cos(th), 0.62 * math.sin(th)),
                xytext=(0.62, 0.0),
                arrowprops=dict(arrowstyle="-|>", color="tab:red", lw=1.3,
                                connectionstyle="arc3,rad=0.25"))
    ax.text(0.80 * math.cos(0.5 * th), 0.80 * math.sin(0.5 * th) + 0.05,
            r"$\theta$", color="tab:red", fontsize=13)
    ax.text(-0.14, -0.16, r"$O$", fontsize=11)
    ax.set_xlim(-0.55, 2.75)
    ax.set_ylim(-0.65, 2.35)
    ax.set_aspect("equal")
    ax.grid(True, alpha=0.25)
    ax.set_xlabel(r"$x_s$ / m")
    ax.set_ylabel(r"$y_s$ / m")
    ax.set_title(r"平面旋转：$\{b\}$ 转动 $\theta=30^\circ$，同一批顶点换了坐标")
    savefig(fig, "4_1_rotation2d")


def draw_basis() -> None:
    """Figure 4-2: one vector, two bases, two coordinate pairs."""
    a, b = 1.6, 1.0
    ang = math.radians(25.0)
    w1 = np.array([math.cos(ang), math.sin(ang)])
    w2 = np.array([-math.sin(ang), math.cos(ang)])
    v = a * w1 + b * w2

    fig, ax = plt.subplots(figsize=(6.2, 5.8))
    draw_frame(ax, np.zeros(2), 0.0, "0.6", "s", 1.0)
    for w, name in ((w1, "1"), (w2, "2")):
        ax.annotate("", xy=w, xytext=np.zeros(2),
                    arrowprops=dict(arrowstyle="-|>", color="tab:blue", lw=1.6))
        ax.text(*(1.12 * w), r"$\hat{\mathbf{w}}_" + name + "$",
                color="tab:blue", fontsize=12, ha="center")

    # Parallelogram decomposition v = a w1 + b w2.
    ax.annotate("", xy=a * w1, xytext=np.zeros(2),
                arrowprops=dict(arrowstyle="-|>", color="tab:green", lw=1.5))
    ax.annotate("", xy=b * w2, xytext=np.zeros(2),
                arrowprops=dict(arrowstyle="-|>", color="tab:orange", lw=1.5))
    ax.annotate("", xy=v, xytext=np.zeros(2),
                arrowprops=dict(arrowstyle="-|>", color="k", lw=2.2))
    ax.plot([a * w1[0], v[0]], [a * w1[1], v[1]], ls="--", color="0.45", lw=1.0)
    ax.plot([b * w2[0], v[0]], [b * w2[1], v[1]], ls="--", color="0.45", lw=1.0)
    ax.text(*(0.5 * a * w1 + np.array([-0.18, -0.10])), r"$a\hat{\mathbf{w}}_1$",
            color="tab:green", fontsize=11)
    ax.text(*(0.5 * b * w2 + np.array([0.08, 0.02])), r"$b\hat{\mathbf{w}}_2$",
            color="tab:orange", fontsize=11)
    ax.text(*(v + np.array([0.04, 0.08])), r"$\mathbf{v}$", fontsize=13)
    ax.set_xlim(-0.75, 2.3)
    ax.set_ylim(-0.55, 2.35)
    ax.set_aspect("equal")
    ax.grid(True, alpha=0.25)
    ax.set_xlabel(r"$x$ / m")
    ax.set_ylabel(r"$y$ / m")
    ax.set_title(r"标准正交基与坐标：$\mathbf{v}=a\hat{\mathbf{w}}_1+b\hat{\mathbf{w}}_2$")
    savefig(fig, "4_1_basis")
    print(f"\n 基 w1 = ({w1[0]:.4f}, {w1[1]:.4f})，w2 = ({w2[0]:.4f}, {w2[1]:.4f})")
    print(f" 同一向量 v 在标准基下 = ({v[0]:.4f}, {v[1]:.4f})，在 w 基下 = ({a:.4f}, {b:.4f})")


def main() -> None:
    print("=== 4.1 平面旋转：同一批顶点，两套坐标 ===")
    out30 = report(30.0)
    report(90.0)

    # ANCHOR: invariants
    R = R2(math.radians(30.0))
    print("\n 30 deg 旋转矩阵的两个不变量：")
    print(f" 正交性 ||R^T R - I||_max = {np.max(np.abs(R.T @ R - np.eye(2))):.2e}")
    print(f" det R = {np.linalg.det(R):.4f}")
    print(f" R^T = R^-1 的最大偏差 = {np.max(np.abs(R.T - np.linalg.inv(R))):.2e}")

    # Composition and the subscript rule R(20) R(10) = R(30).
    print("\n 复合：R(20 deg) R(10 deg) 与 R(30 deg) 的最大元素差 = "
          f"{np.max(np.abs(R2(math.radians(20)) @ R2(math.radians(10)) - R)):.2e}")
    # The same triangle reached by one rotation of 30 deg.
    two_steps = (R2(math.radians(20)) @ rotate(math.radians(10)).T).T
    print(f" 先转 10 deg 再转 20 deg 与一次转 30 deg 的顶点最大差 = "
          f"{np.max(np.abs(two_steps - out30)):.2e} m")

    # ANCHOR_END: invariants
    draw_rotation()
    draw_basis()


if __name__ == "__main__":
    main()
