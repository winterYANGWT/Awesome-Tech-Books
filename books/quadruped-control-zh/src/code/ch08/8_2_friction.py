"""Friction cone versus the linearised friction pyramid (Section 8.2).

The example samples foot forces inside the four-sided pyramid and checks them
against the true cone ``sqrt(fx^2 + fy^2) <= mu fz``.  The pyramid is an
*outer* approximation: every cone-feasible force is pyramid-feasible, but the
corners of the pyramid lie outside the cone, so the linear constraints can
over-estimate the available friction by up to ``sqrt(2)``.

expect: 摩擦圆锥
expect: 四棱锥可行
expect: 圆锥可行但四棱锥不可行
expect: 四棱锥可行但圆锥不可行
expect: 角点
"""

from __future__ import annotations

import logging
import os

os.environ.setdefault("MESA_SHADER_CACHE_DISABLE", "true")

import numpy as np
from quadruped.plotting import savefig, label
import matplotlib.pyplot as plt
from matplotlib.patches import Circle, Rectangle
from mpl_toolkits.mplot3d.art3d import Poly3DCollection


logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

MU = 0.6
N = 1000
RNG = np.random.default_rng(0)


# ANCHOR: constraints
def pyramid_feasible(f: np.ndarray, mu: float = MU) -> np.ndarray:
    f = np.asarray(f, dtype=float).reshape(-1, 3)
    return ((np.abs(f[:, 0]) <= mu * f[:, 2])
            & (np.abs(f[:, 1]) <= mu * f[:, 2])
            & (f[:, 2] >= 0.0))


def cone_feasible(f: np.ndarray, mu: float = MU) -> np.ndarray:
    f = np.asarray(f, dtype=float).reshape(-1, 3)
    return (np.hypot(f[:, 0], f[:, 1]) <= mu * f[:, 2]) & (f[:, 2] >= 0.0)
# ANCHOR_END: constraints


def sample_in_pyramid(n: int) -> np.ndarray:
    fz = RNG.uniform(0.0, 50.0, n)
    fx = RNG.uniform(-1.0, 1.0, n) * MU * fz
    fy = RNG.uniform(-1.0, 1.0, n) * MU * fz
    return np.column_stack([fx, fy, fz])


def sample_in_box(n: int) -> np.ndarray:
    return np.column_stack([RNG.uniform(-40.0, 40.0, n),
                            RNG.uniform(-40.0, 40.0, n),
                            RNG.uniform(0.0, 50.0, n)])


def report(name: str, f: np.ndarray) -> None:
    pyr = pyramid_feasible(f)
    con = cone_feasible(f)
    print(f"{name}（{len(f)} 个力）：")
    print(f"  四棱锥可行 = {pyr.sum():4d} ({100.0 * pyr.mean():6.2f} %)")
    print(f"  圆锥可行   = {con.sum():4d} ({100.0 * con.mean():6.2f} %)")
    both = np.sum(con & ~pyr)
    over = np.sum(pyr & ~con)
    print(f"  圆锥可行但四棱锥不可行 = {both:4d} "
          f"({100.0 * both / max(1, con.sum()):6.2f} % of cone-feasible)")
    print(f"  四棱锥可行但圆锥不可行 = {over:4d} "
          f"({100.0 * over / max(1, pyr.sum()):6.2f} % of pyramid-feasible)")


