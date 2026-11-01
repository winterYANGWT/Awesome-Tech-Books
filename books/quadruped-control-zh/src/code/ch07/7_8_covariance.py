"""Measure R and C_u recursively, and draw the stance window (Section 7.8).

The A1 stands still for 10 s.  The ideal 28-dimensional output y and input u
are computed from MuJoCo at every 2 ms cycle; known white noise is then added,
so the recursive statistics can be compared with the injected covariance.
This mirrors the hardware procedure (stand still, skip the start-up transient,
read the terminal), but the simulator itself has no sensor noise, hence the
explicit injection.  The recursive formulas use the book's 1/n convention;
the package's ``AvgCov`` uses 1/(n-1), which converges to the same value.

expect: 静止 10 s
expect: 递推 R
expect: 递推 C_u
expect: 条件数
expect: 窗口函数
"""

from __future__ import annotations

import logging

import numpy as np

from quadruped import A1Sim, AvgCov, Estimator, JointCmd, savefig
from quadruped.rotation import rpy_to_R

import matplotlib.pyplot as plt

plt.rcParams["mathtext.fontset"] = "dejavusans"
logging.getLogger("matplotlib").setLevel(logging.ERROR)

DT = 0.002
NS = 5000
SEED = 708
RNG = np.random.default_rng(SEED)
SIG_POS = 2.0e-4
SIG_VEL = 5.0e-3
SIG_H = 1.0e-3
SIG_ACC = 0.08


# ANCHOR: RecursiveCov
class RecursiveCov:
    """Mean/covariance with the 1/n convention of Eqs. (7.71) and (7.76)."""

    def __init__(self, dim: int) -> None:
        self.dim = int(dim)
        self.count = 0
        self.mean = np.zeros(dim)
        self.cov = np.zeros((dim, dim))

    def update(self, x: np.ndarray) -> None:
        x = np.asarray(x, dtype=float).reshape(self.dim)
        self.count += 1
        n = self.count
        old = self.mean.copy()
        if n == 1:
            self.mean[:] = x
            return
        self.mean += (x - old) / n
        self.cov = ((n - 1) / n) * self.cov \
            + ((n - 1) / n ** 2) * np.outer(x - old, x - self.mean)
# ANCHOR_END: RecursiveCov



