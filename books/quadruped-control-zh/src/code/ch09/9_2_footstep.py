# expect: 平移
# expect: 相对中性点前移
# expect: 速度跟踪误差
# expect: 转动
# expect: 切向超前
# expect: 角速度跟踪误差
"""Foothold planning: neutral point plus the Raibert correction (9.2).

The base motion is prescribed (this chapter only plans footholds; closing the
loop is Chapter 10).  A 5 % sinusoidal error is added to the commanded speed
so that the Raibert term is not identically zero.  Ten periods are run for a
straight walk at 0.4 m/s and for an in-place turn at 0.5 rad/s; every
touch-down is recorded and compared with the neutral point under the hip.

"""

from __future__ import annotations

import logging

import numpy as np

from quadruped import A1Params, Quadruped, GaitParams, GaitGenerator, WaveStatus
from quadruped.plotting import label, savefig
import matplotlib.pyplot as plt
from matplotlib.patches import Arc, Circle, Polygon

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

DT = 0.002
PERIOD = 0.36
DUTY = 0.60
CYCLES = 10
V_CMD = 0.4
W_CMD = 0.5
KX = KY = 0.06
KYAW = 0.08
RIPPLE_T = 0.55

LEGS = ["FR", "FL", "RR", "RL"]
LEG_ZH = ["右前", "左前", "右后", "左后"]
COLORS = ["C0", "C1", "C2", "C3"]


def make_robot():
    params = A1Params.from_menagerie()
    robot = Quadruped(params)
    gait = GaitParams.trot(period=PERIOD, duty=DUTY, step_height=0.06)
    gait.kx, gait.ky, gait.kyaw = KX, KY, KYAW
    gait.ground_z = 0.02
    return params, robot, gait


def neutral_and_hip(calc, base: np.ndarray, yaw: float, leg: int) -> tuple[np.ndarray, np.ndarray]:
    """Neutral foot point and the projection of the hip joint, both world frame."""
    c, s = np.cos(yaw), np.sin(yaw)
    px, py = calc.neutral_body[leg, 0], calc.neutral_body[leg, 1]
    neutral = np.array([base[0] + c * px - s * py,
                        base[1] + s * px + c * py,
                        calc.ground_z])
    hx, hy = calc.robot.params.hip_offset[leg, 0], calc.robot.params.hip_offset[leg, 1]
    hip = np.array([base[0] + c * hx - s * hy,
                    base[1] + s * hx + c * hy,
                    calc.ground_z])
    return neutral, hip


# ANCHOR: run
def run_case(robot, params, gait, turn: bool) -> list[dict]:
    gen = GaitGenerator(robot, params, gait)
    gen.set_status(WaveStatus.WAVE_ALL, 0.0)
    calc = gen.calc
    base = np.array([0.0, 0.0, 0.27])
    yaw = 0.0
    records: list[dict] = []
    prev = np.ones(4, dtype=bool)
    n = int(round(CYCLES * gait.period / DT))
    for k in range(n + 1):
        t = k * DT
        if turn:
            v_cur = np.zeros(3)
            w_cur = W_CMD + 0.02 * np.sin(2.0 * np.pi * t / RIPPLE_T)
        else:
            vx = V_CMD + 0.02 * np.sin(2.0 * np.pi * t / RIPPLE_T)
            v_cur = np.array([vx, 0.0, 0.0])
            w_cur = 0.0
        gen.set_gait(v_cur[:2], w_cur if turn else 0.0)
        gen.update(t, base, yaw, v_cur, w_cur)
        contact = gen.contact.copy()
        for i in range(4):
            if contact[i] and not prev[i]:
                neutral, hip = neutral_and_hip(calc, base, yaw, i)
                records.append({"leg": i, "t": t, "foot": gen.foot_target(i).copy(),
                                "neutral": neutral.copy(), "hip": hip.copy(),
                                "yaw": yaw, "v": v_cur.copy(), "w": w_cur})
        prev = contact
        if turn:
            yaw += w_cur * DT
        else:
            base += v_cur * DT
    return records


# ANCHOR_END: run


