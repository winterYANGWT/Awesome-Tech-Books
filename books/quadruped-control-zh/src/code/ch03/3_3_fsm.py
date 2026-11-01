# expect: 初始状态: Passive
# expect: \[FSM\] 拒绝切换: 未注册的状态 'Jump'
# expect: \[FSM\] FixedStand 插值 p=0\.20 未完成，拒绝切换到 FreeStand
# expect: \[FSM\] 登记切换 FixedStand -> FreeStand
# expect: \[FSM\] CHANGE: FixedStand.exit\(\) -> FreeStand.enter\(\)
# expect: 当前状态: Passive
"""A five-state controller and a two-phase finite-state machine (section 3.3).

The point of the example is the order of the calls, not the robot dynamics.
Each state implements ``enter`` / ``run`` / ``exit`` / ``check_transition``;
``FSM.step`` is called every 2 ms and uses two phases:

* NORMAL: run the current state, ask it for the next state, and only
  *register* the request;
* CHANGE: on the next call, run the old state's ``exit``, switch the pointer,
  run the new state's ``enter``, and then ``run`` once.

A scripted keyboard sends the same keys as the real remote (2, 3, 4, 1).  One
request asks for an unregistered state, and one asks to leave ``FixedStand``
before its interpolation has finished; both are refused.  The figure shows
the five states, the twelve edges, and the key table.
"""

from __future__ import annotations

import logging
from dataclasses import dataclass, field

import numpy as np

from quadruped.plotting import savefig

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

import matplotlib.path as mpath  # noqa: E402
import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.patches import Circle, FancyArrowPatch, FancyBboxPatch  # noqa: E402

KEYS = {
    "Passive": {"2": "FixedStand"},
    "FixedStand": {"1": "Passive", "3": "FreeStand", "4": "Trotting",
                   "5": "move_base"},
    "FreeStand": {"1": "Passive", "2": "FixedStand", "4": "Trotting"},
    "Trotting": {"1": "Passive", "2": "FixedStand"},
    "move_base": {"1": "Passive", "2": "FixedStand"},
}
STATE_ORDER = ["Passive", "FixedStand", "FreeStand", "Trotting", "move_base"]


@dataclass
class Context:
    dt: float = 0.002
    stand_cycles: int = 20


class State:
    def __init__(self, ctx: Context, name: str) -> None:
        self.ctx = ctx
        self.name = name
        self.fsm = None
        self.entered = False

    def enter(self) -> None:
        self.entered = True

    def run(self) -> None:
        pass

    def exit(self) -> None:
        self.entered = False

    def check_transition(self) -> str | None:
        """Return the externally requested state for this cycle, if any."""
        target = self.fsm.pending if self.fsm is not None else None
        if target is not None:
            self.fsm.pending = None
        return target


class Passive(State):
    def __init__(self, ctx: Context) -> None:
        super().__init__(ctx, "Passive")

    def enter(self) -> None:
        super().enter()
        print("[Passive] enter: 12 个关节 kp=0, kd=2.0，进入阻尼")

    def run(self) -> None:
        print("[Passive] run: 发阻尼命令")


class FixedStand(State):
    def __init__(self, ctx: Context) -> None:
        super().__init__(ctx, "FixedStand")
        self.p = 0.0

    def enter(self) -> None:
        super().enter()
        self.p = 0.0
        print("[FixedStand] enter: 记录当前角度，p=0")

    def run(self) -> None:
        self.p = min(1.0, self.p + 1.0 / self.ctx.stand_cycles)
        print(f"[FixedStand] run: 插值 p={self.p:.2f}")

    def exit(self) -> None:
        super().exit()
        print("[FixedStand] exit: 清零插值进度")

    def check_transition(self) -> str | None:
        target = super().check_transition()
        if target in ("FreeStand", "Trotting") and self.p < 1.0:
            print(f"[FSM] FixedStand 插值 p={self.p:.2f} 未完成，拒绝切换到 {target}")
            return None
        return target


class FreeStand(State):
    def __init__(self, ctx: Context) -> None:
        super().__init__(ctx, "FreeStand")

    def enter(self) -> None:
        super().enter()
        print("[FreeStand] enter: 位姿保持器接管，目标高度 0.27 m")

    def run(self) -> None:
        print("[FreeStand] run: 位姿 PD → QP → 关节角")

    def exit(self) -> None:
        super().exit()
        print("[FreeStand] exit: 清空位姿指令")


class Trotting(State):
    def __init__(self, ctx: Context) -> None:
        super().__init__(ctx, "Trotting")
        self.k = 0

    def enter(self) -> None:
        super().enter()
        self.k = 0
        print("[Trotting] enter: 相位清零，先全触地 0.2 s")

    def run(self) -> None:
        self.k += 1
        print(f"[Trotting] run: 步态相位 {self.k * 0.002:.3f} s")

    def exit(self) -> None:
        super().exit()
        print("[Trotting] exit: 速度指令归零")


