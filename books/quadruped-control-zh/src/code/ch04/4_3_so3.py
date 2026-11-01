"""Axis-angle, the exponential map and the logarithm (section 4.3).

The script implements [v]x, exp and log from the formulas of section 4.3,
checks them against ``quadruped.rotation`` and against
``scipy.spatial.transform.Rotation``, and draws the three figures of the
section: the cross product, the velocity of a rotating point, and the powers
of the skew matrix together with the exponential trajectory.

expect: 叉乘
expect: log->exp
expect: scipy
expect: theta = pi
"""

from __future__ import annotations

import logging
import math

import numpy as np

from quadruped.plotting import savefig
from quadruped.rotation import so3_exp as exp_ref
from quadruped.rotation import so3_log as log_ref
import matplotlib.pyplot as plt

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

RNG = np.random.default_rng(4)


# ANCHOR: algebra
def skew(v: np.ndarray) -> np.ndarray:
    x, y, z = np.asarray(v, dtype=float).reshape(3)
    return np.array([[0.0, -z, y], [z, 0.0, -x], [-y, x, 0.0]])


def so3_exp(w: np.ndarray) -> np.ndarray:
    """Rodrigues: exp([w]x) with w the rotation vector."""
    w = np.asarray(w, dtype=float).reshape(3)
    th = float(np.linalg.norm(w))
    K = skew(w)
    if th < 1e-12:
        return np.eye(3) + K + 0.5 * K @ K
    a = w / th
    K = skew(a)
    return np.eye(3) + math.sin(th) * K + (1.0 - math.cos(th)) * (K @ K)


def so3_log(R: np.ndarray) -> np.ndarray:
    """Inverse of exp: theta from atan2, axis from the antisymmetric part."""
    R = np.asarray(R, dtype=float).reshape(3, 3)
    vee = np.array([R[2, 1] - R[1, 2], R[0, 2] - R[2, 0], R[1, 0] - R[0, 1]])
    s = 0.5 * float(np.linalg.norm(vee))       # sin(theta)
    c = 0.5 * (float(np.trace(R)) - 1.0)       # cos(theta)
    th = math.atan2(s, c)
    if s < 1e-12:
        if c > 0.0:
            return np.zeros(3)                 # theta = 0
        # theta = pi: R + I = 2 a a^T, take the best-conditioned column.
        A = 0.5 * (R + np.eye(3))
        axis = A[:, int(np.argmax(np.diag(A)))]
        axis = axis / np.linalg.norm(axis)
        if np.dot(axis, vee) < 0.0:
            axis = -axis
        return axis * math.pi
    return vee * (th / (2.0 * s))


# ANCHOR_END: algebra

def angle_of(R: np.ndarray) -> float:
    """Geodesic angle, evaluated with atan2 so that it stays accurate near pi."""
    R = np.asarray(R, dtype=float).reshape(3, 3)
    vee = np.array([R[2, 1] - R[1, 2], R[0, 2] - R[2, 0], R[1, 0] - R[0, 1]])
    return math.atan2(0.5 * float(np.linalg.norm(vee)),
                      0.5 * (float(np.trace(R)) - 1.0))


