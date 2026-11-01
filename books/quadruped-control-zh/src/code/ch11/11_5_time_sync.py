"""Time sync, point-cloud topics, DDS and startup checklist for the real Go1 (11.5).

Nothing here touches the system clock.  The script parses the T08 sample of
``timedatectl``, reads two hypothetical ``date +%s.%N`` samples, computes the
clock offset, evaluates a ROS 2/DDS checklist and prints pass/fail.  It also
draws the network/TF topology (Fig. 11-9) and the two ``timedatectl`` outputs
(Fig. 11-10).

The T08 values are used verbatim; the user machine in Fig. 11-10 is a second,
synthetic sample in Europe/Berlin to show that UTC can agree while local time
differs.  The IP addresses follow the original's 192.168.123.0/24 network.

expect: T08
expect: 两机时差
expect: NTP
expect: 检查清单
expect: 结论
expect: 图 11-9
"""

from __future__ import annotations

import logging
import math
import os
import re
import warnings
from datetime import datetime, timedelta, timezone

os.environ.setdefault("MPLCONFIGDIR", "/tmp/quadruped-mpl")

import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch, Rectangle

from quadruped.plotting import label, savefig

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)
logging.getLogger("matplotlib").setLevel(logging.ERROR)
warnings.filterwarnings("ignore", message=".*glyph.*")
warnings.filterwarnings("ignore", message=".*tight_layout.*")
plt.rcParams["mathtext.fontset"] = "dejavusans"

T08 = {
    "Local time": "四 2021-09-30 14:54:20 CST",
    "Universal time": "四 2021-09-30 06:54:20 UTC",
    "RTC time": "四 2021-09-30 06:54:20",
    "Time zone": "Asia/Shanghai (CST, +0800)",
    "System clock synchronized": "yes",
    "systemd-timesyncd.service active": "yes",
    "RTC in local TZ": "no",
}


def parse_stamp(text: str) -> datetime:
    match = re.search(r"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})", text)
    if match is None:
        raise ValueError(f"no timestamp in {text!r}")
    return datetime.strptime(match.group(1), "%Y-%m-%d %H:%M:%S")


