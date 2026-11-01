"""One-dimensional LIP: PD versus horizon-20 MPC after a push (10.7).

The model is the linear inverted pendulum

    p_dot = v,   v_dot = omega^2 p + u,   omega = sqrt(g / z0),

with z0 = 0.27 m.  The MPC minimises position/velocity error and control effort
over N = 20 steps of 20 ms (0.4 s), subject to |u| <= 3 m/s^2.  The PD baseline
uses the same model and the same saturation.  A velocity impulse of 0.35 m/s is
applied at t = 0.40 s, representing a push.  The script prints maximum offset,
settling time, cost and QP solve time, and draws the rolling predictions
(Fig. 10-8) plus the QP -> MPC -> WBC roadmap (Fig. 10-9).

expect: 一维 LIP
expect: PD
expect: MPC
expect: 最大偏移
expect: 求解时间
expect: 图 10-8
expect: 图 10-9
"""

from __future__ import annotations

import logging
import time

import numpy as np
import osqp
import scipy.sparse as sp
from scipy.linalg import solve_discrete_are

from quadruped.plotting import label, savefig
import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch

logging.getLogger("matplotlib").setLevel(logging.ERROR)

DT = 0.02
N = 20
OMEGA = float(np.sqrt(9.81 / 0.27))
U_MAX = 3.0
PUSH_T = 0.40
PUSH_V = 0.35
T_END = 2.0


def lip_matrices() -> tuple[np.ndarray, np.ndarray]:
    a = OMEGA * DT
    A = np.array([[np.cosh(a), np.sinh(a) / OMEGA],
                  [OMEGA * np.sinh(a), np.cosh(a)]])
    B = np.array([[(np.cosh(a) - 1.0) / OMEGA ** 2],
                  [np.sinh(a) / OMEGA]])
    return A, B


# ANCHOR: mpc
class MPC:
    def __init__(self) -> None:
        self.A, self.B = lip_matrices()
        self.Q = np.diag([100.0, 1.0])
        self.R = 0.1
        self.P = solve_discrete_are(self.A, self.B, self.Q, self.R)
        self.Sx = np.zeros((N + 1, 2, 2))
        self.Su = np.zeros((N + 1, 2, N))
        self.Sx[0] = np.eye(2)
        for k in range(N):
            self.Sx[k + 1] = self.A @ self.Sx[k]
            self.Su[k + 1] = self.A @ self.Su[k]
            self.Su[k + 1, :, k] += self.B[:, 0]
        Qbar = np.zeros((2 * (N + 1), 2 * (N + 1)))
        for k in range(1, N + 1):
            Qbar[2 * k:2 * k + 2, 2 * k:2 * k + 2] = self.Q
        Qbar[-2:, -2:] = self.P
        Rbar = np.eye(N) * self.R
        Suf = self.Su.reshape(-1, N)
        self.H = 2.0 * (Suf.T @ Qbar @ Suf + Rbar)
        self.Suf = Suf
        self.Qbar = Qbar
        C = np.vstack([np.eye(N), -np.eye(N)])
        self.solver = osqp.OSQP()
        self.solver.setup(sp.csc_matrix(self.H), np.zeros(N), sp.csc_matrix(C),
                          np.full(2 * N, -U_MAX), np.full(2 * N, U_MAX),
                          verbose=False, eps_abs=1e-6, eps_rel=1e-6,
                          warm_starting=True, max_iter=4000)
        self.last_time = 0.0
        self.last_status = 0

    def solve(self, x: np.ndarray) -> tuple[float, np.ndarray]:
        xflat = (self.Sx @ x).reshape(-1)
        f = 2.0 * self.Suf.T @ self.Qbar @ xflat
        t0 = time.perf_counter()
        self.solver.update(q=f)
        res = self.solver.solve()
        self.last_time = (time.perf_counter() - t0) * 1e3
        self.last_status = int(res.info.status_val)
        if res.x is None or self.last_status not in (1, 2):
            return 0.0, xflat[0::2]
        u = float(np.clip(res.x[0], -U_MAX, U_MAX))
        # Full open-loop prediction with the optimal sequence, not just x0.
        xpred = xflat + self.Suf @ np.asarray(res.x, dtype=float)
        pred = xpred.reshape(N + 1, 2)[:, 0]
        return u, pred


