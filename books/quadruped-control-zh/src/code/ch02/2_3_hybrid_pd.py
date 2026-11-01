"""Step responses of the hybrid law on a single joint (section 2.3).

Plant: inertia J, viscous friction b and a constant load torque.  The command
is a unit position step, so the steady-state error is load/kp and the
transient is governed by kp, kd.  Nine combinations of kp and kd are run and
the overshoot, settling time and steady-state error are printed.

expect: kp=  5, kd=0.0
expect: kp= 80, kd=2.0
expect: 调节时间按 2% 误差带
"""

from __future__ import annotations

import logging

import numpy as np

from quadruped.plotting import savefig
import matplotlib.pyplot as plt
from matplotlib.patches import Circle, FancyArrowPatch, FancyBboxPatch

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

J = 0.05              # rotor-side inertia, kg m^2
B = 0.10              # viscous friction, N m s/rad
LOAD = 0.50           # constant load torque, N m
TAU_MAX = 33.5        # A1 joint limit, N m
TARGET = 1.0


def simulate(kp: float, kd: float, t_end: float = 8.0, dt: float = 1e-4):
    steps = int(t_end / dt)
    t = np.arange(steps + 1) * dt
    q = np.zeros(steps + 1)
    dq = np.zeros(steps + 1)
    tau = np.zeros(steps + 1)
    for i in range(steps):
        u = kp * (TARGET - q[i]) + kd * (0.0 - dq[i])
        u = float(np.clip(u, -TAU_MAX, TAU_MAX))
        tau[i] = u
        acc = (u - B * dq[i] - LOAD) / J
        dq[i + 1] = dq[i] + acc * dt
        q[i + 1] = q[i] + dq[i + 1] * dt
    tau[-1] = tau[-2]
    return t, q, tau


def metrics(t, q, tau):
    peak = float(np.max(q))
    overshoot = max(0.0, (peak - TARGET) / TARGET * 100.0)
    final = float(np.mean(q[-5000:]))
    outside = np.where(np.abs(q - final) > 0.02 * TARGET)[0]
    settling = float(t[outside[-1]]) if outside.size else 0.0
    steady = float(TARGET - final)
    return overshoot, settling, steady, float(np.max(np.abs(tau)))