def draw_network() -> None:
    fig = plt.figure(figsize=(12.4, 5.4))
    gs = fig.add_gridspec(1, 2, width_ratios=[1.2, 1.0], wspace=0.12)
    ax = fig.add_subplot(gs[0, 0])
    ax.set_xlim(0, 12)
    ax.set_ylim(0, 8)
    ax.axis("off")

    def box(x, y, w, h, text, color):
        ax.add_patch(FancyBboxPatch((x - w / 2, y - h / 2), w, h,
                                    boxstyle="round,pad=0.06,rounding_size=0.10",
                                    fc=color, ec="0.3", lw=1.0))
        ax.text(x, y, text, ha="center", va="center", fontsize=8)

    def arrow(x0, y0, x1, y1, text, color="0.25", ls="-", tx=None, ty=None):
        ax.add_patch(FancyArrowPatch((x0, y0), (x1, y1), arrowstyle="-|>",
                                     mutation_scale=10, lw=1.2, color=color,
                                     linestyle=ls))
        if text:
            ax.text(tx if tx is not None else (x0 + x1) / 2,
                    ty if ty is not None else (y0 + y1) / 2 + 0.15,
                    text, fontsize=7.2, color=color, ha="center")

    box(3.0, 7.0, 4.2, 0.75,
        "用户计算机 192.168.123.162\nNav2 / RViz2 / 时钟", "#dbe9f6")
    box(9.5, 7.0, 3.6, 0.75,
        "交换机 192.168.123.1\n同一网段、多播可达", "#eeeeee")
    box(9.5, 4.9, 4.0, 1.1,
        "Go1 树莓派 192.168.123.161\njunior_ctrl / TF / 时间同步", "#fde2cf")
    box(4.5, 4.6, 2.6, 0.75, "头部 Jetson\n192.168.123.163", "#d9ead3")
    box(4.5, 3.2, 2.6, 0.75, "左侧 Jetson\n192.168.123.164", "#d9ead3")
    box(4.5, 1.8, 2.6, 0.75, "右侧 Jetson\n192.168.123.165", "#d9ead3")
    box(3.0, 5.6, 2.4, 0.65, "NTP / chrony\nUTC", "#e6d7f0")

    arrow(5.2, 7.0, 7.6, 7.0, "以太网", tx=6.4, ty=7.15)
    arrow(9.5, 6.6, 9.5, 5.45, "网线", tx=9.75, ty=6.0)
    # Point-cloud bus: three Jetsons -> vertical bus -> switch -> user PC.
    ax.plot([7.4, 7.4], [1.8, 6.7], color="tab:green", lw=1.6)
    for y in (4.6, 3.2, 1.8):
        arrow(5.8, y, 7.35, y, "", color="tab:green")
    ax.text(0.3, 1.35, "绿线：PointCloud2（ROS 2 / DDS）", fontsize=7.5,
            color="tab:green")
    arrow(7.4, 6.7, 8.3, 6.95, "", color="tab:green")
    arrow(4.6, 6.65, 8.0, 5.55, "/cmd_vel", color="tab:red",
          tx=6.3, ty=6.35)
    arrow(3.0, 5.25, 3.0, 6.6, "", color="0.4", ls="--")
    ax.text(0.2, 7.85, "(a) 实机网络：用户机、树莓派、3 台 Jetson、控制器",
            fontsize=9.5)
    ax.text(0.3, 0.55, "T06 的 5 路点云从 Jetson 经交换机发到用户机；"
                       " /cmd_vel 从用户机发到树莓派上的控制器",
            fontsize=7.5, color="0.35")

    ax = fig.add_subplot(gs[0, 1])
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 8)
    ax.axis("off")

    def node(x, y, text, color="#d9ead3", w=2.2, h=0.6):
        ax.add_patch(FancyBboxPatch((x - w / 2, y - h / 2), w, h,
                                    boxstyle="round,pad=0.05,rounding_size=0.08",
                                    fc=color, ec="0.3", lw=1.0))
        ax.text(x, y, text, ha="center", va="center", fontsize=8)

    node(2.0, 6.7, "odom", "#dbe9f6")
    node(5.0, 6.7, "base", "#d9ead3")
    node(8.2, 7.3, "front_cam", "#fff2cc", w=2.0)
    node(8.2, 6.1, "left/right_cam", "#fff2cc", w=2.2)
    node(8.2, 4.9, "chin/rearDown", "#fff2cc", w=2.2)
    for y in (7.3, 6.1, 4.9):
        ax.add_patch(FancyArrowPatch((6.1, 6.7), (7.1, y), arrowstyle="-|>",
                                     mutation_scale=9, lw=1.0, color="0.35"))
    ax.annotate("", xy=(4.0, 6.7), xytext=(3.1, 6.7),
                arrowprops=dict(arrowstyle="-|>", color="0.35", lw=1.2))
    ax.text(3.55, 7.15, "50 Hz", fontsize=7, ha="center")
    ax.text(1.0, 7.55, "(b) TF 树与频率", fontsize=9.5)
    ax.text(1.0, 3.8, "odom → base：估计器，50 Hz\n"
                       "base → 传感器：静态 TF\n"
                       "base → 连杆：状态发布器，50–100 Hz\n"
                       "时钟差 < 100 ms，否则 TF 过期",
            fontsize=8, va="top")
    ax.text(1.0, 2.0, "没有 map → odom：本书不用 amcl\n"
                       "RViz2 Fixed Frame 设 odom",
            fontsize=8, va="top", color="0.35")
    fig.tight_layout()
    savefig(fig, "11_5_network")
    plt.close(fig)


