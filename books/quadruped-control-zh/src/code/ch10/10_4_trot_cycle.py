"""One 2 ms cycle inside a 5 s trot: order, budget and measured stages (10.4).

The controller is the shared ``TrottingController``.  Each of its stages is
wrapped with a perf_counter so the script can print the average time of
read/estimate/gait/QP/torque-map/IK/write per cycle.  The same run records one
gait period of phase, contact, foot forces and joint tracking, and draws the
2 ms timing bar above the three cycle panels (Fig. 10-4).

Truth from ``get_state`` is used only for metrics and plots, never by the
controller.

expect: 5 s trot
expect: 阶段平均耗时
expect: QP 成功
expect: 最大关节力矩
expect: 高度 RMS
expect: 图 10-4
"""

from __future__ import annotations

import logging
import os
import time

os.environ.setdefault("MESA_SHADER_CACHE_DISABLE", "true")

import numpy as np

from quadruped import (A1Params, A1Sim, ControlContext, Quadruped,
                       TrottingController)
from quadruped.plotting import label, savefig
import matplotlib.pyplot as plt

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

DT = 0.002
DURATION = 5.0
N = int(round(DURATION / DT))
VX = 0.30
LEGS = ["FR", "FL", "RR", "RL"]
COLORS = ["C0", "C1", "C2", "C3"]


class StageTimer:
    """Accumulate wall time of named stages; report per-cycle averages."""

    def __init__(self) -> None:
        self.total = {name: 0.0 for name in
                      ("read", "estimate", "gait", "qp", "torque", "ik", "write")}
        self.calls = {name: 0 for name in self.total}

    def wrap(self, name: str, fn):
        def timed(*args, **kwargs):
            t0 = time.perf_counter()
            out = fn(*args, **kwargs)
            self.total[name] += (time.perf_counter() - t0) * 1e3
            self.calls[name] += 1
            return out
        return timed