# ANCHOR: main
def main() -> None:
    sim = A1Sim()
    from quadruped import A1Params, Quadruped
    p = A1Params.from_menagerie()
    robot = Quadruped(p)
    est = Estimator(robot=robot, dt=DT)
    q0 = sim.home_q.copy()
    cmd = JointCmd(q0, np.zeros(12), np.array([100.0, 300.0, 400.0] * 4),
                   np.array([4.0, 8.0, 8.0] * 4), np.zeros(12))
    for _ in range(1500):
        sim.step(cmd)

    rc_R = RecursiveCov(28)
    rc_u = RecursiveCov(3)
    rc_noise = RecursiveCov(3)
    hist_R = []
    hist_u = []
    for k in range(NS):
        sim.step(cmd)
        sens = sim.read_sensors()
        base = sens.base_state()
        y, _ = est.measurement(base, sens.joints, sens.contact)
        noise = np.concatenate([
            RNG.normal(0, SIG_POS, 12),
            RNG.normal(0, SIG_VEL, 12),
            RNG.normal(0, SIG_H, 4)])
        y_noisy = y + noise
        acc_noisy = base.acc_b + RNG.normal(0, SIG_ACC, 3)
        R = rpy_to_R(base.rpy)
        u = R @ acc_noisy + np.array([0.0, 0.0, -9.81])
        u_ideal = R @ base.acc_b + np.array([0.0, 0.0, -9.81])
        rc_R.update(y_noisy)
        rc_u.update(u)
        rc_noise.update(u - u_ideal)
        if (k + 1) in (10, 50, 100, 500, 1000, 2500, 5000):
            hist_R.append((k + 1, rc_R.cov.copy()))
            hist_u.append((k + 1, rc_u.cov.copy()))
    sim.close()

    injected = np.concatenate([np.full(12, SIG_POS ** 2),
                               np.full(12, SIG_VEL ** 2),
                               np.full(4, SIG_H ** 2)])
    diag = np.diag(rc_R.cov)
    print("=== 7.8 协方差测量（A1 静止 10 s，5000 个样本） ===")
    print("注入的测量噪声标准差：")
    print(f"  足端位置 {SIG_POS:.1e} m，足端速度 {SIG_VEL:.1e} m/s，"
          f"高度 {SIG_H:.1e} m")
    print("递推 R 的对角块均值（估计 / 注入）：")
    for name, sl, sig in (("位置", slice(0, 12), SIG_POS),
                          ("速度", slice(12, 24), SIG_VEL),
                          ("高度", slice(24, 28), SIG_H)):
        print(f"  {name}: {diag[sl].mean():.3e} / {sig ** 2:.3e}")
    print(f"递推 R 的最大对角项 {diag.max():.3e}，"
          f"最小对角项 {diag.min():.3e}")
    print(f"R 的条件数 cond(R) = {np.linalg.cond(rc_R.cov):.3e}")
    print(f"R 的对称性误差 ||R-R^T||_inf = "
          f"{np.abs(rc_R.cov - rc_R.cov.T).max():.3e}")

    print("递推 C_u（输入 u 的三轴方差）：")
    print(f"  diag = {np.array2string(np.diag(rc_u.cov), precision=2)}")
    print(f"  注入加速度计方差 {SIG_ACC ** 2:.3e} m^2/s^4")
    print(f"  C_u 条件数 {np.linalg.cond(rc_u.cov):.3e}")
    print(f"  其中纯加速度计噪声的方差均值 "
          f"{np.diag(rc_noise.cov).mean():.3e} m^2/s^4（注入 {SIG_ACC ** 2:.3e}）")
    print("  总输入方差略大，因为静止站立仍有机身微动，真实实验也会包含这一项")

    # Cross-check with the package's unbiased AvgCov on the last 1000 samples.
    ac = AvgCov(3, "Cu", print_min=False)
    for v in RNG.normal(0, SIG_ACC, (1000, 3)):
        ac.update(v)
    print(f"对照：AvgCov(1000 个样本) 对角均值 "
          f"{np.diag(ac.cov).mean():.3e}，1/n 版本应略小")

    fig, ax = plt.subplots(1, 2, figsize=(8.8, 3.5))
    ns = np.array([h[0] for h in hist_R])
    ax[0].semilogx(ns, [np.diag(h[1])[0] for h in hist_R], "o-",
                   label="R 位置 (0,0)")
    ax[0].semilogx(ns, [np.diag(h[1])[12] for h in hist_R], "s-",
                   label="R 速度 (0,0)")
    ax[0].semilogx(ns, [np.diag(h[1])[24] for h in hist_R], "^-",
                   label="R 高度 (0)")
    for yv, c in ((SIG_POS ** 2, "C0"), (SIG_VEL ** 2, "C1"), (SIG_H ** 2, "C2")):
        ax[0].axhline(yv, color=c, ls="--", lw=0.7)
    ax[0].set_yscale("log")
    ax[0].set_xlabel("样本数 $n$")
    ax[0].set_ylabel("对角项")
    ax[0].set_title("递推 R 的收敛")
    ax[0].legend(fontsize=7)
    ax[0].grid(True, alpha=0.3, which="both")

    nu = np.array([h[0] for h in hist_u])
    ax[1].semilogx(nu, [np.diag(h[1])[2] for h in hist_u], "o-",
                   label="C_u (z,z)")
    ax[1].axhline(SIG_ACC ** 2, color="C0", ls="--", lw=0.7,
                  label="注入值")
    ax[1].set_xlabel("样本数 $n$")
    ax[1].set_ylabel("方差 (m/s$^2$)$^2$")
    ax[1].set_title("递推 C_u 的收敛")
    ax[1].legend(fontsize=8)
    ax[1].grid(True, alpha=0.3, which="both")
    fig.tight_layout()
    savefig(fig, "7_8_covariance")
    plt.close(fig)

    # Stance window of Eq. (7.86) and the covariance factor of Eq. (7.87).
    x = np.linspace(0.0, 1.0, 501)
    wr = 0.18
    sigma = np.where(x < wr, x / wr,
                     np.where(x <= 1 - wr, 1.0, (1.0 - x) / wr))
    L = 1.0e4
    factor = 1.0 + (1.0 - sigma) * L
    print("窗口函数：")
    print(f"  过渡区宽度 w_r={wr:.2f}，平台区 sigma=1，"
          f"两端 sigma=0 时协方差放大 {1 + L:.0f} 倍")
    print(f"  sigma(x) 在 x=0, {wr:.2f}, 0.5, {1 - wr:.2f}, 1 处的值："
          + ", ".join(f"{s:.2f}" for s in
                      np.interp([0, wr, 0.5, 1 - wr, 1], x, sigma)))

    fig, ax = plt.subplots(1, 2, figsize=(8.4, 3.3))
    ax[0].plot(x, sigma, lw=1.8, color="C0")
    ax[0].axvline(wr, color="k", ls=":", lw=0.8)
    ax[0].axvline(1 - wr, color="k", ls=":", lw=0.8)
    ax[0].text(wr, 0.08, "$w_r$", fontsize=10)
    ax[0].text(1 - wr - 0.06, 0.08, "$1-w_r$", fontsize=10)
    ax[0].set_xlabel("归一化触地相位 $x$")
    ax[0].set_ylabel("$\\sigma(x)$")
    ax[0].set_title("梯形窗口：中段可信")
    ax[0].set_ylim(-0.02, 1.08)
    ax[0].grid(True, alpha=0.3)

    ax[1].semilogy(x, factor, lw=1.8, color="C1")
    ax[1].set_xlabel("归一化触地相位 $x$")
    ax[1].set_ylabel("$C_{stance}/C_{init}$")
    ax[1].set_title("协方差放大倍数（$L=10^4$）")
    ax[1].grid(True, alpha=0.3, which="both")
    fig.tight_layout()
    savefig(fig, "7_9_window")
    plt.close(fig)

    print("说明：R 的完整矩阵在实机上按同样步骤读取；"
          "仿真注入噪声只是为了让统计量可核对")
# ANCHOR_END: main



if __name__ == "__main__":
    main()
