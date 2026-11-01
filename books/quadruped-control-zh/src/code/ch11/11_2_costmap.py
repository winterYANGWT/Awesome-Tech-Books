"""A 2D costmap built from a static map, three laser scans and inflation (11.2).

The example models the data flow of a Nav2 ``costmap_2d`` without ROS:

* a 12 m x 8 m indoor occupancy grid (outer walls, one partition, one pillar);
* three 120-degree ``LaserScan``-like sensors mounted at 0, +90 and -90 degrees;
* an obstacle layer that marks the scan endpoints, including a box that is not
  in the static map;
* an inflation layer with an inscribed radius and an exponential decay, using
  a Euclidean distance transform;
* the combined cost map that a planner would consume.

The printed numbers are the ones quoted in Section 11.2.  The node/data-flow
diagram (Fig. 11-3) is drawn from the same dictionary of nodes and topics.

expect: 静态层
expect: 三路 LaserScan
expect: 障碍层
expect: 膨胀
expect: 可通行
expect: 最大代价
expect: 图 11-3
"""

from __future__ import annotations

import logging
import math
import os
import warnings

os.environ.setdefault("MPLCONFIGDIR", "/tmp/quadruped-mpl")

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch, Rectangle
from scipy.ndimage import distance_transform_edt

from quadruped.nav import GridMap
from quadruped.plotting import label, savefig

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)
logging.getLogger("matplotlib").setLevel(logging.ERROR)
warnings.filterwarnings("ignore", message=".*glyph.*")
warnings.filterwarnings("ignore", message=".*tight_layout.*")
plt.rcParams["mathtext.fontset"] = "dejavusans"

RES = 0.05
X_MIN, Y_MIN, X_MAX, Y_MAX = 0.0, 0.0, 12.0, 8.0
W = int(round((X_MAX - X_MIN) / RES))
H = int(round((Y_MAX - Y_MIN) / RES))
ROBOT = np.array([2.0, 2.0])
BOX = (3.6, 2.85, 0.20, 0.20)          # centre x, y, half x, half y


def rectangle(occ: np.ndarray, x0: float, y0: float, x1: float, y1: float,
              value: bool = True) -> None:
    ix0 = max(0, int((x0 - X_MIN) / RES))
    ix1 = min(W, int((x1 - X_MIN) / RES) + 1)
    iy0 = max(0, int((y0 - Y_MIN) / RES))
    iy1 = min(H, int((y1 - Y_MIN) / RES) + 1)
    occ[iy0:iy1, ix0:ix1] = value


def build_map() -> tuple[np.ndarray, GridMap]:
    occ = np.zeros((H, W), dtype=bool)
    # Outer shell, 0.2 m thick.
    rectangle(occ, 0.0, 0.0, 12.0, 0.2)
    rectangle(occ, 0.0, 7.8, 12.0, 8.0)
    rectangle(occ, 0.0, 0.0, 0.2, 8.0)
    rectangle(occ, 11.8, 0.0, 12.0, 8.0)
    # Partition from the south wall to y=5.5, doorway at the top.
    rectangle(occ, 6.0, 0.2, 6.2, 5.5)
    # Horizontal partition on the right, with a 1.4 m doorway.
    rectangle(occ, 6.2, 3.9, 8.8, 4.1)
    rectangle(occ, 10.2, 3.9, 11.8, 4.1)
    # A square pillar in the left room.
    rectangle(occ, 4.4, 5.4, 4.9, 5.9)
    grid = GridMap.from_occupancy(occ, RES, origin=(X_MIN, Y_MIN),
                                  robot_half_width=0.16, safety_margin=0.06)
    return occ, grid


def raycast(occ: np.ndarray, origin: np.ndarray, yaw: float,
            fov_deg: float = 120.0, n_beams: int = 181,
            max_range: float = 7.0) -> tuple[np.ndarray, np.ndarray]:
    """Sample rays on the grid; return ranges and endpoint cells."""
    angles = yaw + np.deg2rad(np.linspace(-fov_deg / 2, fov_deg / 2, n_beams))
    step = 0.02
    ranges = np.full(n_beams, np.inf)
    for i, angle in enumerate(angles):
        c, s = math.cos(angle), math.sin(angle)
        for r in np.arange(0.05, max_range + step, step):
            x = origin[0] + r * c
            y = origin[1] + r * s
            ix = int((x - X_MIN) / RES)
            iy = int((y - Y_MIN) / RES)
            if not (0 <= ix < W and 0 <= iy < H):
                break
            if occ[iy, ix]:
                ranges[i] = r
                break
    return angles, ranges


