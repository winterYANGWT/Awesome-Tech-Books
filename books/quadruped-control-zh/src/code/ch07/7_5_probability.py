"""Expectation, covariance, independence and conditional expectation (7.5).

Three samples are compared:

* correlated Gaussian: nonzero off-diagonal covariance and covariance
  ellipses along the principal axes;
* independent uniform variables: off-diagonal sample covariance near zero;
* y = x^2 with x uniform on [-1, 1]: covariance is zero, but the variables are
  clearly dependent, so zero covariance does not imply independence.

The conditional mean of a joint Gaussian is also evaluated, to connect the
conditioning notation of Section 7.5.5 with the Kalman update.

expect: 相关高斯样本
expect: 独立均匀样本
expect: 协方差为零但不独立
expect: 条件期望
"""

from __future__ import annotations

import logging

import numpy as np

from quadruped import savefig

import matplotlib.pyplot as plt

plt.rcParams["mathtext.fontset"] = "dejavusans"
logging.getLogger("matplotlib").setLevel(logging.ERROR)
RNG = np.random.default_rng(705)


def sample_cov(x: np.ndarray, y: np.ndarray) -> np.ndarray:
    X = np.column_stack([x, y])
    Xc = X - X.mean(axis=0)
    return Xc.T @ Xc / len(x)


def ellipse(mu: np.ndarray, cov: np.ndarray, n_std: float, n: int = 200):
    vals, vecs = np.linalg.eigh(cov)
    order = vals.argsort()[::-1]
    vals, vecs = vals[order], vecs[:, order]
    t = np.linspace(0.0, 2 * np.pi, n)
    circle = np.vstack([np.cos(t), np.sin(t)])
    return mu[:, None] + vecs @ (n_std * np.sqrt(vals)[:, None] * circle)


# ANCHOR: main
def main() -> None:
    n = 4000
    rho = 0.75
    cov_true = np.array([[1.0, 0.45], [0.45, 0.6]])
    L = np.linalg.cholesky(cov_true)
    z = RNG.normal(size=(n, 2))
    gauss = z @ L.T + np.array([0.5, -0.4])
    Cg = sample_cov(gauss[:, 0], gauss[:, 1])
    print("=== 7.5 概率与协方差 ===")
    print("相关高斯样本（4000 个）：")
    print(f"  样本协方差\n{np.array2string(Cg, precision=4)}")
    print(f"  相关系数 {Cg[0, 1] / np.sqrt(Cg[0, 0] * Cg[1, 1]):.4f}")
    print(f"  特征值 {np.array2string(np.linalg.eigvalsh(Cg), precision=4)}")

    xu = RNG.uniform(-1.0, 1.0, n)
    yu = RNG.uniform(-1.0, 1.0, n)
    Ci = sample_cov(xu, yu)
    print("独立均匀样本（4000 个）：")
    print(f"  样本协方差\n{np.array2string(Ci, precision=4)}")
    print(f"  非对角项 {Ci[0, 1]:+.5f}（理论 0）")

    xq = RNG.uniform(-1.0, 1.0, n)
    yq = xq ** 2 + 0.02 * RNG.normal(size=n)
    Cq = sample_cov(xq, yq)
    print("协方差为零但不独立的反例：x~U(-1,1)，y=x^2+小噪声")
    print(f"  样本协方差\n{np.array2string(Cq, precision=4)}")
    print(f"  Cov(x,y) = {Cq[0, 1]:+.5f}，接近 0")
    Cxx = sample_cov(xq ** 2, xq ** 2)
    print(f"  Corr(x^2, y) = "
          f"{sample_cov(xq ** 2, yq)[0, 1] / np.sqrt(Cxx[0, 0] * Cq[1, 1]):.4f}，"
          "说明 y 由 x 的平方决定")
    # Conditional mean of the fitted Gaussian, E[y|x].
    Cxy = Cg[0, 1] / Cg[0, 0]
    mean = gauss.mean(axis=0)
    x0 = 1.2
    cond_mean = mean[1] + Cxy * (x0 - mean[0])
    print("条件期望（用样本矩近似联合高斯）：")
    print(f"  E[y | x={x0:.1f}] = {cond_mean:.4f}")
    print(f"  直接用样本中 x∈[{x0 - 0.1:.1f},{x0 + 0.1:.1f}] 的 y 均值 = "
          f"{gauss[np.abs(gauss[:, 0] - x0) < 0.1, 1].mean():.4f}")

    fig, ax = plt.subplots(1, 2, figsize=(8.6, 3.8))
    ax[0].scatter(gauss[:, 0], gauss[:, 1], s=6, alpha=0.35, color="C0")
    for k, ls in ((1, "-"), (2, "--")):
        e = ellipse(Cg.mean(axis=1), Cg, k)
        ax[0].plot(e[0], e[1], color="C1", ls=ls, lw=1.4,
                   label=f"{k}$\\sigma$")
    ax[0].set_aspect("equal")
    ax[0].set_xlabel("$x_1$")
    ax[0].set_ylabel("$x_2$")
    ax[0].set_title("相关高斯与 1σ/2σ 协方差椭圆")
    ax[0].legend(fontsize=8)
    ax[0].grid(True, alpha=0.3)

    ax[1].scatter(xq, yq, s=6, alpha=0.35, color="C2")
    ax[1].set_xlabel("$x$")
    ax[1].set_ylabel("$y=x^2$")
    ax[1].set_title("协方差为零，但明显不独立")
    ax[1].grid(True, alpha=0.3)
    fig.tight_layout()
    savefig(fig, "7_5_probability")
    plt.close(fig)
# ANCHOR_END: main


if __name__ == "__main__":
    main()
