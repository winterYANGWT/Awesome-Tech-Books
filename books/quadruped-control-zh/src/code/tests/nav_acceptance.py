"""End-to-end acceptance for the Chapter 11 navigation stack.

Run from the repository root:

    MUJOCO_GL=egl PYTHONPATH=src/code build/venv/bin/python \
        src/code/tests/nav_acceptance.py

The A1 starts from its home stance in ``assets/nav_scene.xml``, stands for
1 s, enters the trot state, and then navigates to a goal 3 m ahead.  The
middle crate blocks the straight line, so the robot has to steer around it.

The navigation stack reads only ``A1Sim.read_sensors()`` and the Chapter 7
estimator.  MuJoCo's true base pose is used here only to score the result and
to draw the figures; it never enters ``NavStack``.

Figures and a text summary go to ``build/scratch/pkg-nav/``.
"""

from __future__ import annotations

import os
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src" / "code"))
os.environ.setdefault("MUJOCO_GL", "egl")
os.environ.setdefault("MPLCONFIGDIR",
                      str(Path(tempfile.gettempdir()) / "quadruped-mpl"))

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Rectangle

from quadruped.fsm import ControlContext
from quadruped.nav import GridMap, NavStack
from quadruped.plotting import label, savefig
from quadruped.robot import A1Params, Quadruped
from quadruped.sim import A1Sim
from quadruped.trotting import TrottingController
from quadruped.types import JointCmd

OUT = ROOT / "build" / "scratch" / "pkg-nav"
OUT.mkdir(parents=True, exist_ok=True)
os.environ["FIG_DIR"] = str(OUT)
XML = ROOT / "src" / "code" / "assets" / "nav_scene.xml"

GOAL = np.array([3.0, 0.0])
START = np.array([0.0, 0.0])
ARRIVAL_TRUTH = 0.30          # acceptance criterion, ground truth
MAX_TIME = 60.0
CONTROL_DT = 0.002

# Scene boxes: centres and half-sizes, used only for scoring/plotting.  The
# values mirror nav_scene.xml; the navigation stack itself sees them only
# through the lidar.
BOXES = (
    ("crate_mid", np.array([1.35, 0.0]), np.array([0.30, 0.45])),
    ("crate_left", np.array([2.25, 0.95]), np.array([0.25, 0.30])),
    ("crate_right", np.array([2.25, -0.95]), np.array([0.25, 0.30])),
)


def boot():
    params = A1Params.from_menagerie()
    robot = Quadruped(params)
    sim = A1Sim(model_path=str(XML), render_size=(640, 480))
    # Let the free base settle on its four feet before the controller starts.
    q0 = params.home_q.copy()
    for _ in range(500):
        sim.step(JointCmd(q0, np.zeros(12),
                          np.array([100.0, 300.0, 400.0] * 4),
                          np.array([4.0, 8.0, 8.0] * 4), np.zeros(12)))
    context = ControlContext.from_sim(sim, params)
    trot = TrottingController(context, robot=robot, params=params)
    trot.enter()
    for _ in range(300):                 # 0.6 s of trot-in-place transition
        trot.run()
        sim.step()
    return params, robot, sim, trot


def box_distance(point_xy: np.ndarray) -> float:
    """Distance from a point to the nearest crate surface (2D, metres)."""
    best = np.inf
    for _, centre, half in BOXES:
        dx = max(abs(point_xy[0] - centre[0]) - half[0], 0.0)
        dy = max(abs(point_xy[1] - centre[1]) - half[1], 0.0)
        best = min(best, float(np.hypot(dx, dy)))
    return best


def obstacle_contacts(sim, crate_ids: set[int]) -> list[str]:
    """Return contact descriptions between a crate and the robot."""
    hits: list[str] = []
    for i in range(sim.data.ncon):
        contact = sim.data.contact[i]
        g1, g2 = int(contact.geom1), int(contact.geom2)
        if g1 not in crate_ids and g2 not in crate_ids:
            continue
        other = g2 if g1 in crate_ids else g1
        body = sim.model.body(sim.model.geom_bodyid[other]).name
        hits.append(f"{sim.model.geom(g1 if g1 in crate_ids else g2).name}"
                    f"<->{body}")
    return hits


