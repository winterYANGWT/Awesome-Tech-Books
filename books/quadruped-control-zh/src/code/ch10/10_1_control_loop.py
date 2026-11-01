"""Call order and data shapes of one 2 ms walking-control cycle (10.1).

The pipeline is exercised with synthetic numbers, not with MuJoCo.  Each
stage receives exactly the array that the previous stage would produce, so a
wrong order shows up as a shape mismatch or a missing input.  Wall-clock time
is not printed: to keep the output reproducible, every stage carries a fixed
*time budget* in microseconds instead.  Section 10.4 measures the real cost on
the simulator.

The same script draws Fig. 10-1, the two-chain data flow: the support chain
(pose -> acceleration -> foot forces -> joint torque) and the swing chain
(foothold -> cycloid -> foot target -> joint angles), merging at the joint
command.

expect: 调用顺序
expect: 数据形状
expect: 总预算
expect: 支撑链与摆动链在
"""

from __future__ import annotations

import logging

import numpy as np

from quadruped.plotting import label, savefig
import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

DT = 0.002
LEGS = ["FR", "FL", "RR", "RL"]
COLORS = ["C0", "C1", "C2", "C3"]

# name, synthetic budget in microseconds, input shape, output shape
STAGES = [
    ("读 IMU/编码器/接触", 40, "(4,) + (12,) + (12,) + (4,)", "(3,) (3,) (3,3) (12,)"),
    ("估计机身与足端状态", 180, "(3,) (3,) (3,3) (12,) (4,)",
     "(3,) (3,) (3,3) (4,3) (4,3)"),
    ("读手柄并转到世界系", 12, "(3,) (3,3)", "(3,) (3,)"),
    ("积分目标位姿", 18, "(3,) (3,) (1,) (3,)", "(3,) (3,) (1,)"),
    ("步态相位与落脚点", 62, "(4,) (3,) (3,) (1,)", "(4,) (4,) (4,3) (4,3)"),
    ("支撑腿 QP", 520, "(6,12) (6,) (4,3) (4,)", "(4,3) (6,12)"),
    ("摆动腿笛卡尔 PD", 28, "(4,3) (4,3) (4,3) (4,)", "(4,3)"),
    ("J^T 映射关节力矩", 45, "(12,) (3,3) (4,3) (4,)", "(12,)"),
    ("逆运动学", 118, "(4,3) (3,3) (4,)", "(12,) (12,)"),
    ("写 LowCmd", 15, "(12,)×5", "(12,)"),
]


SHAPES = {
    "quat": (4,), "q": (12,), "dq": (12,), "contact": (4,),
    "p": (3,), "v": (3,), "R": (3, 3), "feet": (4, 3), "feet_vel": (4, 3),
    "v_body": (3,), "v_world": (3,), "yaw": (1,),
    "phase": (4,), "feet_goal": (4, 3),
    "A": (6, 12), "b": (6,), "f_stance": (4, 3), "f_swing": (4, 3),
    "tau": (12,), "q_des": (12,), "dq_des": (12,), "kp": (12,), "kd": (12,),
}

# ANCHOR: pipeline
# stage -> (reads, writes).  A missing producer is a KeyError, exactly like
# calling a downstream function before its input exists in C++.
DEPS = {
    "读 IMU/编码器/接触": ((), ("quat", "q", "dq", "contact")),
    "估计机身与足端状态": (("quat", "q", "dq", "contact"),
                          ("p", "v", "R", "feet", "feet_vel")),
    "读手柄并转到世界系": (("R",), ("v_body", "v_world")),
    "积分目标位姿": (("v_world", "p"), ("p", "yaw")),
    "步态相位与落脚点": (("contact", "p", "yaw", "v_world"),
                        ("phase", "contact", "feet_goal", "feet_vel")),
    "支撑腿 QP": (("feet", "p", "R", "contact"), ("A", "b", "f_stance")),
    "摆动腿笛卡尔 PD": (("feet_goal", "feet", "feet_vel", "contact"),
                      ("f_swing",)),
    "J^T 映射关节力矩": (("q", "R", "f_stance", "f_swing", "contact"),
                       ("tau",)),
    "逆运动学": (("feet_goal", "R", "contact", "q"), ("q_des", "dq_des")),
    "写 LowCmd": (("q_des", "dq_des", "tau", "contact"), ("kp", "kd")),
}


def run_pipeline() -> tuple[list[str], float]:
    """Touch each stage in order and check that its inputs already exist."""
    trace: list[str] = []
    available: set[str] = set()
    for name, _cost, _fin, _fout in STAGES:
        need, produce = DEPS[name]
        for key in need:
            if key not in available:
                raise KeyError(f"stage {name!r} reads {key!r} before it is produced")
        trace.append(name)
        for key in produce:
            assert key in SHAPES, key
            available.add(key)
    # A downstream-only key is never produced by an upstream-only list.
    assert {"tau", "q_des", "dq_des"} <= available
    return trace, float(sum(s[1] for s in STAGES))


# ANCHOR_END: pipeline


