"""Offline deployment checklist for A1/Go1 (10.6).

Nothing here talks to a robot.  Given a model and a source path, the script
prints the CMake options, the scp/ssh commands, the sport-mode shutdown steps
and the autostart enable/disable commands.  It then reports four local checks:
architecture, build dependencies, real-time permission and autostart.  A
"待机载确认" result is normal: the path, the vendor process and the autostart
entry only exist on the robot's computer.

The book's options replace the original six CMake switches:
``PLATFORM`` -> ``QUADRUPED_ARCH``, ``CATKIN_MAKE`` -> ``QUADRUPED_BACKEND``,
``SIMULATION``/``REAL_ROBOT`` -> ``QUADRUPED_BACKEND``, ``ROBOT`` ->
``QUADRUPED_ROBOT``; ``DEBUG``/``MOVE_BASE`` stay as ordinary CMake options.

expect: 部署检查
expect: 架构
expect: 依赖
expect: 权限
expect: 自启
expect: 没有执行任何远程命令
"""

from __future__ import annotations

import argparse
import logging
import os
import platform
import shutil
import warnings

from quadruped.plotting import label, savefig
import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch

logging.getLogger("matplotlib").setLevel(logging.ERROR)
warnings.filterwarnings("ignore", message=".*findfont.*")

ROBOTS = {
    "a1": {"name": "A1", "arch": "amd64", "user": "unitree",
           "home": "/home/unitree", "sport": "A1_sport",
           "autostart": "gnome-session-properties（取消 A1-Sport）",
           "restore": "重新勾选 A1-Sport，重启"},
    "go1": {"name": "Go1", "arch": "arm64", "user": "pi",
            "home": "/home/pi", "sport": "Legged_sport",
            "autostart": "cd ~/Unitree/autostart && mv 02sportMode sportMode",
            "restore": "cd ~/Unitree/autostart && mv sportMode 02sportMode，重启"},
}


def host_arch() -> str:
    machine = platform.machine().lower()
    if machine in ("x86_64", "amd64"):
        return "amd64"
    if machine in ("aarch64", "arm64"):
        return "arm64"
    return machine


def fifo_probe() -> tuple[bool, str]:
    if not hasattr(os, "sched_setscheduler"):
        return False, "当前平台没有 sched_setscheduler"
    try:
        prio = os.sched_get_priority_max(os.SCHED_FIFO)
        os.sched_setscheduler(0, os.SCHED_FIFO, os.sched_param(prio))
    except (PermissionError, OSError) as exc:
        return False, f"本机不可用（errno={getattr(exc, 'errno', None)}）"
    try:
        os.sched_setscheduler(0, os.SCHED_OTHER, os.sched_param(0))
    except OSError:
        pass
    return True, "本机可用（已恢复普通策略）"


# ANCHOR: checks
def checks(robot: str, path: str) -> dict[str, str]:
    cfg = ROBOTS[robot]
    out = {}
    h = host_arch()
    if h == cfg["arch"]:
        out["架构"] = f"PASS：本机 {h} 与 {cfg['name']} 目标 {cfg['arch']} 一致，可在本机编译"
    elif h in ("amd64", "arm64"):
        out["架构"] = (f"WARN：本机 {h}，目标 {cfg['arch']}；需要交叉工具链，"
                       f"或直接在 {cfg['name']} 的机载计算机上编译")
    else:
        out["架构"] = f"WARN：本机 {h}，目标 {cfg['arch']}，请确认工具链"

    tools = {name: shutil.which(name) for name in ("cmake", "make", "c++", "clang++")}
    eigen = os.path.exists("/usr/include/eigen3/Eigen/Core")
    missing = [k for k, v in tools.items() if v is None]
    dep = []
    dep.append("cmake/编译器齐全" if not missing else "缺少 " + ",".join(missing))
    dep.append("Eigen3 头文件存在" if eigen else "未找到 /usr/include/eigen3/Eigen/Core")
    out["依赖"] = ("PASS：" if not missing and eigen else "WARN：") + "；".join(dep)

    euid = os.geteuid()
    ok, msg = fifo_probe()
    perm = f"euid={euid}（root）" if euid == 0 else f"euid={euid}（普通用户）"
    out["权限"] = f"{'PASS' if ok else 'WARN'}：{perm}；SCHED_FIFO {msg}"

    sport_path = (os.path.expanduser("~/Unitree/autostart/02sportMode")
                  if robot == "go1" else "")
    exists = os.path.exists(sport_path) if sport_path else False
    out["自启"] = ("待机载确认：登录机载计算机后先 ps 查 "
                   f"{cfg['sport']}，再执行 {cfg['autostart']}")
    if sport_path and exists:
        out["自启"] += "；本机也看到了 02sportMode"
    return out
# ANCHOR_END: checks


