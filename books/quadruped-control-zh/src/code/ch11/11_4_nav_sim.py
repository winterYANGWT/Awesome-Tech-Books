"""Closed-loop navigation in an 8 m x 6 m MuJoCo room (11.4).

The robot is the A1 trot controller from Chapter 10.  The navigation stack is
``quadruped.nav``: ``Lidar2D`` ray-casts the room, ``GridMap`` integrates the
scan into a log-odds grid, ``AStarPlanner`` plans on the inflated grid and
``PurePursuit`` turns the path into ``(vx, wz)``.  The controller never reads
the simulator's ground-truth base pose; the truth is used only for scoring and
for the figures.

The room has an outer shell, three partitions (a serpentine) and two pillars.
The start is (0.5, 0.5), the goal is (7.0, 5.0).  Planning runs at 1 Hz, the
scan at 10 Hz and the control loop at 500 Hz.

expect: 场景
expect: 到达
expect: 终点
expect: 定位漂移
expect: 最小障碍距离
expect: 重规划
expect: A* 规划
expect: 速度
expect: 图 11-6
expect: 图 11-7
expect: 图 11-8
"""

from __future__ import annotations

import logging
import math
import os
import warnings
from dataclasses import dataclass
from pathlib import Path

os.environ.setdefault("MPLCONFIGDIR", "/tmp/quadruped-mpl")
os.environ.setdefault("MESA_SHADER_CACHE_DISABLE", "true")

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Circle, Rectangle

from quadruped import (A1Params, A1Sim, ControlContext, Quadruped,
                       TrottingController)
from quadruped.nav import GridMap, NavStack, PurePursuit, wrap_angle
from quadruped.plotting import label, savefig
from quadruped.types import JointCmd

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)
logging.getLogger("matplotlib").setLevel(logging.ERROR)
warnings.filterwarnings("ignore", message=".*glyph.*")
warnings.filterwarnings("ignore", message=".*tight_layout.*")
plt.rcParams["mathtext.fontset"] = "dejavusans"

ROOT = Path(__file__).resolve().parents[3]
SCENE = ROOT / "build" / "scratch" / "ch11" / "nav_room.xml"
A1_XML = ROOT / "src" / "code" / "assets" / "menagerie" / "unitree_a1" / "a1.xml"
A1_ASSETS = A1_XML.parent / "assets"

ROOM_XML = f"""<mujoco model="a1 ch11 room">
  <include file="{A1_XML}"/>
  <compiler angle="radian" meshdir="{A1_ASSETS}" texturedir="{A1_ASSETS}"/>

  <visual>
    <headlight diffuse="0.65 0.65 0.65" ambient="0.30 0.30 0.30" specular="0 0 0"/>
    <rgba haze="0.16 0.24 0.32 1"/>
    <global azimuth="125" elevation="-18"/>
  </visual>

  <asset>
    <texture type="skybox" builtin="gradient" rgb1="0.32 0.48 0.66" rgb2="0 0 0"
             width="512" height="3072"/>
    <texture type="2d" name="floor_tex" builtin="checker" mark="edge"
             rgb1="0.25 0.31 0.36" rgb2="0.16 0.21 0.26"
             markrgb="0.75 0.75 0.75" width="300" height="300"/>
    <material name="floor_mat" texture="floor_tex" texuniform="true"
              texrepeat="8 6" reflectance="0.15"/>
    <material name="wall_mat" rgba="0.58 0.60 0.62 1"/>
    <material name="pillar_mat" rgba="0.72 0.42 0.22 1"/>
  </asset>

  <worldbody>
    <light pos="4 3 4" dir="0 0 -1" directional="true"/>
    <body name="room_center" pos="4.0 3.0 0.0"/>
    <camera name="overview" mode="targetbody" target="room_center"
            pos="4.0 -4.2 7.2" fovy="45"/>
    <camera name="nav_view" mode="targetbody" target="trunk"
            pos="3.0 -3.0 3.4" fovy="55"/>
    <geom name="floor" size="0 0 0.05" type="plane" material="floor_mat"/>

    <geom name="wall_south" type="box" pos="4.0 0.05 0.30"
          size="4.0 0.05 0.30" material="wall_mat" group="1"/>
    <geom name="wall_north" type="box" pos="4.0 5.95 0.30"
          size="4.0 0.05 0.30" material="wall_mat" group="1"/>
    <geom name="wall_west" type="box" pos="0.05 3.0 0.30"
          size="0.05 3.0 0.30" material="wall_mat" group="1"/>
    <geom name="wall_east" type="box" pos="7.95 3.0 0.30"
          size="0.05 3.0 0.30" material="wall_mat" group="1"/>

    <geom name="wall_A" type="box" pos="1.8 1.70 0.30"
          size="0.05 1.70 0.30" material="wall_mat" group="1"/>
    <geom name="wall_B" type="box" pos="4.2 4.30 0.30"
          size="0.05 1.70 0.30" material="wall_mat" group="1"/>
    <geom name="wall_C" type="box" pos="6.2 1.9 0.30"
          size="0.05 1.9 0.30" material="wall_mat" group="1"/>

    <geom name="pillar_1" type="cylinder" pos="2.9 4.9 0.30"
          size="0.18 0.30" material="pillar_mat" group="1"/>
    <geom name="pillar_2" type="cylinder" pos="6.9 1.5 0.30"
          size="0.18 0.30" material="pillar_mat" group="1"/>
  </worldbody>
</mujoco>
"""


