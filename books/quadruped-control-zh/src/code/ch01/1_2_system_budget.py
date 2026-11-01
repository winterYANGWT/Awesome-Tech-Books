# expect: A1 整机质量: 12.453 kg
# expect: 静止时每腿法向力: 30.54 N
# expect: 0.5 m/s² 加速时总推力: 122.32 N
# expect: 加速时单腿最大关节力矩: 4.978 N·m
# expect: 力矩余量: 85.1%
# expect: 功率下限: 6.23 W
# expect: 蹬伸峰值: 91.62 W
"""Budget the A1 from statics, one acceleration case and one push-off case.

The joint torque is obtained from the same leg Jacobian used in Chapter 5,
``tau = J^T f``.  The point is not to design a controller but to show which
number (torque or power) limits a small quadruped, and why the answer changes
between quasi-static standing and a dynamic push-off.

The second half draws the four-system block diagram and the 12-joint leg.
"""

from __future__ import annotations

import logging
import warnings

import numpy as np

from quadruped import A1Params, Quadruped
from quadruped.plotting import label, savefig

import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)
warnings.filterwarnings("ignore", message=".*Glyph.*")

G = 9.81
TAU_MAX = 33.5
V_REF = 1.0


# ANCHOR: budget
def joint_budget():
    p = A1Params.from_menagerie()
    robot = Quadruped(p)
    m, g = p.mass, G
    weight = m * g
    fz = weight / 4.0
    q = p.home_q[:3]
    J = robot.legs[0].jacobian(q)
    tau_static = J.T @ np.array([0.0, 0.0, fz])
    tau_static_max = float(np.max(np.abs(tau_static)))
    margin_static = (TAU_MAX - tau_static_max) / TAU_MAX

    a = 0.5
    fx_total = m * a
    fx = fx_total / 4.0
    thrust_total = float(np.hypot(weight, fx_total))
    thrust_leg = thrust_total / 4.0
    tau_acc = J.T @ np.array([fx, 0.0, fz])
    tau_acc_max = float(np.max(np.abs(tau_acc)))
    margin_acc = (TAU_MAX - tau_acc_max) / TAU_MAX

    # Lower bound on the mechanical power of the whole body while it is
    # accelerating at 0.5 m/s^2 and already moving at 1 m/s.
    p_lower = fx_total * V_REF

    # Push-off: each leg carries 2.5 times its static share and extends at
    # 0.3 m/s.  Joint speed comes from the same Jacobian.
    fz_push = 2.5 * fz
    tau_push = J.T @ np.array([0.0, 0.0, fz_push])
    dq_push = np.linalg.solve(J, np.array([0.0, 0.0, 0.3]))
    p_leg_push = float(abs(tau_push @ dq_push))
    p_push = 4.0 * p_leg_push
    current_push = p_push / 24.0

    print(f"A1 整机质量: {m:.3f} kg")
    print(f"重力: {weight:.2f} N")
    print(f"静止时每腿法向力: {fz:.2f} N")
    print(f"静止时单腿最大关节力矩: {tau_static_max:.3f} N·m")
    print(f"静止力矩余量: {100.0 * margin_static:.1f}%")
    print(f"0.5 m/s² 加速所需水平力: {fx_total:.2f} N "
          f"（每腿 {fx:.2f} N）")
    print(f"0.5 m/s² 加速时总推力: {thrust_total:.2f} N")
    print(f"加速时每腿推力: {thrust_leg:.2f} N")
    print(f"加速时单腿最大关节力矩: {tau_acc_max:.3f} N·m")
    print(f"力矩余量: {100.0 * margin_acc:.1f}%")
    print(f"功率下限: {p_lower:.2f} W（v={V_REF:.1f} m/s，只计水平加速功）")
    print(f"蹬伸峰值: {p_push:.2f} W（每腿 2.5 倍静态分担载荷、0.3 m/s，"
          f"单腿 {p_leg_push:.2f} W）")
    print(f"24 V 母线电流下限: {current_push:.2f} A（不含损耗与摆动腿）")
    return dict(mass=m, weight=weight, fz=fz, tau_static=tau_static,
                tau_acc=tau_acc, p_lower=p_lower, p_push=p_push)
