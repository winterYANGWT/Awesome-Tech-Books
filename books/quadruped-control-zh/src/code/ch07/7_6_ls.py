"""Batch weighted least squares versus recursive least squares (Section 7.6).

The same noisy measurements are processed twice.  The batch solution builds
the normal equations; the recursive solution keeps only ``x`` and ``P`` and
updates them with the new information.  Their final estimates and covariances
agree apart from the finite initial covariance and round-off.

expect: 批量加权最小二乘
expect: 递推最小二乘
expect: 最终估计差
expect: 最终协方差差
"""

from __future__ import annotations

import logging

import numpy as np

from quadruped import savefig

import matplotlib.pyplot as plt

plt.rcParams["mathtext.fontset"] = "dejavusans"
logging.getLogger("matplotlib").setLevel(logging.ERROR)
RNG = np.random.default_rng(706)

X_TRUE = np.array([1.25, -0.60])
H = np.array([[1.0, 0.0], [0.0, 1.0], [1.0, 1.0]])
R = np.diag([0.04, 0.09, 0.16])
N = 200


def batch_wls(Y: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    Rinv = np.linalg.inv(R)
    A = H.T @ Rinv @ H * len(Y)
    b = H.T @ Rinv @ Y.sum(axis=0)
    P = np.linalg.inv(A)
    return P @ b, P


# ANCHOR: recursive_ls
def recursive_ls(Y: np.ndarray) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    x = np.zeros(2)
    P = np.eye(2) * 1e6
    hist = []
    for y in Y:
        S = H @ P @ H.T + R
        K = np.linalg.solve(S, H @ P).T
        x = x + K @ (y - H @ x)
        IKH = np.eye(2) - K @ H
        P = IKH @ P @ IKH.T + K @ R @ K.T
        P = 0.5 * (P + P.T)
        hist.append(x.copy())
    return x, P, np.asarray(hist)
# ANCHOR_END: recursive_ls



# ANCHOR: main
def main() -> None:
    Y = (H @ X_TRUE)[None, :] + RNG.multivariate_normal(
        np.zeros(3), R, size=N)
    x_batch, P_batch = batch_wls(Y)
    x_ols = np.linalg.lstsq(H, Y.mean(axis=0), rcond=None)[0]
    print("=== 7.6 最小二乘：批量与递推 ===")
    print(f"真值 x = {np.array2string(X_TRUE, precision=4)}")
    print(f"普通最小二乘（只用均值） x_OLS = "
          f"{np.array2string(x_ols, precision=4)}")
    print("批量加权最小二乘：")
    print(f"  x_WLS = {np.array2string(x_batch, precision=4)}")
    print(f"  P_WLS = {np.array2string(P_batch, precision=3)}")

    x_rls, P_rls, hist = recursive_ls(Y)
    print("递推最小二乘（P0=1e6 I）：")
    print(f"  x_RLS = {np.array2string(x_rls, precision=4)}")
    print(f"  P_RLS = {np.array2string(P_rls, precision=3)}")
    print("最终估计差：")
    print(f"  ||x_RLS - x_WLS||_inf = {np.abs(x_rls - x_batch).max():.3e}")
    print("最终协方差差：")
    print(f"  ||P_RLS - P_WLS||_inf = {np.abs(P_rls - P_batch).max():.3e}")
    print("收敛过程（样本数 n 时的估计）：")
    for n in (1, 5, 20, 50, 200):
        print(f"  n={n:3d}: {np.array2string(hist[n - 1], precision=4)}")
    print(f"  批量:    {np.array2string(x_batch, precision=4)}")
    print("新息：前 5 个样本的 ||y-Hx_old|| =")
    print("  " + " ".join(
        f"{np.linalg.norm(Y[k] - H @ (hist[k - 1] if k else np.zeros(2))):.3f}"
        for k in range(5)))

    fig, ax = plt.subplots(1, 2, figsize=(8.6, 3.5))
    ax[0].plot(np.arange(1, N + 1), hist[:, 0], lw=1.2, label="$x_1$ RLS")
    ax[0].plot(np.arange(1, N + 1), hist[:, 1], lw=1.2, label="$x_2$ RLS")
    ax[0].axhline(X_TRUE[0], color="C0", ls="--", lw=0.8)
    ax[0].axhline(X_TRUE[1], color="C1", ls="--", lw=0.8)
    ax[0].set_xlabel("样本数 $n$")
    ax[0].set_ylabel("估计值")
    ax[0].set_title("递推最小二乘的收敛")
    ax[0].legend(fontsize=8)
    ax[0].grid(True, alpha=0.3)

    sig = np.sqrt(np.array([P_batch[0, 0], P_batch[1, 1]]))
    ax[1].plot(np.arange(1, N + 1), np.abs(hist[:, 0] - x_batch[0]), lw=1.0,
               label="$|x_1-x_{WLS}|$")
    ax[1].plot(np.arange(1, N + 1), np.abs(hist[:, 1] - x_batch[1]), lw=1.0,
               label="$|x_2-x_{WLS}|$")
    ax[1].set_yscale("log")
    ax[1].set_xlabel("样本数 $n$")
    ax[1].set_ylabel("与批量解的差")
    ax[1].set_title(f"批量 1σ：{sig[0]:.4f}, {sig[1]:.4f}")
    ax[1].legend(fontsize=8)
    ax[1].grid(True, alpha=0.3, which="both")
    fig.tight_layout()
    savefig(fig, "7_6_ls")
    plt.close(fig)
# ANCHOR_END: main



if __name__ == "__main__":
    main()