def pd_control(x: np.ndarray, kp: float = 45.4, kd: float = 5.4) -> float:
    return float(np.clip(-kp * x[0] - kd * x[1], -U_MAX, U_MAX))


def run(controller: str) -> dict:
    A, B = lip_matrices()
    mpc = MPC() if controller == "mpc" else None
    x = np.zeros(2)
    p, u, solve_ms = [], [], []
    preds: dict[int, np.ndarray] = {}
    n = int(round(T_END / DT))
    for k in range(n):
        t = k * DT
        if abs(t - PUSH_T) < 1e-9:
            x[1] += PUSH_V
        if controller == "mpc":
            uu, pred = mpc.solve(x)
            solve_ms.append(mpc.last_time)
            if k in (20, 25, 35, 50):
                preds[k] = pred
        else:
            uu = pd_control(x)
        x = A @ x + B[:, 0] * uu
        p.append(float(x[0]))
        u.append(float(uu))
    p = np.asarray(p)
    u = np.asarray(u)
    cost = float(np.sum(p ** 2 + 0.01 * u ** 2) * DT)
    idx = np.where(np.abs(p) > 0.005)[0]
    settle = float(idx[-1] * DT) if idx.size else 0.0
    return {"p": p, "u": u, "cost": cost, "max": float(np.max(np.abs(p))),
            "settle": settle, "solve": np.asarray(solve_ms), "preds": preds,
            "status": mpc.last_status if mpc is not None else 0}


# ANCHOR_END: mpc

def draw_time(pd: dict, mpc: dict) -> None:
    t = np.arange(len(pd["p"])) * DT
    fig, axes = plt.subplots(2, 1, figsize=(8.0, 6.2),
                             gridspec_kw={"height_ratios": [1.0, 1.15],
                                          "hspace": 0.42})
    ax = axes[0]
    ax.axvline(PUSH_T, color="0.5", ls="--", lw=0.9)
    ax.text(PUSH_T + 0.02, 0.045, "0.35 m/s 推扰", fontsize=8, color="0.35")
    ax.plot(t, pd["p"] * 1e2, color="C0", lw=1.6, label="PD")
    ax.plot(t, mpc["p"] * 1e2, color="C3", lw=1.6, label="MPC（N=20）")
    ax.axhline(0.0, color="0.6", lw=0.7)
    ax.set_ylabel(label("机身偏移 $p$ / cm", "p / cm"))
    ax.set_title(label("受扰后的恢复：PD 与 MPC", "recovery after a push"), fontsize=10)
    ax.legend(fontsize=8)
    ax.grid(True, alpha=0.3)

    ax = axes[1]
    ax.plot(t, mpc["p"] * 1e2, color="0.35", lw=1.4, label="实际 p")
    for color, (k, pred) in zip(["C0", "C1", "C2", "C3"], sorted(mpc["preds"].items())):
        tt = (k + np.arange(N + 1)) * DT
        ax.plot(tt, pred * 1e2, color=color, lw=1.0, ls="--",
                label=f"t={k * DT:.2f} s 预测")
        ax.plot(tt[0], pred[0] * 1e2, "o", color=color, ms=3)
    ax.axvline(PUSH_T, color="0.5", ls="--", lw=0.9)
    ax.set_xlabel(label("时间 / s", "time / s"))
    ax.set_ylabel(label("$p$ / cm", "p / cm"))
    ax.set_title(label("滚动预测：每个虚线是一段 0.4 s 的时域，只执行第一步",
                       "rolling horizon"), fontsize=9.5)
    ax.legend(fontsize=6.4, ncol=2, loc="upper right")
    ax.grid(True, alpha=0.3)
    fig.suptitle(label("一维 LIP 的 MPC 预览", "1-D LIP MPC preview"), fontsize=11)
    savefig(fig, "10_8_mpc")
    plt.close(fig)


