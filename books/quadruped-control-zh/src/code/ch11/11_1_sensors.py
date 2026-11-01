"""Four range-sensor principles and a first look at 2D navigation (11.1).

Panels and printed numbers are produced from the same model:

* stereo depth from disparity, ``Z = f B / d``, with the first-order error
  ``dZ = Z^2 / (f B) * dd``;
* a 360-beam single-line lidar in a 6 m x 4 m room with one partition and one
  cylindrical pillar, ray-cast analytically against line segments and a circle;
* an ultrasonic ping: a 15-degree half-angle cone, one echo from a wall, and
  the footprint radius that explains why the bearing is so coarse.

The lidar here is a pure-geometry model.  Chapter 11.4 uses the MuJoCo ray
caster in ``quadruped.nav.Lidar2D`` instead; the scan geometry is the same.

expect: 双目
expect: 有效量程
expect: 单线激光
expect: 命中
expect: 超声波
expect: 四类传感器
expect: 图 11-1
"""

from __future__ import annotations

import math
import os

os.environ.setdefault("MPLCONFIGDIR", "/tmp/quadruped-mpl")
os.environ.setdefault("MESA_SHADER_CACHE_DISABLE", "true")

import logging
import warnings

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Circle
from matplotlib.ticker import NullFormatter

from quadruped.plotting import label, savefig

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)
logging.getLogger("matplotlib").setLevel(logging.ERROR)
warnings.filterwarnings("ignore", message=".*glyph.*")
warnings.filterwarnings("ignore", message=".*tight_layout.*")
plt.rcParams["mathtext.fontset"] = "dejavusans"

C_LIGHT = 2.998e8
T_CELSIUS = 20.0
V_SOUND = 331.4 + 0.6 * T_CELSIUS


# ---------------------------------------------------------------------------
# Stereo
# ---------------------------------------------------------------------------
# ANCHOR: stereo
def stereo_table(f_px: float = 320.0, baseline: float = 0.12,
                 sigma_d: float = 0.25) -> tuple[np.ndarray, np.ndarray]:
    z = np.array([0.3, 0.5, 1.0, 2.0, 4.0, 8.0])
    disparity = f_px * baseline / z
    depth_sigma = z ** 2 / (f_px * baseline) * sigma_d
    z_limit = math.sqrt(0.10 * f_px * baseline / sigma_d)
    return z, disparity, depth_sigma, z_limit
# ANCHOR_END: stereo


