"""A tiny message bus for the navigation-controller interface (11.3).

The script does not import ROS.  It models the six things that must agree
between a navigation stack and the walking controller:

* topic name and message type,
* publish rate and timestamp age,
* coordinate frame (``odom``, ``base``, sensor frames),
* units and sign of the three velocity components,
* the ``odom -> base`` TF and the static sensor TF,
* the joint names used by ``robot_state_publisher``.

It publishes odometry/TF at 50 Hz, ``cmd_vel`` at 20 Hz (with a 0.6 s outage
to exercise the watchdog), ``joint_states`` at 100 Hz and a camera scan at
10 Hz with 40 ms transport delay.  At the end it deliberately publishes a
cloud whose ``frame_id`` is ``camera_link`` while the TF tree only has
``front_cam``, and verifies that the lookup fails.  It then corrects the frame
and checks the transform again.

expect: 话题统计
expect: 看门狗
expect: TF 链
expect: 断裂测试
expect: joint_states
expect: 图 11-5
"""

from __future__ import annotations

import logging
import math
import os
import warnings
from dataclasses import dataclass, field

os.environ.setdefault("MPLCONFIGDIR", "/tmp/quadruped-mpl")

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch

from quadruped.plotting import label, savefig
from quadruped.types import JOINT_NAMES

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)
logging.getLogger("matplotlib").setLevel(logging.ERROR)
warnings.filterwarnings("ignore", message=".*glyph.*")
warnings.filterwarnings("ignore", message=".*tight_layout.*")
plt.rcParams["mathtext.fontset"] = "dejavusans"

DT = 0.002
DURATION = 2.0
WATCHDOG = 0.50


# ---------------------------------------------------------------------------
# Messages and bus
# ---------------------------------------------------------------------------
@dataclass
class Header:
    stamp: float
    frame_id: str


@dataclass
class Twist:
    vx: float = 0.0
    vy: float = 0.0
    wz: float = 0.0


@dataclass
class TwistStamped:
    header: Header
    twist: Twist


@dataclass
class Odometry:
    header: Header
    child_frame_id: str
    x: float
    y: float
    yaw: float


@dataclass
class JointState:
    header: Header
    name: list[str]
    position: list[float]
    velocity: list[float]


@dataclass
class Transform:
    stamp: float
    parent: str
    child: str
    x: float
    y: float
    yaw: float


# ANCHOR: bus
class Bus:
    """Deterministic publish/subscribe bus with a fixed transport delay."""

    def __init__(self) -> None:
        self.subscribers: dict[str, list] = {}
        self.published: dict[str, list[float]] = {}
        self.delays: dict[str, float] = {}
        self.log: dict[str, list[tuple[float, object]]] = {}

    def subscribe(self, topic: str, callback) -> None:
        self.subscribers.setdefault(topic, []).append(callback)

    def advertise(self, topic: str, delay: float = 0.0) -> None:
        self.delays[topic] = float(delay)
        self.published.setdefault(topic, [])
        self.log.setdefault(topic, [])

    def publish(self, topic: str, msg, now: float) -> None:
        self.published.setdefault(topic, []).append(now)
        arrive = now + self.delays.get(topic, 0.0)
        self.log.setdefault(topic, []).append((arrive, msg))
        for callback in self.subscribers.get(topic, []):
            callback(msg, arrive)
# ANCHOR_END: bus


# ---------------------------------------------------------------------------
# Nodes
# ---------------------------------------------------------------------------
class Estimator:
    """Publishes odometry and the odom -> base transform at 50 Hz."""

    RATE = 50.0

    def __init__(self, bus: Bus) -> None:
        self.bus = bus
        bus.advertise("/odom", 0.002)
        bus.advertise("/tf", 0.002)

    def step(self, now: float) -> None:
        if not self._due(now):
            return
        x, y, yaw = 0.30 * now, 0.02 * now, 0.10 * now
        self.bus.publish("/odom", Odometry(Header(now, "odom"), "base",
                                           x, y, yaw), now)
        self.bus.publish("/tf", Transform(now, "odom", "base", x, y, yaw), now)

    def _due(self, now: float) -> bool:
        k = round(now / (1.0 / self.RATE))
        return abs(now - k / self.RATE) < 1e-9