# ANCHOR: costs
def cost_layers(occ: np.ndarray, obstacle: np.ndarray,
                inscribed: float = 0.25, inflation: float = 0.55,
                weight: float = 3.0) -> tuple[np.ndarray, np.ndarray]:
    """Return (cost, distance) for the static+obstacle map.

    Lethal cells are 254, cells inside the inscribed radius are 253, and the
    rest of the inflation band decays as 252*exp(-weight*(d-inscribed)).
    """
    blocked = occ | obstacle
    dist = distance_transform_edt(~blocked) * RES
    cost = np.zeros_like(dist)
    free = ~blocked
    cost[blocked] = 254.0
    inside = free & (dist <= inscribed)
    cost[inside] = 253.0
    band = free & (dist > inscribed) & (dist <= inflation)
    cost[band] = 252.0 * np.exp(-weight * (dist[band] - inscribed))
    return cost, dist
# ANCHOR_END: costs


def draw_flow() -> None:
    fig, ax = plt.subplots(figsize=(12.2, 6.2))
    ax.set_xlim(0, 14)
    ax.set_ylim(0, 8)
    ax.axis("off")

    colors = {
        "map": "#dbe9f6", "sensor": "#fde2cf", "cost": "#d9ead3",
        "plan": "#e6d7f0", "ctrl": "#f4cccc", "viz": "#eeeeee",
    }

    def box(x, y, w, h, text, kind, fs=8.5):
        patch = FancyBboxPatch((x - w / 2, y - h / 2), w, h,
                               boxstyle="round,pad=0.06,rounding_size=0.10",
                               fc=colors[kind], ec="0.35", lw=1.0)
        ax.add_patch(patch)
        ax.text(x, y, text, ha="center", va="center", fontsize=fs)

    def arrow(x0, y0, x1, y1, text="", style="-|>", ls="-", color="0.25",
              rad=0.0, fs=7.5, tx=None, ty=None):
        arr = FancyArrowPatch((x0, y0), (x1, y1), arrowstyle=style,
                              mutation_scale=10, lw=1.1, color=color,
                              linestyle=ls,
                              connectionstyle=f"arc3,rad={rad}")
        ax.add_patch(arr)
        if text:
            ax.text(tx if tx is not None else (x0 + x1) / 2,
                    ty if ty is not None else (y0 + y1) / 2 + 0.12,
                    text, fontsize=fs, color=color, ha="center")

    box(1.6, 7.0, 2.0, 0.65, "map_server\n静态地图", "map")
    box(1.6, 5.3, 2.0, 0.65, "深度相机 / 2D 雷达\nPointCloud2 / LaserScan", "sensor")
    box(4.4, 5.3, 2.4, 0.65, "点云转激光节点\n高度带 + 角度扇面", "sensor")
    box(1.6, 3.5, 2.0, 0.65, "状态估计器\nodom + TF", "map")
    box(7.4, 6.5, 2.4, 0.75, "global_costmap\nstatic + obstacle + inflation", "cost")
    box(7.4, 4.7, 2.4, 0.75, "local_costmap\nrolling window", "cost")
    box(10.6, 6.5, 2.2, 0.65, "planner_server\nNavFn / Smac", "plan")
    box(10.6, 4.7, 2.2, 0.65, "controller_server\nDWB / RPP", "plan")
    box(10.6, 3.0, 2.2, 0.65, "cmd_vel\nTwistStamped", "ctrl")
    box(13.0, 3.0, 1.5, 0.65, "State_\nmove_base", "ctrl", fs=8)
    box(13.0, 1.1, 1.5, 0.65, "Trot\n(第 10 章)", "ctrl", fs=8)
    box(4.4, 1.1, 2.4, 0.65, "RViz2\nFixed Frame: odom", "viz")
    box(7.4, 1.1, 2.4, 0.65, "robot_state_publisher\nURDF + joint_states", "viz")

    arrow(2.6, 7.0, 6.1, 6.7, "/map")
    arrow(2.6, 5.3, 3.1, 5.3, "/points")
    arrow(5.6, 5.3, 6.2, 4.95, "/scan")
    arrow(2.6, 3.5, 6.1, 4.5, "/odom, /tf")
    arrow(8.6, 6.5, 9.4, 6.5, "")
    arrow(8.6, 4.7, 9.4, 4.7, "")
    arrow(10.6, 6.1, 10.6, 5.1, "/plan")
    arrow(11.6, 4.7, 11.6, 3.4, "/cmd_vel", tx=11.25, ty=4.0)
    arrow(11.7, 3.0, 12.2, 3.0, "")
    arrow(13.0, 2.6, 13.0, 1.5, "vx, vy, wz")
    arrow(8.0, 1.45, 8.0, 4.3, "/tf", tx=8.2, ty=2.6)
    arrow(12.2, 0.9, 8.65, 0.9, "joint_states", tx=10.4, ty=0.72)
    arrow(6.15, 4.3, 5.15, 1.5, "", ls="--", color="0.5", rad=-0.10)

    # TF note and the amcl path that this book does not enable.
    ax.plot([2.6, 3.6], [3.0, 3.0], color="0.35", lw=1.1, ls="--")
    ax.text(3.75, 3.0, "map → odom：nav2_amcl（本书不启用，虚线）",
            fontsize=7.5, va="center", color="0.35")
    ax.plot([2.6, 3.6], [2.65, 2.65], color="0.35", lw=1.1)
    ax.text(3.75, 2.65, "odom → base：第 7 章估计器（实线）",
            fontsize=7.5, va="center", color="0.35")

    ax.text(0.2, 7.8, label("感知数据（橙）→ 代价地图（绿）→ 规划（紫）→ 速度（红）",
                            "perception -> costmap -> planning -> velocity"),
            fontsize=9, color="0.2")
    ax.set_title(label("Nav2 的节点与数据流（对应 move_base 的全局/局部规划）",
                       "Nav2 nodes and data flow"), fontsize=11)
    fig.tight_layout()
    savefig(fig, "11_2_flow")
    plt.close(fig)


