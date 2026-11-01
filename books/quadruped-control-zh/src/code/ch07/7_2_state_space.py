"""18-state / 28-output state-space model (Section 7.2).

Builds the continuous matrices of Eqs. (7.8)-(7.14), discretises them with
forward Euler at 2 ms, checks the eigenvalue structure and the observability
rank, then compares a one-step Euler prediction with an exactly integrated
trajectory.  The rank deficiency is the point: absolute x/y position is not
observable from relative foot positions, relative velocities and foot heights.

expect: 状态维数 18
expect: 输出维数 28
expect: 可观测性矩阵
expect: 一步预测误差
expect: 开环 2 s 漂移
"""

from __future__ import annotations

import logging

import numpy as np

from quadruped import savefig

import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, Rectangle

plt.rcParams["mathtext.fontset"] = "dejavusans"
logging.getLogger("matplotlib").setLevel(logging.ERROR)

logging.getLogger("matplotlib").setLevel(logging.ERROR)

DT = 0.002
N = 18
M = 28
P = 3


# ANCHOR: build_matrices
def build_matrices(dt: float = DT) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    """Continuous A_c, B_c and C_c of Eqs. (7.11) and (7.14)."""
    Ac = np.zeros((N, N))
    Ac[0:3, 3:6] = np.eye(3)
    Bc = np.zeros((N, 3))
    Bc[3:6, :] = np.eye(3)
    Cc = np.zeros((M, N))
    eye = np.eye(3)
    for i in range(4):
        Cc[3 * i:3 * i + 3, 0:3] = -eye
        Cc[3 * i:3 * i + 3, 6 + 3 * i:9 + 3 * i] = eye
    for i in range(4):
        Cc[12 + 3 * i:15 + 3 * i, 3:6] = -eye
    for i in range(4):
        Cc[24 + i, 6 + 3 * i + 2] = 1.0
    return Ac, Bc, Cc
# ANCHOR_END: build_matrices



def observability(A: np.ndarray, C: np.ndarray) -> np.ndarray:
    blocks = [C]
    for _ in range(1, N):
        blocks.append(blocks[-1] @ A)
    return np.vstack(blocks)


# ANCHOR: check_rank
def check_rank(A: np.ndarray, C: np.ndarray) -> tuple[int, np.ndarray]:
    O = observability(A, C)
    u, s, vh = np.linalg.svd(O, full_matrices=True)
    tol = max(O.shape) * np.finfo(float).eps * s[0]
    rank = int(np.sum(s > tol))
    null = vh[rank:].copy()
    return rank, null
# ANCHOR_END: check_rank



# ANCHOR: trajectory
def trajectory(n: int = 1000) -> tuple[np.ndarray, np.ndarray]:
    """'True' state from exact constant-acceleration integration."""
    x = np.zeros((n + 1, N))
    x[0, 0:3] = [0.15, -0.05, 0.27]
    x[0, 3:6] = [0.10, 0.02, 0.00]
    feet = np.array([[0.183, -0.132, -0.021], [0.183, 0.132, -0.021],
                     [-0.183, -0.132, -0.021], [-0.183, 0.132, -0.021]])
    x[0, 6:18] = feet.reshape(12)
    u = np.tile(np.array([0.40, -0.20, 0.05]), (n + 1, 1))
    for k in range(n):
        # Exact solution for a constant input over one sample.
        x[k + 1, 0:3] = x[k, 0:3] + DT * x[k, 3:6] + 0.5 * DT ** 2 * u[k]
        x[k + 1, 3:6] = x[k, 3:6] + DT * u[k]
        x[k + 1, 6:18] = x[k, 6:18]
    return x, u
# ANCHOR_END: trajectory