def cross_case() -> None:
    a = np.array([1.0, 0.0, 0.3])
    b = np.array([0.2, 1.0, 0.1])
    c = np.cross(a, b)
    th = math.acos(np.dot(a, b) / np.linalg.norm(a) / np.linalg.norm(b))
    print("=== 4.3 叉乘、指数映射与对数 ===")
    print(f"\n a = ({a[0]:.4f}, {a[1]:.4f}, {a[2]:.4f}), "
          f"b = ({b[0]:.4f}, {b[1]:.4f}, {b[2]:.4f})")
    print(f" 叉乘 a x b = ({c[0]:.4f}, {c[1]:.4f}, {c[2]:.4f})")
    print(f" |a x b| = {np.linalg.norm(c):.4f}，"
          f"|a||b|sin(theta) = {np.linalg.norm(a) * np.linalg.norm(b) * math.sin(th):.4f}")
    print(f" [a]x b 与 a x b 的最大差 = {np.max(np.abs(skew(a) @ b - c)):.2e}")
    print(f" 反对称性 ||[a]x + [a]x^T||_max = "
          f"{np.max(np.abs(skew(a) + skew(a).T)):.2e}")
    print(f" 反交换 [a]x b + [b]x a = "
          f"{np.max(np.abs(skew(a) @ b + skew(b) @ a)):.2e}")
    # BAC-CAB: a x (b x c) = (a.c) b - (a.b) c
    d = np.array([0.4, -0.7, 1.1])
    lhs = np.cross(a, np.cross(b, d))
    rhs = np.dot(a, d) * b - np.dot(a, b) * d
    print(f" 二重向量积 BAC-CAB 误差 = {np.max(np.abs(lhs - rhs)):.2e}")

    fig = plt.figure(figsize=(6.8, 6.2))
    ax = fig.add_subplot(projection="3d")
    ax.quiver(0, 0, 0, *a, color="tab:blue", lw=2.2, arrow_length_ratio=0.12)
    ax.quiver(0, 0, 0, *b, color="tab:green", lw=2.2, arrow_length_ratio=0.12)
    ax.quiver(0, 0, 0, *c, color="tab:red", lw=2.4, arrow_length_ratio=0.12)
    # Parallelogram spanned by a and b, whose area is |a x b|.
    quad = np.array([[0, 0, 0], a, a + b, b, [0, 0, 0]])
    ax.plot(quad[:, 0], quad[:, 1], quad[:, 2], color="0.55", ls="--", lw=1.0)
    ax.plot([a[0], c[0]], [a[1], c[1]], [a[2], c[2]], color="0.6", ls=":", lw=0.9)
    ax.plot([b[0], c[0]], [b[1], c[1]], [b[2], c[2]], color="0.6", ls=":", lw=0.9)
    ax.text(*(1.05 * a), r"$\mathbf{a}$", color="tab:blue", fontsize=12)
    ax.text(*(1.05 * b), r"$\mathbf{b}$", color="tab:green", fontsize=12)
    ax.text(*(1.06 * c), r"$\mathbf{c}$", color="tab:red", fontsize=12)
    ax.text(0.75, 0.55, 0.55, r"$S=|\mathbf{a}||\mathbf{b}|\sin\theta$",
            fontsize=10, color="0.25")
    ax.set_xlim(-0.2, 1.3)
    ax.set_ylim(-0.4, 1.4)
    ax.set_zlim(-0.2, 1.0)
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.set_zlabel("z")
    ax.view_init(elev=20, azim=-58)
    ax.grid(False)
    for pane in (ax.xaxis.pane, ax.yaxis.pane, ax.zaxis.pane):
        pane.set_alpha(0.0)
    ax.set_title(r"$\mathbf{c}=\mathbf{a}\times\mathbf{b}$：方向由右手定则确定，模长为平行四边形面积")
    fig.tight_layout()
    savefig(fig, "4_3_cross")