def scene_path() -> Path:
    """Write the chapter's MuJoCo scene next to the draft output."""
    SCENE.parent.mkdir(parents=True, exist_ok=True)
    if not SCENE.exists() or SCENE.read_text(encoding="utf-8") != ROOM_XML:
        SCENE.write_text(ROOM_XML, encoding="utf-8")
    return SCENE


START = np.array([0.5, 0.5])
GOAL = np.array([7.0, 5.0])
DT = 0.002
MAX_TIME = 60.0

# name, centre x, centre y, half x, half y
WALLS = [
    ("wall_south", 4.0, 0.05, 4.0, 0.05),
    ("wall_north", 4.0, 5.95, 4.0, 0.05),
    ("wall_west", 0.05, 3.0, 0.05, 3.0),
    ("wall_east", 7.95, 3.0, 0.05, 3.0),
    ("wall_A", 1.8, 1.70, 0.05, 1.70),
    ("wall_B", 4.2, 4.30, 0.05, 1.70),
    ("wall_C", 6.2, 1.9, 0.05, 1.9),
]
PILLARS = [(2.9, 4.9, 0.18), (6.9, 1.5, 0.18)]


@dataclass
class Run:
    truth: np.ndarray
    estimate: np.ndarray
    command: np.ndarray
    clearance: np.ndarray
    error: np.ndarray
    goal_distance: np.ndarray
    paths: list[np.ndarray]
    plan_times: list[float]
    plan_lengths: list[float]
    plan_expanded: list[int]
    scans: int
    plans: int
    failed: int
    collisions: int
    fell: bool
    arrival_nav: float | None
    arrival_truth: float | None
    scene_image: np.ndarray


def obstacle_distance(point: np.ndarray) -> float:
    """Body-centre distance to the nearest wall or pillar surface (2D)."""
    best = math.inf
    for _, cx, cy, hx, hy in WALLS:
        dx = max(abs(point[0] - cx) - hx, 0.0)
        dy = max(abs(point[1] - cy) - hy, 0.0)
        best = min(best, math.hypot(dx, dy))
    for cx, cy, radius in PILLARS:
        best = min(best, max(math.hypot(point[0] - cx, point[1] - cy)
                             - radius, 0.0))
    return float(best)


GRID_RES = 0.05
GRID_X_MIN, GRID_Y_MIN = -0.5, -0.5
GRID_X_MAX, GRID_Y_MAX = 8.5, 6.5


def known_grid() -> GridMap:
    """Build the surveyed room map through the public occupancy constructor."""
    width = int(round((GRID_X_MAX - GRID_X_MIN) / GRID_RES))
    height = int(round((GRID_Y_MAX - GRID_Y_MIN) / GRID_RES))
    xs = GRID_X_MIN + (np.arange(width) + 0.5) * GRID_RES
    ys = GRID_Y_MIN + (np.arange(height) + 0.5) * GRID_RES
    X, Y = np.meshgrid(xs, ys)
    inside = (X > 0.10) & (X < 7.90) & (Y > 0.10) & (Y < 5.90)
    blocked = ~inside
    for _, cx, cy, hx, hy in WALLS:
        blocked |= (np.abs(X - cx) <= hx) & (np.abs(Y - cy) <= hy)
    for cx, cy, radius in PILLARS:
        blocked |= (X - cx) ** 2 + (Y - cy) ** 2 <= radius ** 2
    return GridMap.from_occupancy(
        blocked, GRID_RES, origin=(GRID_X_MIN, GRID_Y_MIN),
        robot_half_width=0.16, safety_margin=0.39)


