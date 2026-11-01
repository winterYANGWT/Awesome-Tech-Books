# expect: A1 遥控手柄：8 个键位
# expect: 1  右摇杆
# expect: 8  START
# expect: <L2\+B>        阻尼（被动）
# expect: <L1\+START>    运动模式
# expect: <L2\+A>        固定站立
# expect: <L2\+Y>        导航模式
"""Draw the A1 joystick key map and print the eight keys / six operations.

No hardware is opened.  The key names follow the eight numbered parts of the
original figure; the two sticks are listed as operations because they are not
button combinations.  The last two lines of the output say explicitly that the
firmware decides the exact behaviour, which is why the book re-checks every
combination on the real machine in Chapters 9-10.
"""

from __future__ import annotations

import logging
import warnings

from quadruped.plotting import label, savefig

import matplotlib.pyplot as plt
from matplotlib.patches import Circle, Ellipse, FancyBboxPatch, Polygon

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)
warnings.filterwarnings("ignore", message=".*Glyph.*")

# ANCHOR: keys
KEYS = {
    1: ("右摇杆", "前后 / 侧向平移"),
    2: ("左摇杆", "偏航 / 转向"),
    3: ("R1/R2", "右肩键：组合用"),
    4: ("L1/L2", "左肩键：组合用"),
    5: ("左方向键", "模式 / 参数选择"),
    6: ("右字母键", "A/B/X/Y 确认"),
    7: ("SELECT", "选择 / 切换"),
    8: ("START", "启动 / 停止"),
}

OPERATIONS = [
    ("<L2+B>", "阻尼（被动）"),
    ("<L1+START>", "运动模式"),
    ("<L2+A>", "固定站立"),
    ("<L2+Y>", "导航模式（仅 Go1 教育版）"),
    ("右摇杆", "前后 / 侧向速度"),
    ("左摇杆", "偏航角速度"),
]
# ANCHOR_END: keys


def draw_stick(ax, cx, cy, r=0.40):
    ax.add_patch(Circle((cx, cy), r, fc="#dddddd", ec="#333333", lw=1.6,
                        zorder=5))
    ax.add_patch(Circle((cx, cy), 0.22, fc="#666666", ec="#333333", lw=1.2,
                        zorder=6))
    ax.plot([cx - 0.14, cx + 0.14], [cy, cy], color="#222222", lw=1.0,
            zorder=7)
    ax.plot([cx, cx], [cy - 0.14, cy + 0.14], color="#222222", lw=1.0,
            zorder=7)


def draw_dpad(ax, cx, cy):
    arm, w = 0.34, 0.15
    ax.add_patch(Polygon([[cx - w / 2, cy - arm], [cx + w / 2, cy - arm],
                          [cx + w / 2, cy - w / 2], [cx + arm, cy - w / 2],
                          [cx + arm, cy + w / 2], [cx + w / 2, cy + w / 2],
                          [cx + w / 2, cy + arm], [cx - w / 2, cy + arm],
                          [cx - w / 2, cy + w / 2], [cx - arm, cy + w / 2],
                          [cx - arm, cy - w / 2], [cx - w / 2, cy - w / 2]],
                         closed=True, fc="#dce9f7", ec="#1f5fa8", lw=1.4,
                         zorder=5))
    ax.text(cx, cy, "+", ha="center", va="center", fontsize=11,
            color="#1f5fa8", zorder=6)


def draw_abxy(ax, cx, cy):
    for dx, dy, letter in ((0, -0.31, "A"), (0.31, 0, "B"),
                           (-0.31, 0, "X"), (0, 0.31, "Y")):
        ax.add_patch(Circle((cx + dx, cy + dy), 0.20, fc="#f4f4f4",
                            ec="#333333", lw=1.2, zorder=5))
        ax.text(cx + dx, cy + dy, letter, ha="center", va="center",
                fontsize=8, zorder=6)


def badge(ax, number, xy, target):
    ax.annotate("", xy=target, xytext=xy,
                arrowprops=dict(arrowstyle="-", color="#777777", lw=1.0,
                                shrinkA=8, shrinkB=3), zorder=9)
    ax.add_patch(Circle(xy, 0.21, fc="#d2691e", ec="white", lw=1.0,
                        zorder=10))
    ax.text(xy[0], xy[1], str(number), ha="center", va="center",
            fontsize=9, color="white", zorder=11)


