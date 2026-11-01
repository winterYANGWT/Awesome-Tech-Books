"""Three-phase currents synthesise a rotating field (section 2.1).

The stator axes are 120 degrees apart.  If the three currents are balanced
sinusoids, the vector sum of the three pulsating fields has constant length
1.5*I and an angle equal to the electrical angle.  The script checks that
identity, draws the vector composition at one instant, and draws the twelve
snapshots used in the text.

expect: 理想三相电流
expect: 12 位量化电流
"""

from __future__ import annotations

import logging

import numpy as np

from quadruped.plotting import savefig          # sets MPLCONFIGDIR before pyplot
import matplotlib.pyplot as plt

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

TWO_PI = 2.0 * np.pi
AXIS_ANGLES = np.array([0.0, TWO_PI / 3.0, 2.0 * TWO_PI / 3.0])
AXES = np.column_stack((np.cos(AXIS_ANGLES), np.sin(AXIS_ANGLES)))
CURRENT_AMPLITUDE = 1.0


def phase_currents(theta, amplitude: float = CURRENT_AMPLITUDE) -> np.ndarray:
    """Balanced three-phase currents at electrical angle ``theta``."""
    return amplitude * np.array([np.cos(theta),
                                 np.cos(theta - TWO_PI / 3.0),
                                 np.cos(theta + TWO_PI / 3.0)])


def field_vector(theta, amplitude: float = CURRENT_AMPLITUDE) -> np.ndarray:
    """Sum of the three pulsating fields, each along its winding axis."""
    return phase_currents(theta, amplitude) @ AXES


def wrap_angle(angle: np.ndarray) -> np.ndarray:
    return (angle + np.pi) % TWO_PI - np.pi


def errors(samples: int = 721, quantise: bool = False):
    theta = np.linspace(0.0, TWO_PI, samples)
    currents = np.array([phase_currents(t) for t in theta])
    if quantise:                       # 12-bit DAC/current-sense resolution
        currents = np.round(currents * 4095.0) / 4095.0
    vectors = currents @ AXES
    magnitude = np.linalg.norm(vectors, axis=1)
    angle = np.arctan2(vectors[:, 1], vectors[:, 0])
    mag_error = np.max(np.abs(magnitude - 1.5 * CURRENT_AMPLITUDE)) / (1.5 * CURRENT_AMPLITUDE)
    ang_error = np.max(np.abs(wrap_angle(angle - theta)))
    return mag_error, ang_error


def draw_composition(ax) -> None:
    theta = np.deg2rad(45.0)
    currents = phase_currents(theta)
    components = currents[:, None] * AXES
    total = components.sum(axis=0)

    circle = plt.Circle((0.0, 0.0), 1.5, fill=False, ls="--", lw=0.8, color="0.6")
    ax.add_patch(circle)
    for k, (axis_name, ang) in enumerate(zip("abc", AXIS_ANGLES)):
        ax.annotate("", xy=(1.72 * np.cos(ang), 1.72 * np.sin(ang)), xytext=(0, 0),
                    arrowprops=dict(arrowstyle="-|>", color="0.45", lw=1.0))
        ax.text(1.82 * np.cos(ang), 1.82 * np.sin(ang), f"轴 {axis_name}",
                ha="center", va="center", color="0.35")

    colors = ("#1f77b4", "#2ca02c", "#9467bd")
    handles = []
    for k in range(3):
        ax.annotate("", xy=components[k], xytext=(0, 0),
                    arrowprops=dict(arrowstyle="-|>", color=colors[k], lw=2.0))
        handles.append(plt.Line2D([], [], color=colors[k], lw=2.0,
                                  label=rf"$\mathbf{{B}}_{{\mathrm{{{'abc'[k]}}}}}$"))
    partial = components[0] + components[2]
    ax.plot([components[0, 0], partial[0]], [components[0, 1], partial[1]],
            ls=":", color="0.55", lw=0.9)
    ax.plot([components[2, 0], partial[0]], [components[2, 1], partial[1]],
            ls=":", color="0.55", lw=0.9)
    ax.plot([partial[0], total[0]], [partial[1], total[1]],
            ls=":", color="0.55", lw=0.9)
    ax.annotate("", xy=total, xytext=(0, 0),
                arrowprops=dict(arrowstyle="-|>", color="#d62728", lw=2.8))
    ax.text(total[0] + 0.05, total[1] + 0.10, r"$\mathbf{B}$", color="#d62728", fontsize=14)

    # Permanent magnet: a short bar through the origin, N at +x.
    half, width = 0.30, 0.07
    ax.add_patch(plt.Rectangle((-half, -width), 2 * half, 2 * width,
                               facecolor="#f2c14e", edgecolor="0.3", lw=0.8))
    ax.text(half + 0.04, 0.12, "N", va="center", fontsize=9)
    ax.text(-half - 0.13, -0.12, "S", va="center", fontsize=9)
    ax.annotate(r"永磁体", xy=(-0.18, 0.04), xytext=(-1.15, 0.78),
                arrowprops=dict(arrowstyle="->", lw=0.8, color="0.3"), fontsize=9)

    arc = np.linspace(0.0, theta, 40)
    ax.plot(0.62 * np.cos(arc), 0.62 * np.sin(arc), color="0.2", lw=0.9)
    ax.text(0.68 * np.cos(theta / 2), 0.68 * np.sin(theta / 2), r"$\delta$", fontsize=12)
    handles.append(plt.Line2D([], [], color="#d62728", lw=2.8, label=r"$\mathbf{B}$"))
    ax.legend(handles=handles, loc="lower right", fontsize=9, framealpha=0.9)

    ax.set_xlim(-2.05, 2.05)
    ax.set_ylim(-2.05, 2.05)
    ax.set_aspect("equal")
    ax.axhline(0, color="0.9", lw=0.5, zorder=0)
    ax.axvline(0, color="0.9", lw=0.5, zorder=0)
    ax.set_title("(a) 一个时刻的三相分量与合成矢量")
    ax.set_xlabel("定子平面 x")
    ax.set_ylabel("定子平面 y")


