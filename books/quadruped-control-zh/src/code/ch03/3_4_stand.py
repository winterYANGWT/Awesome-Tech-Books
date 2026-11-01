# expect: 阻尼结束（0\.5 s）机身高度: 0\.1056 m
# expect: 站立结束（2\.5 s）机身高度: 0\.2675 m
# expect: 四足接触: \[True, True, True, True\]
# expect: 四足法向力之和: 122\.2 N
# expect: 12 关节最大跟踪误差: 0\.0165 rad
"""Damping then a linear stand-up, with the joint numbering figures.

The run starts from a folded pose at 0.12 m.  ``Passive`` applies kp=0,
kd=2.0 for 0.5 s; ``FixedStand`` then interpolates every joint from the angle
it had at entry to the A1 home pose over 1.5 s and holds it for another
0.5 s.  The example prints the start/target/end angles, the body height and
the four contact forces, renders six frames during the motion, and plots the
joint and height curves.

It also draws the three static figures of section 3.4: the joint numbers on
one leg, the four leg numbers and body frame, and the joint frames at the
zero pose.
"""

from __future__ import annotations

import logging
import os

os.environ.setdefault("MESA_SHADER_CACHE_DISABLE", "true")

import mujoco
import numpy as np

from quadruped import A1Params, A1Sim
from quadruped.fsm import ControlContext, FixedStand, FSM, Passive, StateID
from quadruped.plotting import savefig

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.patches import (Arc, Circle, FancyArrowPatch,  # noqa: E402
                                FancyBboxPatch, Polygon)

DT = 0.002
DAMP_TIME = 0.5
STAND_TIME = 1.5
HOLD_TIME = 0.5
FOLDED = np.array([0.0, 1.35, -2.55] * 4)
FRAME_TIMES = (0.50, 0.80, 1.10, 1.40, 1.80, 2.50)
WIDTH, HEIGHT = 480, 360


# ---------------------------------------------------------------------------
# Static joint-numbering figures
# ---------------------------------------------------------------------------
SCALE = 10.0


def proj(x, y, z):
    """Oblique projection of a point in metres; x forward, y left, z up."""
    return SCALE * (x - 0.45 * y), SCALE * (z + 0.25 * y)


