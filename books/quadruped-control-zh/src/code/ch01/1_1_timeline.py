# expect: 样本机型数: 12
# expect: 电机驱动占比: 9/12 = 75.0%
# expect: 开源机型数: 1
"""Draw the 2013-2024 quadruped timeline and the drive/form comparison.

The timeline is not a product catalogue: each node records the team, the
machine, the drive type and whether the design or the control software was
released.  The two statistics printed at the end are the ones used in the
text; the node count is deliberately small enough to be checked by hand.
"""

from __future__ import annotations

import logging
import warnings

from quadruped.plotting import label, savefig

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)
warnings.filterwarnings("ignore", message=".*Glyph.*")

import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D

# ANCHOR: machines
# year, team, machine, drive, form, open, note
MACHINES = [
    (2013, "Boston Dynamics", "Atlas", "hydraulic", "biped", False, "液压双足"),
    (2015, "IIT", "HyQ2Max", "hydraulic", "quadruped", False, "液压四足"),
    (2015, "Boston Dynamics", "Spot", "hydraulic", "quadruped", False, "液压原型"),
    (2016, "ETH", "ANYmal", "electric", "quadruped", False, "工业巡检"),
    (2017, "Unitree", "Laikago", "electric", "quadruped", False, "公开零售"),
    (2018, "MIT", "Cheetah 3", "electric", "quadruped", False, "高动态"),
    (2019, "MIT", "Mini Cheetah", "electric", "quadruped", True, "后空翻/开源"),
    (2019, "Unitree", "Aliengo", "electric", "quadruped", False, "中型平台"),
    (2020, "Unitree", "A1", "electric", "quadruped", False, "本书仿真"),
    (2021, "Unitree", "Go1", "electric", "quadruped", False, "教育版"),
    (2023, "Unitree", "Go2", "electric", "quadruped", False, "量产"),
    (2024, "Unitree", "B2-W", "electric", "wheeled", False, "轮足"),
]
# ANCHOR_END: machines

COLORS = {"hydraulic": "#d95f02", "electric": "#1f77b4"}


def draw_body(ax, x, y, s, color, form, open_source):
    """A five-segment sketch of a machine, centred at (x, y)."""
    lw = 2.6 if open_source else 1.6
    edge = "#b8860b" if open_source else color
    if form == "biped":
        ax.add_patch(mpatches.FancyBboxPatch(
            (x - 0.16 * s, y + 0.02 * s), 0.32 * s, 0.20 * s,
            boxstyle="round,pad=0.02", fc=color, ec=edge, lw=lw, alpha=0.9))
        for dx in (-0.08, 0.08):
            ax.plot([x + dx * s, x + dx * s, x + (dx + 0.05) * s],
                    [y + 0.02 * s, y - 0.22 * s, y - 0.34 * s],
                    color=edge, lw=lw, solid_capstyle="round")
    elif form == "wheeled":
        ax.add_patch(mpatches.FancyBboxPatch(
            (x - 0.20 * s, y + 0.01 * s), 0.40 * s, 0.17 * s,
            boxstyle="round,pad=0.02", fc=color, ec=edge, lw=lw, alpha=0.9))
        for dx, sign in ((-0.14, -1), (0.14, 1)):
            knee_x = x + (dx + 0.035 * sign) * s
            foot_x = x + dx * s
            ax.plot([x + dx * s, knee_x, foot_x],
                    [y + 0.01 * s, y - 0.15 * s, y - 0.21 * s],
                    color=edge, lw=lw, solid_capstyle="round")
            ax.plot([foot_x, foot_x], [y - 0.21 * s, y - 0.25 * s],
                    color=edge, lw=lw)
            ax.add_patch(mpatches.Circle((foot_x, y - 0.27 * s), 0.052 * s,
                                         fc="white", ec=edge, lw=lw))
    else:
        ax.add_patch(mpatches.FancyBboxPatch(
            (x - 0.20 * s, y + 0.005 * s), 0.40 * s, 0.16 * s,
            boxstyle="round,pad=0.02", fc=color, ec=edge, lw=lw, alpha=0.9))
        for dx, sign in ((-0.14, -1), (0.14, 1)):
            knee_x = x + (dx + 0.05 * sign) * s
            foot_x = x + (dx - 0.02 * sign) * s
            ax.plot([x + dx * s, knee_x, foot_x],
                    [y + 0.01 * s, y - 0.17 * s, y - 0.30 * s],
                    color=edge, lw=lw, solid_capstyle="round")
            ax.plot([knee_x], [y - 0.17 * s], marker="o", ms=1.6,
                    color=edge, zorder=6)
        # head / sensor bump at the front
        ax.add_patch(mpatches.Rectangle((x + 0.17 * s, y + 0.06 * s),
                                        0.07 * s, 0.06 * s, fc=color, ec=edge, lw=lw))