def print_translation(records: list[dict], gen: GaitGenerator) -> None:
    d = np.array([r["foot"][:2] - r["neutral"][:2] for r in records])
    along = d[:, 0]
    v_err = np.array([r["v"][0] - V_CMD for r in records])
    print("=== 平移：vx = 0.400 m/s，10 个周期 ===")
    print(f"落脚点样本 {len(records)} 个（每腿 {len(records) // 4} 个）")
    print(f"相对中性点前移：均值 {along.mean() * 1000:.2f} mm，"
          f"标准差 {along.std() * 1000:.2f} mm"
          f"（理论 vx*rP/2 = {V_CMD * gen.wave.stance_time / 2 * 1000:.2f} mm）")
    print(f"横向偏差：均值 {np.abs(d[:, 1]).mean() * 1000:.3f} mm")
    print(f"Raibert 修正项：均值 {KX * v_err.mean() * 1000:.2f} mm，"
          f"RMS {KX * np.sqrt((v_err ** 2).mean()) * 1000:.2f} mm（kx = {KX:.3f} s）")
    print(f"速度跟踪误差：RMS {np.sqrt((v_err ** 2).mean()):.4f} m/s，"
          f"最大 {np.abs(v_err).max():.4f} m/s")


def print_rotation(records: list[dict], gen: GaitGenerator) -> None:
    tang, radial, werr = [], [], []
    for r in records:
        # Compare with the neutral point, not with the hip joint: the A1 foot
        # is splayed outboard of the hip, so their fixed angles differ.
        d = r["foot"][:2] - r["neutral"][:2]
        er = r["neutral"][:2] / np.linalg.norm(r["neutral"][:2])
        et = np.array([-er[1], er[0]])
        tang.append(float(d @ et))
        radial.append(float(d @ er))
        werr.append(r["w"] - W_CMD)
    tang = np.array(tang)
    radial = np.array(radial)
    werr = np.array(werr)
    R = float(gen.calc.radius[0])
    print("=== 转动：wz = 0.500 rad/s，10 个周期 ===")
    print(f"落脚点样本 {len(records)} 个（每腿 {len(records) // 4} 个）")
    print(f"相对中性点的切向超前：均值 {tang.mean() * 1000:.2f} mm，"
          f"标准差 {tang.std() * 1000:.2f} mm"
          f"（理论 R*wz*rP/2 = {R * W_CMD * gen.wave.stance_time / 2 * 1000:.2f} mm）")
    print(f"径向偏差：均值 {radial.mean() * 1000:.3f} mm，"
          f"最大 {np.abs(radial).max() * 1000:.3f} mm（应接近 0）")
    print(f"角速度跟踪误差：RMS {np.sqrt((werr ** 2).mean()):.4f} rad/s，"
          f"最大 {np.abs(werr).max():.4f} rad/s")
    print(f"累计转角 1.800 rad = {np.degrees(1.8):.2f}°；中性点半径 R = {R:.4f} m")


def draw_leg(ax, hip, foot, color="0.2", lw=1.8):
    hip = np.asarray(hip, dtype=float)
    foot = np.asarray(foot, dtype=float)
    d = foot - hip
    L = np.linalg.norm(d)
    perp = np.array([-d[1], d[0]]) / L
    knee = 0.5 * (hip + foot) + 0.22 * L * perp
    ax.plot([hip[0], knee[0], foot[0]], [hip[1], knee[1], foot[1]],
            color=color, lw=lw, solid_capstyle="round")
    ax.plot(*hip, "o", color=color, ms=5)
    ax.plot(*foot, "o", color=color, ms=6, mfc="white", mew=1.5)


