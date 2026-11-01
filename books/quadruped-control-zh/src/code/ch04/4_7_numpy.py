"""The same matrix operations in numpy, as a reference for the C++ example (4.7).

This script mirrors ``4_7_eigen.cpp`` step by step: comma-style construction,
diagonal matrices, 6x6 blocks, segments, an SE(3) assembled from blocks, the
analytic inverse, a 3x3 solve and 100 random transforms.  It also draws figure
4-11: the Eigen matrix layout and an excerpt of the program output.

expect: M6
expect: 解析逆
expect: 100 组随机
"""

from __future__ import annotations

import logging
import math

import numpy as np

from quadruped.plotting import savefig
from matplotlib import font_manager
import matplotlib.pyplot as plt

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

_NAMES = {f.name for f in font_manager.fontManager.ttflist}
MONO = "WenQuanYi Zen Hei Mono" if "WenQuanYi Zen Hei Mono" in _NAMES else "monospace"

RNG = np.random.default_rng(7)


def rot_x(t: float) -> np.ndarray:
    c, s = math.cos(t), math.sin(t)
    return np.array([[1.0, 0.0, 0.0], [0.0, c, -s], [0.0, s, c]])


def rot_y(t: float) -> np.ndarray:
    c, s = math.cos(t), math.sin(t)
    return np.array([[c, 0.0, s], [0.0, 1.0, 0.0], [-s, 0.0, c]])


def rot_z(t: float) -> np.ndarray:
    c, s = math.cos(t), math.sin(t)
    return np.array([[c, -s, 0.0], [s, c, 0.0], [0.0, 0.0, 1.0]])


def skew(v: np.ndarray) -> np.ndarray:
    x, y, z = v
    return np.array([[0.0, -z, y], [z, 0.0, -x], [-y, x, 0.0]])


def make_T(R: np.ndarray, p: np.ndarray) -> np.ndarray:
    T = np.eye(4)
    T[:3, :3] = R
    T[:3, 3] = p
    return T


def inv_T(T: np.ndarray) -> np.ndarray:
    out = np.eye(4)
    out[:3, :3] = T[:3, :3].T
    out[:3, 3] = -T[:3, :3].T @ T[:3, 3]
    return out


def mat(M: np.ndarray) -> str:
    return "\n".join(" ".join(f"{x:7.4f}" for x in row) for row in M)


