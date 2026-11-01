"""Y-P-R Euler angles: forward map, inverse map and gimbal lock (4.6).

The example converts 1000 random rotation matrices to roll-pitch-yaw and back,
walks the pitch towards +90 deg to show how the inverse map loses accuracy,
and prints the yaw jump at the +-pi wrap.

expect: 往返
expect: 万向锁
expect: yaw 跳变
"""

from __future__ import annotations

import logging
import math

import numpy as np

from quadruped.plotting import savefig
from quadruped.rotation import Rx, Ry, Rz, rpy_to_R, R_to_rpy, so3_exp, wrap_angle
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter, NullFormatter

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

RNG = np.random.default_rng(6)
D = math.radians


def rot_error(A: np.ndarray, B: np.ndarray) -> float:
    """Geodesic angle between two rotation matrices, via atan2."""
    R = A.T @ B
    vee = np.array([R[2, 1] - R[1, 2], R[0, 2] - R[2, 0], R[1, 0] - R[0, 1]])
    return math.atan2(0.5 * float(np.linalg.norm(vee)),
                      0.5 * (float(np.trace(R)) - 1.0))


def draw_frame(ax, R, color, lw=1.6, text=True, scale=1.0):
    for k, name in enumerate(("x", "y", "z")):
        d = R[:, k] * scale
        ax.quiver(0, 0, 0, *d, color=color, linewidth=lw, arrow_length_ratio=0.14,
                  linestyle="-" if color != "0.55" else "--")
        if text:
            ax.text(*(1.12 * d), f"${name}$", color=color, fontsize=8.5)


def setup3d(ax, lim=1.2):
    ax.set_xlim(-lim, lim)
    ax.set_ylim(-lim, lim)
    ax.set_zlim(-lim, lim)
    ax.set_box_aspect((1, 1, 1))
    ax.view_init(elev=20, azim=-58)
    ax.grid(False)
    for pane in (ax.xaxis.pane, ax.yaxis.pane, ax.zaxis.pane):
        pane.set_alpha(0.0)
    ax.tick_params(labelsize=6)