def draw_stereo(z_limit: float) -> None:
    fig, (ax, ax2) = plt.subplots(1, 2, figsize=(11.0, 4.6),
                                  gridspec_kw={"width_ratios": [1.05, 1.0]})

    # --- plan view (schematic, not to scale) --------------------------------
    b = 1.2
    z_obj = 2.8
    f_draw = 0.9
    left = np.array([-b / 2, 0.0])
    right = np.array([b / 2, 0.0])
    point = np.array([0.0, z_obj])

    ax.plot([left[0], point[0]], [left[1], point[1]], color="C0", lw=1.4)
    ax.plot([right[0], point[0]], [right[1], point[1]], color="C3", lw=1.4)

    # Image plane at depth f_draw, drawn oversize for visibility.
    x_l = -b / 2 + (point[0] - left[0]) * f_draw / z_obj
    x_r = b / 2 + (point[0] - right[0]) * f_draw / z_obj
    ax.plot([-1.02, 1.02], [f_draw, f_draw], color="k", lw=1.2)
    ax.plot([x_l, x_l], [f_draw - 0.06, f_draw + 0.06], color="C0", lw=3)
    ax.plot([x_r, x_r], [f_draw - 0.06, f_draw + 0.06], color="C3", lw=3)
    ax.annotate("", xy=(x_r, f_draw + 0.22), xytext=(x_l, f_draw + 0.22),
                arrowprops=dict(arrowstyle="<->", color="k", lw=1.0))
    ax.text((x_l + x_r) / 2, f_draw + 0.28, "d", ha="center", fontsize=10)
    ax.text(x_l - 0.05, f_draw - 0.16, r"$x_{\rm L}$", ha="right", fontsize=9)
    ax.text(x_r + 0.05, f_draw - 0.16, r"$x_{\rm R}$", ha="left", fontsize=9)

    ax.scatter([left[0], right[0]], [0, 0], s=70, c=["C0", "C3"],
               edgecolors="k", zorder=4)
    ax.text(left[0], -0.18, label("左相机", "left camera"), ha="center")
    ax.text(right[0], -0.18, label("右相机", "right camera"), ha="center")
    ax.scatter([point[0]], [point[1]], s=60, c="k", zorder=4)
    ax.text(point[0] + 0.07, point[1] + 0.02, r"$P=(X,Z)$", va="center")
    ax.annotate("", xy=(left[0], -0.36), xytext=(right[0], -0.36),
                arrowprops=dict(arrowstyle="<->", color="k", lw=1.0))
    ax.text(0.0, -0.46, r"基线 $B$", ha="center")
    ax.annotate("", xy=(0.95, 0.0), xytext=(0.95, z_obj),
                arrowprops=dict(arrowstyle="<->", color="k", lw=1.0))
    ax.text(1.0, z_obj / 2, r"深度 $Z$", rotation=90, va="center")
    ax.text(0.0, -0.95,
            r"$Z=\dfrac{fB}{d}$",
            fontsize=13, ha="center", va="center",
            bbox=dict(boxstyle="round", fc="white", ec="0.7"))
    ax.set_xlim(-1.15, 1.15)
    ax.set_ylim(-1.18, 3.35)
    ax.set_aspect("equal")
    ax.axis("off")
    ax.set_title(label("(a) 双目几何：同名点的视差", "(a) stereo geometry"))

    # --- depth error -------------------------------------------------------
    z = np.logspace(math.log10(0.3), math.log10(8.0), 200)
    for dd, color in ((0.10, "C0"), (0.25, "C2"), (0.50, "C3")):
        dz = z ** 2 / (320.0 * 0.12) * dd * 1000.0
        ax2.loglog(z, dz, color=color, lw=1.6,
                   label=rf"$\Delta d={dd:.2f}$ px")
    ax2.axhline(100.0, color="0.4", ls="--", lw=1.0)
    ax2.text(0.32, 115.0, label("100 mm 误差线", "100 mm error"), fontsize=8,
             color="0.35")
    ax2.axvline(z_limit, color="C2", ls=":", lw=1.0)
    ax2.plot([z_limit], [100.0], "o", color="C2")
    ax2.annotate(rf"$Z={z_limit:.2f}$ m", xy=(z_limit, 100.0),
                 xytext=(z_limit * 0.55, 18.0), fontsize=9,
                 arrowprops=dict(arrowstyle="->", color="0.3", lw=0.9))
    ax2.set_xlabel(label("深度 Z (m)", "depth Z (m)"))
    ax2.set_ylabel(label("深度误差 1σ (mm)", "depth error 1σ (mm)"))
    ax2.set_xticks([0.3, 0.5, 1.0, 2.0, 4.0, 8.0])
    ax2.set_xticklabels(["0.3", "0.5", "1", "2", "4", "8"])
    ax2.set_yticks([1, 10, 100, 1000])
    ax2.set_yticklabels(["1", "10", "100", "1000"])
    ax2.xaxis.set_minor_formatter(NullFormatter())
    ax2.yaxis.set_minor_formatter(NullFormatter())
    ax2.grid(True, which="both", alpha=0.25)
    ax2.legend(fontsize=8, loc="upper left")
    ax2.set_title(label("(b) 误差随 Z² 增长（f=320 px, B=0.12 m）",
                        "(b) error grows as Z²"))
    fig.tight_layout()
    savefig(fig, "11_1_stereo")
    plt.close(fig)


# ---------------------------------------------------------------------------
# Lidar room
# ---------------------------------------------------------------------------
SEGMENTS = [
    ((0.0, 0.0), (6.0, 0.0)),      # south outer wall
    ((6.0, 0.0), (6.0, 4.0)),      # east
    ((6.0, 4.0), (0.0, 4.0)),      # north
    ((0.0, 4.0), (0.0, 0.0)),      # west
    ((3.0, 0.0), (3.0, 2.5)),      # partition, gap at the top
]
PILLAR = (4.5, 1.2, 0.25)
SENSOR = (1.0, 1.0)


def ray_segment(origin: np.ndarray, direction: np.ndarray,
                a: np.ndarray, b: np.ndarray) -> float:
    """Distance to segment ``a-b`` or ``inf``."""
    e = b - a
    denom = direction[0] * e[1] - direction[1] * e[0]
    if abs(denom) < 1e-12:
        return math.inf
    diff = a - origin
    t = (diff[0] * e[1] - diff[1] * e[0]) / denom
    u = (diff[0] * direction[1] - diff[1] * direction[0]) / denom
    if t > 1e-9 and -1e-9 <= u <= 1.0 + 1e-9:
        return float(t)
    return math.inf


# ANCHOR: raycast
def ray_circle(origin: np.ndarray, direction: np.ndarray,
               center: np.ndarray, radius: float) -> float:
    oc = origin - center
    b = 2.0 * float(direction @ oc)
    c = float(oc @ oc) - radius * radius
    disc = b * b - 4.0 * c
    if disc < 0.0:
        return math.inf
    root = math.sqrt(disc)
    hits = [(-b - root) / 2.0, (-b + root) / 2.0]
    hits = [h for h in hits if h > 1e-9]
    return min(hits) if hits else math.inf


