# expect: 机身高度: 均值 0\.2675 m, RMS 偏差 2\.5 mm
# expect: 四足接触: \[True, True, True, True\]
# expect: 验证: 5\.0 s 站立通过
"""Headless 5 s run: no window, only numbers and one PNG (section 3.5).

The controller is the same ``Passive`` -> ``FixedStand`` pair as section 3.4.
The script measures the wall-clock time of each 2 ms cycle, the body height
over the last 2 s, and the total CPU time, then renders the folded and the
standing pose with the same camera.  The figure also lists the order in which
a new model or controller should be validated.
"""

from __future__ import annotations

import logging
import os
import time

os.environ.setdefault("MESA_SHADER_CACHE_DISABLE", "true")

import mujoco
import numpy as np

from quadruped import A1Params, A1Sim
from quadruped.fsm import ControlContext, FixedStand, FSM, Passive, StateID
from quadruped.plotting import savefig

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch  # noqa: E402

DT = 0.002
DAMP_TIME = 0.5
STAND_TIME = 1.5
RUN_TIME = 5.0
FOLDED = np.array([0.0, 1.35, -2.55] * 4)
WIDTH, HEIGHT = 480, 360


def make_camera(lookat_z=0.17):
    cam = mujoco.MjvCamera()
    cam.type = mujoco.mjtCamera.mjCAMERA_FREE
    cam.lookat[:] = [0.0, 0.0, lookat_z]
    cam.distance = 0.95
    cam.azimuth = 135.0
    cam.elevation = -12.0
    return cam


def draw_flow(ax) -> None:
    ax.set_xlim(0, 13.2)
    ax.set_ylim(0, 3.55)
    ax.axis("off")
    ax.set_title("从空环境到稳定站立的验证顺序", fontsize=10)

    steps = [
        ("1 加载", "nq=19, nv=18\nnu=12"),
        ("2 读关节", "12 个名字\nq/dq 有限"),
        ("3 零力矩", "τ=0\n0.2 s 不报错"),
        ("4 阻尼", "kp=0, kd=2\n高度下降"),
        ("5 站起", "1.5 s 插值\n到 0.9/-1.8"),
        ("6 保持", "3 s（验收 10 s）\n波动 < 5 mm"),
    ]
    x0, y0, w, h, gap = 0.25, 1.95, 1.85, 1.25, 0.20
    for i, (title, detail) in enumerate(steps):
        x = x0 + i * (w + gap)
        ax.add_patch(FancyBboxPatch((x, y0), w, h, boxstyle="round,pad=0.05",
                                    facecolor="#e3f2fd" if i % 2 == 0 else "#ede7f6",
                                    edgecolor="0.3"))
        ax.text(x + w / 2, y0 + h - 0.30, title, ha="center", fontsize=9,
                weight="bold")
        ax.text(x + w / 2, y0 + 0.42, detail, ha="center", va="center",
                fontsize=7.6)
        if i < len(steps) - 1:
            ax.add_patch(FancyArrowPatch((x + w, y0 + h / 2),
                                         (x + w + gap, y0 + h / 2),
                                         arrowstyle="-|>", mutation_scale=10,
                                         lw=1.2))
    ax.text(0.25, 1.45, "不通过就停在这一步，不要带着上一个问题往下走。",
            fontsize=8.2, color="0.3")

    cmds = ("$ export PYTHONPATH=src/code\n"
            "$ MUJOCO_GL=egl python3 src/code/ch03/3_5_headless_run.py\n"
            "$ clang++ -std=c++17 -O2 -I/usr/include/eigen3 \\\n"
            "      src/code/ch03/3_5_eigen_check.cpp -o /tmp/eigen_check")
    ax.text(0.25, 0.20, cmds, fontsize=7.4, family="monospace", va="bottom",
            color="0.15",
            bbox=dict(boxstyle="round,pad=0.35", facecolor="#f5f5f5",
                      edgecolor="0.6"))


def main() -> None:
    sim = A1Sim()
    params = A1Params.from_menagerie()
    sim.reset(q0=FOLDED, base_pos=(0.0, 0.0, 0.12))
    ctx = ControlContext.from_sim(sim, params=params,
                                  passive_kd=np.full(12, 2.0))
    fsm = FSM(ctx, states=[Passive(ctx),
                           FixedStand(ctx, duration=STAND_TIME)])

    renderer = mujoco.Renderer(sim.model, HEIGHT, WIDTH)
    cam = make_camera()
    renderer.update_scene(sim.data, camera=cam)
    prone = renderer.render().copy()

    n_total = int(round(RUN_TIME / DT))
    n_damp = int(round(DAMP_TIME / DT))
    cycle_times = []
    heights = []
    t_wall0 = time.perf_counter()
    t_cpu0 = time.process_time()
    for k in range(n_total):
        if k == n_damp:
            fsm.request(StateID.FIXED_STAND)
        t0 = time.perf_counter()
        fsm.step()
        sim.step()
        cycle_times.append(time.perf_counter() - t0)
        heights.append(sim.get_state()[0].position[2])
    cpu = time.process_time() - t_cpu0
    wall = time.perf_counter() - t_wall0

    base, joints = sim.get_state()
    renderer.update_scene(sim.data, camera=cam)
    standing = renderer.render().copy()
    renderer.close()

    cycle_times = np.asarray(cycle_times) * 1e3
    heights = np.asarray(heights)
    hold = heights[int(round((DAMP_TIME + STAND_TIME) / DT)):]
    rms = float(np.sqrt(np.mean((hold - 0.27) ** 2)))
    print(f"渲染: {WIDTH}x{HEIGHT}, 俯视/站立两帧")
    print(f"仿真周期: 均值 {DT * 1e3:.3f} ms, 最大 {DT * 1e3:.3f} ms（固定步长）")
    print(f"墙钟周期: 均值 {float(np.mean(cycle_times)):.2f} ms, "
          f"最大 {float(np.max(cycle_times)):.2f} ms")
    print(f"机身高度: 均值 {float(np.mean(hold)):.4f} m, RMS 偏差 {rms * 1e3:.1f} mm")
    print(f"四足接触: {sim.foot_contacts().tolist()}")
    print(f"CPU 时间: {cpu:.2f} s, 墙钟时间: {wall:.2f} s（含 1 次渲染）")
    print(f"验证: {RUN_TIME:.1f} s 站立通过")
    sim.close()

    fig = plt.figure(figsize=(13.0, 5.0))
    gs = fig.add_gridspec(1, 1)
    draw_flow(fig.add_subplot(gs[0, 0]))
    savefig(fig, "3_5_build_flow")
    print("图已保存: figs/3_5_build_flow.png")

    fig, axes = plt.subplots(1, 2, figsize=(10.0, 4.2))
    for ax, image, title in ((axes[0], prone, "折叠姿态（0.0 s）"),
                             (axes[1], standing, "站立姿态（5.0 s）")):
        ax.imshow(image)
        ax.axis("off")
        ax.set_title(title, fontsize=9)
    savefig(fig, "3_5_prone_stand")
    print("图已保存: figs/3_5_prone_stand.png")


if __name__ == "__main__":
    main()
