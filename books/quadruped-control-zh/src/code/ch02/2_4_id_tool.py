"""Simulated bus for the A1 change-ID procedure (section 2.4).

Three motors share one RS-485 line.  A broadcast puts all of them into the
electronic-ratchet mode; one, two or three detents select ID 0, 1 or 2; the
save command writes the IDs.  The first attempt deliberately produces a
duplicate and is rejected, the second attempt succeeds.

expect: 广播 motorID=0xBB
expect: 冲突
expect: 最终 ID 表
"""

from __future__ import annotations

import logging
from dataclasses import dataclass, field

import numpy as np

from quadruped.plotting import savefig
import matplotlib.pyplot as plt
from matplotlib.patches import Circle, FancyArrowPatch, FancyBboxPatch, Rectangle

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

BROADCAST_ID = 0xBB


@dataclass
class SimMotor:
    name: str
    motor_id: int = 0
    id_mode: bool = False
    detents: int = 0
    saved_id: int | None = None


@dataclass
class SimMotorBus:
    motors: list[SimMotor] = field(default_factory=list)

    def broadcast_enter_id_mode(self) -> None:
        print(f"[广播] motorID=0x{BROADCAST_ID:02X}，命令所有电机进入改 ID 模式")
        for m in self.motors:
            m.id_mode = True
            m.detents = 0
            m.saved_id = None

    def turn_output(self, index: int, detents: int) -> bool:
        m = self.motors[index]
        if not m.id_mode:
            print(f"[错误] {m.name} 不在改 ID 模式")
            return False
        if detents not in (1, 2, 3):
            print(f"[拒绝] {m.name} 转了 {detents} 次；只接受 1、2、3 次")
            return False
        m.detents = detents
        print(f"[棘轮] {m.name} 转过 {detents} 次 → 目标 ID {detents - 1}")
        return True

    def save_ids(self) -> bool:
        pending = [m.detents - 1 for m in self.motors]
        if any(m.detents == 0 for m in self.motors):
            print("[拒绝] 有电机没有转动过，无法分配 ID")
            return False
        seen: dict[int, str] = {}
        conflict = False
        for m, target in zip(self.motors, pending):
            if target in seen:
                print(f"[冲突] ID {target} 同时分配给 {seen[target]} 和 {m.name}，拒绝保存")
                conflict = True
            else:
                seen[target] = m.name
        if conflict:
            return False
        for m, target in zip(self.motors, pending):
            m.motor_id = target
            m.saved_id = target
            m.id_mode = False
        print("[保存] 三台电机 ID 写入成功")
        return True


def draw_bus() -> None:
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11.0, 4.4),
                                   gridspec_kw={"width_ratios": [1.0, 1.5]})

    # Top view of one motor with its three connectors.
    ax1.set_aspect("equal")
    ax1.set_xlim(-1.7, 1.7)
    ax1.set_ylim(-1.7, 1.7)
    ax1.axis("off")
    ax1.add_patch(Circle((0, 0), 1.0, facecolor="#eceff1", edgecolor="0.3", lw=1.2))
    ax1.add_patch(Circle((0, 0), 0.35, facecolor="#cfd8dc", edgecolor="0.4"))
    ax1.text(0, 0.12, "转子", ha="center", va="center", fontsize=8)
    for ang, name, color in ((180, "RS-485", "#1565c0"),
                             (270, "24 V", "#c62828"),
                             (0, "RS-485", "#1565c0")):
        a = np.deg2rad(ang)
        x, y = 1.0 * np.cos(a), 1.0 * np.sin(a)
        ax1.add_patch(Rectangle((x - 0.13, y - 0.10), 0.26, 0.20,
                                facecolor=color, edgecolor="0.25"))
        ax1.text(1.42 * np.cos(a), 1.42 * np.sin(a), name, ha="center", va="center",
                 fontsize=8, color=color)
    ax1.plot([-0.95, 0.95], [0.0, 0.0], ls="--", color="#1565c0", lw=1.2)
    ax1.text(0, -0.20, "内部连通", ha="center", fontsize=7, color="#1565c0")
    ax1.set_title("(a) 三个接口：中间电源，两侧 RS-485")

    # Daisy chain topology.
    ax2.set_xlim(0, 10.0)
    ax2.set_ylim(0, 4.2)
    ax2.axis("off")

    def box(x, y, w, h, text, fc):
        ax2.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.04",
                                     facecolor=fc, edgecolor="0.25"))
        ax2.text(x + w / 2, y + h / 2, text, ha="center", va="center", fontsize=8)

    def arrow(x0, y0, x1, y1):
        ax2.add_patch(FancyArrowPatch((x0, y0), (x1, y1), arrowstyle="<|-|>",
                                      mutation_scale=8, lw=1.0, color="#1565c0"))

    box(0.1, 1.65, 1.35, 0.85, "上位机", "#e3f2fd")
    box(1.75, 1.65, 1.55, 0.85, "USB 转\nRS-485", "#fff3e0")
    arrow(1.45, 2.08, 1.75, 2.08)
    xs = (3.65, 5.45, 7.25)
    for i, x in enumerate(xs):
        box(x, 1.55, 1.35, 1.05, f"电机 {i}\nID={i}", "#e8f5e9")
        if i == 0:
            arrow(3.52, 2.08, x, 2.08)
        else:
            arrow(xs[i - 1] + 1.35, 2.08, x, 2.08)
    ax2.plot([9.05, 9.05], [1.45, 2.70], color="0.3", lw=1.0)
    ax2.plot([9.05, 9.22], [2.70, 2.70], color="0.3", lw=1.0)
    ax2.plot([9.05, 9.22], [1.45, 1.45], color="0.3", lw=1.0)
    ax2.text(9.30, 2.08, "120 Ω\n终端", fontsize=7.5, va="center")
    ax2.plot([3.38, 3.38], [1.45, 2.70], color="0.3", lw=1.0)
    ax2.plot([3.38, 3.52], [2.70, 2.70], color="0.3", lw=1.0)
    ax2.plot([3.38, 3.52], [1.45, 1.45], color="0.3", lw=1.0)
    ax2.text(2.55, 1.20, "总线两端可加 120 Ω，线尽量短", fontsize=7.5)
    ax2.text(0.1, 0.45, "A 对 A、B 对 B、GND 对 GND；一条总线最多 3 台，ID 0/1/2 唯一",
             fontsize=8, color="0.3")
    ax2.set_title("(b) 菊花链：电脑—转接器—电机 0—1—2")

    fig.tight_layout()
    savefig(fig, "2_4_bus")