def velocity_case() -> None:
    """A point turning about a fixed axis at 1 rad/s."""
    axis = np.array([0.35, 0.25, 0.9])
    axis = axis / np.linalg.norm(axis)
    p = np.array([1.0, -0.55, 0.2])
    A = np.dot(p, axis) * axis                 # projection on the axis
    r = p - A                                  # radius vector AP
    v = np.cross(axis, p)                      # unit angular velocity
    print(f"\n 转轴 w = ({axis[0]:.4f}, {axis[1]:.4f}, {axis[2]:.4f})，|w| = {np.linalg.norm(axis):.4f}")
    print(f" 位置 p = ({p[0]:.4f}, {p[1]:.4f}, {p[2]:.4f})，|p| = {np.linalg.norm(p):.4f}")
    print(f" 半径 |AP| = {np.linalg.norm(r):.4f}，"
          f"|p|sin(angle) = {np.linalg.norm(p) * math.sin(math.acos(np.dot(p, axis) / np.linalg.norm(p))):.4f}")
    print(f" 速度 v = w x p = ({v[0]:.4f}, {v[1]:.4f}, {v[2]:.4f})，|v| = {np.linalg.norm(v):.4f}")
    print(f" v 与 p 的夹角 = {math.degrees(math.acos(np.dot(v, r) / np.linalg.norm(v) / np.linalg.norm(r))):.2f} deg")

    fig = plt.figure(figsize=(6.8, 6.2))
    ax = fig.add_subplot(projection="3d")
    ax.quiver(0, 0, 0, *axis, color="0.25", lw=2.2, arrow_length_ratio=0.1)
    ax.quiver(0, 0, 0, *p, color="tab:blue", lw=2.0, arrow_length_ratio=0.1)
    ax.quiver(*A, *r, color="tab:orange", lw=1.8, arrow_length_ratio=0.12)
    ax.quiver(*p, *v, color="tab:red", lw=2.0, arrow_length_ratio=0.14)
    # Circle traced by p about the axis.
    t = np.linspace(0.0, 2.0 * math.pi, 120)
    e1 = r / np.linalg.norm(r)
    e2 = np.cross(axis, e1)
    circle = A + np.outer(np.cos(t), r) + np.outer(np.sin(t), np.linalg.norm(r) * e2)
    ax.plot(circle[:, 0], circle[:, 1], circle[:, 2], color="tab:blue", ls="--", lw=1.0)
    ax.plot([A[0], p[0]], [A[1], p[1]], [A[2], p[2]], color="0.6", ls=":", lw=1.0)
    ax.plot(*A, "o", color="tab:orange", ms=5)
    # Right-angle mark between the axis and the radius AP.
    ea = axis / np.linalg.norm(axis)
    er = r / np.linalg.norm(r)
    q = 0.09 * (ea + er)
    ax.plot([A[0] + q[0], A[0] + q[0] + 0.09 * er[0]],
            [A[1] + q[1], A[1] + q[1] + 0.09 * er[1]],
            [A[2] + q[2], A[2] + q[2] + 0.09 * er[2]], color="0.4", lw=1.0)
    ax.plot([A[0] + q[0], A[0] + q[0] + 0.09 * ea[0]],
            [A[1] + q[1], A[1] + q[1] + 0.09 * ea[1]],
            [A[2] + q[2], A[2] + q[2] + 0.09 * ea[2]], color="0.4", lw=1.0)
    ax.text(*(1.08 * axis), r"$\hat{\boldsymbol{\omega}}$", fontsize=12)
    ax.text(*(1.05 * p), r"$\mathbf{p}$", color="tab:blue", fontsize=12)
    ax.text(*(p + 0.70 * v + np.array([0.0, 0.0, 0.10])),
            r"$\mathbf{v}=\boldsymbol{\omega}\times\mathbf{p}$",
            color="tab:red", fontsize=10)
    ax.text(*(A + 0.55 * r + np.array([0.0, 0.0, 0.08])), r"$|AP|$",
            color="tab:orange", fontsize=10)
    ax.text(*(A + np.array([-0.12, -0.06, 0.06])), r"$A$", fontsize=10)
    ax.set_xlim(-0.5, 1.55)
    ax.set_ylim(-1.15, 1.0)
    ax.set_zlim(-0.15, 1.25)
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.set_zlabel("z")
    ax.view_init(elev=20, azim=-55)
    ax.grid(False)
    for pane in (ax.xaxis.pane, ax.yaxis.pane, ax.zaxis.pane):
        pane.set_alpha(0.0)
    ax.set_title("绕轴旋转的线速度：大小 $\\dot\\alpha|AP|$，方向同时垂直 $\\hat{\\boldsymbol{\\omega}}$ 与 $\\mathbf{p}$")
    fig.tight_layout()
    savefig(fig, "4_3_velocity")