def figure_neutral():
    fig, (ax, ax2) = plt.subplots(1, 2, figsize=(9.6, 4.0),
                                  gridspec_kw={"width_ratios": [1.35, 1.0]})
    h = 1.0
    mass = np.array([0.0, h])
    ax.plot([-0.75, 0.75], [0, 0], color="0.2", lw=2)
    for x in np.arange(-0.72, 0.75, 0.08):
        ax.plot([x, x - 0.04], [0, -0.05], color="0.45", lw=0.8)
    for x, name, col in [(-0.38, r"$O_1$", "C3"), (0.0, r"$O$", "C2"),
                         (0.38, r"$O_2$", "C1")]:
        ax.plot([0, x], [h, 0], color=col, lw=1.6, ls="-" if x == 0 else "--")
        ax.plot(x, 0, "o", color=col, ms=7)
        ax.text(x, -0.13, name, ha="center", color=col, fontsize=11)
    ax.add_patch(Circle(mass, 0.075, facecolor="0.85", edgecolor="0.2", zorder=4))
    ax.annotate("", xy=(0.0, h - 0.30), xytext=(0.0, h - 0.02),
                arrowprops=dict(arrowstyle="-|>", color="0.1", lw=1.6))
    ax.text(0.04, h - 0.22, r"$G$", fontsize=11)

    arc1 = Arc((-0.38, 0), 0.42, 0.42, theta1=250, theta2=290, color="C3", lw=1.8)
    ax.add_patch(arc1)
    ax.annotate("", xy=(-0.25, -0.19), xytext=(-0.30, -0.15),
                arrowprops=dict(arrowstyle="-|>", color="C3", lw=1.4))
    arc2 = Arc((0.38, 0), 0.42, 0.42, theta1=70, theta2=110, color="C1", lw=1.8)
    ax.add_patch(arc2)
    ax.annotate("", xy=(0.25, -0.19), xytext=(0.30, -0.15),
                arrowprops=dict(arrowstyle="-|>", color="C1", lw=1.4))
    ax.text(-0.80, -0.32, label("偏后 → 向前加速", "behind: accelerate forward"),
            color="C3", fontsize=9)
    ax.text(0.24, -0.32, label("偏前 → 向后加速", "ahead: accelerate backward"),
            color="C1", fontsize=9)
    ax.text(0.0, h + 0.14, label("中性落脚点：重力作用线过接地点", "neutral point"),
            ha="center", fontsize=9)
    ax.set_xlim(-0.85, 0.85)
    ax.set_ylim(-0.38, 1.25)
    ax.set_aspect("equal")
    ax.axis("off")
    ax.set_title(label("(a) 倒立摆与中性落脚点", "(a) inverted pendulum"), fontsize=10)

    x_off = np.linspace(-0.12, 0.12, 201)
    a = -(9.81 / 0.27) * x_off
    ax2.plot(x_off * 1000, a, color="C0", lw=2)
    ax2.axhline(0, color="0.7", lw=0.8)
    ax2.axvline(0, color="0.7", lw=0.8)
    ax2.fill_between(x_off * 1000, 0, a, where=a > 0, color="C0", alpha=0.12)
    ax2.set_xlabel(label(r"足端相对髋的偏移 $\delta=x_f-x_{hip}$ / mm", "foot offset / mm"))
    ax2.set_ylabel(label(r"机身水平加速度 $a_x$ / (m/s$^2$)", "body acceleration / (m/s$^2$)"))
    ax2.set_title(label("(b) 偏后产生向前加速度", "(b) acceleration vs offset"), fontsize=10)
    ax2.text(-110, 3.0, label("落点偏后", "behind"), color="C3", fontsize=9)
    ax2.text(55, -3.6, label("落点偏前", "ahead"), color="C1", fontsize=9)
    ax2.grid(alpha=0.25)
    fig.tight_layout()
    savefig(fig, "9_2_neutral")
    plt.close(fig)


def figure_translation(records: list[dict]):
    fig, (ax, ax2) = plt.subplots(1, 2, figsize=(10.4, 4.3),
                                  gridspec_kw={"width_ratios": [1.0, 1.1]})
    h = 0.27
    hips = [0.40, 0.80, 1.00, 1.20]
    feet = [0.20, 1.00, 1.00, 1.00]
    ax.plot([0.15, 1.35], [h + 0.03, h + 0.03], color="0.25", lw=3)
    for k, (xh, xf) in enumerate(zip(hips, feet)):
        draw_leg(ax, (xh, h), (xf, 0.0), color="0.25", lw=1.5)
        ax.text(xh, h + 0.10, "①②③④"[k], ha="center", fontsize=12)
    ts = np.linspace(0.0, 1.0, 101)
    xs = 0.20 + 0.80 * (ts - np.sin(2 * np.pi * ts) / (2 * np.pi))
    zs = 0.42 * 0.5 * (1 - np.cos(2 * np.pi * ts))
    ax.plot(xs, zs, ls="--", color="C0", lw=1.2)
    ax.plot([hips[0], hips[0]], [0, h], ls=":", color="0.6", lw=0.8)
    ax.plot([feet[1], feet[1]], [0, -0.10], ls=":", color="0.6", lw=0.8)
    ax.annotate("", xy=(hips[0], -0.07), xytext=(0.0, -0.07),
                arrowprops=dict(arrowstyle="<->", color="0.3", lw=0.9))
    ax.text(hips[0] / 2, -0.10, r"$x_b$", ha="center", fontsize=10)
    ax.annotate("", xy=(feet[1], -0.15), xytext=(0.0, -0.15),
                arrowprops=dict(arrowstyle="<->", color="0.3", lw=0.9))
    ax.text(feet[1] / 2, -0.18, r"$x_f$", ha="center", fontsize=10)
    for x0, x1 in [(hips[1], hips[2]), (hips[2], hips[3])]:
        ax.annotate("", xy=(x1, -0.02), xytext=(x0, -0.02),
                    arrowprops=dict(arrowstyle="<->", color="C3", lw=1.2))
        ax.text(0.5 * (x0 + x1), 0.03, r"$\Delta x$", color="C3",
                ha="center", fontsize=9)
    ax.set_xlim(0.0, 1.38)
    ax.set_ylim(-0.22, 0.45)
    ax.set_xlabel(label("前进方向 x / m（示意，Δx 已放大）", "x / m (schematic)"))
    ax.set_yticks([])
    ax.set_title(label("(a) 匀速平移的四个时刻", "(a) four instants"), fontsize=10)

    for i in range(4):
        rec = [r for r in records if r["leg"] == i]
        n = np.array([r["neutral"][:2] for r in rec])
        f = np.array([r["foot"][:2] for r in rec])
        ax2.plot(n[:, 0], n[:, 1], "o", mfc="white", mec=COLORS[i], ms=4.0,
                 label=f"{LEG_ZH[i]} {LEGS[i]}")
        ax2.plot(f[:, 0], f[:, 1], ".", color=COLORS[i], ms=5)
        for k in range(len(rec)):
            ax2.plot([n[k, 0], f[k, 0]], [n[k, 1], f[k, 1]],
                     color=COLORS[i], lw=0.7)
    ax2.plot([0.0, 1.55], [0.0, 0.0], ls="--", color="0.6", lw=0.9,
             label=label("机身中心轨迹", "body path"))
    ax2.set_xlim(-0.02, 1.75)
    ax2.set_ylim(-0.30, 0.28)
    ax2.set_aspect("equal")
    ax2.set_xlabel(label("x / m", "x / m"))
    ax2.set_ylabel(label("y / m", "y / m"))
    ax2.set_title(label("(b) 10 个周期的落脚点（实心）与中性点（空心）",
                        "(b) footholds (filled) and neutral points"), fontsize=10)
    ax2.legend(loc="lower center", fontsize=7, ncol=3, framealpha=0.95)
    ax2.grid(alpha=0.2)
    fig.tight_layout()
    savefig(fig, "9_2_translate")
    plt.close(fig)