def draw_layers(occ: np.ndarray, obstacle: np.ndarray, cost: np.ndarray,
                dist: np.ndarray, scans: list[tuple[np.ndarray, np.ndarray]],
                n_before: int, n_after: int) -> None:
    extent = (X_MIN, X_MAX, Y_MIN, Y_MAX)
    fig, axes = plt.subplots(2, 2, figsize=(11.6, 7.2))

    ax = axes[0, 0]
    ax.imshow(occ, origin="lower", extent=extent, cmap="Greys",
              vmin=0, vmax=1, interpolation="nearest")
    ax.set_title(label("(a) 静态层：地图中的墙、隔墙、柱子",
                       "(a) static layer"))
    ax.plot(*ROBOT, "o", color="tab:blue", ms=5)
    ax.text(ROBOT[0] + 0.2, ROBOT[1] + 0.2, label("机器人", "robot"),
            fontsize=8, color="tab:blue")

    ax = axes[0, 1]
    layer = np.zeros((H, W))
    layer[obstacle] = 1.0
    ax.imshow(layer, origin="lower", extent=extent, cmap="Reds",
              vmin=0, vmax=1, interpolation="nearest")
    for angles, ranges in scans:
        finite = np.isfinite(ranges)
        for angle, r in list(zip(angles[finite], ranges[finite]))[::6]:
            ax.plot([ROBOT[0], ROBOT[0] + r * math.cos(angle)],
                    [ROBOT[1], ROBOT[1] + r * math.sin(angle)],
                    color="0.35", lw=0.35, alpha=0.55)
    ax.plot(*ROBOT, "o", color="tab:blue", ms=5)
    ax.set_title(label("(b) 障碍层：三路扫描的命中点（含未建图箱子）",
                       "(b) obstacle layer"))
    ax.add_patch(Rectangle((BOX[0] - BOX[2], BOX[1] - BOX[3]),
                           2 * BOX[2], 2 * BOX[3], fill=False,
                           edgecolor="tab:orange", lw=1.2))
    ax.text(BOX[0] + 0.18, BOX[1] + 0.18, label("未建图箱子", "unmapped box"),
            fontsize=8, color="tab:orange")

    ax = axes[1, 0]
    im = ax.imshow(cost, origin="lower", extent=extent, cmap="viridis",
                   vmin=0, vmax=254, interpolation="nearest")
    ax.set_title(label("(c) 膨胀层：内切 253、指数衰减到 0",
                       "(c) inflation layer"))
    cb = fig.colorbar(im, ax=ax, fraction=0.046, pad=0.02)
    cb.set_label(label("代价值", "cost"), fontsize=8)

    ax = axes[1, 1]
    ax.imshow(cost, origin="lower", extent=extent, cmap="magma",
              vmin=0, vmax=254, interpolation="nearest")
    ax.contour(occ, levels=[0.5], colors="w", linewidths=0.5,
               extent=extent, origin="lower")
    ax.plot(*ROBOT, "o", color="tab:cyan", ms=5)
    ax.set_title(label("(d) 合成代价地图：规划器看到的最终结果",
                       "(d) combined costmap"))
    cb = fig.colorbar(ax.images[-1], ax=ax, fraction=0.046, pad=0.02)
    cb.set_label(label("代价值", "cost"), fontsize=8)

    for ax in axes.ravel():
        ax.set_aspect("equal")
        ax.set_xlim(X_MIN, X_MAX)
        ax.set_ylim(Y_MIN, Y_MAX)
        ax.set_xlabel("x (m)")
        ax.set_ylabel("y (m)")
    fig.suptitle(label(
        f"代价地图三层：可通行栅格 {n_before} → {n_after}（分辨率 0.05 m）",
        f"costmap layers: free cells {n_before} -> {n_after}"), fontsize=11)
    fig.tight_layout()
    savefig(fig, "11_2_costmap")
    plt.close(fig)