def draw_snapshots(ax) -> None:
    ax.add_patch(plt.Circle((0.0, 0.0), 1.5, fill=False, ls="--", lw=0.8, color="0.6"))
    colors = plt.cm.viridis(np.linspace(0.05, 0.95, 12))
    for i, deg in enumerate(range(0, 360, 30)):
        vec = field_vector(np.deg2rad(deg))
        ax.annotate("", xy=vec, xytext=(0, 0),
                    arrowprops=dict(arrowstyle="-|>", color=colors[i], lw=1.6))
        if deg % 60 == 0:
            ax.text(1.60 * np.cos(np.deg2rad(deg)), 1.60 * np.sin(np.deg2rad(deg)),
                    f"{deg}°", fontsize=8, ha="center", va="center")
    ax.set_xlim(-1.9, 1.9)
    ax.set_ylim(-1.9, 1.9)
    ax.set_aspect("equal")
    ax.axhline(0, color="0.9", lw=0.5, zorder=0)
    ax.axvline(0, color="0.9", lw=0.5, zorder=0)
    ax.set_title("(b) 一个电周期内的 12 个时刻")
    ax.set_xlabel("定子平面 x")
    ax.set_ylabel("定子平面 y")


def draw_waveform() -> None:
    theta = np.linspace(0.0, TWO_PI, 721)
    currents = np.array([phase_currents(t) for t in theta])
    vectors = currents @ AXES
    magnitude = np.linalg.norm(vectors, axis=1)
    angle = np.unwrap(np.arctan2(vectors[:, 1], vectors[:, 0]))

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(7.2, 6.0), sharex=True)
    for k, name in enumerate("abc"):
        ax1.plot(theta, currents[:, k], lw=1.5, label=rf"$i_{name}$")
    ax1.set_ylabel("相电流 (归一化)")
    ax1.set_title("三相对称电流与合成磁场")
    ax1.legend(ncol=3, fontsize=8, loc="upper right")
    ax1.grid(True, alpha=0.3)

    ax2.plot(theta, magnitude, color="#d62728", lw=1.8, label=r"$|\mathbf{B}|$")
    ax2.set_ylabel(r"$|\mathbf{B}|$ (归一化)", color="#d62728")
    ax2.tick_params(axis="y", labelcolor="#d62728")
    ax2.set_ylim(0.0, 1.8)
    ax2.grid(True, alpha=0.3)
    ax3 = ax2.twinx()
    ax3.plot(theta, angle, color="#1f77b4", lw=1.2, ls="--", label=r"$\angle\mathbf{B}$")
    ax3.set_ylabel(r"$\angle\mathbf{B}$ (rad)", color="#1f77b4")
    ax3.tick_params(axis="y", labelcolor="#1f77b4")
    ax3.set_yticks([0.0, np.pi / 2, np.pi, 3 * np.pi / 2, TWO_PI])
    ax3.set_yticklabels(["0", r"$\pi/2$", r"$\pi$", r"$3\pi/2$", r"$2\pi$"])
    ax2.set_xlabel("电角度 (rad)")
    ax2.set_xticks([0.0, np.pi / 2, np.pi, 3 * np.pi / 2, TWO_PI])
    ax2.set_xticklabels(["0", r"$\pi/2$", r"$\pi$", r"$3\pi/2$", r"$2\pi$"])
    savefig(fig, "2_1_foc_wave")


def main() -> None:
    ideal_mag, ideal_ang = errors(quantise=False)
    quant_mag, quant_ang = errors(quantise=True)
    print(f"合成矢量理论幅值: {1.5 * CURRENT_AMPLITUDE:.6f} I")
    print(f"理想三相电流: 最大相对幅值误差 {ideal_mag:.2e}, 最大角度误差 {ideal_ang:.2e} rad")
    print(f"12 位量化电流: 最大相对幅值误差 {quant_mag:.2e}, 最大角度误差 {quant_ang:.2e} rad")
    print("一个电周期内的采样角度 (deg): " +
          ", ".join(str(a) for a in range(0, 360, 30)))

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10.6, 5.0))
    draw_composition(ax1)
    draw_snapshots(ax2)
    fig.tight_layout()
    savefig(fig, "2_1_foc_field")
    draw_waveform()


if __name__ == "__main__":
    main()
