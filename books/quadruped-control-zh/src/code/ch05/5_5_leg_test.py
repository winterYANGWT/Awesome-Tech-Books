"""Single-leg bench test with three legs fixed (section 5.5).

The A1 base is pinned at 0.62 m, so all four feet are clear of the ground.
Three legs hold the home pose; the right-front foot follows a rounded square
in the sagittal plane (forward, up, backward, down) for 3 s.  The script
prints the joint-command errors, the foot tracking error, six rendered frames
and the foot/joint plots.  It also demonstrates the joystick rescaling of
section 5.5.2.

expect: 命令逆归一化
expect: 12 个关节
expect: 足端跟踪
expect: 6 帧
"""

from __future__ import annotations

import logging
import math
import os

# Keep Mesa's shader cache out of the read-only home directory.
os.environ.setdefault("MESA_SHADER_CACHE_DIR", "/tmp/mesa-shader-cache-ch05")

import numpy as np

from quadruped import A1Params, A1Sim, JointCmd, Quadruped
from quadruped.plotting import label, savefig
import matplotlib.pyplot as plt

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

BASE_Z = 0.62
DT = 0.002
T_TOTAL = 3.0


# ANCHOR: rescale
def rescale(x: float, x_min: float, x_max: float,
            y_min: float, y_max: float) -> float:
    """Linear inverse normalisation: [x_min,x_max] -> [y_min,y_max]."""
    if x_max == x_min:
        raise ValueError("输入区间不能为零")
    return y_min + (x - x_min) * (y_max - y_min) / (x_max - x_min)
# ANCHOR_END: rescale


def smoothstep(u: float) -> float:
    return u * u * (3.0 - 2.0 * u)


def dsmoothstep(u: float) -> float:
    return 6.0 * u * (1.0 - u)


def make_square(p0: np.ndarray):
    """Four smooth segments: forward, up, backward, down."""
    start = p0.copy()
    p1 = p0 + np.array([0.08, 0.0, 0.0])
    p2 = p0 + np.array([0.08, 0.0, 0.06])
    p3 = p0 + np.array([0.00, 0.0, 0.06])
    segs = [(0.00, 0.75, start, p1), (0.75, 1.50, p1, p2),
            (1.50, 2.25, p2, p3), (2.25, 3.00, p3, start)]

    def desired(t: float):
        if t <= 0.0 or t >= T_TOTAL:
            return start.copy(), np.zeros(3)
        for t0, t1, a, b in segs:
            if t0 <= t < t1:
                u = (t - t0) / (t1 - t0)
                p = a + (b - a) * smoothstep(u)
                v = (b - a) * dsmoothstep(u) / (t1 - t0)
                # Small lateral bump; zero at t=0 and t=3.
                p[1] += 0.006 * (1.0 - math.cos(2.0 * math.pi * t / T_TOTAL))
                v[1] += 0.006 * (2.0 * math.pi / T_TOTAL) * math.sin(
                    2.0 * math.pi * t / T_TOTAL)
                return p, v
        return start.copy(), np.zeros(3)

    return desired, start, p1, p2, p3


def pin_base(sim: A1Sim, base: np.ndarray) -> None:
    sim.data.qpos[0:3] = base
    sim.data.qpos[3:7] = [1.0, 0.0, 0.0, 0.0]
    sim.data.qvel[0:6] = 0.0