def draw_block_diagram() -> None:
    fig, ax = plt.subplots(figsize=(10.0, 4.6))
    ax.set_xlim(0, 10.6)
    ax.set_ylim(0, 5.4)
    ax.axis("off")

    def box(x, y, w, h, text, fc="#e3f2fd"):
        ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.04",
                                    facecolor=fc, edgecolor="0.25"))
        ax.text(x + w / 2, y + h / 2, text, ha="center", va="center", fontsize=9)

    def arrow(x0, y0, x1, y1, color="0.2"):
        ax.add_patch(FancyArrowPatch((x0, y0), (x1, y1), arrowstyle="-|>",
                                     mutation_scale=10, lw=1.0, color=color))

    ax.text(0.05, 4.75, r"$\tau_{\rm ff}$", fontsize=11)
    arrow(0.55, 4.75, 3.75, 4.75)
    ax.text(0.05, 3.75, r"$p_{\rm des}$", fontsize=11)
    arrow(0.80, 3.75, 1.45, 3.75)
    ax.text(0.05, 2.75, r"$\omega_{\rm des}$", fontsize=11)
    arrow(0.90, 2.75, 1.45, 2.75)

    box(1.45, 3.45, 0.55, 0.60, r"$+$", "#fff8e1")
    box(1.45, 2.45, 0.55, 0.60, r"$+$", "#fff8e1")
    box(2.35, 3.45, 0.75, 0.60, r"$k_p$")
    box(2.35, 2.45, 0.75, 0.60, r"$k_d$")
    arrow(2.00, 3.75, 2.35, 3.75)
    arrow(2.00, 2.75, 2.35, 2.75)

    ax.add_patch(Circle((3.75, 3.45), 0.22, facecolor="#fff8e1", edgecolor="0.25"))
    ax.text(3.75, 3.45, r"$+$", ha="center", va="center", fontsize=10)
    arrow(3.10, 3.75, 3.55, 3.55)
    arrow(3.10, 2.75, 3.55, 3.30)
    arrow(3.75, 4.75, 3.75, 3.67)
    ax.text(2.02, 3.48, r"$-$", fontsize=11, color="#c62828")
    ax.text(2.02, 2.48, r"$-$", fontsize=11, color="#c62828")

    box(4.45, 3.05, 1.25, 0.85, "电流环\n(力矩环)", "#e8f5e9")
    arrow(3.97, 3.45, 4.45, 3.45)
    box(6.10, 3.05, 1.10, 0.85, "电机\n$J,b$", "#e8f5e9")
    arrow(5.70, 3.45, 6.10, 3.45)
    box(7.60, 3.05, 1.05, 0.85, "减速器\n9.1", "#f3e5f5")
    arrow(7.20, 3.45, 7.60, 3.45)
    arrow(8.65, 3.45, 9.35, 3.45)
    ax.text(9.40, 3.32, r"$p_{\rm out}$", fontsize=10)

    ax.plot([6.65, 6.65], [3.05, 1.55], color="0.25", lw=1.0)
    ax.plot([6.65, 1.75], [1.55, 1.55], color="0.25", lw=1.0)
    arrow(1.75, 1.55, 1.75, 2.45)
    arrow(1.75, 1.55, 1.75, 3.45)
    box(4.85, 1.15, 1.45, 0.62, "编码器 (转子侧)", "#fff3e0")
    ax.text(6.72, 1.42, r"$p,\ \omega$", fontsize=10)
    ax.text(3.05, 1.25, "位置/速度反馈（转子侧）", fontsize=8.5, color="0.3")

    ax.text(0.05, 0.55, "整定顺序：先小 $k_p$ 确认方向与零位，再加 $k_d$ 抑制振荡，最后加 $\\tau_{\\rm ff}$ 补偿重力",
            fontsize=8.5, color="0.25")
    ax.set_title("关节混合控制：前馈 + 位置 PD + 电流环 + 减速器")
    savefig(fig, "2_3_block")


def main() -> None:
    kps = (5.0, 20.0, 80.0)
    kds = (0.0, 0.5, 2.0)
    fig, axes = plt.subplots(1, 3, figsize=(11.0, 3.8), sharey=True)
    for ax, kd in zip(axes, kds):
        for kp in kps:
            t, q, tau = simulate(kp, kd)
            over, settle, steady, peak_tau = metrics(t, q, tau)
            wn = np.sqrt(kp / J)
            zeta = (B + kd) / (2.0 * np.sqrt(kp * J))
            print(f"kp={kp:3.0f}, kd={kd:3.1f}: 超调 {over:5.1f}%, "
                  f"调节时间 {settle:4.2f} s, 稳态误差 {steady:7.4f} rad, "
                  f"最大力矩 {peak_tau:5.2f} N·m, ζ={zeta:4.2f}")
            ax.plot(t[:30001], q[:30001], lw=1.4, label=rf"$k_p={kp:.0f}$")
        ax.axhline(TARGET, color="0.3", lw=0.8, ls="--")
        ax.set_title(rf"$k_d={kd:.1f}$")
        ax.set_xlabel("时间 (s)")
        ax.set_xlim(0, 3.0)
        ax.grid(True, alpha=0.3)
        ax.legend(fontsize=8)
    axes[0].set_ylabel("转子位置 q (rad)")
    axes[0].text(0.35, 1.33, "振荡", fontsize=9, color="#c62828")
    axes[2].text(0.35, 0.33, "过阻尼", fontsize=9, color="#1565c0")
    fig.suptitle("位置阶跃：位置刚度与速度刚度的作用（负载 0.5 N·m）", fontsize=11)
    fig.tight_layout()
    savefig(fig, "2_3_gain_step")

    print("调节时间按最终值 ±2% 误差带、最后一次离开误差带的时刻计；稳态误差取最后 0.5 s 均值。")
    print("稳态误差理论值 load/kp: " +
          ", ".join(f"{LOAD / kp:.4f} rad (kp={kp:.0f})" for kp in kps))
    draw_block_diagram()


if __name__ == "__main__":
    main()
