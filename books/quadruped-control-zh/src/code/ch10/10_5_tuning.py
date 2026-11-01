"""Mass/gain sweep and swing-leg x tracking (10.5).

Part 1 runs a 3x3x3 grid: balance position gain Kp, damping Kd and the mass
used by the QP.  The robot is the same A1 in every trial; only the controller's
model changes.  The cost is the normalised sum of height RMS error, attitude
RMS and peak joint torque.  It is a *relative* score for comparing settings,
not an absolute ride-quality metric.

Part 2 reproduces the idea of the original Fig. 10.1 with a second-order
equivalent of the swing leg: a low-bandwidth, overdamped response lags the
reference, while a high-bandwidth, underdamped one reaches the target near
touch-down and would overshoot afterwards.  That is the "use the overshoot"
argument, made quantitative.

expect: 参数扫描
expect: 最优三组
expect: 摆动腿 x 跟踪
expect: c1 在触地时刻
expect: c2 首次到达
expect: 图 10-5
expect: 图 10-6
"""

from __future__ import annotations

import logging
import os

os.environ.setdefault("MESA_SHADER_CACHE_DISABLE", "true")

import numpy as np

from quadruped import (A1Params, A1Sim, ControlContext, Quadruped,
                       TrottingController)
from quadruped.balance import BalanceCtrl
from quadruped.plotting import label, savefig
import matplotlib.pyplot as plt

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

DT = 0.002
VX = 0.20
DURATION = 1.5
SKIP = 0.4
KPS = [60.0, 100.0, 160.0]
KDS = [14.0, 20.0, 28.0]
MASSES = [10.5, 12.45, 14.5]


# ANCHOR: sweep
def trial(sim, context, robot, params, kp: float, kd: float,
          mass: float) -> tuple[float, float, float, float, float]:
    """One 1.5 s trot; metrics start after 0.4 s of settling."""
    sim.reset()
    balance = BalanceCtrl(
        mass=mass,
        inertia=params.body_equivalent_inertia(params.home_q),
    )
    balance.Kp = np.diag([kp, kp, 2.5 * kp])
    balance.Kd = np.diag([kd, kd, 2.0 * kd])
    state = TrottingController(context, robot=robot, params=params,
                               balance=balance)
    state.enter()
    state.set_command(vx=VX, vy=0.0, vyaw=0.0, body_height=0.27)
    hs, rolls, pitches, taus = [], [], [], []
    ok = 0
    n = int(round(DURATION / DT))
    for _ in range(n):
        cmd = state.run()
        sim.step()
        truth, _ = sim.get_state()
        if sim.data.time >= SKIP:
            hs.append(float(truth.position[2]))
            rolls.append(float(truth.rpy[0]))
            pitches.append(float(truth.rpy[1]))
            taus.append(float(np.max(np.abs(cmd.tau_ff))))
            ok += int(state.balance.last_status in (1, 2))
    hs = np.asarray(hs)
    rolls = np.asarray(rolls)
    pitches = np.asarray(pitches)
    if not np.all(np.isfinite(hs)) or hs.min() < 0.15 or np.max(np.abs(rolls)) > 0.6:
        return 1.0, 1.0, 99.0, 99.0, 0.0
    h_rmse = float(np.sqrt(np.mean((hs - 0.27) ** 2)))
    att_rms = float(np.sqrt(np.mean(rolls ** 2 + pitches ** 2)))
    tau_max = float(np.max(taus))
    cost = ((h_rmse / 0.02) ** 2 + (att_rms / 0.05) ** 2
            + (tau_max / 33.5) ** 2)
    return h_rmse, att_rms, tau_max, cost, float(hs.mean())