# ANCHOR: boot
def boot() -> tuple[A1Params, Quadruped, A1Sim, TrottingController]:
    params = A1Params.from_menagerie()
    robot = Quadruped(params)
    sim = A1Sim(model_path=str(scene_path()), render_size=(640, 480))
    sim.reset(base_pos=(START[0], START[1], 0.27))

    # Let the free base settle on its four feet, then trot in place briefly.
    q0 = params.home_q.copy()
    for _ in range(500):
        sim.step(JointCmd(q0, np.zeros(12),
                          np.array([100.0, 300.0, 400.0] * 4),
                          np.array([4.0, 8.0, 8.0] * 4), np.zeros(12)))
    context = ControlContext.from_sim(sim, params)
    trot = TrottingController(context, robot=robot, params=params)
    trot.enter()

    # The estimator and the gait anchor start at the origin.  Declare the
    # surveyed start pose by translating both, including the four foot
    # positions; translating only x/y would leave the filter inconsistent.
    trot.estimator.x[0:2] = START
    feet = trot.estimator.x[6:18].reshape(4, 3)
    feet[:, 0] += START[0]
    feet[:, 1] += START[1]
    trot.estimator.x[6:18] = feet.reshape(12)
    trot.p_des[0:2] = START

    for _ in range(800):
        trot.run()
        sim.step()
    return params, robot, sim, trot
# ANCHOR_END: boot


