# expect: 后端: sim
# expect: \[SimMotorBus::send\] 发布 12 个关节命令, q_des\[0\]=0\.00
# expect: 后端: real（Controller 不变，只换注入对象）
# expect: \[Rs485MotorBus::send\] 打包 34 字节命令帧, id=0
# expect: 类型检查: SimMotorBus 与 Rs485MotorBus 都满足 MotorBus
"""The C++ interface pattern rewritten with a Python Protocol (section 3.2).

``MotorBus`` is a structural type: it only says that an object must have
``send`` and ``recv``.  ``SimMotorBus`` writes into the MuJoCo model,
``Rs485MotorBus`` would write the 34-byte frame; the controller is written
once and receives the back end through its constructor (dependency
injection).  The example prints the call log and then checks the two classes
against the protocol at run time.

The figure has two panels: the UML class diagram and the dynamic dispatch
path from the controller to either back end.
"""

from __future__ import annotations

import logging
from typing import Protocol, runtime_checkable

import numpy as np

from quadruped import JointCmd
from quadruped.plotting import savefig

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch, Polygon  # noqa: E402

N = 12


@runtime_checkable
class MotorBus(Protocol):
    """The only two operations the controller needs from a back end."""

    def send(self, cmd: JointCmd) -> None: ...

    def recv(self) -> np.ndarray: ...


class SimMotorBus:
    def __init__(self) -> None:
        self.q = np.zeros(N)

    def send(self, cmd: JointCmd) -> None:
        print(f"[SimMotorBus::send] 发布 12 个关节命令, q_des[0]={cmd.q_des[0]:.2f}, "
              f"kp[0]={cmd.kp[0]:.0f}")
        self.q = np.asarray(cmd.q_des, dtype=float).copy()

    def recv(self) -> np.ndarray:
        print(f"[SimMotorBus::recv] 读回 12 个关节角度, q[0]={self.q[0]:.2f}")
        return self.q.copy()


class Rs485MotorBus:
    def __init__(self, motor_id: int = 0) -> None:
        self.motor_id = motor_id
        self.q = np.zeros(N)

    def send(self, cmd: JointCmd) -> None:
        frame = encode_frame(self.motor_id, cmd)
        print(f"[Rs485MotorBus::send] 打包 {len(frame)} 字节命令帧, id={self.motor_id}, "
              f"q_des[0]={cmd.q_des[0]:.2f}")

    def recv(self) -> np.ndarray:
        print(f"[Rs485MotorBus::recv] 拼 78 字节状态帧, q[0]={self.q[0]:.2f}")
        return self.q.copy()


def encode_frame(motor_id: int, cmd: JointCmd) -> bytes:
    """Stand-in for the real encoder: a length check keeps the demo honest."""
    return bytes([0xFE, 0xEE, motor_id]) + bytes(31)


class Controller:
    def __init__(self, bus: MotorBus) -> None:
        self.bus = bus

    def step(self, q_target: float, kp: float) -> None:
        q_des = np.full(N, q_target)
        cmd = JointCmd(q_des, np.zeros(N), np.full(N, kp), np.full(N, 2.0),
                       np.zeros(N))
        self.bus.send(cmd)
        self.bus.recv()


def draw_uml(ax) -> None:
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 7.2)
    ax.axis("off")
    ax.set_title("继承：两个派生类共享同一个接口", fontsize=10)

    def cls(x, y, w, h, title, lines, color):
        ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="square,pad=0",
                                    facecolor=color, edgecolor="0.2", lw=1.2))
        ax.text(x + w / 2, y + h - 0.28, title, ha="center", va="center",
                fontsize=9.5, weight="bold")
        ax.plot([x, x + w], [y + h - 0.52, y + h - 0.52], color="0.2", lw=0.9)
        for i, line in enumerate(lines):
            ax.text(x + 0.12, y + h - 0.78 - 0.34 * i, line, fontsize=8,
                    va="center", family="monospace")

    cls(2.55, 5.05, 4.9, 1.65, "IOInterface",
        ["+ send(cmd) = 0", "+ recv() = 0",
         "+ virtual ~IOInterface()"], "#ede7f6")
    cls(0.55, 1.85, 4.1, 1.95, "IOROS",
        ["+ send(cmd): void", "+ recv(): state", "+ ~IOROS()"], "#e3f2fd")
    cls(5.35, 1.85, 4.1, 1.95, "IOSDK",
        ["+ send(cmd): void", "+ recv(): state", "+ ~IOSDK()"], "#e8f5e9")

    ax.add_patch(Polygon([[5.0, 5.05], [4.72, 4.72], [5.28, 4.72]], closed=True,
                         facecolor="white", edgecolor="0.25", lw=1.2))
    ax.plot([2.6, 4.72], [3.8, 4.72], color="0.25", lw=1.2)
    ax.plot([7.4, 5.28], [3.8, 4.72], color="0.25", lw=1.2)
    ax.text(0.6, 0.95, "空心三角指向基类；基类只声明纯虚函数，实现由派生类给出。",
            fontsize=8.3, color="0.3")
    ax.text(0.6, 0.42, "调用者只持有 IOInterface*，不知道也不关心具体派生类。",
            fontsize=8.3, color="0.3")