def sweep(sim, context, robot, params):
    rows = []
    for mass in MASSES:
        for kp in KPS:
            for kd in KDS:
                h, att, tau, cost, z = trial(sim, context, robot, params,
                                             kp, kd, mass)
                rows.append({"mass": mass, "kp": kp, "kd": kd, "h": h,
                             "att": att, "tau": tau, "cost": cost, "z": z})
                print(f"m={mass:5.2f} kg  Kp={kp:3.0f}  Kd={kd:2.0f}  "
                      f"高度 RMS={h * 1e3:5.2f} mm  姿态 RMS={att:.4f} rad  "
                      f"力矩峰值={tau:5.2f} N·m  代价={cost:.4f}")
    return rows
# ANCHOR_END: sweep


def draw_heatmap(rows) -> None:
    fig, axes = plt.subplots(1, 3, figsize=(11.0, 3.7), sharey=True)
    all_costs = np.array([r["cost"] for r in rows])
    vmin, vmax = float(all_costs.min()), float(all_costs.max())
    for ax, mass in zip(axes, MASSES):
        grid = np.zeros((len(KDS), len(KPS)))
        for r in rows:
            if abs(r["mass"] - mass) < 1e-9:
                j = KPS.index(r["kp"])
                i = KDS.index(r["kd"])
                grid[i, j] = r["cost"]
        im = ax.imshow(grid, origin="lower", cmap="viridis", vmin=vmin,
                       vmax=vmax, aspect="auto")
        for i in range(len(KDS)):
            for j in range(len(KPS)):
                ax.text(j, i, f"{grid[i, j]:.2f}", ha="center", va="center",
                        fontsize=8, color="white" if grid[i, j] < 0.55 * vmax else "black")
        best = np.unravel_index(np.argmin(grid), grid.shape)
        ax.plot(best[1], best[0], marker="*", ms=14, mfc="none", mec="white",
                mew=1.4)
        ax.set_xticks(range(len(KPS)))
        ax.set_xticklabels([f"{v:.0f}" for v in KPS])
        ax.set_yticks(range(len(KDS)))
        ax.set_yticklabels([f"{v:.0f}" for v in KDS])
        ax.set_xlabel(label("位置刚度 $K_p$", "Kp"))
        ax.set_title(label(f"模型质量 {mass:.2f} kg", f"mass {mass:.2f} kg"),
                     fontsize=10)
    axes[0].set_ylabel(label("位置阻尼 $K_d$", "Kd"))
    cb = fig.colorbar(im, ax=axes, fraction=0.025, pad=0.02)
    cb.set_label(label("归一化代价", "normalised cost"))
    fig.suptitle(label("Kp × Kd × 模型质量的扫描（白星为各质量下的最优）",
                       "Kp x Kd x mass sweep"), fontsize=11)
    savefig(fig, "10_6_tuning")
    plt.close(fig)


# ANCHOR: swing
def swing_response(wn: float, zeta: float, t1: float = 0.12,
                   x1: float = 0.08, t_end: float = 0.18,
                   dt: float = 1e-4):
    """Second-order tracking of a smooth 0 -> x1 reference, in metres."""
    n = int(round(t_end / dt))
    t = np.arange(n + 1) * dt
    x = np.zeros_like(t)
    v = 0.0
    for k in range(n):
        s = min(1.0, t[k] / t1)
        ref = x1 * (3.0 * s * s - 2.0 * s ** 3)
        a = wn * wn * (ref - x[k]) - 2.0 * zeta * wn * v
        v += a * dt
        x[k + 1] = x[k] + v * dt
    return t, x


