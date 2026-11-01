"""Full 18-state estimator during a 10 s trot (Section 7.9).

The controller, estimator and simulator form the same loop as on hardware:
``read_sensors`` -> ``TrottingController`` (which calls ``Estimator.step``) ->
joint command.  MuJoCo's base position/velocity are recorded only for scoring
and plotting, never fed back.  The horizontal error is measured after removing
the arbitrary initial origin of the estimator, because absolute x/y is not
observable.

expect: 10 s 小跑
expect: 位置 RMSE
expect: 速度 RMSE
expect: 最大误差
expect: 协方差
"""

from __future__ import annotations

import logging

import numpy as np

from quadruped import (A1Params, A1Sim, ControlContext, Estimator, FSM,
                       FixedStand, JointCmd, Quadruped, StateID,
                       TrottingController, savefig)

import matplotlib.pyplot as plt

plt.rcParams["mathtext.fontset"] = "dejavusans"
logging.getLogger("matplotlib").setLevel(logging.ERROR)

DT = 0.002
N_TROT = 5000          # 10 s


# ANCHOR: boot
def boot():
    params = A1Params.from_menagerie()
    robot = Quadruped(params)
    sim = A1Sim()
    q0 = params.home_q.copy()
    cmd = JointCmd(q0, np.zeros(12), np.array([100.0, 300.0, 400.0] * 4),
                   np.array([4.0, 8.0, 8.0] * 4), np.zeros(12))
    for _ in range(1000):
        sim.step(cmd)
    ctx = ControlContext.from_sim(sim, params)
    states = [FixedStand(ctx),
              TrottingController(ctx, robot=robot, params=params)]
    fsm = FSM(ctx, states=states, initial=StateID.FIXED_STAND)
    for _ in range(700):
        fsm.step()
        sim.step()
    fsm.request(StateID.TROTTING)
    fsm.step()
    sim.step()
    trot = fsm.states[StateID.TROTTING]
    trot.set_command(vx=0.25)
    return params, robot, sim, fsm, trot
# ANCHOR_END: boot



