"""Single-turn absolute encoder and the power-cycle reset (section 2.2).

The encoder measures the rotor angle modulo 2*pi.  While the drive is on it
adds the number of full turns it has seen, so the reading can exceed 2*pi.
After a power cycle the turn counter is lost and only the current angle
inside one turn remains.  The script also draws the exploded view used to
explain the joint's 12 parts and the sun-planet-ring gear train.

expect: 断电前
expect: 断电后
expect: 输出轴角度
"""

from __future__ import annotations

import logging

import numpy as np

from quadruped.plotting import savefig
import matplotlib.pyplot as plt
from matplotlib.patches import Circle, Ellipse, FancyBboxPatch, Polygon, Rectangle

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

TWO_PI = 2.0 * np.pi
GEAR_RATIO = 9.1


def wrap_2pi(angle: float) -> float:
    return float(angle % TWO_PI)


def run_encoder(power_cycle_at: float, end_turns: float = 3.0, samples: int = 1201):
    """Return rotor angle, single-turn angle and drive reading (in rad)."""
    rotor = np.linspace(0.0, end_turns * TWO_PI, samples)
    single = np.mod(rotor, TWO_PI)
    reading = np.where(rotor < power_cycle_at, rotor, single)
    # Before the power cycle the reading is theta0 + 2*pi*n = rotor itself.
    return rotor, single, reading