def draw_single_leg(ax) -> None:
    ax.set_xlim(-3.4, 3.0)
    ax.set_ylim(-4.3, 1.8)
    ax.set_aspect("equal")
    ax.axis("off")
    ax.set_title("单腿三个关节：0 外摆、1 大腿、2 小腿", fontsize=10)

    hip = proj(0.0, 0.0, 0.0)
    knee = proj(0.157, 0.0, -0.124)
    foot = proj(0.0, 0.0, -0.249)

    body = [proj(-0.13, -0.08, 0.10), proj(0.13, -0.08, 0.10),
            proj(0.13, 0.08, 0.10), proj(-0.13, 0.08, 0.10)]
    ax.add_patch(Polygon(body, closed=True, facecolor="#cfd8dc",
                         edgecolor="0.25", lw=1.0))
    ax.plot([hip[0], hip[0]], [proj(0, 0, 0.10)[1], hip[1]],
            color="0.25", lw=3.0)

    ax.plot([hip[0], knee[0]], [hip[1], knee[1]], color="#37474f", lw=11,
            solid_capstyle="round")
    ax.plot([knee[0], foot[0]], [knee[1], foot[1]], color="#546e7a", lw=8,
            solid_capstyle="round")
    ax.add_patch(Circle(foot, 0.28, facecolor="#37474f", edgecolor="0.2"))

    # Joint 0: axis along x (forward).
    j0a, j0b = proj(-0.10, 0.0, 0.0), proj(0.10, 0.0, 0.0)
    ax.plot([j0a[0], j0b[0]], [j0a[1], j0b[1]], color="#1565c0", lw=4.0)
    ax.add_patch(Circle(hip, 0.22, facecolor="#1565c0", edgecolor="0.2",
                        zorder=5))
    ax.text(hip[0] - 0.62, hip[1] + 0.42, "0", fontsize=13, color="#1565c0",
            weight="bold")

    # Joint 1: axis along y (left).
    j1 = proj(0.02, 0.0, -0.015)
    j1a, j1b = proj(0.02, -0.11, -0.015), proj(0.02, 0.11, -0.015)
    ax.plot([j1a[0], j1b[0]], [j1a[1], j1b[1]], color="#2e7d32", lw=4.0)
    ax.add_patch(Circle(j1, 0.20, facecolor="#2e7d32", edgecolor="0.2", zorder=5))
    ax.text(j1[0] - 0.15, j1[1] - 0.65, "1", fontsize=13, color="#2e7d32",
            weight="bold")

    # Joint 2: axis along y at the knee.
    k0, k1 = proj(0.157, -0.10, -0.124), proj(0.157, 0.10, -0.124)
    ax.plot([k0[0], k1[0]], [k0[1], k1[1]], color="#ef6c00", lw=4.0)
    ax.text(knee[0] + 0.30, knee[1] + 0.08, "2", fontsize=13, color="#ef6c00",
            weight="bold")

    # Positive rotation q1 about the y axis at the hip.
    ax.add_patch(Arc(hip, 2.2, 2.2, theta1=270, theta2=322, color="#2e7d32",
                     lw=1.4))
    ax.add_patch(FancyArrowPatch((hip[0] + 0.72, hip[1] - 0.88),
                                 (hip[0] + 0.92, hip[1] - 0.72),
                                 arrowstyle="-|>", mutation_scale=10,
                                 color="#2e7d32", lw=1.4))
    ax.text(hip[0] + 0.75, hip[1] - 0.45, "+q1", fontsize=9, color="#2e7d32")

    # q2 is negative: the calf folds backwards.
    ax.add_patch(Arc(knee, 1.9, 1.9, theta1=219, theta2=322, color="#ef6c00",
                     lw=1.3))
    ax.add_patch(FancyArrowPatch((knee[0] - 0.72, knee[1] - 0.55),
                                 (knee[0] - 0.88, knee[1] - 0.36),
                                 arrowstyle="-|>", mutation_scale=10,
                                 color="#ef6c00", lw=1.3))
    ax.text(knee[0] - 1.45, knee[1] - 0.35, "q2<0", fontsize=9, color="#ef6c00")

    # Triad at the hip.
    ox, oy = -2.9, 0.9
    for vec, text, color in (((1, 0, 0), "x 前", "#c62828"),
                             ((0, 0, 1), "z 上", "#2e7d32"),
                             ((0, 1, 0), "y 左", "#1565c0")):
        dx, dy = proj(*vec)
        dx, dy = dx / SCALE, dy / SCALE
        ax.add_patch(FancyArrowPatch((ox, oy), (ox + 0.55 * dx, oy + 0.55 * dy),
                                     arrowstyle="-|>", mutation_scale=9,
                                     color=color, lw=1.3))
        ax.text(ox + 0.70 * dx, oy + 0.70 * dy, text, fontsize=8, color=color)

    legend = [("0  外摆（abduction），绕 x 轴", "#1565c0"),
              ("1  大腿（thigh），绕 y 轴", "#2e7d32"),
              ("2  小腿（calf），绕 y 轴", "#ef6c00")]
    for i, (text, color) in enumerate(legend):
        ax.text(-3.25, -2.75 - 0.48 * i, text, fontsize=9, color=color)
    ax.text(-3.25, -4.10, "在零位定义三轴：x 前、y 左、z 上；角度用弧度。",
            fontsize=8.5, color="0.25")