def timeline():
    fig, ax = plt.subplots(figsize=(15.0, 7.6))
    ax.set_xlim(-0.65, len(MACHINES) - 0.35)
    ax.set_ylim(-1.16, 1.16)
    ax.axis("off")
    ax.plot([-0.55, len(MACHINES) - 0.45], [0, 0], color="#444444", lw=2.0,
            zorder=1)
    ax.annotate("", xy=(len(MACHINES) - 0.45, 0), xytext=(len(MACHINES) - 0.75, 0),
                arrowprops=dict(arrowstyle="-|>", color="#444444", lw=2.0))

    for i, (year, team, name, drive, form, open_source, note) in enumerate(MACHINES):
        above = i % 2 == 0
        sign = 1 if above else -1
        color = COLORS[drive]
        draw_body(ax, i, sign * 0.36, 1.05, color, form, open_source)
        ax.plot([i, i], [0, sign * 0.15], color="#777777", lw=0.8, ls=":")
        ax.text(i, sign * 0.73, f"{year}  {name}", ha="center",
                va="bottom" if above else "top", fontsize=8.6)
        ax.text(i, sign * 0.87, team, ha="center",
                va="bottom" if above else "top", fontsize=7.4, color="#333333")
        ax.text(i, sign * 1.01, note, ha="center",
                va="bottom" if above else "top", fontsize=7.0, color=color)
        if open_source:
            ax.plot([i + 0.20], [sign * 0.47], marker="*", ms=11,
                    color="#b8860b", mec="white", mew=0.7, zorder=8)

    handles = [
        Line2D([], [], color=COLORS["hydraulic"], lw=3, label=label("液压驱动", "hydraulic")),
        Line2D([], [], color=COLORS["electric"], lw=3, label=label("电机驱动", "electric")),
        Line2D([], [], color="#b8860b", lw=3, label=label("开源（粗边 + 星号）", "open source")),
    ]
    ax.legend(handles=handles, loc="lower center", ncol=4, fontsize=8,
              frameon=False, bbox_to_anchor=(0.5, -0.02))
    ax.set_title(label("2013–2024 年代表性足式机器人时间轴（节点按年份排序，同年并列）",
                       "Quadruped timeline, 2013-2024 (nodes sorted by year)"),
                 fontsize=12, pad=8)
    savefig(fig, "1_1_timeline")
    plt.close(fig)


def comparison():
    fig, axes = plt.subplots(1, 3, figsize=(11.4, 4.2))
    cases = [
        ("hydraulic", "quadruped", label("(a) 液压四足", "(a) hydraulic"),
         label("HyQ / 液压四足平台", "HyQ / hydraulic quadruped"),
         [label("伺服阀控制液压缸", "servo-valve cylinders"),
          label("功率密度高", "high power density"),
          label("泄漏、维护复杂", "leaks, high maintenance")]),
        ("electric", "quadruped", label("(b) 电机四足", "(b) electric"),
         label("Spot / A1 / ANYmal", "Spot / A1 / ANYmal"),
         [label("永磁同步电机 + 减速器", "PMSM + gearbox"),
          label("12 关节、可量产", "12 joints, mass production"),
          label("控制精度高、噪声低", "precise, quiet")]),
        ("electric", "wheeled", label("(c) 轮足", "(c) wheeled"),
         label("B2-W / 轮足平台", "B2-W / wheel-legged"),
         [label("轮子负责平地效率", "wheels for flat ground"),
          label("腿负责越障与调姿", "legs for obstacles"),
          label("与纯轮式 Stretch 不同", "unlike wheeled Stretch")]),
    ]
    for ax, (drive, form, title, examples, notes) in zip(axes, cases):
        ax.set_xlim(-0.62, 0.62)
        ax.set_ylim(-0.55, 0.72)
        ax.axis("off")
        ax.plot([-0.55, 0.55], [-0.36, -0.36], color="#999999", lw=1.2)
        draw_body(ax, 0.0, 0.12, 1.55, COLORS[drive], form, False)
        ax.set_title(title, fontsize=10.5, pad=2)
        ax.text(0.0, 0.63, examples, ha="center", fontsize=8.5, color="#333333")
        for k, line in enumerate(notes):
            ax.text(-0.55, -0.46 - 0.09 * k, "- " + line, ha="left", fontsize=8)
    fig.suptitle(label("三种驱动与形态：同一时期为什么会出现完全不同的机器狗",
                       "Three drive/form combinations behind the diversity"),
                 fontsize=11.5, y=1.02)
    fig.tight_layout()
    savefig(fig, "1_1_forms")
    plt.close(fig)


def main():
    for row in MACHINES:
        print(f"{row[0]}  {row[2]:<13s} {row[1]:<16s} {row[3]:<9s} "
              f"{'open' if row[5] else 'closed'}")
    # ANCHOR: stats
    n = len(MACHINES)
    electric = sum(m[3] == "electric" for m in MACHINES)
    hydraulic = sum(m[3] == "hydraulic" for m in MACHINES)
    opened = sum(m[5] for m in MACHINES)
    print(f"样本机型数: {n}")
    print(f"电机驱动占比: {electric}/{n} = {100.0 * electric / n:.1f}%")
    print(f"液压驱动占比: {hydraulic}/{n} = {100.0 * hydraulic / n:.1f}%")
    print(f"开源机型数: {opened}（Mini Cheetah：硬件设计 + 2019 年控制算法）")
    # ANCHOR_END: stats
    timeline()
    comparison()
    print("图已保存: figs/1_1_timeline.png, figs/1_1_forms.png")


if __name__ == "__main__":
    main()