# ANCHOR: main
def main() -> None:
    params, robot, sim, fsm, trot = boot()
    est = trot.estimator
    truth_p, truth_v, est_p, est_v = [], [], [], []
    p_sd, v_sd, contacts, ptrace = [], [], [], []
    for _ in range(N_TROT):
        fsm.step()
        base = sim.step()
        e = trot.est_base
        truth_p.append(base.position.copy())
        truth_v.append(base.velocity.copy())
        est_p.append(e.position.copy())
        est_v.append(e.velocity.copy())
        p_sd.append(np.sqrt(np.diag(est.P[0:3, 0:3])).copy())
        v_sd.append(np.sqrt(np.diag(est.P[3:6, 3:6])).copy())
        contacts.append(sim.foot_contacts().copy())
        ptrace.append(np.trace(est.P))
    sim.close()

    truth_p = np.asarray(truth_p)
    truth_v = np.asarray(truth_v)
    est_p = np.asarray(est_p)
    est_v = np.asarray(est_v)
    p_sd = np.asarray(p_sd)
    v_sd = np.asarray(v_sd)
    contacts = np.asarray(contacts)
    t = np.arange(N_TROT) * DT

    # The estimator declares its start point as the origin.  Remove the same
    # constant from the truth so the comparison measures drift, not the
    # arbitrary choice of world origin.
    truth_p_aligned = truth_p - truth_p[0] + est_p[0]
    ep = est_p - truth_p_aligned
    ev = est_v - truth_v
    rmse_p = np.sqrt(np.mean(ep ** 2, axis=0))
    rmse_v = np.sqrt(np.mean(ev ** 2, axis=0))
    max_p = np.abs(ep).max(axis=0)
    print("=== 7.9 状态估计器：10 s 小跑（vx=0.25 m/s） ===")
    print(f"控制周期 {DT * 1000:.0f} ms，记录 {N_TROT} 个周期")
    print("位置 RMSE（m）：")
    print(f"  x {rmse_p[0]:.4f}，y {rmse_p[1]:.4f}，z {rmse_p[2]:.4f}，"
          f"水平 {np.sqrt(np.mean(ep[:, :2] ** 2)):.4f}")
    print("速度 RMSE（m/s）：")
    print(f"  x {rmse_v[0]:.4f}，y {rmse_v[1]:.4f}，z {rmse_v[2]:.4f}，"
          f"三轴 {np.sqrt(np.mean(ev ** 2)):.4f}")
    print("最大误差（m）：")
    print(f"  x {max_p[0]:.4f}，y {max_p[1]:.4f}，z {max_p[2]:.4f}")
    print(f"末时刻水平漂移（估计 - 对齐真值） "
          f"{np.array2string(ep[-1, :2], precision=4)} m")
    print(f"平均高度误差 {np.mean(ep[:, 2]):+.4f} m，"
          f"高度 RMSE {rmse_p[2]:.4f} m")

    swing = ~contacts.all(axis=1)
    print("足端腾空处理（按 MuJoCo 实际接触）：")
    print(f"  至少一条腿腾空的周期 {swing.sum()} / {N_TROT} "
          f"（{100 * swing.mean():.1f}%），四足同时触地 "
          f"{100 * (~swing).mean():.1f}%")
    print(f"  腾空周期的 P 迹最大 {np.max(np.asarray(ptrace)[swing]):.3e}，"
          f"触地周期最大 {np.max(np.asarray(ptrace)[~swing]):.3e}")
    print(f"  全过程 P 的最大元素 {np.max(np.abs(est.P)):.3e}，"
          f"对称性 {np.abs(est.P - est.P.T).max():.3e}")
    print("每周期运算：组装 28 维 y，解 28×28 线性方程一次，更新 18 维状态；"
          "不显式求逆")
    print(f"R 的配置对角项：位置 {est.R0[0]:.1e}，速度 {est.R0[12]:.1e}，"
          f"高度 {est.R0[24]:.1e}")

    # Score only the steady part for the covariance coverage statement.
    k0 = 1000
    print("稳态段误差落在 ±2σ 内的比例：")
    for i, name in enumerate("xyz"):
        cov_ratio = np.mean(np.abs(ep[k0:, i]) <= 2 * p_sd[k0:, i])
        print(f"  {name}: {100 * cov_ratio:.1f}%")
    print("x 方向为不可观测的绝对位置，P 已收缩但漂移不受它约束，"
          "所以覆盖率不能当作精度承诺")

    fig = plt.figure(figsize=(9.2, 6.0))
    gs = fig.add_gridspec(2, 2, height_ratios=[1.0, 1.0])
    ax0 = fig.add_subplot(gs[0, 0])
    for i, name in enumerate(("x", "y", "z")):
        ax0.plot(t, truth_p_aligned[:, i] - truth_p_aligned[0, i],
                 color=f"C{i}", lw=1.0, ls="--")
        ax0.plot(t, est_p[:, i] - est_p[0, i], color=f"C{i}", lw=1.0,
                 label=f"${name}$")
    ax0.set_xlabel("时间 (s)")
    ax0.set_ylabel("相对位置 (m)")
    ax0.set_title("位置：实线估计，虚线真值")
    ax0.legend(fontsize=8)
    ax0.grid(True, alpha=0.3)

    ax1 = fig.add_subplot(gs[0, 1])
    for i, name in enumerate(("x", "y", "z")):
        ax1.plot(t, truth_v[:, i], color=f"C{i}", lw=1.0, ls="--")
        ax1.plot(t, est_v[:, i], color=f"C{i}", lw=1.0, label=f"$v_{name}$")
    ax1.set_xlabel("时间 (s)")
    ax1.set_ylabel("速度 (m/s)")
    ax1.set_title("速度：实线估计，虚线真值")
    ax1.legend(fontsize=8, ncol=3)
    ax1.grid(True, alpha=0.3)

    ax2 = fig.add_subplot(gs[1, 0])
    ax2.plot(t, ep[:, 0], lw=0.9, label="$e_x$")
    ax2.fill_between(t, -2 * p_sd[:, 0], 2 * p_sd[:, 0], color="C0",
                     alpha=0.18, label="±2σ (x)")
    ax2.plot(t, ep[:, 2], lw=0.9, color="C2", label="$e_z$")
    ax2.set_xlabel("时间 (s)")
    ax2.set_ylabel("误差 (m)")
    ax2.set_title("位置误差与滤波器 2σ 带")
    ax2.legend(fontsize=8)
    ax2.grid(True, alpha=0.3)

    ax3 = fig.add_subplot(gs[1, 1])
    Rimg = np.log10(np.diag(est.R_nominal) + 1e-8)
    im = ax3.imshow(Rimg, cmap="magma", aspect="equal")
    ax3.set_title("估计器使用的 $\\log_{10}R$")
    ax3.set_xlabel("输出通道")
    ax3.set_ylabel("输出通道")
    fig.colorbar(im, ax=ax3, shrink=0.85)
    fig.tight_layout()
    savefig(fig, "7_9_estimator")
    plt.close(fig)
# ANCHOR_END: main



if __name__ == "__main__":
    main()