# ANCHOR: loop
def run_navigation() -> Run:
    params, robot, sim, trot = boot()
    start_truth, _ = sim.get_state()
    print("=== 场景 ===")
    print(f"MuJoCo 房间 8.0 x 6.0 m，外墙 + 3 道隔墙 + 2 根圆柱")
    print(f"起点 {START.round(2)}，目标 {GOAL.round(2)}，栅格 0.05 m，"
          f"膨胀 0.16 + 0.39 m，已知地图 + 激光障碍层")
    print(f"起步真值 {start_truth.position[:2].round(3)}，"
          f"估计 {trot.est_base.position[:2].round(3)}")
    scene_image = sim.render(camera="overview", width=640, height=480)

    grid = known_grid()
    tracker = PurePursuit(lookahead=0.50, v_max=0.25, w_max=0.9,
                          goal_tolerance=0.15, slow_radius=0.9)
    nav = NavStack(sim, trot, GOAL, grid=grid, scan_hz=10.0, plan_hz=1.0,
                   control_dt=DT, arrival_tolerance=0.15, goal_margin=2.0,
                   robot_half_width=0.16, safety_margin=0.39,
                   noise_std=0.01, seed=0, pursuit=tracker)
    nav.reset(keep_map=True)   # keep the surveyed static layer

    wall_ids = {sim.model.geom(name).id
                for name, *_ in WALLS}
    wall_ids |= {sim.model.geom(f"pillar_{i + 1}").id
                 for i in range(len(PILLARS))}

    truth, estimate, command, clearance = [], [], [], []
    error, goal_distance = [], []
    paths: list[np.ndarray] = []
    plan_times: list[float] = []
    plan_lengths: list[float] = []
    plan_expanded: list[int] = []
    last_plans = 0
    collisions = 0
    fell = False
    arrival_nav = None
    arrival_truth = None
    settle_start = None
    first_scan = None

    for k in range(int(MAX_TIME / DT)):
        nav.step()
        trot.run()
        base = sim.step()
        t = (k + 1) * DT

        if first_scan is None and nav.scan_history:
            first_scan = nav.scan_history[0]
        if nav.stats.plans != last_plans:
            last_plans = nav.stats.plans
            if len(nav.path) >= 2:
                paths.append(nav.path.copy())
            plan_times.append(nav.planner.last_time)
            plan_lengths.append(nav.planner.last_length)
            plan_expanded.append(nav.planner.last_expanded)

        p = np.asarray(base.position, dtype=float).copy()
        e = np.asarray(trot.est_base.position, dtype=float).copy()
        truth.append([t, *p, *np.asarray(base.rpy, dtype=float)])
        estimate.append([t, *e, float(trot.est_base.rpy[2])])
        command.append([t, *nav.last_command])
        error.append([t, float(np.linalg.norm(e[:2] - p[:2]))])
        goal_distance.append([t, float(np.linalg.norm(p[:2] - GOAL))])
        clearance.append([t, obstacle_distance(p[:2])])

        if arrival_truth is None and goal_distance[-1][1] < 0.30:
            arrival_truth = t
        if nav.arrived and arrival_nav is None:
            arrival_nav = t
            settle_start = t

        hits = []
        for c in range(sim.data.ncon):
            contact = sim.data.contact[c]
            g1, g2 = int(contact.geom1), int(contact.geom2)
            if g1 in wall_ids:
                hits.append(sim.model.geom(g1).name)
            elif g2 in wall_ids:
                hits.append(sim.model.geom(g2).name)
        if hits:
            collisions += 1
            print(f"碰撞 at t={t:.2f}: {hits[:2]}")
            break
        if p[2] < 0.12 or abs(base.rpy[0]) > 0.6 or abs(base.rpy[1]) > 0.6:
            fell = True
            print(f"摔倒 at t={t:.2f}: z={p[2]:.3f} rpy={np.round(base.rpy, 3)}")
            break
        if settle_start is not None and t - settle_start >= 1.0:
            break
    # ANCHOR_END: loop

    truth = np.asarray(truth)
    estimate = np.asarray(estimate)
    command = np.asarray(command)
    clearance = np.asarray(clearance)
    error = np.asarray(error)
    goal_distance = np.asarray(goal_distance)

    final = truth[-1, 1:4]
    final_est = estimate[-1, 1:4]
    print("=== 到达 ===")
    print(f"NavStack 判定到达: {arrival_nav if arrival_nav is not None else '未到达'} s")
    print(f"真值首次 <0.30 m: {arrival_truth if arrival_truth is not None else '未到达'} s")
    print(f"终点真值 {final[:2].round(3)}，到目标 "
          f"{np.linalg.norm(final[:2] - GOAL):.3f} m")
    print(f"终点估计 {final_est[:2].round(3)}，定位漂移 "
          f"{np.linalg.norm(final_est[:2] - final[:2]):.3f} m")
    print(f"漂移 RMSE / 最大: {np.sqrt(np.mean(error[:, 1] ** 2)):.3f} / "
          f"{error[:, 1].max():.3f} m")
    yaw_err = np.array([abs(wrap_angle(estimate[i, 3] - truth[i, 4]))
                        for i in range(len(truth))])
    print(f"偏航 RMSE: {np.sqrt(np.mean(yaw_err ** 2)):.3f} rad")
    print(f"实际路径长度: "
          f"{np.sum(np.linalg.norm(np.diff(truth[:, 1:3], axis=0), axis=1)):.3f} m")
    print(f"最小障碍距离（机身中心）: {clearance[:, 1].min():.3f} m，"
          f"碰撞 {collisions} 次，摔倒 {'是' if fell else '否'}")
    print("=== 规划 ===")
    print(f"扫描/规划 {nav.stats.scans} / {nav.stats.plans}，失败 "
          f"{nav.stats.failed_plans}，重规划 {max(0, nav.stats.plans - 1)} 次")
    if plan_expanded:
        print(f"A* 首次: 扩展 {plan_expanded[0]} 节点，路径 "
              f"{plan_lengths[0]:.2f} m；最大扩展 {max(plan_expanded)} 节点，"
              f"最长路径 {max(plan_lengths):.2f} m")
    acc = np.diff(command[:, 1]) / DT
    print("=== 速度 ===")
    print(f"vx 最大 {command[:, 1].max():.3f} m/s，wz 最大 "
          f"{np.abs(command[:, 2]).max():.3f} rad/s，"
          f"加速度 RMS {np.sqrt(np.mean(acc ** 2)):.2f} m/s²")

    global run_global_grid, run_global_last_scan, run_global_first_scan
    run_global_grid = grid
    run_global_last_scan = nav.scan_history[-1] if nav.scan_history else first_scan
    run_global_first_scan = first_scan
    sim.close()
    return Run(truth, estimate, command, clearance, error, goal_distance,
               paths, plan_times, plan_lengths, plan_expanded,
               nav.stats.scans, nav.stats.plans, nav.stats.failed_plans,
               collisions, fell, arrival_nav, arrival_truth, scene_image)