def draw_swing() -> None:
    t1, x1 = 0.12, 0.08
    t, x1_curve = swing_response(30.0, 1.30, t1, x1)
    _, x2_curve = swing_response(45.0, 0.35, t1, x1)
    ref = np.where(t <= t1, x1 * (3 * (t / t1) ** 2 - 2 * (t / t1) ** 3), x1)
    i1 = int(round(t1 / (t[-1] / (len(t) - 1))))
    cross = np.where(x2_curve >= x1)[0]
    tc = t[cross[0]] if cross.size else float("nan")
    peak = float(np.max(x2_curve))

    fig, ax = plt.subplots(figsize=(7.0, 3.9))
    ax.axvspan(t1, t[-1], color="0.92", zorder=0)
    ax.plot(t * 1e3, ref * 1e3, "k--", lw=1.1, label="参考 $x_d$")
    ax.plot(t * 1e3, x1_curve * 1e3, "C0", lw=1.8,
            label=r"$c_1$：低刚度、大阻尼")
    ax.plot(t * 1e3, x2_curve * 1e3, "C3", lw=1.8,
            label=r"$c_2$：高刚度、小阻尼")
    ax.axhline(x1 * 1e3, color="0.55", ls=":", lw=0.9)
    ax.axvline(t1 * 1e3, color="0.3", ls="--", lw=0.9)
    ax.plot([t1 * 1e3], [x1 * 1e3], "ko", ms=4)
    ax.text(t1 * 1e3 + 2, x1 * 1e3 + 3, "触地 $t_1$", fontsize=8)
    ax.text(2, x1 * 1e3 + 3, "$x_1$", fontsize=8)
    ax.text(t1 * 1e3 + 2, 101, "触地后只作对比", fontsize=7.2, color="0.4",
            va="top")
    ax.set_xlim(0, t[-1] * 1e3)
    ax.set_ylim(0, 105)
    ax.set_xlabel(label("时间 / ms", "time / ms"))
    ax.set_ylabel(label("足端 $x$ 坐标 / mm", "foot x / mm"))
    ax.set_title(label("摆动腿 $x$ 方向的跟踪：利用超调在触地时到点",
                       "swing-leg x tracking"), fontsize=10)
    ax.legend(fontsize=7.5, loc="lower right")
    ax.grid(True, alpha=0.3)
    savefig(fig, "10_5_swing")
    plt.close(fig)
    return float(x1_curve[i1]), float(tc), peak
# ANCHOR_END: swing


def main() -> None:
    params = A1Params.from_menagerie()
    robot = Quadruped(params)
    sim = A1Sim()
    context = ControlContext(sim=sim, params=params, robot=robot, dt=DT)

    print("=== 10.5 参数扫描：Kp × Kd × 模型质量 ===")
    print(f"每组：从 home 起步，随即指令 vx={VX:.1f} m/s，跑 {DURATION:.1f} s，"
          f"前 {SKIP:.1f} s 不计入指标")
    print("代价 = (高度 RMS/0.02 m)^2 + (姿态 RMS/0.05 rad)^2 + (力矩峰值/33.5 N·m)^2")
    rows = sweep(sim, context, robot, params)
    ranked = sorted(rows, key=lambda r: r["cost"])
    print("\n最优三组：")
    for k, r in enumerate(ranked[:3], start=1):
        print(f"  {k}. 质量 {r['mass']:.2f} kg，Kp={r['kp']:.0f}，Kd={r['kd']:.0f}；"
              f"高度 RMS {r['h'] * 1e3:.2f} mm，姿态 RMS {r['att']:.4f} rad，"
              f"力矩峰值 {r['tau']:.2f} N·m，代价 {r['cost']:.4f}")
    draw_heatmap(rows)

    print("\n=== 10.5 摆动腿 x 跟踪（二阶等效模型） ===")
    xt1, tc, peak = draw_swing()
    print(f"参考：0 → 80 mm，触地时刻 t1 = 120 ms")
    print(f"c1 在触地时刻到达 {xt1 * 1e3:.1f} mm（目标的 {100 * xt1 / 0.08:.1f}%），"
          f"明显落后")
    print(f"c2 首次到达 80 mm 的时间 = {tc * 1e3:.1f} ms，峰值 {peak * 1e3:.1f} mm"
          f"（超调 {100 * (peak - 0.08) / 0.08:.1f}%）")
    print("图 10-5 已保存：10_5_swing.png；图 10-6 已保存：10_6_tuning.png")
    sim.close()


if __name__ == "__main__":
    main()