def draw_four_legs(ax) -> None:
    ax.set_xlim(-0.36, 0.38)
    ax.set_ylim(-0.30, 0.32)
    ax.set_aspect("equal")
    ax.axis("off")
    ax.set_title("四条腿编号与机身坐标系（俯视）", fontsize=10)

    ax.add_patch(FancyBboxPatch((-0.18, -0.07), 0.36, 0.14,
                                boxstyle="round,pad=0.01", facecolor="#cfd8dc",
                                edgecolor="0.25"))
    hips = [(0.183, -0.047), (0.183, 0.047), (-0.183, -0.047), (-0.183, 0.047)]
    feet = [(0.183, -0.155), (0.183, 0.155), (-0.183, -0.155), (-0.183, 0.155)]
    names = ["Leg0 右前", "Leg1 左前", "Leg2 右后", "Leg3 左后"]
    colors = ["#1565c0", "#2e7d32", "#ef6c00", "#6a1b9a"]
    for i, (hip, foot) in enumerate(zip(hips, feet)):
        ax.plot([hip[0], foot[0]], [hip[1], foot[1]], color=colors[i], lw=6,
                solid_capstyle="round")
        ax.add_patch(Circle(foot, 0.017, facecolor=colors[i], edgecolor="0.2"))
        dy = 0.028 if foot[1] > 0 else -0.040
        ax.text(foot[0], foot[1] + dy, names[i], fontsize=8.5, color=colors[i],
                ha="center", va="center")

    ax.annotate("", xy=(0.34, 0.0), xytext=(0.235, 0.0),
                arrowprops=dict(arrowstyle="-|>", lw=1.4, color="#c62828"))
    ax.text(0.29, 0.018, "x 前", fontsize=8.5, color="#c62828", ha="center")
    ax.annotate("", xy=(-0.25, 0.285), xytext=(-0.25, 0.20),
                arrowprops=dict(arrowstyle="-|>", lw=1.4, color="#1565c0"))
    ax.text(-0.25, 0.30, "y 左", fontsize=8.5, color="#1565c0", ha="center")
    ax.text(-0.35, -0.27, "腿号 = 3×腿 + 关节；右前 0、左前 1、右后 2、左后 3。",
            fontsize=8.2, color="0.25")