# ---------------------------------------------------------------------------
# Figures
# ---------------------------------------------------------------------------
def draw_walls(ax, color="0.35", lw=2.0, labels=False):
    for name, cx, cy, hx, hy in WALLS:
        ax.add_patch(Rectangle((cx - hx, cy - hy), 2 * hx, 2 * hy,
                               fc=color, ec="none", zorder=2))
        if labels:
            ax.text(cx, cy, name.replace("wall_", ""), fontsize=6,
                    ha="center", va="center", color="white", zorder=3)
    for cx, cy, radius in PILLARS:
        ax.add_patch(Circle((cx, cy), radius, fc=color, ec="none", zorder=2))


def draw_scene(run: Run) -> None:
    first_scan = run_global_first_scan
    fig = plt.figure(figsize=(12.4, 4.8))
    gs = fig.add_gridspec(1, 2, width_ratios=[1.0, 1.05], wspace=0.12)
    ax = fig.add_subplot(gs[0, 0])
    ax.imshow(run.scene_image)
    ax.axis("off")
    ax.set_title(label("(a) MuJoCo 室内场景（俯瞰）", "(a) MuJoCo room"))
    ax.text(0.03, 0.06, label("起点：左下角", "start: bottom-left"),
            transform=ax.transAxes, fontsize=8, color="white",
            bbox=dict(fc="0.1", alpha=0.65, ec="none"))
    ax.text(0.72, 0.82, label("目标：右上角", "goal: top-right"),
            transform=ax.transAxes, fontsize=8, color="white",
            bbox=dict(fc="0.1", alpha=0.65, ec="none"))

    ax = fig.add_subplot(gs[0, 1])
    draw_walls(ax, color="0.30", lw=2.0, labels=True)
    if first_scan is not None:
        origin, directions, ranges = first_scan
        finite = np.isfinite(ranges)
        for d, r in list(zip(directions[finite], ranges[finite]))[::2]:
            end = origin + float(r) * d
            ax.plot([origin[0], end[0]], [origin[1], end[1]], color="C3",
                    lw=0.35, alpha=0.55)
        ends = origin[None, :] + np.where(finite, ranges, 0.0)[:, None] * directions
        ax.plot(ends[finite, 0], ends[finite, 1], ".", color="C3", ms=1.8)
        ax.plot(origin[0], origin[1], "s", color="C2", ms=6,
                label=label("雷达", "lidar"))
        ax.set_title(label(
            f"(b) 起点 360 束扫描：命中 {int(finite.sum())}，"
            f"无回波 {int((~finite).sum())}",
            f"(b) 360-beam scan: {int(finite.sum())} hits"))
    ax.scatter([START[0]], [START[1]], marker="o", s=40, c="tab:blue",
               edgecolors="k", zorder=6, label=label("起点", "start"))
    ax.scatter([GOAL[0]], [GOAL[1]], marker="*", s=180, c="tab:orange",
               edgecolors="k", zorder=6, label=label("目标", "goal"))
    ax.set_aspect("equal")
    ax.set_xlim(-0.2, 8.2)
    ax.set_ylim(-0.2, 6.2)
    ax.set_xlabel("x (m)")
    ax.set_ylabel("y (m)")
    ax.legend(loc="upper left", fontsize=8)
    ax.grid(True, alpha=0.2)
    fig.tight_layout()
    savefig(fig, "11_4_scene")
    plt.close(fig)