def draw(times: dict[str, float], t: np.ndarray, phase: np.ndarray,
         contact: np.ndarray, force: np.ndarray, q: np.ndarray,
         q_des: np.ndarray) -> None:
    fig = plt.figure(figsize=(11.0, 7.0))
    gs = fig.add_gridspec(2, 3, height_ratios=[1.05, 1.8], hspace=0.42,
                          wspace=0.28)
    ax0 = fig.add_subplot(gs[0, :])
    ax1 = fig.add_subplot(gs[1, 0])
    ax2 = fig.add_subplot(gs[1, 1])
    ax3 = fig.add_subplot(gs[1, 2])

    # 2 ms timing bar
    order = ["read", "estimate", "gait", "qp", "torque", "ik", "write", "other"]
    colors = ["#4c78a8", "#72b7b2", "#54a24b", "#e45756", "#f58518",
              "#b279a2", "#9d755d", "#d9d9d9"]
    left = 0.0
    for name, color in zip(order, colors):
        dur = float(times.get(name, 0.0))
        ax0.barh(0.0, dur, left=left, height=0.55, color=color,
                 edgecolor="white", lw=0.6)
        if dur > 0.035:
            ax0.text(left + dur / 2, 0.0, name, ha="center", va="center",
                     fontsize=7.2, rotation=0)
        left += dur
    ax0.axvline(2.0, color="k", ls="--", lw=1.0)
    ax0.text(2.0, 0.42, "2 ms 截止", fontsize=8, ha="right", va="bottom")
    ax0.set_xlim(0.0, 2.35)
    ax0.set_ylim(-0.55, 0.55)
    ax0.set_yticks([])
    ax0.set_xlabel(label("每周期的时间 / ms（阶段均值为本次 5 s 运行）", "time per cycle / ms"))
    ax0.set_title(label("一个 2 ms 周期的阶段划分", "one 2 ms cycle"), fontsize=10)
    ax0.grid(True, axis="x", alpha=0.3)
    lines = [f"read {times['read']:.3f}", f"estimate {times['estimate']:.3f}",
             f"gait {times['gait']:.3f}", f"QP {times['qp']:.3f}",
             f"J^T {times['torque']:.3f}", f"IK {times['ik']:.3f}",
             f"write {times['write']:.3f}", f"other {times['other']:.3f}"]
    ax0.text(1.08, 0.34, "ms\n".join(lines[:4]), fontsize=7.2, va="top",
             family="monospace")
    ax0.text(1.52, 0.34, "ms\n".join(lines[4:]), fontsize=7.2, va="top",
             family="monospace")

    # phases and contact
    for i, (leg, color) in enumerate(zip(LEGS, COLORS)):
        c = contact[:, i]
        ax1.plot(t[c], phase[c, i], color=color, lw=1.8, label=leg)
        ax1.plot(t[~c], phase[~c, i], color=color, lw=1.0, ls=":")
    ax1.set_ylim(-0.05, 1.08)
    ax1.set_xlabel(label("时间 / s", "time / s"))
    ax1.set_ylabel(label("相位（实线触地，点线摆动）", "phase"))
    ax1.set_title(label("四个足端的相位与接触", "phase and contact"), fontsize=10)
    ax1.legend(ncol=4, fontsize=7, loc="upper center")
    ax1.text(0.02, 0.03, "FR/RL、FL/RR 同相，线重合", fontsize=7,
             transform=ax1.transAxes, color="0.35")
    ax1.grid(True, alpha=0.3)

    # foot normal force (truth, for plotting only)
    for i, (leg, color) in enumerate(zip(LEGS, COLORS)):
        ax2.plot(t, force[:, i], color=color, lw=1.3, label=leg)
    ax2.set_xlabel(label("时间 / s", "time / s"))
    ax2.set_ylabel(label("法向力 / N", "normal force / N"))
    ax2.set_title(label("QP 给出的足端法向力", "commanded normal force"), fontsize=10)
    ax2.legend(ncol=4, fontsize=7, loc="upper center")
    ax2.grid(True, alpha=0.3)

    # joint tracking of the right-front leg
    names = ["髋", "大腿", "膝"]
    for j in range(3):
        ax3.plot(t, q[:, j], color=f"C{j}", lw=1.5,
                 label=f"{names[j]} 实际")
        ax3.plot(t, q_des[:, j], color=f"C{j}", lw=1.0, ls="--",
                 label=f"{names[j]} 目标")
    ax3.set_xlabel(label("时间 / s", "time / s"))
    ax3.set_ylabel(label("关节角 / rad", "joint angle / rad"))
    ax3.set_title(label("右前腿三个关节的实际与目标", "RF joint tracking"), fontsize=10)
    ax3.legend(ncol=2, fontsize=6.4, loc="best")
    ax3.grid(True, alpha=0.3)

    fig.suptitle(label("小跑的一个步态周期与 2 ms 时间预算", "trot cycle and 2 ms budget"),
                 fontsize=11.5)
    savefig(fig, "10_4_trot_cycle")
    plt.close(fig)


