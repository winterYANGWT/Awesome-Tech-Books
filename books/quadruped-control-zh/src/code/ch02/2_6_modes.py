"""Six control modes on the same joint model (section 2.6).

Every mode is the same law tau = tau_ff + kp (q_des - q) + kd (dq_des - dq)
with a different choice of the five parameters.  Each run lasts 1.5 s and
reports a steady-state error, a peak torque and an overshoot.

expect: 位置模式
expect: 混合模式
expect: 六种模式共用式
"""

from __future__ import annotations

import logging

import numpy as np

from quadruped.plotting import savefig
import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch, Rectangle

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

J = 0.05
B = 0.10
LOAD = 0.20
TAU_MAX = 33.5
DT = 1e-4
T_END = 1.5
DECIMATE = 10


def simulate(kp: float, kd: float, q_des: float, dq_des: float,
             tau_ff: float, q0: float = 0.0, dq0: float = 0.0,
             load: float = LOAD):
    steps = int(round(T_END / DT))
    t = np.arange(steps + 1) * DT
    q = np.zeros(steps + 1)
    dq = np.zeros(steps + 1)
    tau = np.zeros(steps + 1)
    q[0], dq[0] = q0, dq0
    for i in range(steps):
        u = tau_ff + kp * (q_des - q[i]) + kd * (dq_des - dq[i])
        u = float(np.clip(u, -TAU_MAX, TAU_MAX))
        tau[i] = u
        acc = (u - B * dq[i] - load) / J
        dq[i + 1] = dq[i] + acc * DT
        q[i + 1] = q[i] + dq[i + 1] * DT
    tau[-1] = tau[-2]
    return t, q, dq, tau


def draw_relation() -> None:
    fig, ax = plt.subplots(figsize=(9.6, 5.4))
    ax.set_xlim(0, 12)
    ax.set_ylim(0, 8)
    ax.axis("off")
    ax.add_patch(Rectangle((0.4, 6.1), 2.6, 0.18, facecolor="#90a4ae",
                           edgecolor="0.25"))
    ax.add_patch(Rectangle((2.9, 5.75), 0.16, 0.88, facecolor="#cfd8dc",
                           edgecolor="0.25"))
    ax.text(1.55, 6.55, "转轴（转子侧）", ha="center", fontsize=9)
    ax.text(1.55, 5.55, "命令 Pos/W/T 作用在这里", ha="center", fontsize=8,
            color="#c62828")

    ax.add_patch(FancyBboxPatch((3.5, 5.55), 2.8, 1.25,
                                boxstyle="round,pad=0.05", facecolor="#f3e5f5",
                                edgecolor="0.25"))
    ax.text(4.9, 6.45, "行星减速器", ha="center", fontsize=9)
    ax.text(4.9, 5.95, "9.1 : 1", ha="center", fontsize=12)
    ax.add_patch(FancyArrowPatch((2.6, 6.2), (3.5, 6.2), arrowstyle="-|>",
                                 mutation_scale=9, lw=1.1))
    ax.add_patch(FancyArrowPatch((6.3, 6.2), (7.5, 6.2), arrowstyle="-|>",
                                 mutation_scale=9, lw=1.1))
    ax.add_patch(Rectangle((7.5, 5.85), 2.9, 0.70, facecolor="#e8f5e9",
                           edgecolor="0.25"))
    ax.text(8.95, 6.55, "输出轴（关节侧）", ha="center", fontsize=9)
    ax.text(8.95, 5.55, "读出后除以 9.1", ha="center", fontsize=8, color="#1565c0")
    ax.text(5.0, 7.00, "转子 9.1 圈 = 输出 1 圈", ha="center", fontsize=8,
            color="0.3")

    modes = [
        ("位置", r"$k_p>0,\ k_d>0$\n$\tau_{\rm ff}=0,\ \omega_{\rm des}=0$", "#e3f2fd"),
        ("速度", r"$k_p=0,\ k_d>0$\n$\tau_{\rm ff}=0$", "#e8f5e9"),
        ("阻尼", r"$k_p=0,\ \omega_{\rm des}=0$\n$\tau=-k_d\omega$", "#fff8e1"),
        ("力矩", r"$k_p=k_d=0$\n$\tau=\tau_{\rm ff}$", "#fce4ec"),
        ("零力矩", r"$\tau_{\rm ff}=k_p=k_d=0$\n$\tau=0$", "#eceff1"),
        ("混合", r"五个量同时有效\n$\tau=\tau_{\rm ff}+\tau_p+\tau_d$", "#ede7f6"),
    ]
    for i, (name, text, color) in enumerate(modes):
        col, row = i % 3, i // 3
        x, y = 0.35 + col * 3.95, 3.55 - row * 2.35
        ax.add_patch(FancyBboxPatch((x, y), 3.55, 1.65,
                                    boxstyle="round,pad=0.05", facecolor=color,
                                    edgecolor="0.25"))
        ax.text(x + 0.15, y + 1.38, name, fontsize=10)
        for j, line in enumerate(text.split("\\n")):
            ax.text(x + 1.75, y + 1.02 - 0.42 * j, line, ha="center", va="center",
                    fontsize=8.5)
    ax.text(0.35, 0.35, "六种模式都是式（2-1）的特例；参数范围是协议范围，不是安全范围。",
            fontsize=8.5, color="0.3")
    ax.set_title("转轴、减速器与输出轴；六个小方块列出各模式保留的项")
    savefig(fig, "2_6_modes_relation")