def lidar_scan(origin=SENSOR, n_beams: int = 360,
               max_range: float = 8.0) -> tuple[np.ndarray, np.ndarray]:
    origin = np.asarray(origin, dtype=float)
    angles = np.deg2rad(np.arange(n_beams, dtype=float) - 180.0)
    ranges = np.full(n_beams, np.inf)
    for i, angle in enumerate(angles):
        direction = np.array([math.cos(angle), math.sin(angle)])
        best = max_range
        for a, b in SEGMENTS:
            best = min(best, ray_segment(origin, direction,
                                         np.asarray(a), np.asarray(b)))
        best = min(best, ray_circle(origin, direction,
                                    np.array(PILLAR[:2]), PILLAR[2]))
        if best <= max_range:
            ranges[i] = best
    return angles, ranges
# ANCHOR_END: raycast


def draw_compare(stereo_limit: float, angles: np.ndarray,
                 ranges: np.ndarray) -> None:
    specs = [
        ("1 双目", 0.30, stereo_limit, 90.0 / 640.0, "C0"),
        ("2 结构光", 0.30, 3.0, 87.0 / 1280.0, "C1"),
        ("3 单线激光", 0.05, 30.0, 0.5, "C2"),
        ("4 超声波", 0.02, 4.0, 30.0, "C3"),
    ]
    fig = plt.figure(figsize=(12.6, 4.4))
    gs = fig.add_gridspec(1, 3, width_ratios=[1.0, 1.1, 1.0], wspace=0.30)
    ax = fig.add_subplot(gs[0, 0])
    for name, rmin, rmax, res, color in specs:
        ax.plot([res, res], [rmin, rmax], color=color, lw=2.4, alpha=0.8)
        ax.plot([res], [rmax], "o", color=color, ms=7)
        ax.plot([res], [rmin], "v", color=color, ms=5)
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlim(0.04, 60.0)
    ax.set_ylim(0.012, 90.0)
    ax.set_xticks([0.05, 0.1, 0.5, 1, 10, 30])
    ax.set_xticklabels(["0.05", "0.1", "0.5", "1", "10", "30"])
    ax.set_yticks([0.02, 0.1, 1, 4, 30])
    ax.set_yticklabels(["0.02", "0.1", "1", "4", "30"])
    ax.xaxis.set_minor_formatter(NullFormatter())
    ax.yaxis.set_minor_formatter(NullFormatter())
    ax.set_xlabel(label("角分辨率 (°，越小越好)", "angular resolution (deg)"))
    ax.set_ylabel(label("有效量程 (m)", "effective range (m)"))
    ax.grid(True, which="both", alpha=0.25)
    legend = [f"{name}   {rmin:g}–{rmax:.2f} m, {res:.2f}°"
              for name, rmin, rmax, res, _ in specs]
    ax.legend(legend, fontsize=7.5, loc="lower right", framealpha=0.95)
    ax.set_title(label("(a) 量程—角分辨率对比", "(a) range vs angular resolution"))

    ax = fig.add_subplot(gs[0, 1])
    for a, b in SEGMENTS:
        ax.plot([a[0], b[0]], [a[1], b[1]], color="0.25", lw=3.0,
                solid_capstyle="butt")
    ax.add_patch(Circle(PILLAR[:2], PILLAR[2], color="0.35"))
    ax.scatter([SENSOR[0]], [SENSOR[1]], marker="s", s=42, c="C2",
               edgecolors="k", zorder=5,
               label=label("激光雷达", "lidar"))
    finite = np.isfinite(ranges)
    # Every fourth ray is drawn; all 360 hit points are plotted.
    for angle, r in list(zip(angles[finite], ranges[finite]))[::4]:
        ax.plot([SENSOR[0], SENSOR[0] + r * math.cos(angle)],
                [SENSOR[1], SENSOR[1] + r * math.sin(angle)], color="C3",
                lw=0.4, alpha=0.45)
    ax.plot(SENSOR[0] + ranges[finite] * np.cos(angles[finite]),
            SENSOR[1] + ranges[finite] * np.sin(angles[finite]), ".",
            color="C3", ms=2.0)
    ax.set_aspect("equal")
    ax.set_xlim(-0.1, 6.1)
    ax.set_ylim(-0.1, 4.1)
    ax.set_xlabel("x (m)")
    ax.set_ylabel("y (m)")
    ax.set_title(label("(b) 360 束单线扫描（6 m × 4 m 房间）",
                       "(b) 360-beam scan in a 6 m x 4 m room"))
    ax.legend(loc="upper left", fontsize=8)

    ax = fig.add_subplot(gs[0, 2])
    wall_x = 1.2
    half = math.radians(15.0)
    ax.plot([0, wall_x], [0, 0], color="C2", lw=1.2)
    ax.plot([0, wall_x * math.cos(half)],
            [0, wall_x * math.sin(half)], color="C3", lw=1.0)
    ax.plot([0, wall_x * math.cos(half)],
            [0, -wall_x * math.sin(half)], color="C3", lw=1.0)
    ax.plot([wall_x, wall_x], [-0.75, 0.75], color="0.25", lw=3.0)
    footprint = wall_x * math.tan(half)
    ax.annotate("", xy=(wall_x, -footprint), xytext=(wall_x, footprint),
                arrowprops=dict(arrowstyle="<->", color="k", lw=1.0))
    ax.text(wall_x + 0.05, 0.0,
            f"足印 ±{footprint:.2f} m\n回波 {2 * wall_x / V_SOUND * 1e3:.2f} ms",
            fontsize=8, va="center")
    ax.text(0.10, 0.22, r"$2\theta=30^\circ$", fontsize=9)
    ax.text(0.10, -0.28, label("发射 + 回波", "ping + echo"), fontsize=9)
    ax.set_xlim(-0.12, 1.90)
    ax.set_ylim(-0.9, 0.9)
    ax.set_aspect("equal")
    ax.set_xlabel("x (m)")
    ax.set_ylabel("y (m)")
    ax.set_title(label("(c) 15° 半角超声锥与一次回波", "(c) ultrasonic cone"))
    fig.tight_layout()
    savefig(fig, "11_1_compare")
    plt.close(fig)