def draw_id_flow() -> None:
    fig, ax = plt.subplots(figsize=(8.6, 5.6))
    ax.set_xlim(0, 10.0)
    ax.set_ylim(0, 8.2)
    ax.axis("off")

    def box(x, y, w, h, text, fc="#e3f2fd"):
        ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.05",
                                    facecolor=fc, edgecolor="0.25"))
        ax.text(x + w / 2, y + h / 2, text, ha="center", va="center", fontsize=8.5)

    def down(x0, y0, x1, y1, text=""):
        ax.add_patch(FancyArrowPatch((x0, y0), (x1, y1), arrowstyle="-|>",
                                     mutation_scale=9, lw=1.0, color="0.25"))
        if text:
            ax.text((x0 + x1) / 2 + 0.08, (y0 + y1) / 2, text, fontsize=7.5,
                    color="0.35")

    box(3.2, 7.2, 3.6, 0.7, "广播 0xBB：进入改 ID 模式", "#e8f5e9")
    box(3.2, 5.9, 3.6, 0.7, "输出轴变成电子棘轮", "#fff3e0")
    down(5.0, 7.2, 5.0, 6.6)
    box(1.0, 4.3, 3.2, 0.9, "转 1 次 → ID 0\n转 2 次 → ID 1\n转 3 次 → ID 2", "#fff8e1")
    box(5.8, 4.3, 3.0, 0.9, "输入 a 回车\n保存 ID", "#fff8e1")
    down(5.0, 5.9, 5.0, 5.35, "逐台转动")
    down(4.2, 4.75, 5.8, 4.75)
    down(7.3, 4.3, 7.3, 3.55)
    box(5.8, 2.85, 3.0, 0.7, "断电，重新上电", "#eceff1")
    down(7.3, 2.85, 7.3, 2.15)
    box(5.5, 1.45, 3.6, 0.7, "逐台发命令验证 ID", "#e8f5e9")
    down(7.3, 1.45, 7.3, 0.85)
    ax.text(7.3, 0.55, "只有目标 ID 的电机响应", ha="center", fontsize=8, color="0.3")

    box(0.9, 2.6, 3.2, 0.85, "ID 重复 → 拒绝保存\n重新进入改 ID 模式", "#ffebee")
    ax.add_patch(FancyArrowPatch((2.5, 4.3), (2.5, 3.45), arrowstyle="-|>",
                                 mutation_scale=9, lw=1.0, color="#c62828"))
    ax.add_patch(FancyArrowPatch((2.5, 3.45), (3.2, 3.45), arrowstyle="-|>",
                                 mutation_scale=9, lw=1.0, color="#c62828"))
    ax.text(1.0, 1.65, "工装：M4 螺钉，旋入深度 ≤ 5 mm，\n用杠杆转输出轴；一次只改一台。",
            fontsize=8, color="0.25")
    ax.set_title("修改电机 ID 的流程")
    savefig(fig, "2_4_id_flow")


def main() -> None:
    bus = SimMotorBus([SimMotor("M0", 0), SimMotor("M1", 1), SimMotor("M2", 2)])
    print("初始 ID: " + ", ".join(f"{m.name}={m.motor_id}" for m in bus.motors))

    print("\n第一次尝试（故意制造冲突）：")
    bus.broadcast_enter_id_mode()
    bus.turn_output(0, 1)
    bus.turn_output(1, 1)
    bus.turn_output(2, 2)
    bus.save_ids()

    print("\n第二次尝试（重新广播后逐台设置）：")
    bus.broadcast_enter_id_mode()
    for i, detents in enumerate((1, 2, 3)):
        bus.turn_output(i, detents)
    bus.save_ids()

    print("\n最终 ID 表：")
    print("  电机   棘轮次数   ID")
    for m, detents in zip(bus.motors, (1, 2, 3)):
        print(f"  {m.name}     {detents}          {m.motor_id}")
    print("上电后仍需逐台发命令确认：只有 ID 匹配或广播 0xBB 的命令会被执行。")

    draw_bus()
    draw_id_flow()


if __name__ == "__main__":
    main()