def powers_case() -> None:
    """Powers of [w]x rotate by 90 deg each; exp sweeps the whole arc."""
    axis = np.array([0.0, 0.0, 1.0])
    a = np.array([1.0, 0.0, 0.0])
    K = skew(axis)
    seq = [a]
    for _ in range(4):
        seq.append(K @ seq[-1])
    print("\n [w]x 的幂（w = z 轴，a = x 轴）：")
    for k, v in enumerate(seq):
        print(f" [w]x^{k} a = ({v[0]: .4f}, {v[1]: .4f}, {v[2]: .4f})")
    print(f" [w]x^3 + [w]x 的最大元素 = {np.max(np.abs(K @ K @ K + K)):.2e}")
    print(f" [w]x^4 + [w]x^2 的最大元素 = {np.max(np.abs(K @ K @ K @ K + K @ K)):.2e}")

    fig = plt.figure(figsize=(11.4, 4.8))
    ax = fig.add_subplot(1, 2, 1)
    ax.set_aspect("equal")
    for v, name, color in zip(seq[:4], ("$\\mathbf{a}$", "$K\\mathbf{a}$",
                                       "$K^2\\mathbf{a}$", "$K^3\\mathbf{a}$"),
                              ("k", "tab:blue", "tab:green", "tab:orange")):
        ax.annotate("", xy=v[:2], xytext=(0, 0),
                    arrowprops=dict(arrowstyle="-|>", color=color, lw=2.0))
        ax.text(*(1.12 * v[:2]), name, color=color, fontsize=11, ha="center")
    ax.add_patch(plt.Circle((0, 0), 0.055, color="0.3"))
    ax.add_patch(plt.Circle((0, 0), 0.09, fill=False, color="0.3", lw=1.1))
    ax.text(-0.14, 0.03, r"$\hat{\boldsymbol{\omega}}$", fontsize=11, ha="right")
    ax.set_xlim(-1.35, 1.35)
    ax.set_ylim(-1.35, 1.35)
    ax.grid(True, alpha=0.25)
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.set_title("(a) 每乘一次 $[\\hat{\\boldsymbol{\\omega}}]_\\times$，向量绕轴转 $90^\\circ$")

    ax = fig.add_subplot(1, 2, 2, projection="3d")
    p0 = np.array([0.9, 0.0, 0.45])
    t = np.linspace(0.0, math.pi, 120)
    pts = np.array([so3_exp(axis * s) @ p0 for s in t])
    ax.plot(pts[:, 0], pts[:, 1], pts[:, 2], color="tab:blue", lw=1.6)
    for s, color, name, off in ((0.0, "0.35", r"$\theta=0$", [0.10, -0.10, -0.22]),
                                (math.pi / 2, "tab:green", r"$\theta=\pi/2$",
                                 [-0.10, 0.02, 0.18]),
                                (math.pi, "tab:red", r"$\theta=\pi$",
                                 [-0.55, -0.05, 0.12])):
        q = so3_exp(axis * s) @ p0
        ax.plot(*q, "o", color=color, ms=6)
        ax.text(*(q + np.array(off)), name, color=color, fontsize=10)
    ax.quiver(0, 0, 0, *axis, color="0.25", lw=1.8, arrow_length_ratio=0.1)
    ax.quiver(0, 0, 0, *p0, color="tab:blue", lw=1.6, arrow_length_ratio=0.1, ls="--")
    ax.set_xlim(-1.1, 1.1)
    ax.set_ylim(-1.1, 1.1)
    ax.set_zlim(-0.1, 1.1)
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.set_zlabel("z")
    ax.view_init(elev=26, azim=-66)
    ax.grid(False)
    for pane in (ax.xaxis.pane, ax.yaxis.pane, ax.zaxis.pane):
        pane.set_alpha(0.0)
    ax.set_title("(b) 指数映射把 $\\theta=0$ 的点沿圆弧送到 $\\theta=\\pi$")
    fig.tight_layout()
    savefig(fig, "4_3_powers")
    print(f" 指数映射轨迹：theta=0 的点 ({pts[0][0]:.4f}, {pts[0][1]:.4f}, {pts[0][2]:.4f})，"
          f"theta=pi 的点 ({pts[-1][0]:.4f}, {pts[-1][1]:.4f}, {pts[-1][2]:.4f})")