def main() -> None:
    print("=== 4.7 numpy 对照 ===\n")
    # ANCHOR: numpy
    M = np.array([[1, 2, 3], [4, 5, 6], [7, 8, 9]], dtype=float)
    v = np.array([1.0, 2.0, 3.0])
    print("M (式 4.96):\n" + mat(M))
    print("v = " + " ".join(f"{x:.4f}" for x in v))
    print(f"元素个数 {M.size}，行 {M.shape[0]}，列 {M.shape[1]}")

    D = np.diag(np.array([1.0, 2.0, 3.0]))
    print("\nM = diag(1,2,3) (式 4.97):\n" + mat(D))

    M6 = np.zeros((6, 6))
    M6[0:3, 3:6] = np.eye(3)
    M6[3:6, 0:3] = np.diag(np.array([2.0, 2.0, 2.0]))
    print("\nM6 (式 4.98):\n" + mat(M6))

    v6 = np.zeros(6)
    v6[1:4] = np.array([10.0, 20.0, 30.0])
    print("v6 (式 4.99): " + " ".join(f"{x:7.4f}" for x in v6))
    # ANCHOR_END: numpy

    a = math.radians(30.0)
    R = rot_x(a) @ rot_y(2.0 * a)
    p = np.array([0.4, -0.2, 0.7])
    T = make_T(R, p)
    print("\nT (由 block 拼成):\n" + mat(T))
    print(f"R^T R - I 的最大元素 = {np.max(np.abs(R.T @ R - np.eye(3))):.2e}")
    print(f"det R = {np.linalg.det(R):.4f}")
    print(f"R^-1 - R^T 的最大元素 = {np.max(np.abs(np.linalg.inv(R) - R.T)):.2e}")
    print(f"R^-1 - Ry(-60) Rx(-30) 的最大元素 = "
          f"{np.max(np.abs(np.linalg.inv(R) - rot_y(-2.0 * a) @ rot_x(-a))):.2e}")

    Ti = inv_T(T)
    print("\n解析逆 T^-1:\n" + mat(Ti))
    print(f"T T^-1 - I 的最大元素 = {np.max(np.abs(T @ Ti - np.eye(4))):.2e}")
    print(f"解析逆与通用求逆的最大元素差 = "
          f"{np.max(np.abs(Ti - np.linalg.inv(T))):.2e}")
    print("解析逆乘法次数 = 9；4x4 LU 求逆（含 4 次回代）约 100")

    A = np.array([[4.0, -1.0, 0.0], [-1.0, 4.0, -1.0], [0.0, -1.0, 3.0]])
    b = np.array([2.0, 4.0, 10.0])
    x = np.linalg.solve(A, b)
    print("\nA x = b 的解 = " + " ".join(f"{t:.4f}" for t in x))
    print(f"残差 |Ax-b| = {np.linalg.norm(A @ x - b):.2e}")
    print(f"显式逆解与分解解的差 = {np.linalg.norm(x - np.linalg.inv(A) @ b):.2e}")

    rpy = np.array([0.3, 0.4, 0.5])
    R2 = rot_z(rpy[2]) @ rot_y(rpy[1]) @ rot_x(rpy[0])
    sp = max(-1.0, min(1.0, -R2[2, 0]))
    back = np.array([math.atan2(R2[2, 1], R2[2, 2]), math.asin(sp),
                     math.atan2(R2[1, 0], R2[0, 0])])
    print(f"\nrpyToRot 再 rotToRpy: 误差 = {np.linalg.norm(back - rpy):.2e}")
    print(f"skew(v) v 的最大元素 = {np.max(np.abs(skew(v) @ v)):.2e}")
    print(f"homoVec(p) =  {p[0]:.4f} {p[1]:.4f} {p[2]:.4f}  1.0000，"
          f"noHomoVec =  {p[0]:.4f} {p[1]:.4f} {p[2]:.4f}")

    worst = worst_inv = 0.0
    for _ in range(100):
        axis = RNG.normal(size=3)
        axis /= np.linalg.norm(axis)
        th = RNG.uniform(0.0, math.pi)
        K = skew(axis)
        Rr = np.eye(3) + math.sin(th) * K + (1.0 - math.cos(th)) * (K @ K)
        Tr = make_T(Rr, RNG.normal(size=3))
        Tri = inv_T(Tr)
        worst = max(worst, float(np.max(np.abs(Tr @ Tri - np.eye(4)))))
        worst_inv = max(worst_inv, float(np.max(np.abs(Tri - np.linalg.inv(Tr)))))
    print(f"\n100 组随机 T：||T T^-1 - I||_max = {worst:.2e}，"
          f"解析逆与通用逆的最大差 = {worst_inv:.2e}")

    # Figure 4-11: column-major layout and an excerpt of the output.
    fig = plt.figure(figsize=(12.8, 5.6))
    ax = fig.add_subplot(1, 2, 1)
    ax.set_xlim(-0.15, 4.15)
    ax.set_ylim(-2.3, 4.15)
    ax.axis("off")
    ax.set_title("Eigen 的列优先布局与 block/segment", fontsize=10)
    for r in range(4):
        for c in range(4):
            y = 3.0 - r
            color = "#e3f2fd" if (r < 3 and c < 3) else ("#fff3e0" if c == 3 else "#eeeeee")
            ax.add_patch(plt.Rectangle((c, y), 1.0, 1.0, facecolor=color,
                                       edgecolor="0.5", lw=1.0))
            ax.text(c + 0.5, y + 0.68, f"{T[r, c]:.2f}", ha="center", va="center",
                    fontsize=10)
            ax.text(c + 0.5, y + 0.22, f"[{r + 4 * c}]", ha="center", va="center",
                    fontsize=7.5, color="0.45")
    ax.text(0.0, -0.35,
            "方括号里是线性内存下标：先列后行，T 的第 (r,c) 个元素存在 r+4c。\n"
            "蓝：旋转块 R = block<3,3>(0,0)；橙：平移列 p = block<3,1>(0,3)；\n"
            "灰：底行 [0 0 0 1]。segment<3>(1) 取 v6 的第 2–4 个元素。",
            fontsize=8.6, va="top", color="0.25")

    ax = fig.add_subplot(1, 2, 2)
    ax.axis("off")
    ax.set_title("numpy 对照的输出（节选）", fontsize=10)
    lines = [
        "M6:",
        " 0.00 0.00 0.00 1.00 0.00 0.00",
        " 0.00 0.00 0.00 0.00 1.00 0.00",
        " 0.00 0.00 0.00 0.00 0.00 1.00",
        " 2.00 0.00 0.00 0.00 0.00 0.00",
        " 0.00 2.00 0.00 0.00 0.00 0.00",
        " 0.00 0.00 2.00 0.00 0.00 0.00",
        "v6:  0 10 20 30  0  0",
        "",
        f"R^T R - I: {np.max(np.abs(R.T @ R - np.eye(3))):.2e}",
        f"det R    : {np.linalg.det(R):.4f}",
        f"T T^-1 - I: {np.max(np.abs(T @ Ti - np.eye(4))):.2e}",
        f"解析逆 vs 通用逆: {np.max(np.abs(Ti - np.linalg.inv(T))):.2e}",
        f"100 组随机 T: {worst:.2e} / {worst_inv:.2e}",
    ]
    ax.text(0.0, 0.95, "\n".join(lines), family=MONO, fontsize=9.2,
            va="top", color="0.15")
    fig.suptitle("Eigen 程序的输出与 4×4 齐次矩阵在内存中的排布", fontsize=11)
    fig.tight_layout()
    savefig(fig, "4_7_eigen")


if __name__ == "__main__":
    main()
