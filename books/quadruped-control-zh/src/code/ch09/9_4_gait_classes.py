# expect: 步态
# expect: 周期 0
# expect: 相位
# expect: 落点
# expect: 最大足端速度
# expect: 触地切换
"""The three generators on a 2 s trot (9.4).

``WaveGenerator`` turns time into phase/contact, ``FeetEndCal`` turns phase,
base state and velocity into a foothold, and ``GaitGenerator`` turns the
foothold plus phase into a cycloid foot position and velocity.  The script
runs five trot cycles at 0.3 m/s and prints one block per cycle; the figure
shows the gait bars, the footholds, and the four x/z foot trajectories.

"""

from __future__ import annotations

import logging

import numpy as np

from quadruped import A1Params, Quadruped, GaitParams, GaitGenerator, WaveStatus
from quadruped.plotting import label, savefig
import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

DT = 0.002
PERIOD = 0.40
DUTY = 0.60
CYCLES = 5
V_CMD = 0.30
STEP_HEIGHT = 0.06

LEGS = ["FR", "FL", "RR", "RL"]
LEG_ZH = ["右前", "左前", "右后", "左后"]
COLORS = ["C0", "C1", "C2", "C3"]
LINESTYLES = ["-", "--", ":", "-."]


def make_robot():
    params = A1Params.from_menagerie()
    robot = Quadruped(params)
    gait = GaitParams.trot(period=PERIOD, duty=DUTY, step_height=STEP_HEIGHT)
    gait.kx = gait.ky = 0.06
    gait.kyaw = 0.08
    gait.ground_z = 0.02
    return params, robot, gait


# ANCHOR: run
def run(robot, params, gait):
    gen = GaitGenerator(robot, params, gait)
    gen.set_status(WaveStatus.WAVE_ALL, 0.0)
    gen.set_gait(np.array([V_CMD, 0.0]), 0.0)
    base = np.array([0.0, 0.0, 0.27])
    contacts: list[np.ndarray] = []
    times: list[float] = []
    xs: list[np.ndarray] = []
    zs: list[np.ndarray] = []
    touchdowns: list[dict] = []
    cycles: list[dict] = []
    prev = np.ones(4, dtype=bool)

    t = 0.0
    for k in range(CYCLES):
        gen.update(t, base, 0.0, np.array([V_CMD, 0.0, 0.0]), 0.0)
        snap_phase = gen.phase.copy()
        snap_contact = gen.contact.copy()
        snap_target = gen.target.copy()
        vmax = np.zeros(4)
        steps = int(round(PERIOD / DT))
        for j in range(steps):
            tt = t + j * DT
            if j > 0:
                gen.update(tt, base, 0.0, np.array([V_CMD, 0.0, 0.0]), 0.0)
            for i in range(4):
                vmax[i] = max(vmax[i], float(np.linalg.norm(gen.foot_velocity(i))))
            times.append(tt)
            contacts.append(gen.contact.copy())
            xs.append(gen.position[:, 0].copy())
            zs.append(gen.position[:, 2].copy())
            for i in range(4):
                if gen.contact[i] and not prev[i]:
                    touchdowns.append({"leg": i, "t": tt,
                                       "pos": gen.foot_target(i).copy(),
                                       "base": base.copy(),
                                       "neutral": gen.calc.neutral_world(base, 0.0)[i].copy()})
            prev = gen.contact.copy()
            base = base + np.array([V_CMD * DT, 0.0, 0.0])
        cycles.append({"k": k, "t": t, "phase": snap_phase,
                       "contact": snap_contact, "target": snap_target,
                       "vmax": vmax})
        t += PERIOD
    return cycles, touchdowns, np.array(times), np.array(contacts), np.array(xs), np.array(zs)


# ANCHOR_END: run