def main() -> None:
    cases = [
        ("位置", dict(kp=20.0, kd=1.0, q_des=1.0, dq_des=0.0, tau_ff=0.0),
         0.0, 0.0, "q", LOAD),
        ("速度", dict(kp=0.0, kd=1.5, q_des=0.0, dq_des=2.0, tau_ff=0.0),
         0.0, 0.0, "dq", LOAD),
        ("阻尼", dict(kp=0.0, kd=0.5, q_des=0.0, dq_des=0.0, tau_ff=0.0),
         0.0, 4.0, "damp", 0.0),
        ("力矩", dict(kp=0.0, kd=0.0, q_des=0.0, dq_des=0.0, tau_ff=1.0),
         0.0, 0.0, "none", LOAD),
        ("零力矩", dict(kp=0.0, kd=0.0, q_des=0.0, dq_des=0.0, tau_ff=0.0),
         0.0, 4.0, "zero", 0.0),
        ("混合", dict(kp=20.0, kd=1.0, q_des=1.0, dq_des=0.0, tau_ff=0.2),
         0.0, 0.0, "q", LOAD),
    ]
    results = []
    for name, gains, q0, dq0, kind, load in cases:
        t, q, dq, tau = simulate(q0=q0, dq0=dq0, load=load, **gains)
        q_end = float(np.mean(q[-1000:]))
        dq_end = float(np.mean(dq[-1000:]))
        peak = float(np.max(np.abs(tau)))
        if kind == "q":
            err = abs(gains["q_des"] - q_end)
            over = max(0.0, (float(np.max(q)) - gains["q_des"]) * 100.0)
            unit = "rad"
        elif kind == "dq":
            err = abs(gains["dq_des"] - dq_end)
            over = max(0.0, (float(np.max(dq)) - gains["dq_des"]) * 100.0)
            unit = "rad/s"
        elif kind == "damp":
            err = abs(dq_end)
            over = None
            unit = "rad/s"
        else:
            err = None
            over = None
            unit = ""
        results.append((name, t, q, dq, tau, err, over, peak, unit, dq_end))

        if err is None:
            print(f"{name}模式: 稳态误差 —, 最大力矩 {peak:5.2f} N·m, 超调 —, "
                  f"末速 {dq_end:7.3f} rad/s")
        elif over is None:
            print(f"{name}模式: 稳态误差 {err:7.4f} {unit}, 最大力矩 {peak:5.2f} N·m, "
                  f"超调 —")
        else:
            print(f"{name}模式: 稳态误差 {err:7.4f} {unit}, 最大力矩 {peak:5.2f} N·m, "
                  f"超调 {over:5.1f}%")

    # 2x3 comparison: columns are q, dq, tau; rows are two groups of modes.
    fig, axes = plt.subplots(2, 3, figsize=(11.0, 6.2), sharex=True)
    groups = [results[:3], results[3:]]
    for row, group in enumerate(groups):
        for name, t, q, dq, tau, *_ in group:
            axes[row, 0].plot(t, q, lw=1.3, label=name)
            axes[row, 1].plot(t, dq, lw=1.3, label=name)
            axes[row, 2].plot(t, tau, lw=1.3, label=name)
    for row in range(2):
        axes[row, 0].set_ylabel("位置 q (rad)")
        axes[row, 1].set_ylabel("速度 dq (rad/s)")
        axes[row, 2].set_ylabel("力矩 τ (N·m)")
        for col in range(3):
            axes[row, col].grid(True, alpha=0.3)
            axes[row, col].legend(fontsize=7, loc="best")
    for col, xlabel in enumerate(("时间 (s)", "时间 (s)", "时间 (s)")):
        axes[1, col].set_xlabel(xlabel)
    fig.suptitle("六种模式：位置、速度、力矩的对比（负载 0.2 N·m）", fontsize=11)
    fig.tight_layout(rect=(0.04, 0, 1, 0.96))
    savefig(fig, "2_6_modes_compare")

    print("六种模式共用式（2-1）；阻尼/零力矩模式关闭了 0.2 N·m 恒负载，"
          "否则重力会让关节缓慢下滑。")
    draw_relation()


if __name__ == "__main__":
    main()
