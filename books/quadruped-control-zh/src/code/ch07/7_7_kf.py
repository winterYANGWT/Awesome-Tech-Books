"""Discrete Kalman filter on a one-dimensional constant-velocity model (7.7).

The state is [position, velocity].  Only position is measured.  The filter is
compared with the raw measurement and with a first-order low-pass filter; the
figure also draws the predict-update cycle used by the estimator.

expect: 卡尔曼滤波
expect: 位置 RMSE
expect: 稳态增益
expect: 协方差
"""

from __future__ import annotations

import logging

import numpy as np

from quadruped import savefig

import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, Rectangle

plt.rcParams["mathtext.fontset"] = "dejavusans"
logging.getLogger("matplotlib").setLevel(logging.ERROR)
RNG = np.random.default_rng(707)

DT = 0.01
T_END = 20.0
N = int(T_END / DT)
SIGMA_A = 0.5          # unmodelled acceleration, m/s^2
SIGMA_Y = 0.30         # position measurement noise, m


def model() -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    A = np.array([[1.0, DT], [0.0, 1.0]])
    H = np.array([[1.0, 0.0]])
    Q = SIGMA_A ** 2 * np.array([[DT ** 4 / 4, DT ** 3 / 2],
                                 [DT ** 3 / 2, DT ** 2]])
    R = np.array([[SIGMA_Y ** 2]])
    return A, H, Q, R


def simulate() -> tuple[np.ndarray, np.ndarray]:
    A, _, _, _ = model()
    x = np.array([0.0, 1.0])
    truth = np.zeros((N + 1, 2))
    truth[0] = x
    for k in range(N):
        x = A @ x + np.array([0.5 * DT ** 2, DT]) * RNG.normal(0, SIGMA_A)
        truth[k + 1] = x
    y = truth[1:, 0] + RNG.normal(0, SIGMA_Y, N)
    return truth[1:], y


# ANCHOR: run_kf
def run_kf(y: np.ndarray) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    A, H, Q, R = model()
    x = np.array([0.0, 0.0])
    P = np.diag([1.0, 1.0])
    xs = np.zeros((N, 2))
    Ps = np.zeros((N, 2, 2))
    gains = np.zeros((N, 2))
    for k in range(N):
        # time update
        x = A @ x
        P = A @ P @ A.T + Q
        # measurement update
        S = H @ P @ H.T + R
        K = P @ H.T @ np.linalg.inv(S)
        x = x + (K @ (np.array([y[k]]) - H @ x)).ravel()
        IKH = np.eye(2) - K @ H
        P = IKH @ P @ IKH.T + K @ R @ K.T
        P = 0.5 * (P + P.T)
        xs[k] = x
        Ps[k] = P
        gains[k] = K.ravel()
    return xs, Ps, gains
# ANCHOR_END: run_kf



def lowpass(y: np.ndarray, alpha: float = 0.12) -> np.ndarray:
    out = np.zeros_like(y)
    out[0] = y[0]
    for k in range(1, len(y)):
        out[k] = alpha * y[k] + (1.0 - alpha) * out[k - 1]
    return out