def print_cycles(cycles: list[dict], gait: GaitParams) -> None:
    print(f"步态：trot，P = {gait.period:.3f} s，r = {gait.duty:.3f}，"
          f"触地 {gait.stance_time:.3f} s，腾空 {gait.swing_time:.3f} s，"
          f"步长 vP = {V_CMD * gait.period:.3f} m")
    for c in cycles:
        print(f"周期 {c['k']}  t = {c['t']:.3f} s")
        print("  相位 [" + " ".join(f"{x:.3f}" for x in c["phase"]) + "]  "
              "接触 [" + " ".join(str(int(x)) for x in c["contact"]) + "]")
        rel = c["target"].copy()
        rel[:, 0] -= V_CMD * c["t"]
        print("  落点 x（相对机身中心）/mm ["
              + " ".join(f"{x * 1000:7.1f}" for x in rel[:, 0]) + " ]")
        print("  落点 y（相对机身中心）/mm ["
              + " ".join(f"{x * 1000:7.1f}" for x in rel[:, 1]) + " ]")
        imax = int(np.argmax(c["vmax"]))
        print(f"  本周期最大足端速度 {c['vmax'].max():.4f} m/s（{LEG_ZH[imax]} {LEGS[imax]}）")


def figure_dataflow() -> None:
    fig, ax = plt.subplots(figsize=(10.4, 4.2))
    boxes = [
        (0.02, "WaveGenerator", "时间 → 相位/接触"),
        (0.27, "FeetEndCal", "相位 + 机身状态 → 落点"),
        (0.52, "GaitGenerator", "落点 + 相位 → 摆线"),
        (0.77, "Leg", "逆解 + 雅可比 → 关节"),
    ]
    w, h, y0 = 0.19, 0.30, 0.36
    for x, name, sub in boxes:
        ax.add_patch(FancyBboxPatch((x, y0), w, h, boxstyle="round,pad=0.012",
                                    fc="#eef4fb", ec="C0", lw=1.4))
        ax.text(x + w / 2, y0 + h * 0.66, name, ha="center", va="center", fontsize=10)
        ax.text(x + w / 2, y0 + h * 0.30, sub, ha="center", va="center", fontsize=7.5,
                color="0.35")
    labels_in = [r"$t,\ P,\ r,\ b_i$", r"$v,\ \omega,\ p_\mathrm{base},\ \varphi$",
                 r"$x_0,\ x_1,\ h,\ T_\mathrm{sw}$", r"$q_\mathrm{ref}$"]
    labels_out = [r"$\varphi_i,\ c_i$", r"$x_{f,i},\ y_{f,i}$",
                  r"$p_i(t),\ v_i(t)$", r"$q,\ \dot q,\ \tau$"]
    for (x, _, _), s in zip(boxes, labels_in):
        ax.text(x + w / 2, y0 + h + 0.08, s, ha="center", fontsize=9, color="0.25")
        ax.annotate("", xy=(x + w / 2, y0 + h + 0.01), xytext=(x + w / 2, y0 + h + 0.05),
                    arrowprops=dict(arrowstyle="-|>", color="0.45", lw=0.9))
    for (x, _, _), s in zip(boxes, labels_out):
        ax.text(x + w / 2, y0 - 0.11, s, ha="center", fontsize=9, color="0.25")
        ax.annotate("", xy=(x + w / 2, y0 - 0.02), xytext=(x + w / 2, y0 - 0.07),
                    arrowprops=dict(arrowstyle="-|>", color="0.45", lw=0.9))
    for k in range(3):
        xa = boxes[k][0] + w
        xb = boxes[k + 1][0]
        ax.add_patch(FancyArrowPatch((xa + 0.005, y0 + h / 2), (xb - 0.005, y0 + h / 2),
                                     arrowstyle="-|>", mutation_scale=12,
                                     color="C0", lw=1.4))
    ax.add_patch(FancyArrowPatch((boxes[3][0] + w / 2, y0 - 0.28),
                                 (boxes[0][0] + w / 2, y0 - 0.28),
                                 connectionstyle="arc3,rad=-0.06",
                                 arrowstyle="-|>", mutation_scale=10,
                                 color="0.55", ls="--", lw=1.0))
    ax.text(0.50, y0 - 0.38, label("下一控制周期", "next control cycle"),
            ha="center", fontsize=8, color="0.4")
    ax.text(0.02, 0.93, label("三个生成器类的数据流：时间 → 相位 → 落点 → 足端位置/速度 → 关节",
                              "data flow of the three generators"), fontsize=10)
    ax.set_xlim(0.0, 0.99)
    ax.set_ylim(0.0, 1.0)
    ax.axis("off")
    fig.tight_layout()
    savefig(fig, "9_4_dataflow")
    plt.close(fig)