def draw_timedatectl() -> None:
    user_local = "四 2021-09-30 08:54:20 CEST"
    lines_robot = [
        "bian@go1:~$ timedatectl",
        f"Local time: {T08['Local time']}",
        f"Universal time: {T08['Universal time']}",
        f"RTC time: {T08['RTC time']}",
        f"Time zone: {T08['Time zone']}",
        "System clock synchronized: yes",
        "systemd-timesyncd.service active: yes",
        "RTC in local TZ: no",
    ]
    lines_user = [
        "lab@pc:~$ timedatectl",
        f"Local time: {user_local}",
        f"Universal time: {T08['Universal time']}",
        "RTC time: 四 2021-09-30 06:54:20",
        "Time zone: Europe/Berlin (CEST, +0200)",
        "System clock synchronized: yes",
        "systemd-timesyncd.service active: yes",
        "RTC in local TZ: no",
    ]
    fig, axes = plt.subplots(1, 2, figsize=(12.6, 4.6))
    fig.patch.set_facecolor("#111111")
    for ax, lines, title in ((axes[0], lines_robot, "Go1 树莓派（T08）"),
                             (axes[1], lines_user, "用户计算机（同一 UTC）")):
        ax.set_facecolor("#0b0b0b")
        ax.set_xlim(0, 1)
        ax.set_ylim(0, 1)
        ax.axis("off")
        ax.set_title(title, color="white", fontsize=10)
        for i, line in enumerate(lines):
            y = 0.92 - i * 0.115
            color = "#d8d8d8"
            if line.startswith("Universal time") or \
                    line.startswith("System clock synchronized"):
                color = "#ffe08a"
            if line.startswith("Local time"):
                color = "#9fd3ff"
            ax.text(0.02, y, line, fontsize=8.2, family="WenQuanYi Zen Hei Mono",
                    color=color, va="top")
        # Highlight the two lines that must agree / are easy to misread.
        for i, line in enumerate(lines):
            if line.startswith("Universal time"):
                ax.add_patch(Rectangle((0.005, 0.92 - i * 0.115 - 0.075),
                                       0.99, 0.095, fill=False,
                                       edgecolor="#ffe08a", lw=1.0))
        ax.text(0.02, 0.02, "UTC 相同；Local time 的差别只来自 Time zone",
                fontsize=7.5, color="#9fd3ff", va="bottom")
    fig.suptitle("timedatectl：同步看 UTC，不看本地时区", color="white",
                 fontsize=11)
    fig.tight_layout()
    savefig(fig, "11_5_timedatectl")
    plt.close(fig)


def main() -> None:
    print("=== T08 解析 ===")
    local = parse_stamp(T08["Local time"])
    utc = parse_stamp(T08["Universal time"])
    tz = re.search(r"([+-]\d{4})", T08["Time zone"])
    offset_h = (local - utc).total_seconds() / 3600.0
    print(f"Local time: {local}（{T08['Local time'].split()[-1]}）")
    print(f"Universal time: {utc}（UTC）")
    print(f"时区偏移: {tz.group(1) if tz else 'n/a'}，"
          f"Local - UTC = {offset_h:.3f} h")
    print(f"System clock synchronized: {T08['System clock synchronized']}；"
          f"时间同步服务: {T08['systemd-timesyncd.service active']}；"
          f"RTC in local TZ: {T08['RTC in local TZ']}")

    print("=== 两机时差 ===")
    user_epoch = 1696060460.250000
    robot_epoch = 1696060460.207500
    delta_ms = (robot_epoch - user_epoch) * 1000.0
    print(f"用户机 date +%s.%N: {user_epoch:.6f}")
    print(f"Go1    date +%s.%N: {robot_epoch:.6f}")
    print(f"Go1 - 用户机 = {delta_ms:+.1f} ms，阈值 100 ms -> "
          f"{'通过' if abs(delta_ms) < 100 else '不通过'}")
    bad_ms = 180.0
    print(f"反例：Go1 比用户机快 {bad_ms:.1f} ms -> "
          f"{'通过' if abs(bad_ms) < 100 else '不通过'}")

    print("=== NTP 偏移 ===")
    T1, T2, T3, T4 = 1000.000, 1000.010, 1000.020, 1000.038
    theta = ((T2 - T1) + (T3 - T4)) / 2.0
    delay = ((T4 - T1) - (T3 - T2)) / 2.0
    print(f"T1={T1:.3f} T2={T2:.3f} T3={T3:.3f} T4={T4:.3f} s")
    print(f"偏移 theta = {theta * 1000:+.1f} ms，往返时延 = {delay * 1000:.1f} ms")

    print("=== ROS 2 / DDS 检查清单 ===")
    # ANCHOR: checks
    checks = [
        ("同一网段 192.168.123.0/24", True),
        ("ping 192.168.123.161", True),
        ("ROS_DOMAIN_ID=42（用户机与机器人一致）", True),
        ("ROS_LOCALHOST_ONLY=0", True),
        ("多播 239.255.0.1 可达", True),
        ("防火墙放行 DDS", True),
        ("Go1 与用户机时差 < 100 ms", False),
        ("odom → base TF 存在", True),
    ]
    # ANCHOR_END: checks
    passed = 0
    for name, ok in checks:
        passed += int(ok)
        print(f"[{'通过' if ok else '不通过'}] {name}")
    print(f"结论：{passed} 通过，{len(checks) - passed} 不通过 -> "
          f"{'通过' if passed == len(checks) else '不通过'}")
    checks[-2] = (checks[-2][0], True)
    print("修正时差后：8 通过，0 不通过 -> 通过")

    draw_network()
    draw_timedatectl()
    print("图 11-9 实机导航的网络与 TF；图 11-10 timedatectl 输出")


if __name__ == "__main__":
    main()