def draw_exploded() -> None:
    fig = plt.figure(figsize=(11.0, 7.2))
    gs = fig.add_gridspec(2, 1, height_ratios=[1.15, 1.0], hspace=0.30)
    ax = fig.add_subplot(gs[0])
    ax.set_xlim(0.0, 12.2)
    ax.set_ylim(-1.9, 1.9)
    ax.set_aspect("equal")
    ax.axis("off")
    ax.set_title("关节电机轴向爆炸示意（数字对应文中 12 个零件）", fontsize=11)

    # 1 cover, 2 output shaft, 3 carrier, 4 cross-roller bearing
    ax.add_patch(Rectangle((0.35, -0.62), 0.20, 1.24, facecolor="#cfd8dc",
                           edgecolor="0.25"))
    ax.add_patch(Rectangle((0.35, -0.28), 0.20, 0.56, facecolor="white",
                           edgecolor="0.25"))
    ax.text(0.45, 0.78, "1", ha="center", fontsize=9)
    ax.text(0.45, -0.86, "端盖", ha="center", va="top", fontsize=7.5, rotation=90)

    ax.add_patch(Rectangle((1.02, -0.10), 0.70, 0.20, facecolor="#b0bec5",
                           edgecolor="0.25"))
    ax.add_patch(Rectangle((1.02, -0.34), 0.16, 0.68, facecolor="#90a4ae",
                           edgecolor="0.25"))
    ax.text(1.40, 0.28, "2", ha="center", fontsize=9)
    ax.text(1.40, -0.50, "输出轴", ha="center", va="top", fontsize=7.5, rotation=90)

    ax.add_patch(FancyBboxPatch((1.95, -0.55), 0.18, 1.10,
                                boxstyle="round,pad=0.01,rounding_size=0.04",
                                facecolor="#b0bec5", edgecolor="0.25"))
    for y in (-0.38, 0.0, 0.38):
        ax.add_patch(Circle((2.22, y), 0.05, facecolor="#607d8b", edgecolor="0.25"))
    ax.text(2.04, 0.72, "3", ha="center", fontsize=9)
    ax.text(2.04, -0.78, "输出行星架", ha="center", va="top", fontsize=7.5, rotation=90)

    for r, col in ((0.66, "#90a4ae"), (0.48, "white"), (0.28, "#90a4ae")):
        ax.add_patch(Rectangle((2.90, -r), 0.34, 2 * r, facecolor=col,
                               edgecolor="0.25"))
    for y in np.linspace(-0.55, 0.55, 9):
        ax.add_patch(Circle((3.07, y), 0.035, facecolor="#455a64"))
    ax.text(3.07, 0.82, "4", ha="center", fontsize=9)
    ax.text(3.07, -0.88, "交叉滚子轴承", ha="center", va="top", fontsize=7.5,
            rotation=90)

    # 5 planet gears, 6 ring gear, 7 sun gear
    for y in (-0.48, 0.48):
        ax.add_patch(Circle((4.05, y), 0.24, facecolor="#b0bec5", edgecolor="0.25"))
        ax.add_patch(Circle((4.05, y), 0.09, facecolor="white", edgecolor="0.25"))
    ax.text(4.05, 0.86, "5", ha="center", fontsize=9)
    ax.text(4.05, -0.86, "行星轮", ha="center", va="top", fontsize=7.5, rotation=90)

    ax.add_patch(Rectangle((4.75, -0.72), 0.34, 1.44, facecolor="#90a4ae",
                           edgecolor="0.25"))
    ax.add_patch(Rectangle((4.75, -0.58), 0.34, 1.16, facecolor="white",
                           edgecolor="0.25"))
    for y in np.linspace(-0.68, 0.68, 11):
        ax.plot([4.75, 5.09], [y, y], color="0.45", lw=0.5)
    ax.text(4.92, 0.86, "6", ha="center", fontsize=9)
    ax.text(4.92, -0.86, "齿圈", ha="center", va="top", fontsize=7.5, rotation=90)

    ax.add_patch(Circle((5.72, 0.0), 0.26, facecolor="#ffcc80", edgecolor="0.25"))
    for a in np.linspace(0, TWO_PI, 12, endpoint=False):
        ax.plot([5.72 + 0.26 * np.cos(a), 5.72 + 0.32 * np.cos(a)],
                [0.26 * np.sin(a), 0.32 * np.sin(a)], color="0.35", lw=0.8)
    ax.text(5.72, 0.50, "7", ha="center", fontsize=9)
    ax.text(5.72, -0.52, "太阳轮", ha="center", va="top", fontsize=7.5, rotation=90)

    # 8 rotor, 9 driver board, 10 stator coils
    ax.add_patch(FancyBboxPatch((6.55, -0.58), 0.85, 1.16,
                                boxstyle="round,pad=0.01,rounding_size=0.06",
                                facecolor="#b0bec5", edgecolor="0.25"))
    for x in np.linspace(6.66, 7.28, 4):
        ax.add_patch(Rectangle((x, -0.50), 0.08, 1.00, facecolor="#ef9a9a",
                               edgecolor="none"))
    ax.text(6.98, 0.72, "8", ha="center", fontsize=9)
    ax.text(6.98, -0.78, "转子（永磁体）", ha="center", va="top", fontsize=7.5,
            rotation=90)

    ax.add_patch(Rectangle((7.85, -0.62), 0.14, 1.24, facecolor="#2e7d32",
                           edgecolor="0.25"))
    for y in (-0.30, 0.0, 0.30):
        ax.add_patch(Rectangle((7.90, y - 0.09), 0.20, 0.18, facecolor="0.15"))
    ax.text(7.92, 0.78, "9", ha="center", fontsize=9)
    ax.text(7.92, -0.82, "驱动板", ha="center", va="top", fontsize=7.5, rotation=90)

    for y in (-0.42, 0.0, 0.42):
        ax.add_patch(Ellipse((8.62, y), 0.30, 0.30, facecolor="#c5a880",
                             edgecolor="#6d4c41", lw=1.0))
    ax.add_patch(Rectangle((8.55, -0.60), 0.14, 1.20, facecolor="#d7ccc8",
                           edgecolor="0.3"))
    ax.text(8.62, 0.80, "10", ha="center", fontsize=9)
    ax.text(8.62, -0.86, "定子线圈", ha="center", va="top", fontsize=7.5, rotation=90)

    hexagon = Polygon([[9.12 + 0.16 * np.cos(a), 0.16 * np.sin(a)]
                       for a in np.linspace(np.pi / 6, TWO_PI + np.pi / 6, 7)],
                      closed=True, facecolor="#cfd8dc", edgecolor="0.25")
    ax.add_patch(hexagon)
    ax.add_patch(Circle((9.12, 0.0), 0.07, facecolor="white", edgecolor="0.25"))
    ax.text(9.12, 0.34, "11", ha="center", fontsize=9)
    ax.text(9.12, -0.36, "固定螺母", ha="center", va="top", fontsize=7.5, rotation=90)

    ax.add_patch(Rectangle((9.55, -0.78), 0.18, 1.56, facecolor="#eceff1",
                           edgecolor="0.25"))
    ax.add_patch(Rectangle((10.28, -0.78), 0.18, 1.56, facecolor="#eceff1",
                           edgecolor="0.25"))
    ax.plot([9.73, 10.28], [0.78, 0.78], color="0.35", lw=1.0)
    ax.plot([9.73, 10.28], [-0.78, -0.78], color="0.35", lw=1.0)
    ax.text(9.92, 0.92, "12", ha="center", fontsize=9)
    ax.text(9.92, -0.94, "外壳", ha="center", va="top", fontsize=7.5)

    ax.text(0.55, 1.62, "输出侧", fontsize=9, color="#1565c0")
    ax.text(7.60, 1.62, "转子侧", fontsize=9, color="#c62828")
    ax.annotate("", xy=(7.40, 1.48), xytext=(1.10, 1.48),
                arrowprops=dict(arrowstyle="<->", lw=0.8, color="0.4"))
    ax.text(4.25, 1.60, "减速比 9.1：转子 9.1 圈 → 输出 1 圈", ha="center",
            fontsize=8.2, color="0.3")

    # Gear-train inset.
    ax2 = fig.add_subplot(gs[1])
    ax2.set_aspect("equal")
    ax2.axis("off")
    ax2.set_xlim(-1.45, 1.45)
    ax2.set_ylim(-1.25, 1.25)
    ax2.set_title("行星轮系：太阳轮输入，行星架输出，齿圈固定", fontsize=10)
    ax2.add_patch(Circle((0, 0), 1.02, fill=False, lw=1.2, color="0.3"))
    ax2.add_patch(Circle((0, 0), 0.90, fill=False, lw=0.8, color="0.5", ls="--"))
    for a in np.linspace(0, TWO_PI, 48, endpoint=False):
        ax2.plot([0.90 * np.cos(a), 1.02 * np.cos(a)],
                 [0.90 * np.sin(a), 1.02 * np.sin(a)], color="0.5", lw=0.7)
    for i, a in enumerate(np.deg2rad([90, 210, 330])):
        cx, cy = 0.63 * np.cos(a), 0.63 * np.sin(a)
        ax2.add_patch(Circle((cx, cy), 0.30, facecolor="#b0bec5",
                             edgecolor="0.25"))
        ax2.add_patch(Circle((cx, cy), 0.08, facecolor="white", edgecolor="0.25"))
        ax2.text(cx, cy, f"P{i + 1}", ha="center", va="center", fontsize=7)
    ax2.add_patch(Circle((0, 0), 0.30, facecolor="#ffcc80", edgecolor="0.25"))
    ax2.text(0, 0, "S", ha="center", va="center", fontsize=9)
    ax2.text(0, -1.18, "齿圈 R 固定；太阳轮 S 由转子驱动；行星架 C 带动输出轴",
             ha="center", fontsize=8)
    ax2.annotate("", xy=(0.0, -0.55), xytext=(0.0, -0.32),
                 arrowprops=dict(arrowstyle="->", lw=1.0, color="#1565c0"))
    ax2.text(0.10, -0.50, "输入", fontsize=8, color="#1565c0")
    ax2.annotate("", xy=(0.95, 0.65), xytext=(0.78, 0.53),
                 arrowprops=dict(arrowstyle="->", lw=1.0, color="#c62828"))
    ax2.text(0.80, 0.72, "输出", fontsize=8, color="#c62828")

    savefig(fig, "2_2_exploded")