def figure_rotation(records: list[dict], R: float):
    fig, (ax, ax2, ax3) = plt.subplots(
        1, 3, figsize=(13.2, 4.6), gridspec_kw={"width_ratios": [1.15, 1.0, 0.95]})
    theta = 0.62
    c, s = np.cos(theta), np.sin(theta)
    body = np.array([[0.13, 0.08], [-0.13, 0.08], [-0.13, -0.08], [0.13, -0.08]])
    body = (body @ np.array([[c, s], [-s, c]]).T)
    ax.add_patch(Polygon(body, closed=True, fill=False, ec="0.2", lw=2))
    ax.add_patch(Circle((0, 0), R, fill=False, ec="0.55", ls="--", lw=1.0))
    ax.annotate("", xy=(0.36, 0.0), xytext=(0.0, 0.0),
                arrowprops=dict(arrowstyle="-|>", color="0.2", lw=1.4))
    ax.annotate("", xy=(0.0, 0.36), xytext=(0.0, 0.0),
                arrowprops=dict(arrowstyle="-|>", color="0.2", lw=1.4))
    ax.text(0.37, -0.02, r"$x_s$", fontsize=10)
    ax.text(-0.02, 0.37, r"$y_s$", fontsize=10)
    ax.annotate("", xy=(0.30 * c, 0.30 * s), xytext=(0.0, 0.0),
                arrowprops=dict(arrowstyle="-|>", color="C0", lw=1.5))
    ax.text(0.31 * c, 0.31 * s, r"$x_b$", color="C0", fontsize=10)
    ax.annotate("", xy=(-0.28 * s, 0.28 * c), xytext=(0.0, 0.0),
                arrowprops=dict(arrowstyle="-|>", color="C0", lw=1.5))
    ax.text(-0.30 * s, 0.30 * c, r"$y_b$", color="C0", fontsize=10)
    ax.add_patch(Arc((0, 0), 0.34, 0.34, theta1=0, theta2=np.degrees(theta),
                     color="0.3", lw=1.0))
    ax.text(0.19, 0.07, r"$\theta$", fontsize=10)

    th0 = -np.deg2rad(35.814)
    ang = theta + th0
    ax.plot([0, R * np.cos(ang)], [0, R * np.sin(ang)], color="C3", lw=1.6)
    ax.plot(R * np.cos(ang), R * np.sin(ang), "o", color="C3", ms=7)
    ax.text(1.06 * R * np.cos(ang) - 0.03, -0.055,
            label("中性点", "neutral"), color="C3", fontsize=9)
    ax.add_patch(Arc((0, 0), 0.26, 0.26, theta1=np.degrees(ang),
                     theta2=np.degrees(theta), color="C3", lw=1.0))
    ax.text(0.145 * np.cos(0.5 * (theta + ang)), 0.145 * np.sin(0.5 * (theta + ang)) - 0.03,
            r"$\theta_0$", color="C3", fontsize=9)

    for k, a in enumerate(np.linspace(-1.1, 1.1, 4)):
        p = R * np.array([np.cos(a), np.sin(a)])
        ax.plot(*p, "o", ms=4, color="0.25")
        ax.text(1.13 * p[0], 1.13 * p[1], "①②③④"[k], fontsize=10)
    ax.add_patch(Arc((0, 0), 0.64, 0.64, theta1=205, theta2=250,
                     color="C2", lw=1.4))
    ax.annotate("", xy=(0.32 * np.cos(np.deg2rad(250)),
                        0.32 * np.sin(np.deg2rad(250))),
                xytext=(0.32 * np.cos(np.deg2rad(240)),
                        0.32 * np.sin(np.deg2rad(240))),
                arrowprops=dict(arrowstyle="-|>", color="C2", lw=1.2))
    ax.text(0.32 * np.cos(np.deg2rad(227)), 0.32 * np.sin(np.deg2rad(227)),
            r"$\omega$", color="C2", fontsize=10)
    ax.set_xlim(-0.42, 0.46)
    ax.set_ylim(-0.40, 0.44)
    ax.set_aspect("equal")
    ax.axis("off")
    ax.set_title(label("(a) 原地转动：同一右前腿的四个时刻", "(a) in-place turn"), fontsize=10)

    for i in range(4):
        rec = [r for r in records if r["leg"] == i]
        cen = np.array([r["neutral"][:2] for r in rec])
        f = np.array([r["foot"][:2] for r in rec])
        ax2.plot(cen[:, 0], cen[:, 1], "o", mfc="white", mec=COLORS[i], ms=4,
                 label=f"{LEG_ZH[i]} {LEGS[i]}")
        ax2.plot(f[:, 0], f[:, 1], ".", color=COLORS[i], ms=5)
    ax2.add_patch(Circle((0, 0), R, fill=False, ec="0.55", ls="--", lw=1.0))
    ax2.plot(0, 0, "k+", ms=8)
    ax2.set_aspect("equal")
    ax2.set_xlim(-0.40, 0.40)
    ax2.set_ylim(-0.40, 0.40)
    ax2.set_xlabel(label("x / m", "x / m"))
    ax2.set_ylabel(label("y / m", "y / m"))
    ax2.set_title(label("(b) 10 个周期的落脚点与中性点", "(b) measured footholds"), fontsize=10)
    ax2.legend(loc="upper right", fontsize=7, framealpha=0.9)
    ax2.grid(alpha=0.2)

    for i in range(4):
        rec = [r for r in records if r["leg"] == i]
        tg, rd = [], []
        for r in rec:
            d = r["foot"][:2] - r["neutral"][:2]
            er = r["neutral"][:2] / np.linalg.norm(r["neutral"][:2])
            et = np.array([-er[1], er[0]])
            tg.append(1000.0 * float(d @ et))
            rd.append(1000.0 * float(d @ er))
        ax3.plot(tg, rd, "o", color=COLORS[i], ms=4,
                 label=f"{LEG_ZH[i]} {LEGS[i]}")
    ax3.axhline(0.0, color="0.6", lw=0.8)
    ax3.axvline(12.19, color="C3", ls="--", lw=0.9,
                label=label("理论 R·wz·rP/2", "theory"))
    ax3.set_xlabel(label("切向偏差 / mm", "tangential / mm"))
    ax3.set_ylabel(label("径向偏差 / mm", "radial / mm"))
    ax3.set_xlim(8.0, 16.0)
    ax3.set_ylim(-1.0, 1.0)
    ax3.set_aspect("equal")
    ax3.grid(alpha=0.2)
    ax3.set_title(label("(c) 相对中性点的偏差", "(c) offset from neutral"), fontsize=10)
    ax3.legend(fontsize=7, loc="upper left", framealpha=0.9)
    fig.tight_layout()
    savefig(fig, "9_2_rotate")
    plt.close(fig)


def main() -> None:
    params, robot, gait = make_robot()
    rec_t = run_case(robot, params, gait, turn=False)
    gen = GaitGenerator(robot, params, gait)
    print_translation(rec_t, gen)

    rec_r = run_case(robot, params, gait, turn=True)
    print_rotation(rec_r, gen)

    figure_neutral()
    figure_translation(rec_t)
    figure_rotation(rec_r, float(gen.calc.radius[0]))


if __name__ == "__main__":
    main()
