"""Analytic Jacobian, inverse velocity and static force mapping (section 5.3).

The analytic 3 x 3 Jacobian is compared with central differences.  A foot
force is converted to joint torques by J^T f and checked against the gradient
of the virtual work f . p(q).  The last figure draws the velocity and force
ellipsoids at a folded, strongly anisotropic pose and marks the direction of
the smallest singular value.

expect: 解析 J 与中心差分
expect: J.T f 与虚功
expect: 条件数
expect: 关节力矩
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

L_THIGH = 0.20
L_CALF = 0.20
D_RIGHT = -0.08505
Q_MIN = np.array([-0.802851, -1.0472, -2.69653])
Q_MAX = np.array([0.802851, 4.18879, -0.916298])


def fk(q: np.ndarray) -> np.ndarray:
    q1, q2, q3 = np.asarray(q, dtype=float)
    s1, c1 = math.sin(q1), math.cos(q1)
    s2, c2 = math.sin(q2), math.cos(q2)
    s23, c23 = math.sin(q2 + q3), math.cos(q2 + q3)
    return np.array([
        -L_CALF * s23 - L_THIGH * s2,
        D_RIGHT * c1 + s1 * (L_THIGH * c2 + L_CALF * c23),
        D_RIGHT * s1 - c1 * (L_THIGH * c2 + L_CALF * c23),
    ])
# ANCHOR: jacobian


def jacobian(q: np.ndarray) -> np.ndarray:
    """J = dp/dq, obtained by differentiating Eq. (5-11) term by term."""
    q1, q2, q3 = np.asarray(q, dtype=float)
    s1, c1 = math.sin(q1), math.cos(q1)
    s2, c2 = math.sin(q2), math.cos(q2)
    s23, c23 = math.sin(q2 + q3), math.cos(q2 + q3)
    B = L_THIGH * c2 + L_CALF * c23
    D = L_THIGH * s2 + L_CALF * s23
    return np.array([
        [0.0, -B, -L_CALF * c23],
        [-D_RIGHT * s1 + c1 * B, -s1 * D, -s1 * L_CALF * s23],
        [D_RIGHT * c1 + s1 * B, c1 * D, c1 * L_CALF * s23],
    ])
# ANCHOR_END: jacobian


def jacobian_fd(q: np.ndarray, h: float = 1e-6) -> np.ndarray:
    """Central difference, used only as an independent check."""
    J = np.zeros((3, 3))
    for i in range(3):
        e = np.zeros(3)
        e[i] = h
        J[:, i] = (fk(q + e) - fk(q - e)) / (2.0 * h)
    return J


def singular(q: np.ndarray):
    J = jacobian(q)
    U, s, Vt = np.linalg.svd(J)
    return J, U, s, Vt


def cond_scan() -> tuple[float, np.ndarray]:
    best, best_q = 0.0, np.zeros(3)
    for q1 in np.linspace(Q_MIN[0], Q_MAX[0], 9):
        for q2 in np.linspace(Q_MIN[1], Q_MAX[1], 9):
            for q3 in np.linspace(Q_MIN[2], Q_MAX[2], 9):
                q = np.array([q1, q2, q3])
                s = np.linalg.svd(jacobian(q), compute_uv=False)
                c = s[0] / max(s[-1], 1e-15)
                if c > best:
                    best, best_q = float(c), q
    return best, best_q


def draw_ellipsoids(q: np.ndarray) -> None:
    J, U, s, Vt = singular(q)
    # Normalised display: each ellipsoid keeps its shape but its longest
    # semi-axis is drawn with unit length.
    axes_v = s / s[0]
    axes_f = (1.0 / s) / (1.0 / s[-1])
    uu = np.linspace(0.0, 2.0 * math.pi, 48)
    vv = np.linspace(0.0, math.pi, 24)
    UU, VV = np.meshgrid(uu, vv)
    sphere = np.stack([np.cos(UU) * np.sin(VV),
                       np.sin(UU) * np.sin(VV),
                       np.cos(VV)], axis=-1)

    fig = plt.figure(figsize=(10.6, 4.8))
    for k, (axes, title) in enumerate((
            (axes_v, label("速度椭球：$J\\dot q$ 的像（归一化）",
                           "velocity ellipsoid (normalised)")),
            (axes_f, label("力椭球：$J^{-T}\\tau$ 的像（归一化）",
                           "force ellipsoid (normalised)")))):
        ax = fig.add_subplot(1, 2, k + 1, projection="3d")
        pts = sphere @ (U @ np.diag(axes)).T
        ax.plot_surface(pts[..., 0], pts[..., 1], pts[..., 2],
                        color=("tab:blue" if k == 0 else "tab:orange"),
                        alpha=0.45, linewidth=0.0, shade=True)
        # Three principal axes and the smallest-singular-value direction.
        for i in range(3):
            v = U[:, i] * max(axes[i], 0.02)
            ax.plot([-v[0], v[0]], [-v[1], v[1]], [-v[2], v[2]],
                    color="0.25", lw=0.9)
        v = U[:, -1] * (1.0 if k == 1 else 0.55)
        ax.quiver(0, 0, 0, *v, color="tab:red", lw=2.0, arrow_length_ratio=0.18)
        ax.text(*(1.08 * v), r"$\sigma_{\min}$", color="tab:red", fontsize=10)
        ax.set_xlim(-1.05, 1.05)
        ax.set_ylim(-1.05, 1.05)
        ax.set_zlim(-1.05, 1.05)
        ax.set_box_aspect((1, 1, 1))
        ax.set_xlabel(r"$x_0$")
        ax.set_ylabel(r"$y_0$")
        ax.set_zlabel(r"$z_0$")
        ax.tick_params(labelsize=7)
        ax.view_init(elev=22, azim=38)
        ax.set_title(title, fontsize=10)
    fig.suptitle(label(
        rf"$\sigma=({s[0]:.4f},\ {s[1]:.4f},\ {s[2]:.4f})$ m/rad, "
        rf"条件数 {s[0] / s[-1]:.2f}",
        rf"$\sigma=({s[0]:.4f},\ {s[1]:.4f},\ {s[2]:.4f})$ m/rad, "
        rf"cond {s[0] / s[-1]:.2f}"), fontsize=10)
    fig.tight_layout()
    savefig(fig, "5_3_ellipsoid")
    plt.close(fig)


def main() -> None:
    q = np.array([0.10, -0.20, -2.60])
    p = fk(q)
    J = jacobian(q)
    J_fd = jacobian_fd(q)
    U, s, Vt = np.linalg.svd(J)
    print("=== 5.3 单腿雅可比与静力学 ===")
    print(f"位形 q = {q.round(4)} rad, P = {p.round(5)} m")
    print("\n解析 J =")
    for row in J:
        print("  [" + " ".join(f"{v:9.4f}" for v in row) + "]")
    print(f"\n解析 J 与中心差分（h=1e-6）的最大元素差 = "
          f"{np.max(np.abs(J - J_fd)):.2e}")
    print(f"奇异值 = {s.round(5)}")
    print(f"条件数 = {s[0] / s[-1]:.2f}")
    print(f"最小奇异方向 (输出空间) = {U[:, -1].round(4)}")
    c, cq = cond_scan()
    print(f"关节限位内 9x9x9 网格扫描的最大条件数 = {c:.2f} "
          f"at q = {cq.round(4)}")

    f = np.array([10.0, -5.0, -30.0])
    tau = J.T @ f
    # Numerically differentiate the virtual work W(q)=f.p(q); its gradient is
    # exactly J^T f in the ideal case.
    h = 1e-6
    grad = np.zeros(3)
    for i in range(3):
        e = np.zeros(3)
        e[i] = h
        grad[i] = (f @ fk(q + e) - f @ fk(q - e)) / (2.0 * h)
    print(f"\n足端力 f = {f.round(4)} N")
    print(f"关节力矩 tau = J^T f = {tau.round(4)} N·m")
    print(f"J^T f 与虚功数值积分 W=f·p 的最大差 = {np.max(np.abs(tau - grad)):.2e}")
    dq = np.array([0.7, -0.4, 0.2])
    print(f"功率校验 |tau·dq - f·(J dq)| = "
          f"{abs(tau @ dq - f @ (J @ dq)):.2e} W")

    draw_ellipsoids(q)


if __name__ == "__main__":
    main()