def run():
    params, robot, sim, trot = boot()
    crate_ids = {sim.model.geom(name).id for name, _, _ in BOXES}
    base0, _ = sim.get_state()
    print(f"start truth      : {np.round(base0.position, 4)}")
    print(f"start estimate   : {np.round(trot.est_base.position, 4)}")
    print(f"true distance    : {np.linalg.norm(base0.position[:2] - GOAL):.3f} m")

    grid = GridMap(x_min=-1.0, y_min=-2.5, x_max=5.0, y_max=2.5,
                   resolution=0.05, robot_half_width=0.16,
                   safety_margin=0.06)
    nav = NavStack(sim, trot, GOAL, grid=grid, scan_hz=10.0, plan_hz=2.0,
                   control_dt=CONTROL_DT, arrival_tolerance=0.15,
                   goal_margin=1.5, robot_half_width=0.16,
                   safety_margin=0.06, noise_std=0.01, seed=0)
    nav.reset()

    t = 0.0
    truth = []
    estimate = []
    commands = []
    errors = []
    distances = []
    clearances = []
    collision = []
    fell = False
    arrival_truth_time = None
    arrival_nav_time = None
    settle_start = None
    wall_start = time.perf_counter()
    steps = int(MAX_TIME / CONTROL_DT)
    for k in range(steps):
        nav.step()
        trot.run()
        base = sim.step()
        est = trot.est_base
        t = (k + 1) * CONTROL_DT
        p = np.asarray(base.position, dtype=float).copy()
        e = np.asarray(est.position, dtype=float).copy()
        truth.append([t, *p, *np.asarray(base.rpy, dtype=float)])
        estimate.append([t, *e, *np.asarray(est.rpy, dtype=float)])
        commands.append([t, *nav.last_command])
        errors.append([t, float(np.linalg.norm(e[:2] - p[:2]))])
        d_goal = float(np.linalg.norm(p[:2] - GOAL))
        distances.append([t, d_goal])
        clearances.append([t, box_distance(p[:2])])
        if arrival_truth_time is None and d_goal < ARRIVAL_TRUTH:
            arrival_truth_time = t
        if nav.arrived and arrival_nav_time is None:
            arrival_nav_time = t
            settle_start = t
        hits = obstacle_contacts(sim, crate_ids)
        if hits:
            collision.append((t, hits))
            print(f"collision at t={t:.3f}: {hits}")
            break
        if p[2] < 0.15 or abs(base.rpy[0]) > 0.5 or abs(base.rpy[1]) > 0.5:
            fell = True
            print(f"fall at t={t:.3f}: z={p[2]:.3f} rpy={np.round(base.rpy, 3)}")
            break
        if arrival_nav_time is not None and t - arrival_nav_time >= 1.0:
            break

    wall_time = time.perf_counter() - wall_start
    truth = np.asarray(truth)
    estimate = np.asarray(estimate)
    commands = np.asarray(commands)
    errors = np.asarray(errors)
    distances = np.asarray(distances)
    clearances = np.asarray(clearances)
    final_truth = truth[-1, 1:4]
    final_est = estimate[-1, 1:4]
    final_error = float(np.linalg.norm(final_truth[:2] - GOAL))
    final_drift = float(np.linalg.norm(final_est[:2] - final_truth[:2]))
    drift_rmse = float(np.sqrt(np.mean(errors[:, 1] ** 2)))
    drift_max = float(errors[:, 1].max())
    yaw_err = np.abs(estimate[:, -1] - truth[:, -1])
    yaw_err = np.minimum(yaw_err, 2.0 * np.pi - yaw_err)
    min_clearance = float(clearances[:, 1].min())
    path_length = float(np.sum(np.linalg.norm(np.diff(truth[:, 1:3], axis=0),
                                              axis=1)))
    print("--------------------------------------------------------")
    print(f"arrival (nav/truth): {arrival_nav_time} / {arrival_truth_time} s")
    print(f"final truth        : {np.round(final_truth, 4)}")
    print(f"final estimate     : {np.round(final_est, 4)}")
    print(f"final goal error   : {final_error:.4f} m")
    print(f"final drift        : {final_drift:.4f} m")
    print(f"drift RMSE / max   : {drift_rmse:.4f} / {drift_max:.4f} m")
    print(f"yaw RMSE           : {np.sqrt(np.mean(yaw_err ** 2)):.4f} rad")
    print(f"min crate distance : {min_clearance:.4f} m (body centre)")
    print(f"actual path length : {path_length:.4f} m")
    print(f"scans/plans        : {nav.stats.scans} / {nav.stats.plans}")
    print(f"failed plans       : {nav.stats.failed_plans}")
    print(f"mean scan/plan ms  : {1000 * nav.stats.last_scan_time:.2f} / "
          f"{1000 * nav.stats.last_plan_time:.2f}")
    print(f"wall time          : {wall_time:.2f} s")
    print("--------------------------------------------------------")

    accepted = (collision == [] and not fell and arrival_truth_time is not None
                and final_error < ARRIVAL_TRUTH)
    print("ACCEPTED" if accepted else "FAILED")

    # ------------------------------------------------------------------
    # Figures (after the control loop; no I/O in the 2 ms path).
    # ------------------------------------------------------------------
    prob = grid.probability
    extent = (grid.x_min, grid.x_max, grid.y_min, grid.y_max)
    fig, ax = plt.subplots(figsize=(8.2, 5.4))
    ax.imshow(prob, origin="lower", extent=extent, cmap="Greys", vmin=0.0,
              vmax=1.0, alpha=0.9)
    inflated = grid.inflated_obstacles()
    overlay = np.zeros((*inflated.shape, 4))
    overlay[inflated] = [0.85, 0.2, 0.15, 0.25]
    ax.imshow(overlay, origin="lower", extent=extent)
    if nav.path_history:
        shown = max(nav.path_history, key=lambda pth: float(
            np.sum(np.linalg.norm(np.diff(pth, axis=0), axis=1))))
        ax.plot(shown[:, 0], shown[:, 1], "-", color="tab:blue", lw=2.2,
                label=label("规划路径（最长的一次）", "planned path (longest)"))
    ax.plot(truth[:, 1], truth[:, 2], "-", color="tab:green", lw=1.4,
            label=label("实际轨迹", "ground-truth trajectory"))
    ax.plot(START[0], START[1], "o", color="k", ms=6)
    ax.plot(GOAL[0], GOAL[1], "*", color="tab:orange", ms=14,
            label=label("目标", "goal"))
    for name, centre, half in BOXES:
        ax.add_patch(Rectangle((centre[0] - half[0], centre[1] - half[1]),
                               2 * half[0], 2 * half[1], fill=False,
                               edgecolor="tab:brown", lw=1.2))
        ax.text(centre[0], centre[1] + half[1] + 0.08, name, ha="center",
                fontsize=7, color="tab:brown")
    ax.set_aspect("equal")
    ax.set_xlabel("x (m)")
    ax.set_ylabel("y (m)")
    ax.set_title(label("第 11 章导航：占据栅格、膨胀层、规划与实际轨迹",
                       "Chapter 11 navigation: grid, inflation, plan and trajectory"))
    ax.legend(loc="upper left", fontsize=8)
    savefig(fig, "nav_map")
    plt.close(fig)

    # One lidar scan illustration, chosen when a crate is close enough to
    # produce a clear fan of rays.
    scans = nav.scan_history
    # The first scan is taken at the start pose; it sees the middle crate
    # head-on and is the clearest illustration of the 360-beam fan.
    chosen = scans[0] if scans else None
    fig, ax = plt.subplots(figsize=(6.4, 5.4))
    ax.imshow(prob, origin="lower", extent=extent, cmap="Greys", vmin=0.0,
              vmax=1.0, alpha=0.55)
    if chosen is not None:
        origin, directions, ranges = chosen
        finite = np.isfinite(ranges)
        for d, r in zip(directions[finite], ranges[finite]):
            end = origin + float(r) * d
            ax.plot([origin[0], end[0]], [origin[1], end[1]], "-",
                    color="tab:red", lw=0.5, alpha=0.8)
        ends = origin[None, :] + np.where(finite, ranges, 0.0)[:, None] * directions
        ax.plot(ends[finite, 0], ends[finite, 1], ".", color="tab:red", ms=2.2)
        ax.plot(origin[0], origin[1], "o", color="tab:blue", ms=5,
                label=label("雷达位置", "lidar origin"))
    for _, centre, half in BOXES:
        ax.add_patch(Rectangle((centre[0] - half[0], centre[1] - half[1]),
                               2 * half[0], 2 * half[1], fill=False,
                               edgecolor="tab:brown", lw=1.2))
    ax.set_aspect("equal")
    ax.set_xlim(grid.x_min, grid.x_max)
    ax.set_ylim(grid.y_min, grid.y_max)
    ax.set_xlabel("x (m)")
    ax.set_ylabel("y (m)")
    ax.set_title(label("仿真激光雷达：360 束、1°、量程 6 m",
                       "simulated lidar: 360 beams, 1 deg, 6 m"))
    ax.legend(loc="upper left", fontsize=8)
    savefig(fig, "nav_lidar")
    plt.close(fig)

    fig, axes = plt.subplots(2, 2, figsize=(9.0, 6.4), sharex=True)
    axes[0, 0].plot(truth[:, 0], truth[:, 1], color="tab:green", lw=1.4,
                    label="truth")
    axes[0, 0].plot(estimate[:, 0], estimate[:, 1], "--", color="tab:blue",
                    lw=1.2, label="estimate")
    axes[0, 0].set_ylabel("x (m)")
    axes[0, 0].legend(fontsize=8)
    axes[0, 1].plot(truth[:, 0], truth[:, 2], color="tab:green", lw=1.4,
                    label="truth")
    axes[0, 1].plot(estimate[:, 0], estimate[:, 2], "--", color="tab:blue",
                    lw=1.2, label="estimate")
    axes[0, 1].set_ylabel("y (m)")
    axes[0, 1].legend(fontsize=8)
    axes[1, 0].plot(errors[:, 0], errors[:, 1] * 1000, color="tab:red", lw=1.3)
    axes[1, 0].set_ylabel(label("定位误差 (mm)", "position error (mm)"))
    axes[1, 0].set_xlabel(label("时间 (s)", "time (s)"))
    axes[1, 1].plot(commands[:, 0], commands[:, 1], color="tab:blue", lw=1.2,
                    label=label("vx (m/s)", "vx (m/s)"))
    axes[1, 1].plot(commands[:, 0], commands[:, 2], color="tab:orange", lw=1.2,
                    label=label("wz (rad/s)", "wz (rad/s)"))
    axes[1, 1].set_ylabel("command")
    axes[1, 1].set_xlabel(label("时间 (s)", "time (s)"))
    axes[1, 1].legend(fontsize=8)
    for ax in axes.ravel():
        ax.grid(True, alpha=0.3)
    axes[0, 0].set_title(label("估计与真值", "estimate vs truth"))
    axes[1, 0].set_title(label("定位漂移", "localization drift"))
    axes[1, 1].set_title(label("导航速度指令", "navigation commands"))
    fig.tight_layout()
    savefig(fig, "nav_tracking")
    plt.close(fig)

    try:
        image = sim.render(camera="nav_view", width=640, height=480)
        fig, ax = plt.subplots(figsize=(7.0, 5.2))
        ax.imshow(image)
        ax.axis("off")
        ax.set_title(label("导航终点（MuJoCo 离屏渲染）", "navigation endpoint"))
        savefig(fig, "nav_scene")
        plt.close(fig)
    except Exception as exc:  # rendering is optional on headless machines
        print(f"render skipped: {exc}")

    summary = OUT / "summary.txt"
    summary.write_text(
        f"accepted={accepted}\n"
        f"arrival_nav_s={arrival_nav_time}\n"
        f"arrival_truth_s={arrival_truth_time}\n"
        f"final_goal_error_m={final_error:.6f}\n"
        f"final_drift_m={final_drift:.6f}\n"
        f"drift_rmse_m={drift_rmse:.6f}\n"
        f"drift_max_m={drift_max:.6f}\n"
        f"yaw_rmse_rad={float(np.sqrt(np.mean(yaw_err ** 2))):.6f}\n"
        f"min_crate_distance_m={min_clearance:.6f}\n"
        f"path_length_m={path_length:.6f}\n"
        f"scans={nav.stats.scans}\nplans={nav.stats.plans}\n"
        f"collisions={len(collision)}\nfell={fell}\n",
        encoding="utf-8")
    print(f"figures -> {OUT}")
    sim.close()
    return 0 if accepted else 1


if __name__ == "__main__":
    raise SystemExit(run())