def draw_frames(axes) -> None:
    ax, ax2 = axes
    ax.set_xlim(-1.9, 1.5)
    ax.set_ylim(-3.9, 1.5)
    ax.set_aspect("equal")
    ax.axis("off")
    ax.set_title("(a) 零位：三个关节坐标系的轴向互相平行", fontsize=10)

    def frame(x, y, name):
        ax.annotate("", xy=(x + 0.42, y), xytext=(x, y),
                    arrowprops=dict(arrowstyle="-|>", lw=1.1, color="#c62828"))
        ax.annotate("", xy=(x, y + 0.42), xytext=(x, y),
                    arrowprops=dict(arrowstyle="-|>", lw=1.1, color="#2e7d32"))
        ax.add_patch(Circle((x, y), 0.05, facecolor="#1565c0", edgecolor="none",
                            zorder=4))
        ax.text(x + 0.48, y - 0.05, "x", fontsize=7.5, color="#c62828")
        ax.text(x - 0.02, y + 0.46, "z", fontsize=7.5, color="#2e7d32")
        ax.text(x + 0.08, y + 0.10, "⊙ y", fontsize=6.8, color="#1565c0")
        ax.text(x - 0.12, y - 0.28, name, fontsize=8, color="0.25", ha="right")

    ax.plot([0.0, 0.0], [0.15, -2.55], color="#546e7a", lw=9,
            solid_capstyle="round")
    ax.add_patch(Circle((0.0, 0.0), 0.14, facecolor="#1565c0", edgecolor="0.2"))
    ax.add_patch(Circle((0.0, -1.25), 0.12, facecolor="#ef6c00", edgecolor="0.2"))
    ax.add_patch(Circle((0.0, -2.55), 0.14, facecolor="#37474f", edgecolor="0.2"))
    frame(0.0, 0.0, "关节 0")
    frame(0.0, -1.25, "关节 1")
    frame(0.0, -2.55, "关节 2")
    ax.annotate("q=(0, 0, 0)\n小腿被机械限位挡住，零位不可达",
                xy=(0.06, -2.10), xytext=(0.55, -3.35),
                arrowprops=dict(arrowstyle="-|>", lw=1.0, color="#c62828"),
                fontsize=8.2, color="#c62828", va="center")
    ax.text(-1.55, 1.15, "x 前、y 左、z 上；⊙ 表示 y 轴指向纸外。",
            fontsize=8, color="0.3")

    ax2.set_xlim(-1.2, 4.1)
    ax2.set_ylim(-3.5, 1.5)
    ax2.set_aspect("equal")
    ax2.axis("off")
    ax2.set_title("(b) 站立角：q1=0.90 rad、q2=-1.80 rad", fontsize=10)

    base = (0.0, 0.0)
    knee = (1.57, -1.24)
    foot = (0.0, -2.49)
    ax2.plot([base[0], knee[0]], [base[1], knee[1]], color="#37474f", lw=10,
             solid_capstyle="round")
    ax2.plot([knee[0], foot[0]], [knee[1], foot[1]], color="#546e7a", lw=8,
             solid_capstyle="round")
    ax2.add_patch(Circle(base, 0.14, facecolor="#1565c0", edgecolor="0.2"))
    ax2.add_patch(Circle(knee, 0.12, facecolor="#ef6c00", edgecolor="0.2"))
    ax2.add_patch(Circle(foot, 0.14, facecolor="#37474f", edgecolor="0.2"))
    ax2.axhline(-2.57, color="0.35", lw=1.0)
    ax2.text(2.9, -2.50, "地面", fontsize=7.5, color="0.3")
    ax2.plot([0.0, 0.0], [-2.57, 0.0], color="0.55", ls=":", lw=0.9)
    ax2.annotate("", xy=(0.0, 0.0), xytext=(0.0, -2.57),
                 arrowprops=dict(arrowstyle="<->", lw=0.9, color="0.4"))
    ax2.text(-1.10, -1.20, "0.27 m", fontsize=8, color="0.3")

    ax2.add_patch(Arc(base, 2.2, 2.2, theta1=270, theta2=322, color="#2e7d32",
                      lw=1.3))
    ax2.text(-1.10, -0.55, "q1=0.90 rad", fontsize=8, color="#2e7d32")
    ax2.add_patch(Arc(knee, 1.9, 1.9, theta1=219, theta2=322, color="#ef6c00",
                      lw=1.3))
    ax2.text(1.05, -1.95, "q2=-1.80 rad", fontsize=8, color="#ef6c00")
    ax2.text(-0.55, 1.15, "髋在机身下方 z=0；腿长 0.2+0.2 m。",
             fontsize=7.8, color="0.3")


# ---------------------------------------------------------------------------
# Run
# ---------------------------------------------------------------------------
def render_frame(renderer, sim):
    cam = mujoco.MjvCamera()
    cam.type = mujoco.mjtCamera.mjCAMERA_FREE
    cam.lookat[:] = [0.0, 0.0, 0.17]
    cam.distance = 0.95
    cam.azimuth = 135.0
    cam.elevation = -12.0
    renderer.update_scene(sim.data, camera=cam)
    return renderer.render().copy()