# ANCHOR: roundtrip
def roundtrip() -> None:
    n = 100
    worst = 0.0
    worst_scipy = 0.0
    worst_log = 0.0
    from scipy.spatial.transform import Rotation
    for _ in range(n):
        axis = RNG.normal(size=3)
        axis /= np.linalg.norm(axis)
        theta = RNG.uniform(1e-3, math.pi - 1e-3)
        w = axis * theta
        R = so3_exp(w)
        w2 = so3_log(R)
        worst = max(worst, angle_of(so3_exp(w2).T @ R))
        worst_scipy = max(worst_scipy, float(np.max(np.abs(R - Rotation.from_rotvec(w).as_matrix()))))
        worst_log = max(worst_log, float(np.max(np.abs(R - exp_ref(w2)))))
    print(f"\n {n} 组随机旋转：")
    print(f" log->exp 往返的最大旋转误差 = {worst:.2e} rad")
    print(f" 与 scipy Rotation.from_rotvec 的最大元素差 = {worst_scipy:.2e}")
    print(f" 用 quadruped.rotation 的 exp 重构 log 结果，最大元素差 = {worst_log:.2e}")
    # The library's own log, for reference.
    worst_lib = 0.0
    for _ in range(n):
        axis = RNG.normal(size=3)
        axis /= np.linalg.norm(axis)
        R = so3_exp(axis * RNG.uniform(1e-3, math.pi - 1e-3))
        worst_lib = max(worst_lib, angle_of(exp_ref(log_ref(R)).T @ R))
    print(f" quadruped.rotation 自身 log->exp 的最大旋转误差 = {worst_lib:.2e} rad")

    print("\n 特殊角度：")
    for name, w in (("theta = 0", np.zeros(3)),
                    ("theta = pi/2", np.array([0.0, 0.0, math.pi / 2])),
                    ("theta = pi", np.array([0.0, 0.0, math.pi]))):
        R = so3_exp(w)
        wl = so3_log(R)
        print(f" {name}: tr(R) = {np.trace(R): .4f}, "
              f"theta = {math.degrees(angle_of(R)):.4f} deg, "
              f"log = ({wl[0]: .4f}, {wl[1]: .4f}, {wl[2]: .4f})")
    # theta = pi about an axis with w1 = 0: r11 = -1, the vector formula fails.
    w = np.array([0.0, 1.0, 0.0]) * math.pi
    R = so3_exp(w)
    wl = so3_log(R)
    print(f" theta = pi, 轴 = +y: r11 = {R[0, 0]: .4f}, "
          f"log = ({wl[0]: .4f}, {wl[1]: .4f}, {wl[2]: .4f})")
    # exp of a non-unit vector: the angle is its norm.
    v = np.array([0.2, -0.3, 0.5])
    K = skew(v)                                # unnormalised: a common bug
    naive = np.eye(3) + math.sin(np.linalg.norm(v)) * K \
        + (1.0 - math.cos(np.linalg.norm(v))) * (K @ K)
    print(f" |w| = {np.linalg.norm(v):.4f} rad：正确 exp 与\"直接把 w 当单位轴\"的差 = "
          f"{np.max(np.abs(so3_exp(v) - naive)):.4f}")
    print(f" 正确 exp 与共用包 so3_exp 的最大差 = "
          f"{np.max(np.abs(so3_exp(v) - exp_ref(v))):.2e}")


# ANCHOR_END: roundtrip

def main() -> None:
    cross_case()
    velocity_case()
    powers_case()
    roundtrip()


if __name__ == "__main__":
    main()