class Navigator:
    """Publishes TwistStamped at 20 Hz; silent during [1.0, 1.6) s."""

    RATE = 20.0

    def __init__(self, bus: Bus) -> None:
        self.bus = bus
        bus.advertise("/cmd_vel", 0.005)

    def step(self, now: float) -> None:
        k = round(now / (1.0 / self.RATE))
        if abs(now - k / self.RATE) > 1e-9:
            return
        if 1.0 <= now < 1.6:
            return
        self.bus.publish("/cmd_vel",
                         TwistStamped(Header(now, "base"),
                                      Twist(0.30, 0.0, 0.20)), now)


class WalkController:
    """Subscribes /cmd_vel, applies a 0.5 s watchdog, publishes joint states."""

    RATE = 100.0

    def __init__(self, bus: Bus) -> None:
        self.bus = bus
        self.last_cmd_time = -np.inf
        self.cmd = Twist()
        self.zero_events: list[float] = []
        self.joint_pub = 0
        bus.subscribe("/cmd_vel", self.on_cmd)
        bus.advertise("/joint_states", 0.001)

    def on_cmd(self, msg: TwistStamped, arrive: float) -> None:
        self.last_cmd_time = arrive
        self.cmd = msg.twist

    def update(self, now: float) -> None:
        if now - self.last_cmd_time > WATCHDOG:
            if self.cmd.vx != 0.0 or self.cmd.wz != 0.0:
                self.zero_events.append(now)
            self.cmd = Twist(0.0, 0.0, 0.0)
        k = round(now / (1.0 / self.RATE))
        if abs(now - k / self.RATE) > 1e-9:
            return
        q = [0.05 * math.sin(2.0 * math.pi * now + i * 0.3)
             for i in range(12)]
        self.bus.publish("/joint_states",
                         JointState(Header(now, "base"), list(JOINT_NAMES),
                                    q, [0.0] * 12), now)
        self.joint_pub += 1


class Camera:
    """Publishes a 10 Hz scan with 40 ms delay and a (first) wrong frame."""

    RATE = 10.0

    def __init__(self, bus: Bus) -> None:
        self.bus = bus
        bus.advertise("/scan", 0.040)
        self.frame = "camera_link"

    def step(self, now: float) -> None:
        k = round(now / (1.0 / self.RATE))
        if abs(now - k / self.RATE) > 1e-9:
            return
        self.bus.publish("/scan", Header(now, self.frame), now)


class TfTree:
    """Minimal TF buffer: parent, child and a 2D transform."""

    def __init__(self) -> None:
        self.children: dict[str, Transform] = {}
        self.chain: list[Transform] = []
        self.add(Transform(0.0, "base", "front_cam", 0.25, 0.0, 0.0))
        self.add(Transform(0.0, "base", "left_cam", 0.0, 0.12, 0.0))
        self.add(Transform(0.0, "base", "right_cam", 0.0, -0.12, 0.0))
        self.add(Transform(0.0, "front_cam", "front_optical", 0.0, 0.0,
                           -math.pi / 2))

    def add(self, tf: Transform) -> None:
        self.children[tf.child] = tf

    def update_odom(self, tf: Transform) -> None:
        self.add(tf)
        self.odom_base = tf

    def lookup(self, parent: str, child: str) -> Transform | None:
        """Return a two-link chain (enough for the example) or None."""
        if child not in self.children:
            return None
        first = self.children[child]
        if first.parent == parent:
            return first
        if first.parent in self.children:
            second = self.children[first.parent]
            if second.parent == parent:
                return Transform(first.stamp, parent, child,
                                 second.x + first.x, second.y + first.y,
                                 second.yaw + first.yaw)
        return None