def draw_flow() -> None:
    fig, ax = plt.subplots(figsize=(10.2, 4.6))
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 10)
    ax.axis("off")

    def box(x, y, w, h, text, fc="white", ec="0.25", fs=8.2):
        ax.add_patch(FancyBboxPatch((x, y), w, h,
                                    boxstyle="round,pad=0.05,rounding_size=0.08",
                                    fc=fc, ec=ec, lw=1.0, zorder=3))
        ax.text(x + w / 2, y + h / 2, text, ha="center", va="center",
                fontsize=fs, linespacing=1.35, zorder=4)

    def arrow(x0, y0, x1, y1, color="0.3", ls="-"):
        ax.add_patch(FancyArrowPatch((x0, y0), (x1, y1), arrowstyle="-|>",
                                     mutation_scale=10, color=color, lw=1.1,
                                     linestyle=ls, zorder=2, shrinkA=1, shrinkB=1))

    box(0.15, 7.4, 2.05, 1.25, "PC：编译/自测\ncmake -D...\nmake -j2", fc="#e8f1fb")
    box(2.65, 7.4, 2.05, 1.25, "scp -r\n代码 → 机载 Home\n（或 U 盘）", fc="#e8f1fb")
    box(5.15, 7.4, 2.15, 1.25, "机载：改选项\nBACKEND=real\nROBOT=a1/go1", fc="#fff4e6")
    box(7.75, 7.4, 2.05, 1.25, "关运动模式\nA1: 启动项\nGo1: 改文件夹", fc="#fde8e8")
    arrow(2.22, 8.02, 2.60, 8.02)
    arrow(4.72, 8.02, 5.10, 8.02)
    arrow(7.32, 8.02, 7.70, 8.02)

    box(7.75, 4.55, 2.05, 1.25, "重启并确认\nps 无 sport\n再登录", fc="#fde8e8")
    box(5.15, 4.55, 2.15, 1.25, "机载编译\nmkdir build bin\ncmake .. && make -j2", fc="#e8f1fb")
    box(2.65, 4.55, 2.05, 1.25, "sudo ./bin/junior_ctrl\n确认 PR=FF/rt\n拔掉网线/HDMI", fc="#eef7ea")
    box(0.15, 4.55, 2.05, 1.25, "手柄低速走\n吊架/低平台\n手边急停", fc="#eef7ea")
    arrow(8.77, 7.35, 8.77, 5.85)
    arrow(7.70, 5.17, 7.35, 5.17)
    arrow(5.10, 5.17, 4.75, 5.17)
    arrow(2.60, 5.17, 2.25, 5.17)

    box(2.65, 1.55, 5.0, 1.2, "可选：加入开机自启\n先确认能独立跑，再取消自启才能回到网线调试",
        fc="#f2f2f2", fs=8.0)
    arrow(3.65, 4.50, 3.65, 2.80)
    ax.text(3.72, 3.55, "稳定后", fontsize=7.5, color="0.35")

    ax.text(5.0, 9.65, label("机载部署流程（全部需要实机）", "on-board deployment"),
            ha="center", va="center", fontsize=11)
    ax.text(5.0, 0.55, "先阻尼趴下，再关机/接线；UP Board 先接 HDMI 再上电",
            ha="center", va="center", fontsize=8, color="0.3")
    savefig(fig, "10_7_deploy")
    plt.close(fig)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--robot", choices=sorted(ROBOTS), default="a1")
    parser.add_argument("--path", default="~/unitree_guide")
    args = parser.parse_args()
    cfg = ROBOTS[args.robot]
    path = os.path.expanduser(args.path)
    host = "192.168.123.161" if args.robot == "a1" else "192.168.123.13"

    print("=== 10.6 机载部署检查 ===")
    print(f"机型：{cfg['name']}；目标架构 {cfg['arch']}；机载用户 {cfg['user']}；"
          f"Home {cfg['home']}")
    print(f"代码路径：{path}（{'存在' if os.path.exists(path) else '本机未找到，仅生成命令'}）")
    print("\nCMake 配置（本书选项，替代原书六个开关）：")
    print(f"  cmake -S . -B build -DQUADRUPED_BACKEND=real "
          f"-DQUADRUPED_ROBOT={args.robot} -DQUADRUPED_ARCH={cfg['arch']} "
          f"-DCMAKE_BUILD_TYPE=Release")
    print(f"  cmake --build build -j2")
    print("\n复制与登录：")
    print(f"  scp -r {path} {cfg['user']}@{host}:{cfg['home']}/")
    print(f"  ssh {cfg['user']}@{host}")
    print("\n关闭运动模式（在机载计算机上）：")
    print(f"  ps aux | grep {cfg['sport']} | grep -v grep")
    print(f"  {cfg['autostart']}")
    print(f"  重启：sudo reboot；恢复：{cfg['restore']}")
    print("\n编译与运行：")
    print("  cd ~/unitree_guide && mkdir -p build bin")
    print("  cd build && cmake .. && make -j2")
    print("  sudo ./bin/junior_ctrl    # 需要实时权限")
    print("\n开机自启：")
    print("  A1：把启动项加到 gnome-session-properties；Go1：在 ~/Unitree/autostart 下新建目录和同名 .sh")
    print("  回到网线调试前必须先取消自启，否则两个控制器会同时发关节命令")

    print("\n四项本地检查：")
    result = checks(args.robot, path)
    for name in ("架构", "依赖", "权限", "自启"):
        print(f"  {name}：{result[name]}")
    print("\n没有执行任何远程命令；所有 scp/ssh/ps/mv 都需要在实机上手动执行。")
    draw_flow()
    print("图 10-7 已保存：10_7_deploy.png")


if __name__ == "__main__":
    main()