def key_map():
    fig, ax = plt.subplots(figsize=(13.4, 8.6))
    ax.set_xlim(0, 13.4)
    ax.set_ylim(-0.05, 9.0)
    ax.axis("off")

    # grips behind the body
    ax.add_patch(Ellipse((2.65, 2.65), 1.35, 2.9, angle=18,
                         fc="#e6e6e6", ec="#333333", lw=1.6, zorder=1))
    ax.add_patch(Ellipse((7.35, 2.65), 1.35, 2.9, angle=-18,
                         fc="#e6e6e6", ec="#333333", lw=1.6, zorder=1))
    ax.add_patch(FancyBboxPatch(
        (2.35, 1.55), 5.3, 5.25,
        boxstyle="round,pad=0.12,rounding_size=0.65",
        fc="#f7f7f7", ec="#333333", lw=2.0, zorder=2))

    # shoulder buttons
    for x0, name in ((2.55, "L1/L2"), (5.85, "R1/R2")):
        ax.add_patch(FancyBboxPatch(
            (x0, 6.88), 1.6, 0.34,
            boxstyle="round,pad=0.03,rounding_size=0.12",
            fc="#fdf1e3", ec="#d2691e", lw=1.4, zorder=3))
        ax.text(x0 + 0.8, 7.05, name, ha="center", va="center",
                fontsize=8.5, color="#b04f0a", zorder=4)

    draw_stick(ax, 3.8, 5.45)
    draw_stick(ax, 6.2, 5.45)
    draw_dpad(ax, 3.8, 3.2)
    draw_abxy(ax, 6.2, 3.2)

    for x, name in ((4.75, "SELECT"), (5.95, "START")):
        ax.add_patch(FancyBboxPatch(
            (x - 0.40, 1.83), 0.80, 0.24,
            boxstyle="round,pad=0.02,rounding_size=0.08",
            fc="#eeeeee", ec="#333333", lw=1.0, zorder=5))
        ax.text(x, 1.95, name, ha="center", va="center", fontsize=6.5,
                zorder=6)

    ax.text(5.0, 8.05, label("A1 遥控手柄（俯视）", "A1 joystick (top view)"),
            ha="center", fontsize=12)

    badge(ax, 1, (8.35, 5.65), (6.58, 5.45))
    badge(ax, 2, (1.60, 5.65), (3.42, 5.45))
    badge(ax, 3, (8.05, 7.45), (7.45, 7.05))
    badge(ax, 4, (1.90, 7.45), (2.55, 7.05))
    badge(ax, 5, (1.55, 3.20), (3.44, 3.20))
    badge(ax, 6, (8.35, 3.20), (6.55, 3.20))
    badge(ax, 7, (4.15, 1.15), (4.45, 1.85))
    badge(ax, 8, (6.55, 1.15), (6.05, 1.85))

    # key table
    left, middle, right = 9.15, 9.75, 11.15
    top = 8.25
    ax.text(left, top, "编号", fontsize=8.5, fontweight="normal")
    ax.text(middle, top, "键位", fontsize=8.5)
    ax.text(right, top, "主要作用", fontsize=8.5)
    ax.plot([9.05, 13.25], [8.05, 8.05], color="#999999", lw=0.9)
    for i, (name, func) in KEYS.items():
        y = 7.70 - 0.53 * (i - 1)
        ax.text(left, y, str(i), fontsize=8.2, color="#d2691e")
        ax.text(middle, y, name, fontsize=8.2)
        ax.text(right, y, func, fontsize=7.9)

    ax.text(left, 3.15, "常用操作（前 4 组为组合键）", fontsize=9.2,
            color="#1f5fa8")
    for k, (combo, func) in enumerate(OPERATIONS):
        y = 2.72 - 0.40 * k
        ax.text(left, y, combo, fontsize=8.0, color="#b04f0a")
        ax.text(10.55, y, func, fontsize=7.9)
    ax.text(left, 0.42, "具体映射以实机固件版本为准；\n第一次上电先吊起或放在低处。",
            fontsize=7.0, color="#666666", va="top")

    savefig(fig, "1_3_joystick")
    plt.close(fig)


def main():
    print("A1 遥控手柄：8 个键位（不连接硬件）")
    for i, (name, func) in KEYS.items():
        print(f"{i}  {name:<9s}  {func}")
    print("常用操作（前 4 组为组合键）:")
    for combo, func in OPERATIONS:
        print(f"{combo:<12s}  {func}")
    print("注: 组合键的具体行为以实机固件版本为准；")
    print("    第一次上电必须吊起或放在低处，手边保留阻尼组合键。")
    key_map()
    print("图已保存: figs/1_3_joystick.png")


if __name__ == "__main__":
    main()