def draw_rviz(run: Run) -> None:
    fig = plt.figure(figsize=(11.8, 5.8), facecolor="#202020")
    gs = fig.add_gridspec(1, 2, width_ratios=[0.24, 1.0], wspace=0.02)
    side = fig.add_subplot(gs[0, 0])
    side.set_facecolor("#2b2b2b")
    side.set_xticks([])
    side.set_yticks([])
    side.text(0.06, 0.95, "Displays", color="white", fontsize=10,
              va="top", fontweight="bold")
    items = [
        ("Grid", True), ("LaserScan (/scan)", True),
        ("Map (global_costmap)", True), ("Path (/plan)", True),
        ("RobotModel", True), ("TF", True),
    ]
    for i, (name, on) in enumerate(items):
        side.text(0.08, 0.86 - i * 0.08, ("☑ " if on else "☐ ") + name,
                  color="#d8d8d8", fontsize=8, va="top")
    side.text(0.06, 0.26, "Fixed Frame:", color="#9fd3ff", fontsize=8)
    side.text(0.06, 0.21, "odom", color="white", fontsize=10)
    side.text(0.06, 0.10, "2D Goal Pose", color="#ffd27f", fontsize=8)
    side.text(0.06, 0.05, "Global Plan: /plan", color="#9fd3ff", fontsize=7)
    side.set_title("RViz2", color="white", fontsize=9)

    ax = fig.add_subplot(gs[0, 1])
    ax.set_facecolor("#101010")
    prob = np.asarray(run_global_grid.probability) if run_global_grid else None
    if prob is not None:
        ax.imshow(prob, origin="lower", extent=(-0.5, 8.5, -0.5, 6.5),
                  cmap="magma", vmin=0.0, vmax=1.0, alpha=0.75)
    draw_walls(ax, color="#606060", lw=1.6)
    # All planned paths faint, the longest one bold.
    for path in run.paths:
        if len(path) >= 2:
            ax.plot(path[:, 0], path[:, 1], color="#3a6ea5", lw=0.7,
                    alpha=0.45)
    if run.paths:
        shown = max(run.paths, key=lambda p: float(
            np.sum(np.linalg.norm(np.diff(p, axis=0), axis=1))))
        ax.plot(shown[:, 0], shown[:, 1], color="#59a5ff", lw=2.0,
                label=label("Global Plan", "global plan"))
    ax.plot(run.truth[:, 1], run.truth[:, 2], color="#7CFC00", lw=1.2,
            label=label("实际轨迹", "truth trajectory"))
    # Last scan points.
    if run_global_last_scan is not None:
        origin, directions, ranges = run_global_last_scan
        finite = np.isfinite(ranges)
        ends = origin[None, :] + np.where(finite, ranges, 0.0)[:, None] * directions
        ax.plot(ends[finite, 0], ends[finite, 1], ".", color="#00e5ff", ms=2.0,
                label=label("LaserScan", "laser scan"))
    # Robot footprint and body axes at the final estimate.
    px, py = run.estimate[-1, 1], run.estimate[-1, 2]
    yaw = run.estimate[-1, 3]
    body = np.array([[-0.30, -0.15], [0.30, -0.15],
                     [0.30, 0.15], [-0.30, 0.15], [-0.30, -0.15]])
    R = np.array([[math.cos(yaw), -math.sin(yaw)],
                  [math.sin(yaw), math.cos(yaw)]])
    body = body @ R.T + np.array([px, py])
    ax.plot(body[:, 0], body[:, 1], color="white", lw=1.4,
            label=label("RobotModel", "robot model"))
    ax.arrow(px, py, 0.35 * math.cos(yaw), 0.35 * math.sin(yaw),
             width=0.012, color="tab:red", length_includes_head=True)
    ax.arrow(px, py, -0.25 * math.sin(yaw), 0.25 * math.cos(yaw),
             width=0.012, color="tab:green", length_includes_head=True)
    ax.scatter([GOAL[0]], [GOAL[1]], marker="*", s=180, c="tab:orange",
               edgecolors="white", zorder=6)
    ax.set_aspect("equal")
    ax.set_xlim(-0.4, 8.4)
    ax.set_ylim(-0.4, 6.4)
    ax.set_xlabel("x (m)", color="white")
    ax.set_ylabel("y (m)", color="white")
    ax.tick_params(colors="white", labelsize=8)
    for spine in ax.spines.values():
        spine.set_color("0.6")
    ax.grid(True, color="0.35", alpha=0.35)
    ax.set_title(label("RViz 风格：代价地图、全局路径、激光与机器人模型",
                       "RViz-style view: costmap, path, scan, robot"),
                 color="white", fontsize=10)
    ax.legend(loc="upper left", fontsize=8, facecolor="#202020",
              labelcolor="white", framealpha=0.8)
    fig.tight_layout()
    savefig(fig, "11_4_rviz")
    plt.close(fig)