def draw_interface(tf: TfTree, cmd_times: list[float],
                   joint_times: list[float], odom_times: list[float],
                   zero_events: list[float]) -> None:
    fig = plt.figure(figsize=(13.8, 5.4))
    gs = fig.add_gridspec(1, 2, width_ratios=[1.35, 1.0], wspace=0.16)
    ax = fig.add_subplot(gs[0, 0])
    ax.set_xlim(0, 12)
    ax.set_ylim(0, 9)
    ax.axis("off")

    def box(x, y, w, h, text, color="#dbe9f6", fs=8.5):
        ax.add_patch(FancyBboxPatch((x - w / 2, y - h / 2), w, h,
                                    boxstyle="round,pad=0.06,rounding_size=0.10",
                                    fc=color, ec="0.35", lw=1.0))
        ax.text(x, y, text, ha="center", va="center", fontsize=fs)

    def arrow(x0, y0, x1, y1, text="", color="0.25", ls="-", fs=7.5,
              tx=None, ty=None):
        ax.add_patch(FancyArrowPatch((x0, y0), (x1, y1), arrowstyle="-|>",
                                     mutation_scale=10, lw=1.1, color=color,
                                     linestyle=ls))
        if text:
            ax.text(tx if tx is not None else (x0 + x1) / 2,
                    ty if ty is not None else (y0 + y1) / 2 + 0.12,
                    text, fontsize=fs, color=color, ha="center")

    box(1.8, 7.4, 2.5, 0.65, "导航栈\nNav2 / move_base", "#e6d7f0")
    box(5.4, 7.4, 2.7, 0.65, "行走控制器\nState_move_base", "#f4cccc")
    box(9.0, 7.4, 2.2, 0.65, "Trot\n第 10 章", "#f4cccc")
    box(5.4, 5.2, 3.0, 0.65, "robot_state_publisher\nURDF + joint_states", "#eeeeee")
    box(1.8, 5.2, 2.3, 0.65, "RViz2\n显示 TF / 模型", "#eeeeee")
    box(1.6, 2.4, 2.0, 0.62, "odom", "#d9ead3")
    box(4.6, 2.4, 2.0, 0.62, "base", "#d9ead3")
    box(7.8, 3.7, 2.2, 0.62, "front_cam", "#d9ead3")
    box(7.8, 1.1, 2.4, 0.62, "left/right_cam", "#d9ead3")
    box(10.9, 2.4, 1.4, 0.62, "front\noptical", "#d9ead3", fs=7.5)

    arrow(3.05, 7.4, 4.05, 7.4, "/cmd_vel", tx=3.55, ty=7.80, fs=7)
    arrow(6.75, 7.4, 7.9, 7.4, "vx, vy, wz", tx=7.32, ty=7.80, fs=7)
    arrow(5.4, 7.05, 5.4, 5.55, "joint_states", tx=5.65, ty=6.3)
    arrow(5.4, 4.85, 5.4, 2.75, "/tf", tx=5.65, ty=3.9)
    arrow(5.6, 2.55, 6.7, 3.55, "静态 TF", tx=6.12, ty=2.98, fs=7)
    arrow(5.6, 2.25, 6.6, 1.25, "", tx=6.1, ty=1.7)
    arrow(8.9, 3.6, 10.2, 2.5, "", tx=9.55, ty=3.2)
    arrow(2.6, 2.4, 3.6, 2.4, "odom→base 50 Hz", tx=3.1, ty=1.68, fs=7)
    arrow(4.1, 5.2, 2.95, 5.2, "TF", ls="--", color="0.5", tx=3.5, ty=5.35)
    ax.text(0.2, 8.55, "TF 树：odom → base → 传感器/连杆；"
                       "实线为 TF，虚线为显示", fontsize=8.5, color="0.2")


    ax2 = fig.add_subplot(gs[0, 1])
    for t in odom_times:
        ax2.plot([t, t], [2.55, 2.90], color="C0", lw=1.0)
    for t in joint_times:
        ax2.plot([t, t], [1.55, 1.90], color="C2", lw=0.6)
    for t in cmd_times:
        ax2.plot([t, t], [0.55, 0.90], color="C3", lw=1.0)
    ax2.axvspan(1.0, 1.6, color="0.85", alpha=0.6)
    if zero_events:
        ax2.axvline(zero_events[0], color="k", ls="--", lw=1.0)
        ax2.text(zero_events[0] + 0.02, 2.95,
                 f"看门狗清零 {zero_events[0]:.2f} s", fontsize=8)
    ax2.axvline(1.6, color="C3", ls=":", lw=1.0)
    ax2.text(1.61, 0.25, "恢复发布", fontsize=8, color="C3")
    ax2.text(1.05, 2.95, "0.6 s 中断", fontsize=8, color="0.35")
    ax2.set_yticks([0.72, 1.72, 2.72])
    ax2.set_yticklabels(["/cmd_vel 20 Hz", "/joint_states 100 Hz",
                         "/odom + /tf 50 Hz"], fontsize=8)
    ax2.set_xlim(0.0, 2.0)
    ax2.set_ylim(0.0, 3.25)
    ax2.set_xlabel(label("时间 (s)", "time (s)"))
    ax2.grid(True, axis="x", alpha=0.3)
    ax2.set_title(label("(b) 发布时序与 0.5 s 看门狗", "(b) timing and watchdog"))
    fig.tight_layout()
    savefig(fig, "11_3_interface")
    plt.close(fig)