def main() -> None:
    sim = A1Sim()
    params = A1Params.from_menagerie()
    sim.reset(q0=FOLDED, base_pos=(0.0, 0.0, 0.12))
    ctx = ControlContext.from_sim(sim, params=params,
                                  passive_kd=np.full(12, 2.0))
    fsm = FSM(ctx, states=[Passive(ctx),
                           FixedStand(ctx, duration=STAND_TIME)])
    print(f"起始机身高度: {sim.get_state()[0].position[2]:.4f} m（折叠姿态）")

    # 0--0.5 s: damping.
    for _ in range(int(round(DAMP_TIME / DT))):
        fsm.step()
        sim.step()
    base, joints = sim.get_state()
    q_start = joints.q.copy()
    print(f"阻尼结束（{DAMP_TIME:.1f} s）机身高度: {base.position[2]:.4f} m")
    print("12 个关节起始角（阻尼结束）:")
    for i in range(12):
        print(f"  {i:2d}  leg{i // 3} joint{i % 3}  {q_start[i]: .4f} rad")

    fsm.request(StateID.FIXED_STAND)
    renderer = mujoco.Renderer(sim.model, HEIGHT, WIDTH)
    frames = []
    times, heights, q_hist = [], [], []
    n_hold = int(round((STAND_TIME + HOLD_TIME) / DT))
    for k in range(n_hold):
        fsm.step()
        sim.step()
        t = DAMP_TIME + (k + 1) * DT
        times.append(t)
        heights.append(sim.get_state()[0].position[2])
        q_hist.append(sim.joint_positions)
        if len(frames) < len(FRAME_TIMES):
            target = FRAME_TIMES[len(frames)]
            if t >= target - 1e-9:
                frames.append((t, render_frame(renderer, sim)))

    base, joints = sim.get_state()
    q_end = joints.q.copy()
    q_target = ctx.q_stand
    print("12 个关节目标角（A1 home）:")
    for i in range(12):
        print(f"  {i:2d}  leg{i // 3} joint{i % 3}  {q_target[i]: .4f} rad")
    print(f"站立结束（{DAMP_TIME + STAND_TIME + HOLD_TIME:.1f} s）"
          f"机身高度: {base.position[2]:.4f} m")
    print("12 个关节终止角（实测）:")
    for i in range(12):
        print(f"  {i:2d}  leg{i // 3} joint{i % 3}  {q_end[i]: .4f} rad")
    print(f"四足接触: {sim.foot_contacts().tolist()}")
    forces = sim.foot_forces()
    print("四足法向力: " + ", ".join(f"{f:.1f}" for f in forces[:, 2]) + " N")
    print(f"四足法向力之和: {float(np.sum(forces[:, 2])):.1f} N")
    err = np.abs(q_end - q_target)
    print(f"12 关节最大跟踪误差: {float(np.max(err)):.4f} rad（关节 {int(np.argmax(err))}）")
    for j, name in enumerate(("外摆", "大腿", "小腿")):
        print(f"{name}最大误差: {float(np.max(err[j::3])):.4f} rad")
    renderer.close()

    fig = plt.figure(figsize=(11.0, 9.2))
    gs = fig.add_gridspec(3, 3, height_ratios=(1.0, 1.0, 1.15), hspace=0.30,
                          wspace=0.05)
    for idx, (t, image) in enumerate(frames):
        ax = fig.add_subplot(gs[idx // 3, idx % 3])
        ax.imshow(image)
        ax.axis("off")
        ax.set_title(f"t = {t:.2f} s", fontsize=9)
    axq = fig.add_subplot(gs[2, 0:2])
    q_arr = np.asarray(q_hist)
    for i in range(12):
        axq.plot(times, q_arr[:, i], lw=0.9, alpha=0.75,
                 color=("#1565c0", "#2e7d32", "#ef6c00")[i % 3])
    axq.set_xlabel("时间 (s)")
    axq.set_ylabel("关节角 (rad)")
    axq.grid(True, alpha=0.3)
    axq.set_title("12 个关节角：蓝=外摆，绿=大腿，橙=小腿", fontsize=9)
    axh = fig.add_subplot(gs[2, 2])
    axh.plot(times, heights, lw=1.4, color="#6a1b9a")
    axh.axhline(0.27, color="0.4", ls="--", lw=0.9)
    axh.set_xlabel("时间 (s)")
    axh.set_ylabel("机身高度 (m)")
    axh.grid(True, alpha=0.3)
    axh.set_title("机身高度", fontsize=9)
    savefig(fig, "3_4_sequence")
    print("图已保存: figs/3_4_sequence.png")

    fig, ax = plt.subplots(figsize=(7.2, 6.4))
    draw_single_leg(ax)
    savefig(fig, "3_4_leg_joints")
    print("图已保存: figs/3_4_leg_joints.png")

    fig, ax = plt.subplots(figsize=(6.4, 5.6))
    draw_four_legs(ax)
    savefig(fig, "3_4_legs")
    print("图已保存: figs/3_4_legs.png")

    fig, axes = plt.subplots(1, 2, figsize=(11.0, 5.0))
    draw_frames(axes)
    savefig(fig, "3_4_frames")
    print("图已保存: figs/3_4_frames.png")


if __name__ == "__main__":
    main()