def draw_flow() -> None:
    """Fig. 10-1: support chain over swing chain, merged at JointCmd."""
    fig, ax = plt.subplots(figsize=(9.2, 4.9))
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 10)
    ax.axis("off")

    def box(x, y, w, h, text, fc="white", ec="0.25", fs=8.4, lw=1.0):
        ax.add_patch(FancyBboxPatch((x, y), w, h,
                                    boxstyle="round,pad=0.04,rounding_size=0.08",
                                    fc=fc, ec=ec, lw=lw, zorder=3))
        ax.text(x + w / 2, y + h / 2, text, ha="center", va="center",
                fontsize=fs, zorder=4, linespacing=1.3)

    def arrow(x0, y0, x1, y1, style="-|>", color="0.25", lw=1.1, ls="-"):
        ax.add_patch(FancyArrowPatch((x0, y0), (x1, y1), arrowstyle=style,
                                     mutation_scale=9, color=color, lw=lw,
                                     linestyle=ls, zorder=2,
                                     shrinkA=1, shrinkB=1))

    # dashed group boxes
    ax.add_patch(FancyBboxPatch((2.05, 6.15), 7.75, 2.75,
                                boxstyle="round,pad=0.05", fill=False,
                                ec="C3", lw=1.0, ls="--", zorder=1))
    ax.add_patch(FancyBboxPatch((2.05, 2.05), 7.75, 2.75,
                                boxstyle="round,pad=0.05", fill=False,
                                ec="C0", lw=1.0, ls="--", zorder=1))
    ax.text(2.2, 8.68, "支撑链：力/力矩通路", color="C3", fontsize=9)
    ax.text(2.2, 4.58, "摆动链：位置通路", color="C0", fontsize=9)

    # inputs
    box(0.12, 7.35, 1.55, 0.9, "手柄/键盘\n$v_x,v_y,\\dot\\psi,h$", fc="#fff4e6")
    box(0.12, 3.05, 1.55, 1.0, "估计器\n$p,v,R,p_{fi}$", fc="#e8f1fb")
    box(0.12, 5.65, 1.55, 0.8, "步态相位\n$r,P,b_i$", fc="#f2f2f2")

    # support chain
    box(2.35, 7.35, 1.55, 0.9, "目标位姿\n$p_d,R_d$")
    box(4.25, 7.35, 1.45, 0.9, "位姿 PD\n$\\ddot p_d,\\dot\\omega_d$")
    box(6.05, 7.35, 1.35, 0.9, "支撑腿 QP\n$f_i$", fc="#fde8e8")
    box(7.75, 7.35, 1.35, 0.9, "$J^{\\top}$\n$\\tau_{ff}$", fc="#fde8e8")
    arrow(1.70, 7.80, 2.30, 7.80)
    arrow(3.93, 7.80, 4.20, 7.80)
    arrow(5.73, 7.80, 6.00, 7.80)
    arrow(7.43, 7.80, 7.70, 7.80)

    # swing chain
    box(2.35, 3.20, 1.55, 0.9, "落脚点\n$p_{fi,g}$")
    box(4.25, 3.20, 1.45, 0.9, "摆线轨迹\n$p_{fi},v_{fi}$")
    box(6.05, 3.20, 1.35, 0.9, "逆运动学\n$q_d,\\dot q_d$", fc="#e8f1fb")
    box(7.75, 3.20, 1.35, 0.9, "笛卡尔 PD\n$f_{i,s}$", fc="#e8f1fb")
    arrow(1.70, 3.65, 2.30, 3.65)
    arrow(3.93, 3.65, 4.20, 3.65)
    arrow(5.73, 3.65, 6.00, 3.65)
    arrow(7.43, 3.65, 7.70, 3.65)

    # vertical links
    arrow(1.70, 5.85, 2.30, 7.55, ls=":")
    arrow(1.70, 5.45, 2.30, 3.50, ls=":")

    # merge
    box(3.03, 0.55, 3.95, 1.0,
        "JointCmd：$q_{d}, \\dot q_{d}, \\tau_{ff}, k_p, k_d$", fc="#eef7ea", fs=8.8)
    arrow(8.42, 7.33, 6.98, 1.58)
    arrow(8.42, 3.18, 6.98, 1.58)
    box(7.35, 0.65, 2.45, 0.8, "LowCmd → 关节板", fc="#eef7ea")
    arrow(6.98, 1.05, 7.32, 1.05)

    ax.text(5.0, 9.72, label("行走控制器的两条计算链", "Two computation chains"),
            ha="center", va="center", fontsize=11)
    ax.text(5.0, 0.18, label("两条链在 $J^{\\top}$ 与关节命令处汇合；相位来自同一条步态时钟",
                             "both chains meet at J^T and the joint command"),
            ha="center", va="center", fontsize=8, color="0.3")
    savefig(fig, "10_1_control_loop")
    plt.close(fig)


def main() -> None:
    print("=== 10.1 一个 2 ms 周期的调用顺序（假数据） ===")
    trace, total = run_pipeline()
    print("调用顺序：" + " → ".join(trace))
    print("\n数据形状（每个阶段由上一阶段提供）：")
    for name, cost, fin, fout in STAGES:
        print(f"  {name:<14s} 输入 {fin:<30s} 输出 {fout:<26s} 预算 {cost:4d} µs")
    print(f"\n总预算 {total:.0f} µs，周期 {DT * 1e3:.1f} ms，余量 {DT * 1e6 - total:.0f} µs")
    print("检查：每个阶段读取的数组都在此前的阶段产生；若把 QP 放到估计之前会报 KeyError")
    print("支撑链与摆动链在 J^T 处汇合：支撑腿力矩由 f_stance 映射，摆动腿力矩由 f_swing 叠加")
    draw_flow()
    print("图 10-1 已保存：10_1_control_loop.png")


if __name__ == "__main__":
    main()