def figure_offline(cycles, touchdowns, t, contact, xs, zs, gait):
    fig, axes = plt.subplots(2, 2, figsize=(10.2, 6.4))
    ax = axes[0, 0]
    for i in range(4):
        on = contact[:, i].astype(bool)
        start = None
        for k, flag in enumerate(on):
            if flag and start is None:
                start = t[k]
            if start is not None and (not flag or k == len(on) - 1):
                end = t[k] if not flag else t[-1] + DT
                ax.add_patch(plt.Rectangle((start, i - 0.3), end - start, 0.6,
                                           fc="0.15", ec="0.15"))
                start = None
    ax.set_xlim(0.0, CYCLES * gait.period)
    ax.set_ylim(-0.6, 3.6)
    ax.set_yticks(range(4))
    ax.set_yticklabels([f"{LEG_ZH[i]} {LEGS[i]}" for i in range(4)])
    ax.invert_yaxis()
    ax.set_xlabel(label("时间 t / s", "t / s"))
    ax.set_title(label("(a) 2 s 的接触条图", "(a) contact bars over 2 s"), fontsize=10)

    ax = axes[0, 1]
    for i in range(4):
        rec = [r for r in touchdowns if r["leg"] == i]
        f = np.array([r["pos"][:2] for r in rec])
        n = np.array([r["neutral"][:2] for r in rec])
        ax.plot(n[:, 0], n[:, 1], "o", mfc="white", mec=COLORS[i], ms=4,
                label=f"{LEG_ZH[i]} {LEGS[i]}")
        ax.plot(f[:, 0], f[:, 1], ".", color=COLORS[i], ms=5)
        for k in range(len(rec)):
            ax.plot([n[k, 0], f[k, 0]], [n[k, 1], f[k, 1]], color=COLORS[i], lw=0.7)
    ax.set_aspect("equal")
    ax.set_xlim(0.0, 0.72)
    ax.set_ylim(-0.26, 0.26)
    ax.set_xlabel(label("x / m", "x / m"))
    ax.set_ylabel(label("y / m", "y / m"))
    ax.set_title(label("(b) 触地落脚点（实心）与中性点（空心）",
                       "(b) footholds (filled) and neutral points"), fontsize=10)
    ax.legend(loc="center", fontsize=7, ncol=2, framealpha=0.9)
    ax.grid(alpha=0.2)

    ax = axes[1, 0]
    for i in range(4):
        ax.plot(t, xs[:, i], color=COLORS[i], ls=LINESTYLES[i], lw=1.2,
                label=f"{LEG_ZH[i]} {LEGS[i]}")
    ax.set_xlabel(label("时间 t / s", "t / s"))
    ax.set_ylabel(label("足端 x / m", "foot x / m"))
    ax.set_title(label("(c) 四条腿的 x 轨迹", "(c) foot x trajectories"), fontsize=10)
    ax.grid(alpha=0.2)

    ax = axes[1, 1]
    for i in range(4):
        ax.plot(t, (zs[:, i] - gait.ground_z) * 1000, color=COLORS[i],
                ls=LINESTYLES[i], lw=1.2)
    ax.set_xlabel(label("时间 t / s", "t / s"))
    ax.set_ylabel(label("足端 z - z_g / mm", "foot z - ground / mm"))
    ax.set_title(label("(d) 四条腿的抬腿高度", "(d) foot height"), fontsize=10)
    ax.grid(alpha=0.2)
    axes[1, 0].legend(fontsize=7, ncol=2, loc="upper left")
    fig.tight_layout()
    savefig(fig, "9_4_offline")
    plt.close(fig)


def main() -> None:
    params, robot, gait = make_robot()
    cycles, touchdowns, t, contact, xs, zs = run(robot, params, gait)
    print_cycles(cycles, gait)
    print(f"2 s 共 {CYCLES} 个周期，记录 {len(touchdowns)} 次触地切换"
          f"（t = 0 时右前、右后已在支撑相，不计切换）；"
          f"触地样本占 {100.0 * contact.mean():.1f}%")
    figure_dataflow()
    figure_offline(cycles, touchdowns, t, contact, xs, zs, gait)


if __name__ == "__main__":
    main()