def main() -> None:
    bus = Bus()
    est = Estimator(bus)
    nav = Navigator(bus)
    ctl = WalkController(bus)
    cam = Camera(bus)
    tf_tree = TfTree()

    def on_tf(msg: Transform, arrive: float) -> None:
        tf_tree.update_odom(Transform(msg.stamp, msg.parent, msg.child,
                                      msg.x, msg.y, msg.yaw))

    bus.subscribe("/tf", on_tf)

    times = np.arange(0.0, DURATION + 1e-9, DT)
    first_scan = None
    scan_ages: list[float] = []
    failures: list[float] = []
    for now in times:
        est.step(now)
        nav.step(now)
        cam.step(now)
        ctl.update(now)
        for arrive, msg in bus.log.get("/scan", []):
            if abs(arrive - now) < 1e-9 and isinstance(msg, Header):
                if first_scan is None:
                    first_scan = msg
                scan_ages.append(arrive - msg.stamp)
                if tf_tree.lookup("base", msg.frame_id) is None:
                    failures.append(now)

    print("=== 话题统计 ===")
    print(f"仿真时长 {DURATION:.1f} s，控制周期 {DT * 1000:.0f} ms，"
          f"看门狗 {WATCHDOG:.2f} s")
    for topic, rate in (("/odom", 50.0), ("/tf", 50.0), ("/cmd_vel", 20.0),
                        ("/joint_states", 100.0), ("/scan", 10.0)):
        stamps = bus.published[topic]
        # /cmd_vel has a deliberate outage; use the median interval.
        stamps = np.asarray(stamps)
        gaps = np.diff(stamps)
        nominal = np.median(gaps) if len(gaps) else float("nan")
        print(f"{topic:<14} 发布 {len(stamps):3d} 帧，中位间隔 "
              f"{nominal * 1000:6.2f} ms，约 {1.0 / nominal:5.1f} Hz")

    print("=== 看门狗 ===")
    print(f"最后一条 /cmd_vel 在 0.95 s，暂停 0.60 s")
    print(f"清零事件 {[round(float(t), 3) for t in ctl.zero_events]}，"
          f"恢复到 0.30 m/s 的指令在 1.60 s")

    print("=== TF 链 ===")
    base = tf_tree.lookup("odom", "base")
    cam_tf = tf_tree.lookup("base", "front_cam")
    print(f"链 odom -> base -> front_cam：base 在 ({base.x:.2f}, {base.y:.2f})，"
          f"yaw {base.yaw:.2f} rad")
    print(f"base -> front_cam 平移 {math.hypot(cam_tf.x, cam_tf.y):.3f} m，"
          f"yaw {cam_tf.yaw:.2f} rad")
    bad = tf_tree.lookup("base", "camera_link")
    print(f"断裂测试：点云 frame_id=camera_link，可用链 front_cam；"
          f"lookup(base, camera_link) = {bad}")
    print(f"断裂帧数 {len(failures)}（10 Hz × 2 s 应全断）")
    cam.frame = "front_cam"
    fixed = tf_tree.lookup("base", "front_cam")
    print(f"修正后 frame_id=front_cam：lookup 成功，平移 "
          f"{math.hypot(fixed.x, fixed.y):.3f} m")

    print("=== joint_states ===")
    print(f"关节数 {len(JOINT_NAMES)}，名称匹配 URDF：{len(JOINT_NAMES)}/12")
    bad_names = ["FR_hip"] + list(JOINT_NAMES[1:])
    unknown = [n for n in bad_names if n not in JOINT_NAMES]
    print(f"故意写错第一个名字为 FR_hip：未知关节 {unknown}")
    mean_age = float(np.mean(scan_ages)) if scan_ages else 0.0
    print(f"/scan 时间戳年龄：均值 {mean_age * 1000:.1f} ms，"
          f"最大 {max(scan_ages) * 1000:.1f} ms" if scan_ages else
          "/scan 时间戳年龄：无数据")

    draw_interface(tf_tree, bus.published["/cmd_vel"],
                   bus.published["/joint_states"], bus.published["/odom"],
                   ctl.zero_events)
    print("图 11-5 导航—控制器接口的 TF 树与发布时序")


if __name__ == "__main__":
    main()