class MoveBase(State):
    def __init__(self, ctx: Context) -> None:
        super().__init__(ctx, "move_base")

    def enter(self) -> None:
        super().enter()
        print("[move_base] enter: 订阅 /cmd_vel")

    def run(self) -> None:
        print("[move_base] run: 跟踪导航速度")


class FSM:
    def __init__(self, ctx: Context, initial: str = "Passive") -> None:
        self.ctx = ctx
        self.states = {
            "Passive": Passive(ctx),
            "FixedStand": FixedStand(ctx),
            "FreeStand": FreeStand(ctx),
            "Trotting": Trotting(ctx),
            "move_base": MoveBase(ctx),
        }
        for state in self.states.values():
            state.fsm = self
        self.current = initial
        self.pending: str | None = None
        self.next_name: str | None = None
        self.mode = "NORMAL"
        self.cycle = 0
        self.states[self.current].enter()

    def request(self, target: str) -> bool:
        if target not in self.states:
            print(f"[FSM] 拒绝切换: 未注册的状态 {target!r}")
            return False
        if target == self.current:
            return True
        self.pending = target
        print(f"[FSM] 收到请求 {self.current} -> {target}")
        return True

    def step(self) -> None:
        if self.mode == "NORMAL":
            self.states[self.current].run()
            nxt = self.states[self.current].check_transition()
            if nxt is not None and nxt != self.current:
                self.next_name = nxt
                self.mode = "CHANGE"
                print(f"[FSM] 登记切换 {self.current} -> {nxt}（下一周期执行）")
        else:
            old = self.states[self.current]
            old.exit()
            print(f"[FSM] CHANGE: {old.name}.exit() -> {self.next_name}.enter()")
            self.current = self.next_name
            self.states[self.current].enter()
            self.mode = "NORMAL"
            self.states[self.current].run()
        self.cycle += 1


def draw_machine(ax) -> None:
    ax.set_xlim(-0.1, 12.1)
    ax.set_ylim(-0.3, 7.7)
    ax.axis("off")
    ax.set_title("(a) 五个状态与十二条切换边（编号见表）", fontsize=10)

    pos = {
        "Passive": (1.55, 3.45),
        "FixedStand": (5.65, 3.45),
        "FreeStand": (5.65, 6.55),
        "Trotting": (9.95, 3.45),
        "move_base": (5.65, 0.50),
    }
    widths = {"Passive": 2.0, "FixedStand": 2.5, "FreeStand": 2.4,
              "Trotting": 2.1, "move_base": 2.3}
    for name, (x, y) in pos.items():
        w = widths[name]
        ax.add_patch(FancyBboxPatch((x - w / 2, y - 0.33), w, 0.66,
                                    boxstyle="round,pad=0.06",
                                    facecolor="#e3f2fd" if name == "Passive" else "#ede7f6",
                                    edgecolor="0.25", zorder=3))
        ax.text(x, y, name, ha="center", va="center", fontsize=8.6, zorder=4)

    def straight(n, src, dst, rad, color, label_xy):
        p0, p1 = pos[src], pos[dst]
        ax.add_patch(FancyArrowPatch(p0, p1, connectionstyle=f"arc3,rad={rad}",
                                     arrowstyle="-|>", mutation_scale=9,
                                     lw=1.05, color=color, shrinkA=34, shrinkB=34,
                                     zorder=2))
        ax.add_patch(Circle(label_xy, 0.17, facecolor="white", edgecolor=color,
                            lw=0.8, zorder=5))
        ax.text(*label_xy, str(n), ha="center", va="center", fontsize=6.6,
                color=color, zorder=6)

    def routed(n, points, color, label_xy):
        verts = [points[0]]
        for point in points[1:]:
            verts.append(point)
        path = mpath.Path(verts, [mpath.Path.MOVETO] + [mpath.Path.LINETO] * (len(verts) - 1))
        ax.add_patch(FancyArrowPatch(path=path, arrowstyle="-|>", mutation_scale=9,
                                     lw=1.05, color=color, zorder=2))
        ax.add_patch(Circle(label_xy, 0.17, facecolor="white", edgecolor=color,
                            lw=0.8, zorder=5))
        ax.text(*label_xy, str(n), ha="center", va="center", fontsize=6.6,
                color=color, zorder=6)

    straight(1, "Passive", "FixedStand", 0.0, "#1565c0", (3.55, 3.68))
    straight(2, "FixedStand", "Passive", 0.30, "#c62828", (3.55, 3.16))
    straight(3, "FixedStand", "FreeStand", 0.0, "#2e7d32", (5.25, 4.90))
    straight(4, "FreeStand", "FixedStand", 0.30, "#c62828", (6.05, 4.65))
    straight(5, "FixedStand", "move_base", 0.0, "#6a1b9a", (5.25, 2.00))
    straight(6, "move_base", "FixedStand", 0.30, "#c62828", (6.05, 1.75))
    straight(7, "FixedStand", "Trotting", 0.0, "#ef6c00", (7.75, 3.68))
    straight(8, "Trotting", "FixedStand", 0.30, "#c62828", (7.75, 3.16))

    # Long edges are routed orthogonally around the star, so none of them
    # crosses a state box or another long edge.
    routed(9, [(5.65, 6.88), (5.65, 7.30), (1.55, 7.30), (1.55, 3.85)],
           "#c62828", (3.20, 7.30))
    routed(10, [(6.85, 6.55), (9.95, 6.55), (9.95, 3.85)],
           "#ef6c00", (8.45, 6.55))
    routed(11, [(4.50, 0.50), (1.55, 0.50), (1.55, 3.05)],
           "#c62828", (2.95, 0.50))
    routed(12, [(11.00, 3.45), (11.75, 3.45), (11.75, 0.05),
                (0.55, 0.05), (0.55, 3.05)],
           "#c62828", (6.20, 0.05))

    ax.annotate("初始", xy=(0.55, 3.62), xytext=(0.20, 4.85),
                arrowprops=dict(arrowstyle="-|>", lw=1.0, color="0.3",
                                shrinkA=4, shrinkB=6),
                fontsize=7.8, color="0.3")
    ax.text(0.05, -0.22, "固定站立是集散状态：去 Trotting / move_base 都先经过它。",
            fontsize=8, color="0.3")