def main() -> None:
    print("=== 4.6 欧拉角：Y-P-R 顺序与万向锁 ===")
    # ANCHOR: roundtrip
    n = 1000
    worst = 0.0
    worst_angle = 0.0
    for _ in range(n):
        rpy = RNG.uniform(-math.pi, math.pi, size=3)
        rpy[1] = RNG.uniform(-0.5 * math.pi + 1e-3, 0.5 * math.pi - 1e-3)
        R = rpy_to_R(rpy)
        back = R_to_rpy(R)
        worst = max(worst, rot_error(rpy_to_R(back), R))
        worst_angle = max(worst_angle, float(np.max(np.abs(wrap_angle(back - rpy)))))
    print(f"\n {n} 组随机姿态 R->rpy->R 往返：")
    print(f" 旋转矩阵最大误差 = {worst:.2e} rad")
    print(f" 三个角的最大绝对误差 = {worst_angle:.2e} rad")
    # ANCHOR_END: roundtrip

    # ANCHOR: gimbal
    print("\n 万向锁：roll=0.3000, yaw=0.7000，pitch 逐渐逼近 90 deg")
    print(" 姿态矩阵带 1e-6 rad 的测量噪声，20 次取最大")
    roll, yaw = 0.3, 0.7
    noise = 1e-6
    rows = []
    for pitch_deg in (60.0, 85.0, 89.0, 89.9, 89.99, 89.999, 89.9999):
        p = D(pitch_deg)
        R = rpy_to_R(np.array([roll, p, yaw]))
        err_r = err_y = 0.0
        for _ in range(20):
            axis = RNG.normal(size=3)
            axis /= np.linalg.norm(axis)
            back = R_to_rpy(so3_exp(axis * noise) @ R)
            err_r = max(err_r, abs(wrap_angle(back[0] - roll)))
            err_y = max(err_y, abs(wrap_angle(back[2] - yaw)))
        cond = 1.0 / max(math.cos(p), 1e-12)
        rows.append((pitch_deg, err_r, err_y, cond))
        print(f" pitch = {pitch_deg:8.4f} deg: roll 误差 = {err_r:.2e} rad, "
              f"yaw 误差 = {err_y:.2e} rad, 条件数 1/cos = {cond:.1f}")
    # ANCHOR_END: gimbal

    print("\n yaw 跨过 +-pi 的跳变（roll=0, pitch=0）：")
    for yaw_deg in (178.0, 179.0, 179.9, 180.0, -179.9, -179.0, -178.0):
        y = D(yaw_deg)
        R = Rz(y)
        got = math.degrees(R_to_rpy(R)[2])
        print(f" 输入 yaw = {yaw_deg:8.2f} deg -> atan2 输出 {got:8.2f} deg")

    ## Figure 4-10.
    a, b, c = D(35.0), D(25.0), D(-15.0)
    steps = (("(a) 绕当前 z 轴转 yaw", np.eye(3), Rz(a)),
             ("(b) 再绕当前 y 轴转 pitch", Rz(a), Rz(a) @ Ry(b)),
             ("(c) 最后绕当前 x 轴转 roll", Rz(a) @ Ry(b), Rz(a) @ Ry(b) @ Rx(c)))

    fig = plt.figure(figsize=(12.2, 7.6))
    for k, (title, R0, R1) in enumerate(steps):
        ax = fig.add_subplot(2, 3, k + 1, projection="3d")
        setup3d(ax)
        draw_frame(ax, R0, "0.55", lw=1.0)
        draw_frame(ax, R1, ("tab:red", "tab:green", "tab:blue")[k], lw=2.2)
        ax.text2D(0.02, 0.90, "灰：转动前   彩：转动后", transform=ax.transAxes,
                  fontsize=8, color="0.3")
        ax.set_title(title, fontsize=10)

    ax = fig.add_subplot(2, 3, 4)
    pitches = np.linspace(0.0, 89.9999, 300)
    errs = []
    for pd in pitches:
        R = rpy_to_R(np.array([roll, D(float(pd)), yaw]))
        e = 0.0
        for _ in range(6):
            axis = RNG.normal(size=3)
            axis /= np.linalg.norm(axis)
            back = R_to_rpy(so3_exp(axis * noise) @ R)
            e = max(e, abs(wrap_angle(back[2] - yaw)))
        errs.append(max(e, 1e-18))
    ax.semilogy(pitches, errs, color="tab:red", lw=1.6, label="实测 yaw 误差")
    ax.semilogy(pitches, noise / np.maximum(np.cos(np.radians(pitches)), 1e-12),
                color="0.5", ls="--", lw=1.2, label=r"$10^{-6}/\cos\beta$")
    ax.set_xlabel("pitch / deg")
    ax.set_ylabel("yaw 误差 / rad")
    ax.set_title("(d) 逼近万向锁时反解的误差", fontsize=10)
    ax.yaxis.set_major_formatter(FuncFormatter(lambda v, _: f"{v:.0e}"))
    ax.yaxis.set_minor_formatter(NullFormatter())
    ax.grid(True, which="both", alpha=0.25)
    ax.legend(fontsize=8)

    ax = fig.add_subplot(2, 3, 5)
    yaw_deg = np.linspace(170.0, 190.0, 401)
    raw = np.array([math.degrees(R_to_rpy(Rz(D(float(y))))[2]) for y in yaw_deg])
    ax.plot(yaw_deg, raw, color="tab:red", lw=1.8, label="atan2 输出")
    ax.plot(yaw_deg, yaw_deg, color="0.5", ls="--", lw=1.0, label="真实 yaw")
    ax.axhline(180.0, color="0.7", lw=0.8)
    ax.axhline(-180.0, color="0.7", lw=0.8)
    ax.set_xlabel("真实 yaw / deg")
    ax.set_ylabel("反解 yaw / deg")
    ax.set_title("(e) 跨 +-180° 的跳变", fontsize=10)
    ax.grid(True, alpha=0.25)
    ax.legend(fontsize=8, loc="lower left")

    ax = fig.add_subplot(2, 3, 6)
    ax.axis("off")
    ax.text(0.0, 0.95, r"$R=R_z(\mathrm{yaw})\,R_y(\mathrm{pitch})\,R_x(\mathrm{roll})$",
            fontsize=12)
    ax.text(0.0, 0.72, "roll  = atan2(r32, r33)\n"
                       "pitch = asin(-r31)\n"
                       "yaw   = atan2(r21, r11)",
            fontsize=11, va="top")
    ax.text(0.0, 0.34, "pitch = ±90° 时 cos(pitch)=0，\n"
                       "r32,r33,r21,r11 同时趋于 0，\n"
                       "roll 与 yaw 只剩一个组合量可辨。",
            fontsize=10, va="top")
    fig.suptitle("Y-P-R 三步旋转、万向锁附近的误差与角度跳变", fontsize=11)
    fig.tight_layout()
    savefig(fig, "4_6_euler")


if __name__ == "__main__":
    main()