def main() -> None:
    z, disparity, sigma_z, z_limit = stereo_table()
    print("=== 双目 ===")
    print("f=320 px, B=0.120 m, 视差误差 0.25 px, fB=38.40 px*m")
    for zi, di, si in zip(z, disparity, sigma_z):
        print(f"Z={zi:4.2f} m  d={di:7.2f} px  dZ={si * 1000:7.2f} mm")
    print(f"有效量程（dZ<100 mm）: 0.30-{z_limit:.2f} m")

    print("=== 单线激光 ===")
    angles, ranges = lidar_scan()
    finite = np.isfinite(ranges)
    i_min = int(np.argmin(ranges))
    i_max = int(np.argmax(ranges))
    print("房间 6.0 x 4.0 m，隔墙 1 道（x=3.0, y=0-2.5），圆柱 1 根（r=0.25）")
    print("360 束，1.0 deg，量程 0.05-8.00 m")
    print(f"命中 {finite.sum()} 束，无回波 {(~finite).sum()} 束")
    print(f"最近 {ranges[i_min]:.2f} m @ {math.degrees(angles[i_min]):+.1f} deg，"
          f"最远 {ranges[i_max]:.2f} m @ {math.degrees(angles[i_max]):+.1f} deg")
    print(f"单线激光测距 10 mm 对应计时 {2 * 0.010 / C_LIGHT * 1e12:.1f} ps")

    print("=== 超声波 ===")
    print(f"声速（T={T_CELSIUS:.0f} C）: {V_SOUND:.1f} m/s")
    for dist in (0.5, 1.2, 4.0):
        t = 2.0 * dist / V_SOUND
        print(f"d={dist:.1f} m  回波 {t * 1e3:.3f} ms  "
              f"波束足印半径 {dist * math.tan(math.radians(15.0)):.3f} m")
    radius = 0.05
    footprint = 1.2 * math.tan(math.radians(15.0))
    print(f"1.2 m 处半径 {radius * 100:.0f} cm 小圆柱占波束截面 "
          f"{100 * (radius / footprint) ** 2:.1f}%，回波约低 "
          f"{-20 * math.log10(radius / footprint):.1f} dB")
    print(f"4 m 往返 {2 * 4.0 / V_SOUND * 1e3:.1f} ms，单探头上限 "
          f"{V_SOUND / (2 * 4.0):.1f} Hz")

    print("=== 四类传感器 ===")
    rows = [
        ("双目视觉", 0.30, z_limit, 90.0 / 640.0, "6.5 mm @1 m"),
        ("结构光", 0.30, 3.0, 87.0 / 1280.0, "2 mm @1 m"),
        ("单线激光", 0.05, 30.0, 0.5, "±20 mm"),
        ("超声波", 0.02, 4.0, 30.0, "10 mm + 1% d"),
    ]
    print(f"{'传感器':<8}{'有效量程/m':<16}{'角分辨率/deg':<14}典型误差")
    for name, rmin, rmax, res, err in rows:
        print(f"{name:<8}{rmin:g}-{rmax:.2f}{'':<8}{res:<14.2f}{err}")

    draw_stereo(z_limit)
    draw_compare(z_limit, angles, ranges)
    print("图 11-1 双目视差与深度误差；图 11-2 量程—角分辨率、扫描与超声锥")


if __name__ == "__main__":
    main()