def draw_cycle(ax) -> None:
    ax.set_xlim(0, 12)
    ax.set_ylim(0, 9)
    ax.axis("off")

    def box(x, y, w, h, text, fc="white", fs=7.6):
        ax.add_patch(Rectangle((x, y), w, h, fc=fc, ec="k", lw=1.0))
        ax.text(x + w / 2, y + h / 2, text, ha="center", va="center",
                fontsize=fs, linespacing=1.35)

    box(0.2, 6.9, 2.5, 1.3, "初始化\n$\\hat{x}_0^+,\\ P_0^+$", "#e8f1fb")
    box(3.9, 6.9, 3.7, 1.3,
        "时间更新\n$\\hat{x}^-=A\\hat{x}^++Bu$\n$P^-=AP^+A^T+Q$", "#fdf3e3")
    box(8.25, 6.9, 3.55, 1.3,
        "算增益\n$K=P^-H^T S^{-1}$\n$S=HP^-H^T+R$", "#f7f7f7")
    box(8.25, 3.7, 3.55, 1.3,
        "读测量 $y_k$\n$\\hat{x}^+=\\hat{x}^-$\n$+K(y-H\\hat{x}^-)$", "#e8f1fb")
    box(3.9, 3.7, 3.7, 1.3,
        "协方差更新\nJoseph 形式\n$P^+$ 对称半正定", "#fdf3e3")
    box(0.2, 3.7, 2.5, 1.3, "输出\n位置 / 速度", "#e8f1fb")

    def arrow(p0, p1):
        ax.add_patch(FancyArrowPatch(p0, p1, arrowstyle="-|>", mutation_scale=12,
                                     lw=1.2))

    arrow((2.7, 7.55), (3.9, 7.55))
    arrow((7.6, 7.55), (8.25, 7.55))
    arrow((10.0, 6.9), (10.0, 5.0))
    arrow((8.25, 4.35), (7.6, 4.35))
    arrow((3.9, 4.35), (2.7, 4.35))
    ax.text(6.0, 1.5, "下一周期：只保留 $\\hat{x}^+,P^+$，回到时间更新",
            ha="center", fontsize=8)
    ax.set_title("预测—更新循环")


def main() -> None:
    truth, y = simulate()
    xs, Ps, gains = run_kf(y)
    lp = lowpass(y)
    err_meas = y - truth[:, 0]
    err_kf = xs[:, 0] - truth[:, 0]
    err_lp = lp - truth[:, 0]
    print("=== 7.7 离散卡尔曼滤波器（一维匀速模型） ===")
    print(f"采样周期 {DT:.3f} s，样本数 {N}，"
          f"测量噪声 σ_y={SIGMA_Y:.3f} m，加速度噪声 σ_a={SIGMA_A:.3f} m/s^2")
    print("位置 RMSE：")
    print(f"  直接取测量      {np.sqrt(np.mean(err_meas ** 2)):.4f} m")
    print(f"  一阶低通 alpha=0.12 {np.sqrt(np.mean(err_lp ** 2)):.4f} m")
    print(f"  卡尔曼滤波      {np.sqrt(np.mean(err_kf ** 2)):.4f} m")
    print(f"速度 RMSE（卡尔曼） {np.sqrt(np.mean((xs[:, 1] - truth[:, 1]) ** 2)):.4f} m/s")
    print(f"稳态增益 K_p = {gains[-1, 0]:.4f}，K_v = {gains[-1, 1]:.4f}")
    print("末时刻协方差：")
    print(f"  P = {np.array2string(Ps[-1], precision=6)}")
    print(f"  位置 2σ = {2 * np.sqrt(Ps[-1, 0, 0]):.4f} m")
    inside = np.mean(np.abs(err_kf) <= 2 * np.sqrt(Ps[:, 0, 0]))
    print(f"  落在 ±2σ 带内的样本比例 {inside * 100:.1f}%")

    fig, ax = plt.subplots(1, 2, figsize=(10.2, 4.2),
                           gridspec_kw={"width_ratios": [1.15, 1.4]})
    draw_cycle(ax[0])
    t = np.arange(1, N + 1) * DT
    ax[1].plot(t, truth[:, 0], color="k", lw=1.2, label="真值")
    ax[1].plot(t, y, ".", ms=2, color="0.6", label="测量")
    ax[1].plot(t, xs[:, 0], color="C0", lw=1.2, label="KF")
    sd = np.sqrt(Ps[:, 0, 0])
    ax[1].fill_between(t, xs[:, 0] - 2 * sd, xs[:, 0] + 2 * sd,
                       color="C0", alpha=0.18, label="±2σ")
    ax[1].set_xlabel("时间 (s)")
    ax[1].set_ylabel("位置 (m)")
    ax[1].set_title("真值、测量与滤波估计")
    ax[1].legend(fontsize=8, ncol=2)
    ax[1].grid(True, alpha=0.3)
    fig.tight_layout()
    savefig(fig, "7_7_kf")
    plt.close(fig)


if __name__ == "__main__":
    main()