def main() -> None:
    print("=== 8.2 摩擦圆锥与四棱锥 ===")
    print(f"静摩擦系数 mu = {MU:.3f}，每个足端的力 f = [fx, fy, fz]")
    report("在四棱锥内均匀采样", sample_in_pyramid(N))
    report("在力空间盒内均匀采样", sample_in_box(N))
    print(f"四棱锥角点：|fx| = |fy| = mu fz，水平合力 = sqrt(2) mu fz "
          f"= {np.sqrt(2.0) * MU:.4f} fz，比圆锥边界高 "
          f"{(np.sqrt(2.0) - 1.0) * 100.0:.2f} %。")
    print("因此四棱锥不是保守内近似，而是外近似；工程上要把 mu 乘 0.7–0.8 留裕量。")

    # Figure 8-3: the cone, the pyramid and a horizontal cross-section.
    fig = plt.figure(figsize=(10.8, 4.6))
    ax = fig.add_subplot(1, 2, 1, projection="3d")
    z = np.linspace(0.0, 40.0, 24)
    theta = np.linspace(0.0, 2.0 * np.pi, 48)
    Z, T = np.meshgrid(z, theta)
    X = MU * Z * np.cos(T)
    Y = MU * Z * np.sin(T)
    ax.plot_wireframe(X, Y, Z, color="#1565c0", rstride=3, cstride=4,
                      linewidth=0.55, alpha=0.55)
    top = 40.0
    corners = np.array([[MU * top, MU * top, top],
                        [-MU * top, MU * top, top],
                        [-MU * top, -MU * top, top],
                        [MU * top, -MU * top, top]])
    for c in corners:
        ax.plot([0.0, c[0]], [0.0, c[1]], [0.0, c[2]], color="#c62828", lw=1.4)
    ax.add_collection3d(Poly3DCollection([corners], alpha=0.12,
                                         facecolor="#c62828",
                                         edgecolor="#c62828"))
    ax.quiver(0, 0, 0, 12, 8, 30, color="#2e7d32", lw=1.8,
              arrow_length_ratio=0.10)
    ax.text(12, 8, 30, "  $f$", color="#2e7d32", fontsize=10)
    ax.quiver(0, 0, 0, MU * 30, MU * 30, 30, color="#c62828", lw=1.4,
              arrow_length_ratio=0.10, ls="--")
    ax.text(MU * 30, MU * 30, 30, "  角点", color="#c62828", fontsize=9)
    ax.set_xlabel("$f_x$ (N)")
    ax.set_ylabel("$f_y$ (N)")
    ax.set_zlabel("$f_z$ (N)")
    ax.set_xlim(-30, 30)
    ax.set_ylim(-30, 30)
    ax.set_zlim(0, 45)
    ax.set_title(label("摩擦圆锥（蓝）与四棱锥（红）",
                       "friction cone and pyramid"))
    ax.view_init(elev=20, azim=-55)

    ax = fig.add_subplot(1, 2, 2)
    fz = 30.0
    r = MU * fz
    ax.add_patch(Rectangle((-r, -r), 2 * r, 2 * r, fill=False,
                           edgecolor="#c62828", lw=1.8,
                           label="四棱锥截面 $|f_x|,|f_y|\\leq\\mu f_z$"))
    ax.add_patch(Circle((0.0, 0.0), r, fill=False,
                        edgecolor="#1565c0", lw=1.8,
                        label="圆锥截面 $\\sqrt{f_x^2+f_y^2}\\leq\\mu f_z$"))
    ax.fill([-r, r, r, -r], [-r, -r, r, r], facecolor="#ffcdd2",
            alpha=0.55, hatch="//", edgecolor="#c62828")
    ax.add_patch(Circle((0.0, 0.0), r, facecolor="white", edgecolor="none"))
    ax.plot([0, r], [0, 0], color="0.4", lw=1.0, ls=":")
    ax.text(r * 0.5, -0.12 * r, "$\\mu f_z$", ha="center", fontsize=10)
    ax.annotate("角点 $\\sqrt{2}\\mu f_z$\n超出圆锥",
                xy=(r, r), xytext=(0.35 * r, 1.28 * r),
                arrowprops=dict(arrowstyle="->", color="#c62828"),
                color="#c62828", fontsize=9, ha="center")
    ax.set_aspect("equal")
    ax.set_xlim(-1.6 * r, 1.6 * r)
    ax.set_ylim(-1.6 * r, 1.6 * r)
    ax.set_xlabel("$f_x$ (N)")
    ax.set_ylabel("$f_y$ (N)")
    ax.set_title(label(f"$f_z$ = {fz:.0f} N 的水平截面",
                       f"cross-section at $f_z$ = {fz:.0f} N"))
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=7, loc="lower left")
    fig.tight_layout()
    savefig(fig, "8_3_friction")
    plt.close(fig)


if __name__ == "__main__":
    main()