# ANCHOR_END: budget


def box(ax, x0, y0, x1, y1, text, fc, ec, fs=9.0, lw=1.4):
    ax.add_patch(mpatches.FancyBboxPatch(
        (x0, y0), x1 - x0, y1 - y0,
        boxstyle="round,pad=0.04,rounding_size=0.08",
        fc=fc, ec=ec, lw=lw, zorder=2))
    ax.text((x0 + x1) / 2, (y0 + y1) / 2, text, ha="center", va="center",
            fontsize=fs, zorder=3, linespacing=1.35)


def arrow(ax, a, b, color, lw=1.6, ls="-", style="-|>", rad=0.0, z=4):
    ax.add_patch(mpatches.FancyArrowPatch(
        a, b, arrowstyle=style, mutation_scale=12, lw=lw, color=color,
        linestyle=ls, connectionstyle=f"arc3,rad={rad}", zorder=z))


def system_diagram():
    fig, ax = plt.subplots(figsize=(12.2, 8.3))
    ax.set_xlim(0, 12)
    ax.set_ylim(0, 8.7)
    ax.axis("off")

    blue, orange, purple = "#1f5fa8", "#d2691e", "#6a3d9a"
    # -- control system -------------------------------------------------
    box(ax, 2.6, 5.3, 9.6, 8.0, "", "#eef4fb", blue, lw=1.8)
    ax.text(6.1, 7.72, label("控制系统：机载计算机 + 控制软件",
                             "control: onboard computers + software"),
            ha="center", fontsize=11, color=blue)
    box(ax, 2.85, 5.7, 4.95, 6.75,
        label("规划器\n用户命令/障碍\n→ 目标路径", "planner\ncommand/obstacle\n→ path"),
        "#dce9f7", blue, fs=8.0)
    box(ax, 5.25, 5.7, 7.35, 6.75,
        label("中间层\n步态·落脚点\n平衡·单腿", "middle layer\ngait/foothold\nbalance/leg"),
        "#dce9f7", blue, fs=8.0)
    box(ax, 7.65, 5.7, 9.35, 6.75,
        label("运动控制器\n路径 → 12 个\n关节命令", "motion controller\npath → 12 joint\ncommands"),
        "#dce9f7", blue, fs=8.0)
    arrow(ax, (4.95, 6.22), (5.25, 6.22), blue, lw=1.4, ls="--")
    arrow(ax, (7.35, 6.22), (7.65, 6.22), blue, lw=1.4, ls="--")

    # time reference around the control system
    ax.add_patch(mpatches.FancyBboxPatch(
        (2.45, 5.15), 7.3, 3.0,
        boxstyle="round,pad=0.04,rounding_size=0.1",
        fc="none", ec=purple, lw=1.5, ls=":", zorder=1))
    ax.text(2.5, 8.34, label("时间流：2 ms 周期 + 传感器时间戳", "time: 2 ms cycle + timestamps"),
            ha="left", va="bottom", fontsize=8.4, color=purple)

    # -- communication --------------------------------------------------
    box(ax, 0.7, 4.1, 11.3, 4.9,
        label("通信系统：上行（编码器/IMU/手柄）  ·  内部（计算机之间）  ·  下行（关节命令）",
              "communication: uplink (encoders/IMU/joystick) · internal · downlink (joint commands)"),
        "#fdf1e3", orange, fs=9.0)

    # -- power ----------------------------------------------------------
    box(ax, 3.8, 0.7, 7.4, 3.5, "", "#eef4fb", blue, lw=1.6)
    ax.text(5.6, 3.15, label("动力系统", "power / actuation"), ha="center",
            fontsize=10.5, color=blue)
    ax.text(5.6, 2.82, label("12 个关节电机 + 驱动器", "12 joint motors + drivers"),
            ha="center", fontsize=8.8)
    for k in range(3):
        ax.add_patch(mpatches.Circle((4.55 + 1.0 * k, 1.95), 0.22,
                                     fc="white", ec=blue, lw=1.4))
        ax.text(4.55 + 1.0 * k, 1.95, "M", ha="center", va="center",
                fontsize=8, color=blue)
    ax.text(5.6, 1.45, label("每条腿 3 个：外摆 / 大腿 / 小腿",
                             "3 per leg: abduction / thigh / knee"),
            ha="center", fontsize=8.2)
    ax.text(5.6, 1.12, label("× 4 条腿 = 12 个关节", "× 4 legs = 12 joints"),
            ha="center", fontsize=8.2)
    arrow(ax, (5.6, 0.7), (5.6, 0.38), orange, lw=3.0)
    ax.plot([3.6, 7.6], [0.36, 0.36], color="#666666", lw=2.2)
    ax.text(7.7, 0.36, label("地面", "ground"), va="center", fontsize=8.5)

    # -- energy ---------------------------------------------------------
    box(ax, 0.4, 0.7, 3.4, 3.5, "", "#fff4e8", orange, lw=1.6)
    ax.text(1.9, 3.15, label("能源系统", "energy"), ha="center",
            fontsize=10.5, color=orange)
    ax.text(1.9, 2.80, label("24 V 电池 / 外部直流", "24 V battery / DC input"),
            ha="center", fontsize=8.5)
    ax.add_patch(mpatches.Rectangle((1.25, 1.85), 1.3, 0.6, fc="white",
                                    ec=orange, lw=1.5))
    ax.plot([1.25, 1.25], [1.75, 2.55], color=orange, lw=5)
    ax.plot([2.55, 2.55], [1.75, 2.55], color=orange, lw=5)
    ax.text(1.9, 2.15, "24 V", ha="center", va="center", fontsize=9)
    ax.text(1.9, 1.35, label("母线同时供给控制、通信、传感器",
                             "the bus also feeds control, comms, sensors"),
            ha="center", fontsize=7.8)
    arrow(ax, (3.4, 2.1), (3.8, 2.1), orange, lw=3.0)
    ax.text(3.6, 2.25, "24 V", ha="center", fontsize=7.6, color=orange)

    # -- sensors and joystick ------------------------------------------
    box(ax, 7.8, 0.7, 11.4, 3.5, "", "#eef4fb", blue, lw=1.6)
    ax.text(9.6, 3.15, label("传感器", "sensors"), ha="center",
            fontsize=10.5, color=blue)
    ax.text(9.6, 2.65, label("12 个关节编码器\nIMU（姿态 / 角速度 / 加速度）\n足端接触",
                             "12 encoders\nIMU (attitude / rate / accel)\nfoot contact"),
            ha="center", va="center", fontsize=8.2)
    ax.text(9.6, 1.20, label("状态流：测量 → 估计器", "state: measurement → estimator"),
            ha="center", fontsize=8.0, color=blue)

    box(ax, 0.4, 3.62, 2.5, 4.02,
        label("遥控手柄（操作者意图）", "joystick (operator intent)"),
        "#f2f2f2", "#555555", fs=8.0, lw=1.0)

    # -- information flows ---------------------------------------------
    arrow(ax, (9.6, 3.5), (9.6, 4.1), blue, lw=1.8)
    ax.text(9.75, 3.78, label("上行", "uplink"), fontsize=7.4, color=blue,
            bbox=dict(boxstyle="round,pad=0.10", fc="white", ec="none"))
    arrow(ax, (1.45, 4.02), (1.45, 4.1), blue, lw=1.8)
    arrow(ax, (8.9, 4.9), (8.9, 5.3), blue, lw=1.8)
    ax.text(9.02, 5.06, label("状态", "state"), fontsize=7.4, color=blue,
            bbox=dict(boxstyle="round,pad=0.12", fc="white", ec="none"))
    arrow(ax, (7.2, 5.3), (7.2, 4.9), blue, lw=1.8, ls="--")
    ax.text(6.75, 5.06, label("命令", "command"), fontsize=7.4, color=blue,
            bbox=dict(boxstyle="round,pad=0.12", fc="white", ec="none"))
    arrow(ax, (5.6, 4.1), (5.6, 3.5), blue, lw=1.8, ls="--")
    ax.text(5.05, 3.78, label("下行", "downlink"), fontsize=7.4, color=blue,
            bbox=dict(boxstyle="round,pad=0.10", fc="white", ec="none"))

    handles = [
        Line2D([], [], color=orange, lw=3, label=label("能量流", "energy")),
        Line2D([], [], color=blue, lw=2, label=label("状态流", "state")),
        Line2D([], [], color=blue, lw=2, ls="--", label=label("命令流", "command")),
        Line2D([], [], color=purple, lw=2, ls=":", label=label("时间流", "time")),
    ]
    ax.legend(handles=handles, loc="upper left", bbox_to_anchor=(0.01, 0.99),
              fontsize=8.2, frameon=False, ncol=1)
    savefig(fig, "1_2_system")
    plt.close(fig)


