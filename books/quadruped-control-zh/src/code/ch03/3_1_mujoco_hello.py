# expect: nq=19, nv=18, nu=12
# expect: 总质量: 12\.453 kg
# expect: 机身高度: 0\.0996 m
# expect: 四足接触: \[True, True, True, True\]
"""First contact with MuJoCo: load the A1, list its joints, drop it.

The example does three things that every later controller depends on:

* it reads the model dimensions (``nq``, ``nv``, ``nu``) and the twelve
  joint names, so the mapping from a name to an array index is explicit;
* it drops the robot from 0.5 m with zero joint torque for 1 s, then prints
  the body height and the four foot-contact flags;
* it renders one 640x480 frame off screen and puts it into the framework
  figure together with the ROS 2 analogy and the book's layer diagram.

The renderer is created here rather than through ``A1Sim.render`` so that the
image can be embedded directly into the matplotlib figure.
"""

from __future__ import annotations

import logging
import os

os.environ.setdefault("MESA_SHADER_CACHE_DISABLE", "true")

import mujoco
import numpy as np

from quadruped import A1Sim, JointCmd
from quadruped.plotting import savefig

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch  # noqa: E402

DT = 0.002
FALL_TIME = 1.0
START_HEIGHT = 0.5
WIDTH, HEIGHT = 640, 480


def draw_ros_panel(ax) -> None:
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 6)
    ax.axis("off")
    ax.set_title("(a) ROS 2：进程是节点，节点之间用话题传消息", fontsize=10)

    def box(x, y, w, h, text, color):
        ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.06",
                                    facecolor=color, edgecolor="0.3"))
        ax.text(x + w / 2, y + h / 2, text, ha="center", va="center", fontsize=8.5)

    box(0.4, 2.2, 2.4, 1.4, "控制器节点\ncontroller", "#e3f2fd")
    box(3.7, 3.9, 2.6, 1.1, "话题 /joint_states", "#fff8e1")
    box(3.7, 1.2, 2.6, 1.1, "话题 /joint_cmd", "#fff8e1")
    box(7.1, 2.2, 2.4, 1.4, "仿真节点\ngazebo", "#e8f5e9")

    arrow = dict(arrowstyle="-|>", mutation_scale=11, lw=1.2)
    ax.add_patch(FancyArrowPatch((2.8, 3.15), (3.7, 4.2), **arrow))
    ax.add_patch(FancyArrowPatch((6.3, 4.2), (7.1, 3.15), **arrow))
    ax.add_patch(FancyArrowPatch((7.1, 2.65), (6.3, 1.75), **arrow))
    ax.add_patch(FancyArrowPatch((3.7, 1.75), (2.8, 2.65), **arrow))
    ax.text(6.70, 3.85, "状态", fontsize=7.5, color="0.25", ha="center")
    ax.text(2.35, 1.25, "命令", fontsize=7.5, color="0.25", ha="center")
    ax.text(0.4, 0.35, "订阅状态、发布命令；控制器不知道仿真器的存在。",
            fontsize=8, color="0.25")


def draw_layer_panel(ax) -> None:
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 6)
    ax.axis("off")
    ax.set_title("(b) 本书：同样的读写被包在五层接口里", fontsize=10)

    layers = [
        ("应用/状态层：FSM（Passive/FixedStand/Trotting）", "#ede7f6"),
        ("控制层：BalanceCtrl、GaitGenerator、单腿 PD", "#e3f2fd"),
        ("模型层：Quadruped（FK/IK/雅可比）、A1Params", "#e8f5e9"),
        ("接口层：MotorBus（SimMotorBus / Rs485MotorBus）", "#fff8e1"),
        ("硬件/仿真：MuJoCo A1、RS-485 电机", "#eceff1"),
    ]
    for i, (text, color) in enumerate(layers):
        y = 4.85 - i * 1.02
        ax.add_patch(FancyBboxPatch((0.6, y), 8.2, 0.72,
                                    boxstyle="round,pad=0.04",
                                    facecolor=color, edgecolor="0.3"))
        ax.text(4.7, y + 0.36, text, ha="center", va="center", fontsize=8.5)
    ax.add_patch(FancyArrowPatch((9.15, 5.55), (9.15, 0.70),
                                 arrowstyle="<|-|>", mutation_scale=9,
                                 lw=1.1, color="0.35"))
    ax.text(9.35, 3.12, "命令下行 / 状态下行", fontsize=7.5, rotation=90,
            va="center", color="0.3")
    ax.text(1.35, 0.20, "任意一层都可以单独替换，层与层之间只认数据结构和函数签名。",
            fontsize=8, color="0.25")


def main() -> None:
    sim = A1Sim()
    model = sim.model
    print(f"MuJoCo {mujoco.__version__}")
    print(f"nq={model.nq}, nv={model.nv}, nu={model.nu}")
    print(f"总质量: {float(np.sum(model.body_mass)):.3f} kg")
    print("12 个关节（腿号 × 3 + 关节号）:")
    for i, name in enumerate(sim.joint_names):
        print(f"  {i:2d}  leg{i // 3}  joint{i % 3}  {name}")

    sim.reset(base_pos=(0.0, 0.0, START_HEIGHT))
    print(f"初始机身高度: {START_HEIGHT:.4f} m")
    zero = JointCmd.zero()
    for _ in range(int(round(FALL_TIME / DT))):
        sim.step(zero)

    base, joints = sim.get_state()
    contacts = sim.foot_contacts()
    print(f"{FALL_TIME:.1f} s 自由下落:")
    print(f"  机身高度: {base.position[2]:.4f} m")
    print(f"  四足接触: {contacts.tolist()}")
    print(f"  触地足端法向力之和: {float(np.sum(sim.foot_forces()[:, 2])):.1f} N")

    sim.reset(base_pos=(0.0, 0.0, sim.home_height))
    renderer = mujoco.Renderer(model, HEIGHT, WIDTH)
    cam = mujoco.MjvCamera()
    cam.type = mujoco.mjtCamera.mjCAMERA_FREE
    cam.lookat[:] = [0.0, 0.0, 0.20]
    cam.distance = 1.05
    cam.azimuth = 135.0
    cam.elevation = -14.0
    renderer.update_scene(sim.data, camera=cam)
    image = renderer.render().copy()
    renderer.close()
    sim.close()

    fig = plt.figure(figsize=(12.0, 8.2))
    gs = fig.add_gridspec(2, 2, height_ratios=(1.0, 0.78), hspace=0.25,
                          wspace=0.10)
    draw_ros_panel(fig.add_subplot(gs[0, 0]))
    draw_layer_panel(fig.add_subplot(gs[0, 1]))
    ax = fig.add_subplot(gs[1, :])
    ax.imshow(image)
    ax.axis("off")
    ax.set_title("(c) 本机 MuJoCo 离屏渲染：A1 的 home 站姿", fontsize=10)
    savefig(fig, "3_1_framework")
    print("图已保存: figs/3_1_framework.png")


if __name__ == "__main__":
    main()