def draw_gradient(axg) -> None:
    """Small gradient field used in Section 7.3 (right panel of Figure 7-3)."""
    Q = np.array([[2.0, 0.7], [0.7, 1.4]])
    c = np.array([-1.2, 0.4])
    gx, gy = np.meshgrid(np.linspace(-2.2, 2.2, 19),
                         np.linspace(-2.2, 2.2, 19))
    F = Q[0, 0] * gx ** 2 + 2 * Q[0, 1] * gx * gy + Q[1, 1] * gy ** 2 \
        + c[0] * gx + c[1] * gy
    gxv = 2 * Q[0, 0] * gx + 2 * Q[0, 1] * gy + c[0]
    gyv = 2 * Q[1, 1] * gy + 2 * Q[0, 1] * gx + c[1]
    xstar = -0.5 * np.linalg.solve(Q, c)
    cs = axg.contour(gx, gy, F, levels=10, cmap="viridis", linewidths=0.8)
    axg.quiver(gx, gy, gxv, gyv, color="C1", alpha=0.75)
    axg.plot(*xstar, "r*", ms=12, label="梯度为零")
    axg.set_aspect("equal")
    axg.set_xlabel("$x_1$")
    axg.set_ylabel("$x_2$")
    axg.set_title("二次型的梯度场")
    axg.legend(fontsize=7, loc="upper right")
    axg.grid(True, alpha=0.25)
    return cs


def draw_structure() -> None:
    fig = plt.figure(figsize=(12.4, 4.8))
    ax = fig.add_axes([0.01, 0.03, 0.70, 0.88])
    axg = fig.add_axes([0.76, 0.16, 0.22, 0.70])
    ax.set_xlim(0, 12)
    ax.set_ylim(0, 6.6)
    ax.axis("off")

    def box(x, y, w, h, text, fc="white", fs=9):
        ax.add_patch(Rectangle((x, y), w, h, fc=fc, ec="k", lw=1.0))
        ax.text(x + w / 2, y + h / 2, text, ha="center", va="center", fontsize=fs)

    def arrow(x0, y0, x1, y1, text="", color="k"):
        ax.add_patch(FancyArrowPatch((x0, y0), (x1, y1), arrowstyle="-|>",
                                     mutation_scale=12, lw=1.2, color=color))
        if text:
            ax.text((x0 + x1) / 2, (y0 + y1) / 2 + 0.12, text,
                    ha="center", fontsize=9, color=color)

    box(0.2, 4.9, 1.9, 0.9, "$\\mathbf{u}$  (3)\n$\\mathbf{R}_{sb}\\mathbf{a}_b+\\mathbf{g}$",
        fc="#e8f1fb", fs=8)
    box(2.9, 4.9, 1.1, 0.9, "$\\mathbf{B}_c$\n18×3", fc="#f7f7f7")
    box(4.6, 4.2, 2.0, 2.4, "$\\dot{\\mathbf{x}}=\\mathbf{A}_c\\mathbf{x}+\\mathbf{B}_c\\mathbf{u}$\n\n$\\mathbf{x}$ (18)", fc="#fdf3e3")
    box(7.4, 4.9, 1.1, 0.9, "$\\mathbf{C}_c$\n28×18", fc="#f7f7f7")
    box(9.2, 4.9, 2.4, 0.9, "$\\mathbf{y}$  (28)\n28 维测量", fc="#e8f1fb", fs=8)
    arrow(2.1, 5.35, 2.9, 5.35)
    arrow(4.0, 5.35, 4.6, 5.35)
    arrow(6.6, 5.35, 7.4, 5.35)
    arrow(8.5, 5.35, 9.2, 5.35)

    ax.text(2.3, 2.55, "注：足端位置导数为 0；\n绝对 x/y 方向不可观测，秩 16/18",
            fontsize=8.5, va="top")
    # State blocks.
    blocks = [("$\\mathbf{p}_b$", 3, "#f6c9c9"), ("$\\mathbf{v}_b$", 3, "#f7dfb5"),
              ("$\\mathbf{p}_{f0}$", 3, "#cfe3c8"), ("$\\mathbf{p}_{f1}$", 3, "#cfe3c8"),
              ("$\\mathbf{p}_{f2}$", 3, "#cfe3c8"), ("$\\mathbf{p}_{f3}$", 3, "#cfe3c8")]
    x0, y0, w = 2.3, 0.3, 1.45
    for i, (name, size, color) in enumerate(blocks):
        ax.add_patch(Rectangle((x0 + i * w, y0), w - 0.05, 0.85, fc=color, ec="k"))
        ax.text(x0 + i * w + (w - 0.05) / 2, y0 + 0.55, name, ha="center", fontsize=9)
        ax.text(x0 + i * w + (w - 0.05) / 2, y0 + 0.22, f"{size}", ha="center", fontsize=7)
    ax.annotate("状态 $\\mathbf{x}$：机身位置 + 机身速度 + 四个足端位置（世界系）",
                xy=(2.3, 0.15), xytext=(2.3, 0.05), fontsize=9, va="top")

    # Output groups.
    ax.text(9.2, 3.35, "输出分块：\n前 12：$\\mathbf{p}_i-\\mathbf{p}_b$\n中 12：$-\\mathbf{v}_b$\n后 4：$p_i(2)$",
            fontsize=9, va="top")
    ax.text(0.2, 2.4, "输入：\n世界系机身加速度", fontsize=9, va="top")
    ax.text(0.2, 1.2, "连续模型：\n$\\dot{\\mathbf{x}}=\\mathbf{A}_c\\mathbf{x}+\\mathbf{B}_c\\mathbf{u}$\n$\\mathbf{y}=\\mathbf{C}_c\\mathbf{x}$\n离散：$\\mathbf{A}=\\mathbf{I}+\\Delta t\\mathbf{A}_c$",
            fontsize=8.5, va="top")
    ax.set_title("18 状态 / 3 输入 / 28 输出的状态空间结构")
    cs = draw_gradient(axg)
    fig.colorbar(cs, ax=axg, shrink=0.85, label="$f(x)$")
    savefig(fig, "7_2_state_space")
    plt.close(fig)