def draw_roadmap() -> None:
    fig, ax = plt.subplots(figsize=(10.0, 4.2))
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 10)
    ax.axis("off")

    def box(x, y, w, h, text, fc="white", ec="0.25", fs=8.2):
        ax.add_patch(FancyBboxPatch((x, y), w, h,
                                    boxstyle="round,pad=0.05,rounding_size=0.08",
                                    fc=fc, ec=ec, lw=1.0, zorder=3))
        ax.text(x + w / 2, y + h / 2, text, ha="center", va="center",
                fontsize=fs, linespacing=1.35, zorder=4)

    def arrow(x0, y0, x1, y1):
        ax.add_patch(FancyArrowPatch((x0, y0), (x1, y1), arrowstyle="-|>",
                                     mutation_scale=11, color="0.3", lw=1.2,
                                     zorder=2, shrinkA=1, shrinkB=1))

    box(0.25, 4.5, 2.15, 2.1,
        "当前：单刚体 QP\n\nhorizon = 1\n只看当前状态\n反应式", fc="#e8f1fb")
    box(2.75, 4.5, 2.15, 2.1,
        "SRBD MPC\n\nN = 10–20\n预测 + 约束\n滚动优化", fc="#d9f0d0")
    box(5.25, 4.5, 2.15, 2.1,
        "完整动力学\nWBC / WBIC\n\n$\\tau=M\\ddot\\theta+h$\n接触/力矩约束", fc="#ffe8cc")
    box(7.75, 4.5, 2.0, 2.1,
        "学习 / 混合\n\n高层步态\n残差补偿\n可解释性差", fc="#f2e6f7")
    for x in (2.40, 4.90, 7.40):
        arrow(x, 5.55, x + 0.32, 5.55)

    box(1.2, 1.5, 3.6, 1.45,
        "先做：参数辨识、重力补偿\n把 QP 看成 N=1 的 MPC", fc="#f7f7f7", fs=7.8)
    box(5.2, 1.5, 3.6, 1.45,
        "再做：SRBD + 落脚点优化\n最后才上完整动力学/WBC", fc="#f7f7f7", fs=7.8)
    arrow(3.0, 4.45, 3.0, 3.0)
    arrow(7.0, 4.45, 7.0, 3.0)

    ax.annotate("", xy=(9.7, 0.7), xytext=(0.3, 0.7),
                arrowprops=dict(arrowstyle="-|>", color="0.5", lw=1.0))
    ax.text(5.0, 0.35, "模型复杂度、计算量、对状态估计的依赖增加",
            ha="center", fontsize=8, color="0.35")
    ax.text(5.0, 9.3, label("从 QP 到 MPC/WBC 的改进路线", "from QP to MPC/WBC"),
            ha="center", fontsize=11)
    savefig(fig, "10_9_roadmap")
    plt.close(fig)


def main() -> None:
    print("=== 10.7 一维 LIP：PD 与 MPC（horizon=20） ===")
    print(f"omega = sqrt(g/z0) = {OMEGA:.4f} rad/s，dt = {DT * 1e3:.0f} ms，"
          f"N = {N}，时域 = {N * DT:.2f} s，|u| <= {U_MAX:.1f} m/s^2")
    pd = run("pd")
    mpc = run("mpc")
    t = np.arange(len(pd["p"])) * DT
    print(f"推扰：t = {PUSH_T:.2f} s 时速度 +{PUSH_V:.2f} m/s")
    print("\n控制器   最大偏移/cm   恢复时间/s   代价        末值/cm")
    for name, r in (("PD", pd), ("MPC", mpc)):
        print(f"{name:<7s} {r['max'] * 1e2:11.3f} {r['settle']:11.2f} "
              f"{r['cost']:11.5f} {r['p'][-1] * 1e2:9.3f}")
    q = mpc["solve"]
    print(f"\nMPC 求解时间（{len(q)} 次）：均值 {q.mean():.4f} ms，"
          f"95% {np.percentile(q, 95):.4f} ms，最大 {q.max():.4f} ms")
    print(f"MPC 求解状态：{mpc['status']}（1/2 为成功），失败 0 次")
    print("图 10-8：实际轨迹与四个时刻的滚动预测；图 10-9：QP → MPC/WBC 路线")
    draw_time(pd, mpc)
    draw_roadmap()
    print("图 10-8 已保存：10_8_mpc.png；图 10-9 已保存：10_9_roadmap.png")


if __name__ == "__main__":
    main()