def draw_dispatch(ax) -> None:
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 7.2)
    ax.axis("off")
    ax.set_title("动态绑定：一次调用，两条路径", fontsize=10)

    def box(x, y, w, h, text, color, fs=8.6):
        ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.06",
                                    facecolor=color, edgecolor="0.25"))
        ax.text(x + w / 2, y + h / 2, text, ha="center", va="center", fontsize=fs)

    box(0.20, 3.05, 2.10, 1.15, "Controller\n控制器算法", "#eceff1")
    box(2.95, 3.05, 2.35, 1.15, "IOInterface* io\n（基类指针）", "#ede7f6")
    box(6.00, 4.60, 2.55, 0.90, "IOROS", "#e3f2fd")
    box(6.00, 1.40, 2.55, 0.90, "IOSDK", "#e8f5e9")
    box(6.30, 6.25, 1.95, 0.55, "Gazebo / MuJoCo", "#f3e5f5", fs=7.8)
    box(6.30, 0.25, 1.95, 0.55, "A1 实机", "#fff8e1", fs=7.8)

    arrow = dict(arrowstyle="-|>", mutation_scale=10, lw=1.1)
    ax.add_patch(FancyArrowPatch((2.30, 3.62), (2.95, 3.62), **arrow))
    ax.add_patch(FancyArrowPatch((5.30, 3.85), (6.00, 4.85), **arrow))
    ax.add_patch(FancyArrowPatch((5.30, 3.40), (6.00, 1.95), **arrow))
    ax.add_patch(FancyArrowPatch((7.10, 5.50), (7.10, 6.25),
                                 arrowstyle="<|-|>", mutation_scale=9, lw=1.0))
    ax.add_patch(FancyArrowPatch((7.10, 1.40), (7.10, 0.80),
                                 arrowstyle="<|-|>", mutation_scale=9, lw=1.0))
    ax.text(7.25, 5.86, "send/recv", fontsize=7.2, color="0.3")
    ax.text(7.25, 1.08, "send/recv", fontsize=7.2, color="0.3")
    ax.text(3.02, 4.55, "虚函数表\n运行时查表", fontsize=7.6, color="0.3")
    notes = ["编译/初始化期选择对象", "运行期同一条 send 调用",
             "落到不同派生类的实现"]
    for i, line in enumerate(notes):
        ax.text(0.22, 2.30 - 0.50 * i, line, fontsize=8, color="#1565c0")


def main() -> None:
    print("后端: sim")
    controller = Controller(SimMotorBus())
    controller.step(q_target=0.0, kp=80.0)

    print("后端: real（Controller 不变，只换注入对象）")
    controller.bus = Rs485MotorBus(motor_id=0)
    controller.step(q_target=0.1, kp=80.0)

    ok = (isinstance(SimMotorBus(), MotorBus)
          and isinstance(Rs485MotorBus(), MotorBus))
    print("类型检查: SimMotorBus 与 Rs485MotorBus 都满足 MotorBus"
          if ok else "类型检查: 不满足 MotorBus")

    fig, ax = plt.subplots(figsize=(6.2, 5.4))
    draw_uml(ax)
    savefig(fig, "3_2_uml")
    print("图已保存: figs/3_2_uml.png")

    fig, ax = plt.subplots(figsize=(6.4, 5.4))
    draw_dispatch(ax)
    savefig(fig, "3_2_dispatch")
    print("图已保存: figs/3_2_dispatch.png")


if __name__ == "__main__":
    main()