def draw_encoder(power_cycle_at: float) -> None:
    rotor, single, reading = run_encoder(power_cycle_at)
    turns_axis = rotor / TWO_PI
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(8.2, 6.2), sharex=True)

    ax1.plot(turns_axis, single / np.pi, color="#1f77b4", lw=1.8,
             label="单圈绝对读数 θ0")
    ax1.plot(turns_axis, rotor / np.pi, color="0.6", lw=1.0, ls="--",
             label="转子真实角度")
    ax1.axvline(power_cycle_at / TWO_PI, color="0.2", ls=":", lw=1.0)
    ax1.set_ylim(-0.15, 2.25)
    ax1.set_yticks([0, 1, 2])
    ax1.set_yticklabels(["0", "π", "2π"])
    ax1.set_ylabel("θ0 (π rad)")
    ax1.set_title("单圈绝对读数：每转一圈回到 0")
    ax1.grid(True, alpha=0.3)
    ax1.legend(fontsize=8, loc="upper left")

    ax2.plot(turns_axis, reading / np.pi, color="#d62728", lw=2.0,
             label="驱动器累计读数")
    ax2.plot(turns_axis, rotor / np.pi, color="0.6", lw=1.0, ls="--",
             label="转子真实角度")
    ax2.axvline(power_cycle_at / TWO_PI, color="0.2", ls=":", lw=1.0)
    ax2.axhline(2.3, color="0.3", ls=":", lw=0.8)
    ax2.axhline(0.3, color="0.3", ls=":", lw=0.8)
    ax2.plot([power_cycle_at / TWO_PI], [2.3], "o", color="#d62728", ms=5)
    ax2.plot([power_cycle_at / TWO_PI], [0.3], "o", color="#d62728", ms=5)
    ax2.annotate("断电前 2.3π", xy=(power_cycle_at / TWO_PI, 2.3),
                 xytext=(power_cycle_at / TWO_PI - 0.62, 3.05), fontsize=9,
                 arrowprops=dict(arrowstyle="->", lw=0.8))
    ax2.annotate("重启后 0.3π", xy=(power_cycle_at / TWO_PI, 0.3),
                 xytext=(power_cycle_at / TWO_PI + 0.10, 0.95), fontsize=9,
                 arrowprops=dict(arrowstyle="->", lw=0.8))
    ax2.text(power_cycle_at / TWO_PI + 0.03, 5.05, "断电重启", fontsize=9,
             rotation=90)
    ax2.set_xlabel("转子转过圈数")
    ax2.set_ylabel("累计读数 (π rad)")
    ax2.set_yticks(np.arange(0, 7, 1))
    ax2.set_yticklabels(["0", "π", "2π", "3π", "4π", "5π", "6π"])
    ax2.set_xlim(0, 3)
    ax2.set_ylim(-0.2, 6.5)
    ax2.grid(True, alpha=0.3)
    ax2.legend(fontsize=8, loc="upper left")
    savefig(fig, "2_2_encoder_wrap")