def draw_track(run: Run) -> None:
    fig = plt.figure(figsize=(12.0, 5.4))
    gs = fig.add_gridspec(2, 2, width_ratios=[1.1, 1.0], height_ratios=[1, 1],
                          wspace=0.24, hspace=0.38)
    ax = fig.add_subplot(gs[:, 0])
    draw_walls(ax, color="0.55", lw=1.8, labels=True)
    for path in run.paths:
        if len(path) >= 2:
            ax.plot(path[:, 0], path[:, 1], color="0.65", lw=0.7, alpha=0.8)
    if run.paths:
        shown = max(run.paths, key=lambda p: float(
            np.sum(np.linalg.norm(np.diff(p, axis=0), axis=1))))
        ax.plot(shown[:, 0], shown[:, 1], "--", color="tab:blue", lw=1.4,
                label=label("一条规划路径", "a planned path"))
    sc = ax.scatter(run.truth[:, 1], run.truth[:, 2], c=run.truth[:, 0],
                    cmap="viridis", s=4, label=label("实际轨迹", "truth"))
    ax.scatter([START[0]], [START[1]], marker="o", s=40, c="k")
    ax.scatter([GOAL[0]], [GOAL[1]], marker="*", s=170, c="tab:orange",
               edgecolors="k")
    i = int(np.argmin(run.clearance[:, 1]))
    ax.plot(run.truth[i, 1], run.truth[i, 2], "o", mfc="none", mec="tab:red",
            ms=14, mew=1.6)
    ax.text(run.truth[i, 1] + 0.18, run.truth[i, 2] + 0.18,
            f"最近 {run.clearance[i, 1]:.2f} m", color="tab:red", fontsize=8)
    cb = fig.colorbar(sc, ax=ax, fraction=0.045, pad=0.02)
    cb.set_label(label("时间 (s)", "time (s)"), fontsize=8)
    ax.set_aspect("equal")
    ax.set_xlim(-0.2, 8.2)
    ax.set_ylim(-0.2, 6.2)
    ax.set_xlabel("x (m)")
    ax.set_ylabel("y (m)")
    ax.legend(loc="upper left", fontsize=8)
    ax.grid(True, alpha=0.2)
    ax.set_title(label("(a) 规划路径与实际轨迹", "(a) plan and trajectory"))

    ax = fig.add_subplot(gs[0, 1])
    ax.plot(run.command[:, 0], run.command[:, 1], color="tab:blue", lw=1.2,
            label=r"$v_x$ (m/s)")
    ax.plot(run.command[:, 0], run.command[:, 2], color="tab:orange", lw=1.2,
            label=r"$\omega_z$ (rad/s)")
    ax.set_ylabel(label("速度命令", "command"))
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=8, loc="upper right")
    ax.set_title(label("(b) 500 Hz 控制循环里的速度命令", "(b) commands"))

    ax = fig.add_subplot(gs[1, 1])
    ax.plot(run.goal_distance[:, 0], run.goal_distance[:, 1], color="tab:green",
            lw=1.3, label=label("真值到目标", "truth to goal"))
    ax.plot(run.error[:, 0], run.error[:, 1], color="tab:red", lw=1.1,
            label=label("定位漂移", "drift"))
    ax.set_xlabel(label("时间 (s)", "time (s)"))
    ax.set_ylabel("m")
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=8, loc="upper right")
    ax.set_title(label("(c) 到点距离与定位漂移", "(c) distance and drift"))
    fig.tight_layout()
    savefig(fig, "11_4_track")
    plt.close(fig)


# Globals used by the RViz-style figure (set in main).
run_global_grid: GridMap | None = None
run_global_last_scan = None
run_global_first_scan = None


def main() -> None:
    global run_global_grid, run_global_last_scan
    # Re-run the stack once, keeping the map/scan for the figures.  This is
    # the same code path as run_navigation; the duplicate run is avoided by
    # attaching the objects in run_navigation below.
    run = run_navigation()
    # The map and last scan are captured by the caller through the globals
    # that run_navigation fills in (see the tail of that function).
    draw_scene(run)
    draw_rviz(run)
    draw_track(run)
    print("图 11-6 室内场景与激光扫描；图 11-7 RViz 风格导航视图；"
          "图 11-8 规划路径与实际轨迹")


if __name__ == "__main__":
    main()