def main() -> None:
    params = A1Params.from_menagerie()
    robot = Quadruped(params)
    sim = A1Sim()
    context = ControlContext(sim=sim, params=params, robot=robot, dt=DT)
    state = TrottingController(context, robot=robot, params=params)
    timer = StageTimer()

    # ANCHOR: run
    state.enter()
    state.set_command(vx=VX, vy=0.0, vyaw=0.0, body_height=0.27)
    sim.read_sensors = timer.wrap("read", sim.read_sensors)
    state.estimator.step = timer.wrap("estimate", state.estimator.step)
    state.gait.update = timer.wrap("gait", state.gait.update)
    state.balance.solve_force = timer.wrap("qp", state.balance.solve_force)
    state.balance.to_joint_torque = timer.wrap("torque", state.balance.to_joint_torque)
    state._safe_ik = timer.wrap("ik", state._safe_ik)
    sim.set_joint_pd = timer.wrap("write", sim.set_joint_pd)

    log = {k: [] for k in ("t", "phase", "contact", "force", "q", "qd", "vx", "height", "rpy")}
    qp_ok = 0
    max_tau = 0.0
    run_time = 0.0
    for k in range(N):
        t0 = time.perf_counter()
        cmd = state.run()
        run_time += time.perf_counter() - t0
        sim.step()
        truth, joints = sim.get_state()
        log["t"].append(sim.data.time)
        log["phase"].append(state.gait.phase.copy())
        log["contact"].append(state.gait.contact.copy())
        log["force"].append(state.forces[:, 2].copy())
        log["q"].append(joints.q.copy())
        log["qd"].append(cmd.q_des.copy())
        log["vx"].append(truth.velocity[0])
        log["height"].append(truth.position[2])
        log["rpy"].append(truth.rpy.copy())
        if state.balance.last_status in (1, 2):
            qp_ok += 1
        max_tau = max(max_tau, float(np.max(np.abs(cmd.tau_ff))))

    # ANCHOR_END: run
    stage_mean = {name: timer.total[name] / N for name in timer.total}
    stage_mean["other"] = max(0.0, run_time / N * 1e3 - sum(stage_mean.values()))
    t = np.asarray(log["t"])
    height = np.asarray(log["height"])
    rpy = np.asarray(log["rpy"])
    vx = np.asarray(log["vx"])
    phase = np.asarray(log["phase"])
    contact = np.asarray(log["contact"])
    force = np.asarray(log["force"])
    q = np.asarray(log["q"])
    q_des = np.asarray(log["qd"])

    print("=== 10.4 5 s trot：运行流程与阶段耗时 ===")
    print(f"步态 {state.gait.gait.name}：周期 {state.gait.gait.period:.2f} s，"
          f"占空比 {state.gait.gait.duty:.2f}，步高 {state.step_height * 1e3:.0f} mm，"
          f"指令 vx = {VX:.2f} m/s")
    print("阶段平均耗时（本次运行的墙钟，仅代表本机）：")
    for name in ("read", "estimate", "gait", "qp", "torque", "ik", "write", "other"):
        print(f"  {name:<9s} {stage_mean[name]:7.4f} ms")
    print(f"  {'合计':<9s} {run_time / N * 1e3:7.4f} ms（预算 2.0000 ms）")
    print(f"QP 成功（status=1/2）：{qp_ok}/{N} = {100.0 * qp_ok / N:.2f}%")
    print(f"最大关节力矩 = {max_tau:.4f} N·m（限幅 ±33.5 N·m 的 {100 * max_tau / 33.5:.2f}%）")
    print(f"机身高度：均值 {height.mean():.4f} m，RMS {np.sqrt(np.mean(height ** 2)):.4f} m，"
          f"误差 RMS {np.sqrt(np.mean((height - 0.27) ** 2)) * 1e3:.2f} mm")
    print(f"姿态角 RMS：roll {np.sqrt(np.mean(rpy[:, 0] ** 2)) * 180 / np.pi:.3f}°，"
          f"pitch {np.sqrt(np.mean(rpy[:, 1] ** 2)) * 180 / np.pi:.3f}°")
    print(f"平均前进速度（真值，仅验收）= {vx.mean():.4f} m/s")
    # Pick one full gait period after 3 s for the lower panels.
    t0, t1 = 3.00, 3.00 + state.gait.gait.period
    sel = (t >= t0) & (t < t1)
    print(f"一个周期窗口：{t0:.2f}–{t1:.2f} s；相位、QP 法向力、关节角已画入图 10-4")
    draw(stage_mean, t[sel] - t0, phase[sel], contact[sel], force[sel],
         q[sel], q_des[sel])
    print("图 10-4 已保存：10_4_trot_cycle.png")
    sim.close()


if __name__ == "__main__":
    main()