def main() -> None:
    power_cycle_at = 2.3 * np.pi
    single_before = wrap_2pi(power_cycle_at)
    n_before = int(np.floor(power_cycle_at / TWO_PI))
    print(f"断电点：转子 {power_cycle_at / TWO_PI:.4f} 圈 = {power_cycle_at / np.pi:.4f}π")
    print(f"断电前：单圈读数 {single_before / np.pi:.4f}π，圈数 n={n_before}，"
          f"累计读数 {(single_before + n_before * TWO_PI) / np.pi:.4f}π")
    print(f"断电后：单圈读数 {single_before / np.pi:.4f}π，圈数 n=0，"
          f"累计读数 {single_before / np.pi:.4f}π")
    print(f"读数变化：{-n_before * TWO_PI / np.pi:.4f}π（圈数丢失，单圈角度不变）")

    end = 3.0 * TWO_PI
    print(f"末态：转子 3.0000 圈，单圈读数 {wrap_2pi(end) / np.pi:.4f}π，"
          f"重启后累计读数 {wrap_2pi(end) / np.pi:.4f}π")
    print(f"输出轴角度：转子 {end / np.pi:.4f}π / {GEAR_RATIO} = {end / GEAR_RATIO:.4f} rad")

    draw_exploded()
    draw_encoder(power_cycle_at)


if __name__ == "__main__":
    main()