def main() -> None:
    occ, grid = build_map()
    print("=== 静态层 ===")
    print(f"地图 {X_MAX - X_MIN:.1f} x {Y_MAX - Y_MIN:.1f} m，分辨率 {RES:.2f} m，"
          f"栅格 {W} x {H} = {W * H}")
    print(f"障碍 {int(occ.sum())} 格，空闲 {int((~occ).sum())} 格")
    print(f"GridMap 概率阈值：占据 {grid.occupied_threshold:.2f}，"
          f"空闲 {grid.free_threshold:.2f}")

    print("=== 三路 LaserScan ===")
    world = occ.copy()
    rectangle(world, BOX[0] - BOX[2], BOX[1] - BOX[3],
              BOX[0] + BOX[2], BOX[1] + BOX[3])
    sensors = [("前", 0.0), ("左", math.pi / 2), ("右", -math.pi / 2)]
    scans = []
    obstacle = np.zeros_like(occ)
    box_hits = 0
    for name, yaw in sensors:
        angles, ranges = raycast(world, ROBOT, yaw)
        finite = np.isfinite(ranges)
        scans.append((angles, ranges))
        for angle, r in zip(angles[finite], ranges[finite]):
            x = ROBOT[0] + r * math.cos(angle)
            y = ROBOT[1] + r * math.sin(angle)
            ix = int((x - X_MIN) / RES)
            iy = int((y - Y_MIN) / RES)
            if 0 <= ix < W and 0 <= iy < H:
                obstacle[iy, ix] = True
            if abs(x - BOX[0]) <= BOX[2] and abs(y - BOX[1]) <= BOX[3]:
                box_hits += 1
        print(f"{name}向：{angles.size} 束，命中 {int(finite.sum())}，"
              f"最近 {np.nanmin(ranges):.2f} m")
    print(f"障碍层命中 {int(obstacle.sum())} 格，其中未建图箱子 {box_hits} 束")

    cost, dist = cost_layers(occ, obstacle)
    before = int((~(occ | obstacle)).sum())
    after = int((cost < 253.0).sum())
    print("=== 膨胀 ===")
    print("inscribed_radius=0.25 m, inflation_radius=0.55 m, weight=3.0")
    print(f"可通行（膨胀前）: {before} 格")
    print(f"可通行（cost<253）: {after} 格，减少 {before - after} 格 "
          f"（{100 * (before - after) / before:.1f}%）")
    print(f"最大代价 {cost.max():.0f}，代价>0 的栅格 {int((cost > 0).sum())}")
    print(f"膨胀层最远距离 {dist[cost > 0].max():.2f} m")

    draw_flow()
    draw_layers(occ, obstacle, cost, dist, scans, before, after)
    print("图 11-3 Nav2 节点与数据流；图 11-4 静态/障碍/膨胀/合成代价地图")


if __name__ == "__main__":
    main()