def leg_diagram():
    fig, axes = plt.subplots(1, 3, figsize=(12.6, 4.7),
                             gridspec_kw=dict(width_ratios=(1.0, 0.9, 1.25)))
    p = A1Params.from_menagerie()
    robot = Quadruped(p)
    q = p.home_q
    feet = robot.fk_all(q)
    ids = ["0 FR", "1 FL", "2 RR", "3 RL"]

    # (a) top view: 4 legs and the leg index
    ax = axes[0]
    ax.set_xlim(-0.42, 0.42)
    ax.set_ylim(-0.34, 0.34)
    ax.set_aspect("equal")
    ax.add_patch(mpatches.FancyBboxPatch(
        (-0.20, -0.09), 0.40, 0.18, boxstyle="round,pad=0.01,rounding_size=0.03",
        fc="#dce9f7", ec="#1f5fa8", lw=1.6))
    ax.annotate("", xy=(0.31, 0.27), xytext=(0.20, 0.27),
                arrowprops=dict(arrowstyle="-|>", color="#333333", lw=1.4))
    ax.text(0.32, 0.27, "+x 前", va="center", fontsize=8.5)
    ax.annotate("", xy=(-0.31, 0.30), xytext=(-0.31, 0.19),
                arrowprops=dict(arrowstyle="-|>", color="#333333", lw=1.4))
    ax.text(-0.31, 0.32, "+y 左", ha="center", fontsize=8.5)
    for i in range(4):
        hip = p.hip_offset[i]
        foot = feet[i]
        knee = 0.5 * (hip + foot) + np.array([0.0, 0.035 * (1 if i in (0, 3) else -1), 0.0])
        ax.plot([hip[0], knee[0], foot[0]], [hip[1], knee[1], foot[1]],
                color="#1f5fa8", lw=2.4, solid_capstyle="round")
        ax.add_patch(mpatches.Circle((hip[0], hip[1]), 0.017, fc="white",
                                     ec="#1f5fa8", lw=1.2, zorder=4))
        ax.add_patch(mpatches.Circle((foot[0], foot[1]), 0.012, fc="#d2691e",
                                     ec="#d2691e", zorder=4))
        ax.text(foot[0] * 1.28, foot[1] * 1.45, ids[i], ha="center",
                fontsize=8.2, color="#1f5fa8")
    ax.text(0, 0, "机身", ha="center", va="center", fontsize=8)
    ax.text(-0.39, -0.31, label("每条腿 3 个关节：i-1 外摆、i-2 大腿、i-3 小腿",
                                "3 joints per leg: i-1 abduction, i-2 thigh, i-3 knee"),
            ha="left", fontsize=7.6)
    ax.set_title(label("(a) 俯视：腿编号 0–3", "(a) top view: leg IDs 0-3"),
                 fontsize=10)

    # (b) front view: abduction q1
    ax = axes[1]
    ax.set_xlim(-0.30, 0.30)
    ax.set_ylim(-0.36, 0.12)
    ax.set_aspect("equal")
    ax.plot([-0.12, 0.12], [0.0, 0.0], color="#333333", lw=1.6)
    for ang, alpha in ((0.0, 1.0), (0.35, 0.45), (-0.35, 0.45)):
        x = 0.25 * np.sin(ang)
        z = -0.25 * np.cos(ang)
        ax.plot([0, x], [0, z], color="#1f5fa8", lw=2.0, alpha=alpha)
        ax.add_patch(mpatches.Circle((x, z), 0.018, fc="white",
                                     ec="#1f5fa8", lw=1.2, alpha=alpha))
    ax.plot([0], [0], marker="o", ms=5, color="#d2691e")
    ax.annotate("q1", xy=(0.03, -0.20), xytext=(0.15, -0.27),
                arrowprops=dict(arrowstyle="->", color="#d2691e", lw=1.2),
                fontsize=9, color="#d2691e")
    ax.text(0.0, 0.07, label("q1 绕机身 x 轴（外摆）", "q1 about body x (abduction)"),
            ha="center", fontsize=8.2)
    ax.text(0.0, -0.34, label("正视：腿在 y-z 平面内摆动", "front view: swing in y-z"),
            ha="center", fontsize=7.8)
    ax.set_title(label("(b) q1：外摆", "(b) q1: abduction"), fontsize=10)

    # (c) side view at the home pose
    ax = axes[2]
    ax.set_xlim(-0.38, 0.30)
    ax.set_ylim(-0.35, 0.10)
    ax.set_aspect("equal")
    q2, q3 = q[1], q[2]
    knee = np.array([-p.l_thigh * np.sin(q2), -p.l_thigh * np.cos(q2)])
    foot = knee + np.array([-p.l_calf * np.sin(q2 + q3),
                            -p.l_calf * np.cos(q2 + q3)])
    ax.plot([-0.12, 0.12], [0, 0], color="#333333", lw=1.6)
    ax.plot([0, knee[0], foot[0]], [0, knee[1], foot[1]], color="#1f5fa8",
            lw=2.6, solid_capstyle="round")
    ax.add_patch(mpatches.Circle((knee[0], knee[1]), 0.012, fc="white",
                                 ec="#1f5fa8", lw=1.2, zorder=4))
    ax.add_patch(mpatches.Circle((foot[0], foot[1]), p.foot_radius,
                                 fc="#d2691e", ec="#d2691e", alpha=0.9, zorder=4))
    ax.plot([-0.33, 0.29], [-0.269, -0.269], color="#666666", lw=1.2, ls="--")
    ax.text(0.29, -0.255, label("地面", "ground"), ha="right", fontsize=7.5)
    ax.annotate("q2 (i,2)", xy=(-0.06, -0.05), xytext=(-0.23, 0.02),
                arrowprops=dict(arrowstyle="->", color="#d2691e", lw=1.2),
                fontsize=8.5, color="#d2691e")
    ax.annotate("q3 (i,3)", xy=(knee[0] + 0.02, knee[1] - 0.01),
                xytext=(knee[0] - 0.12, knee[1] + 0.08),
                arrowprops=dict(arrowstyle="->", color="#d2691e", lw=1.2),
                fontsize=8.5, color="#d2691e")
    ax.text(-0.32, -0.10, label("大腿 0.2 m", "thigh 0.2 m"), fontsize=7.8)
    ax.text(0.02, -0.21, label("小腿 0.2 m", "calf 0.2 m"), fontsize=7.8)
    ax.text(0.0, 0.075, label("侧视：q2、q3 绕 y 轴", "side view: q2, q3 about y"),
            ha="center", fontsize=8.2)
    ax.text(0.0, -0.315, label("q2=+0.9 rad，q3=-1.8 rad（home）",
                               "q2=+0.9 rad, q3=-1.8 rad (home)"),
            ha="center", fontsize=7.6)
    ax.set_title(label("(c) q2/q3：大腿与小腿", "(c) q2/q3: thigh and knee"),
                 fontsize=10)
    fig.tight_layout()
    savefig(fig, "1_2_leg")
    plt.close(fig)


def main():
    joint_budget()
    system_diagram()
    leg_diagram()
    print("图已保存: figs/1_2_system.png, figs/1_2_leg.png")


if __name__ == "__main__":
    main()