def main() -> None:
    params = A1Params.from_menagerie()
    robot = Quadruped(params)
    leg = robot.legs[0]
    q_home = params.home_q.copy()
    p0 = leg.fk(q_home[:3])
    desired, start, p1, p2, p3 = make_square(p0)
    base = np.array([0.0, 0.0, BASE_Z])

    print("=== 5.5 右前腿单腿测试（固定机身） ===")
    def shown(p):
        return np.where(np.abs(p) < 5e-5, 0.0, p)

    print(f"home 足端 P0 = {shown(p0).round(4)} m；"
          f"方波起点 {shown(start).round(4)} m")
    print(f"方波顶点: {shown(p1).round(4)}, {shown(p2).round(4)}, "
          f"{shown(p3).round(4)} m")
    print("\n命令逆归一化（摇杆 [-1,1] -> 足端位移）:")
    for axis, (lo, hi) in enumerate(((-0.08, 0.08), (-0.05, 0.05),
                                     (-0.06, 0.00))):
        vals = [rescale(v, -1.0, 1.0, lo, hi) for v in (-1.0, 0.0, 1.0)]
        print(f" 轴 {axis}: [-1,0,1] -> "
              f"[{vals[0]:.3f}, {vals[1]:.3f}, {vals[2]:.3f}] m")

    sim = A1Sim()
    sim.reset(q0=q_home, base_pos=base)
    # Side view: the swing happens in the x-z plane.
    cam = sim.model.camera("track").id
    sim.model.cam_pos[cam] = [0.0, -0.75, 0.05]

    kp = np.tile([60.0, 150.0, 200.0], 4)
    kd = np.tile([2.0, 3.0, 3.0], 4)
    Kp = np.diag([500.0, 500.0, 500.0])
    Kd = np.diag([15.0, 15.0, 15.0])

    # Settle on the stand before the three fixed legs hold.
    for _ in range(150):
        pin_base(sim, base)
        sim.step(JointCmd(q_home, np.zeros(12), kp, kd, np.zeros(12)))

    n = int(round(T_TOTAL / DT))
    t_hist = np.arange(n) * DT
    q_cmd_hist = np.zeros((n, 3))
    q_act_hist = np.zeros((n, 3))
    p_des_hist = np.zeros((n, 3))
    p_act_hist = np.zeros((n, 3))
    q_cmd_all = np.zeros((n, 12))
    q_act_all = np.zeros((n, 12))
    frames, frame_t = [], []
    render_times = [0.15, 0.65, 1.15, 1.65, 2.15, 2.65]

    for i in range(n):
        t = t_hist[i]
        pd, vd = desired(t)
        qd = leg.ik(pd)
        Jd = leg.jacobian(qd)
        dqd = np.linalg.solve(Jd, vd)
        q = sim.joint_positions
        dq = sim.joint_velocities
        p = leg.fk(q[:3])
        v = leg.jacobian(q[:3]) @ dq[:3]
        # ANCHOR: bench_loop
        f = Kp @ (pd - p) + Kd @ (vd - v)
        tau_ff = leg.tau_from_force(q[:3], f)

        q_cmd = q_home.copy()
        q_cmd[:3] = qd
        dq_cmd = np.zeros(12)
        dq_cmd[:3] = dqd
        tau_cmd = np.zeros(12)
        tau_cmd[:3] = tau_ff
        cmd = JointCmd(q_cmd, dq_cmd, kp, kd, tau_cmd)

        if len(frames) < len(render_times) and t >= render_times[len(frames)]:
            frames.append(sim.render("track", 480, 360))
            frame_t.append(t)
        pin_base(sim, base)
        sim.step(cmd)

        q_cmd_hist[i] = qd
        q_act_hist[i] = q[:3]
        p_des_hist[i] = pd
        p_act_hist[i] = p
        q_cmd_all[i] = q_cmd
        q_act_all[i] = q
        # ANCHOR_END: bench_loop

    sim.close()

    qerr = q_cmd_all - q_act_all
    maxq = np.max(np.abs(qerr), axis=0).reshape(4, 3) * 1000
    rmsq = np.sqrt(np.mean(qerr ** 2, axis=0)).reshape(4, 3) * 1000
    print("\n12 个关节的指令跟踪误差（最大 / RMS，mrad）:")
    print("        髋 (hip)      大腿 (thigh)    小腿 (calf)")
    for i, name in enumerate(("FR", "FL", "RR", "RL")):
        print(f" {name}  " + "   ".join(
            f"{maxq[i, j]:7.2f} / {rmsq[i, j]:5.2f}" for j in range(3)))

    perr = np.linalg.norm(p_des_hist - p_act_hist, axis=1)
    print(f"\n足端跟踪 RMS = {1000 * np.sqrt(np.mean(perr ** 2)):.2f} mm, "
          f"最大 = {1000 * np.max(perr):.2f} mm")
    fixed_drift = np.linalg.norm(
        robot.fk_all(q_act_all[-1])[:, :] - robot.fk_all(q_home)[:, :], axis=1) * 1000
    print(f"另外三条腿的足端漂移（末时刻）: "
          f"FL {fixed_drift[1]:.2f} mm, RR {fixed_drift[2]:.2f} mm, "
          f"RL {fixed_drift[3]:.2f} mm")
    print(f"已保存 {len(frames)} 帧渲染图（{', '.join(f'{t:.2f}' for t in frame_t)} s）")

    # Fig. 5-8: six rendered frames.
    fig, axs = plt.subplots(2, 3, figsize=(10.8, 5.6))
    for ax, img, t in zip(axs.ravel(), frames, frame_t):
        ax.imshow(img)
        ax.set_title(f"t = {t:.2f} s", fontsize=9)
        ax.axis("off")
    fig.suptitle(label("固定机身、三条腿保持 home，右前腿按方波摆动",
                       "single-leg bench test"), fontsize=11)
    fig.tight_layout()
    savefig(fig, "5_5_frames")
    plt.close(fig)

    # Fig. 5-9: foot path and joint angles.
    fig = plt.figure(figsize=(10.8, 4.8))
    ax = fig.add_subplot(1, 2, 1, projection="3d")
    ax.plot(*p_des_hist.T, color="0.25", lw=2.0, ls="--", label="目标 $p_d$")
    ax.plot(*p_act_hist.T, color="tab:orange", lw=1.2, label="实际 $p$")
    ax.set_xlabel(r"$x_0$ / m", fontsize=8)
    ax.set_ylabel(r"$y_0$ / m", fontsize=8)
    ax.set_zlabel(r"$z_0$ / m", fontsize=8)
    ax.tick_params(labelsize=7)
    ax.view_init(elev=22, azim=-60)
    ax.legend(fontsize=8)
    ax.set_title(label("右前腿足端轨迹", "FR foot path"), fontsize=10)

    ax2 = fig.add_subplot(1, 2, 2)
    for j, name in enumerate(("$q_1$", "$q_2$", "$q_3$")):
        ax2.plot(t_hist, q_cmd_hist[:, j], color=f"C{j}", lw=1.4, ls="--")
        ax2.plot(t_hist, q_act_hist[:, j], color=f"C{j}", lw=1.0)
    ax2.set_xlabel(label("时间 (s)", "time (s)"))
    ax2.set_ylabel(label("关节角 (rad)", "joint angle (rad)"))
    ax2.grid(True, alpha=0.3)
    ax2.set_title(label("右前腿关节角：虚线指令、实线实际",
                        "FR joints: dashed command, solid actual"), fontsize=10)
    fig.tight_layout()
    savefig(fig, "5_5_joints")
    plt.close(fig)


if __name__ == "__main__":
    main()