def main() -> None:
    draw_structure()
    Ac, Bc, Cc = build_matrices()
    A = np.eye(N) + DT * Ac
    B = DT * Bc
    print("=== 7.2 状态空间模型 ===")
    print(f"状态维数 {N}，输入维数 {B.shape[1]}，输出维数 {M}")
    print(f"A_c {Ac.shape}，B_c {Bc.shape}，C_c {Cc.shape}")
    print(f"A {A.shape}，B {B.shape}，C {Cc.shape}，dt={DT:.3f} s")
    print("A_c 的特征值（全部为零，纯积分型）：")
    print(f"  {np.array2string(np.linalg.eigvals(Ac).real, precision=4)}")
    print("A 的特征值（全部为 1）：")
    print(f"  {np.array2string(np.linalg.eigvals(A).real, precision=6)}")

    rank, null = check_rank(A, Cc)
    print(f"可观测性矩阵 [{Cc.shape[0] * N}×{N}] 的秩 = {rank}，"
          f"缺失 {N - rank} 维")
    for v in null[:2]:
        v = v / np.max(np.abs(v))
        print(f"  零空间向量：p_b {np.array2string(v[0:3], precision=2)}，"
              f"p_f0 {np.array2string(v[6:9], precision=2)}")
    print("零空间解释：给机身位置和四个足端位置同时加同一个 x/y 平移，"
          "输出与状态方程都不变")

    x, u = trajectory()
    pred = np.zeros_like(x)
    pred[0] = x[0]
    for k in range(len(x) - 1):
        pred[k + 1] = A @ pred[k] + B @ u[k]
    err = pred - x
    one = (A @ x[:-1].T).T + (B @ u[:-1].T).T - x[1:]
    print("=== 一步预测（精确连续积分 vs 前向欧拉） ===")
    print(f"一步位置误差上界 {np.abs(one[:, 0:3]).max():.3e} m "
          f"（理论 0.5*a*dt^2）")
    print(f"一步速度误差 {np.abs(one[:, 3:6]).max():.3e} m/s")
    print(f"一步足端误差 {np.abs(one[:, 6:18]).max():.3e} m")
    print(f"1000 步（2 s）后位置开环漂移 "
          f"{np.array2string(pred[-1, 0:3] - x[-1, 0:3], precision=6)} m")
    print("误差来源：Euler 把每个周期的加速度当成常量，漏掉 0.5*a*dt^2 项")

if __name__ == "__main__":
    main()