def draw_table(ax) -> None:
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 7.6)
    ax.axis("off")
    ax.set_title("(b) 边与触发键", fontsize=10)

    rows = [
        ("1", "Passive→FixedStand", "<L2+A> / <2>"),
        ("2", "FixedStand→Passive", "<L2+B> / <1>"),
        ("3", "FixedStand→FreeStand", "<L2+X> / <3>"),
        ("4", "FreeStand→FixedStand", "<L2+A> / <2>"),
        ("5", "FixedStand→move_base", "<L2+Y> / <5>"),
        ("6", "move_base→FixedStand", "<L2+A> / <2>"),
        ("7", "FixedStand→Trotting", "<START> / <4>"),
        ("8", "Trotting→FixedStand", "<L2+A> / <2>"),
        ("9", "FreeStand→Passive", "<L2+B> / <1>"),
        ("10", "FreeStand→Trotting", "<START> / <4>"),
        ("11", "move_base→Passive", "<L2+B> / <1>"),
        ("12", "Trotting→Passive", "<L2+B> / <1>"),
    ]
    ax.text(0.1, 7.05, "编号", fontsize=7.8, weight="bold")
    ax.text(1.0, 7.05, "切换", fontsize=7.8, weight="bold")
    ax.text(6.7, 7.05, "手柄 / 键盘", fontsize=7.8, weight="bold")
    ax.plot([0.05, 9.9], [6.92, 6.92], color="0.3", lw=0.8)
    for i, (num, edge, key) in enumerate(rows):
        y = 6.55 - i * 0.50
        if i % 2 == 1:
            ax.add_patch(plt.Rectangle((0.02, y - 0.20), 9.85, 0.45,
                                       facecolor="#f5f5f5", edgecolor="none"))
        ax.text(0.25, y, num, fontsize=7.6, ha="center")
        ax.text(0.95, y, edge, fontsize=7.8, family="monospace")
        ax.text(6.70, y, key, fontsize=7.8, family="monospace")
    ax.text(0.05, 0.20, "仿真中用键盘数字键；实机上 L2 + A/B/X/Y 与 START 由手柄驱动。",
            fontsize=7.6, color="0.3")


def main() -> None:
    ctx = Context()
    fsm = FSM(ctx)
    print(f"初始状态: {fsm.current}")
    fsm.request("Jump")  # 未注册，必须拒绝

    # 脚本化的“按键”：cycle 号 -> 目标状态
    events = {3: "FixedStand", 7: "FreeStand", 14: "FreeStand",
              23: "FreeStand", 27: "Trotting", 31: "Passive"}
    for cycle in range(34):
        if cycle in events:
            print(f"--- cycle {cycle}: 按键请求 {events[cycle]}")
            fsm.request(events[cycle])
        fsm.step()
        print(f"    当前状态: {fsm.current}")

    assert fsm.current == "Passive"
    print(f"当前状态: {fsm.current}")

    fig = plt.figure(figsize=(12.6, 5.6))
    gs = fig.add_gridspec(1, 2, width_ratios=(1.05, 1.0), wspace=0.05)
    draw_machine(fig.add_subplot(gs[0, 0]))
    draw_table(fig.add_subplot(gs[0, 1]))
    savefig(fig, "3_3_fsm")
    print("图已保存: figs/3_3_fsm.png")


if __name__ == "__main__":
    main()
