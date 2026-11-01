# expect: 步态参数
# expect: 最少触地腿数
# expect: 2c =
"""Gait timing: trot, walking trot, pace and bound as bar charts (9.1).

A gait is only a schedule of contact and flight in time.  For every leg the
stance interval starts at ``b_i`` and lasts ``r P``; the swing interval lasts
``(1-r) P``.  The script computes the four-leg overlap and flight segments
exactly from the interval end points (not by sampling) and prints the minimum
number of stance legs, which is the quantity that matters for support.

"""

from __future__ import annotations

import logging

import numpy as np

from quadruped import WaveGenerator, WaveStatus
from quadruped.plotting import label, savefig
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

LEGS = ["FR", "FL", "RR", "RL"]
LEG_ZH = ["右前", "左前", "右后", "左后"]

# name, period, duty, offsets/P.  The four examples share P = 1 s.
GAITS = [
    ("trot", 1.0, 0.500, [0.0, 0.5, 0.5, 0.0]),
    ("walking trot", 1.0, 0.625, [0.0, 0.5, 0.5, 0.0]),
    ("pace", 1.0, 0.500, [0.0, 0.5, 0.0, 0.5]),
    ("bound", 1.0, 0.400, [0.0, 0.0, 0.5, 0.5]),
]


# ANCHOR: intervals
def stance_intervals(duty: float, offset: float) -> list[tuple[float, float]]:
    """Stance intervals of one leg in one period, in units of P."""
    start = (-offset) % 1.0
    end = start + duty
    if end <= 1.0:
        return [(start, end)]
    return [(start, 1.0), (0.0, end - 1.0)]


def contact_at(t: float, duty: float, offset: float) -> int:
    loc = (t + offset) % 1.0
    return int(loc < duty)


def interval_metrics(duty: float, offsets: list[float]) -> dict[str, float]:
    """Exact overlap/flight/min-contact metrics from interval boundaries."""
    cuts = {0.0, 1.0}
    for off in offsets:
        for a, b in stance_intervals(duty, off):
            cuts.add(a)
            cuts.add(b)
    cuts = sorted(cuts)
    overlap = flight = 0.0
    minimum = 4
    for a, b in zip(cuts, cuts[1:]):
        mid = 0.5 * (a + b)
        count = sum(contact_at(mid, duty, off) for off in offsets)
        if count == 4:
            overlap += b - a
        if count == 0:
            flight += b - a
        minimum = min(minimum, count)
    return {"overlap": overlap, "flight": flight, "min_contact": minimum}


# ANCHOR_END: intervals


def check_with_wave(period: float, duty: float, offsets: list[float]) -> tuple[int, int]:
    """Sample the generator as a cross-check of the interval calculation."""
    wave = WaveGenerator(period, duty, np.array(offsets))
    wave.set_status(WaveStatus.WAVE_ALL, 0.0)
    count = np.zeros(2001, dtype=int)
    for k, t in enumerate(np.linspace(0.0, period, 2001, endpoint=False)):
        _, contact = wave.step(t)
        count[k] = int(contact.sum())
    return int(count.min()), int(count.max())


# ANCHOR: bars
def draw_bars(ax, duty: float, offsets: list[float], title: str) -> None:
    """Black stance / white swing bars for the four legs over one period."""
    for i, off in enumerate(offsets):
        ax.add_patch(Rectangle((0.0, i - 0.32), 1.0, 0.64,
                               facecolor="white", edgecolor="0.6", lw=0.8))
        for a, b in stance_intervals(duty, off):
            ax.add_patch(Rectangle((a, i - 0.32), b - a, 0.64,
                                   facecolor="0.15", edgecolor="0.15"))
    ax.axvline(0.5, color="0.55", ls="--", lw=0.8)
    ax.set_xlim(0.0, 1.0)
    ax.set_ylim(-0.6, 3.6)
    ax.set_yticks(range(4))
    ax.set_yticklabels([f"{LEG_ZH[i]} {LEGS[i]}" for i in range(4)])
    ax.invert_yaxis()
    ax.set_title(title, fontsize=10)
    ax.set_xticks(np.arange(0.0, 1.01, 0.25))


# ANCHOR_END: bars


def figure_single_leg() -> None:
    """Fig. 9-1: b, rP and (1-r)P on one leg's time axis."""
    period, duty, offset = 1.0, 0.6, 0.2
    fig, ax = plt.subplots(figsize=(7.2, 2.4))
    for k in range(2):
        start = offset + k * period
        ax.add_patch(Rectangle((start, 0.0), duty * period, 0.5,
                               facecolor="0.15", edgecolor="0.15"))
        ax.add_patch(Rectangle((start + duty * period, 0.0),
                               (1.0 - duty) * period, 0.5,
                               facecolor="white", edgecolor="0.6", lw=0.8))
    ax.set_xlim(0.0, 2.0)
    ax.set_ylim(-0.55, 0.95)
    ax.set_yticks([])
    ax.set_xlabel(label("时间 t / s（P = 1 s）", "time t / s (P = 1 s)"))
    ax.set_xticks([0.0, offset, offset + period])
    ax.set_xticklabels(["0", r"$b$", r"$b+P$"])

    y = 0.62
    ax.annotate("", xy=(offset, y), xytext=(0.0, y),
                arrowprops=dict(arrowstyle="<->", color="C3", lw=1.2))
    ax.text(offset / 2, y + 0.04, r"$b$", color="C3", ha="center")

    y = -0.22
    ax.annotate("", xy=(offset + duty, y), xytext=(offset, y),
                arrowprops=dict(arrowstyle="<->", color="C0", lw=1.2))
    ax.text(offset + duty / 2, y - 0.16, r"$rP$", color="C0", ha="center")
    ax.annotate("", xy=(offset + 1.0, y), xytext=(offset + duty, y),
                arrowprops=dict(arrowstyle="<->", color="C2", lw=1.2))
    ax.text(offset + duty + (1.0 - duty) / 2, y - 0.16,
            r"$(1-r)P$", color="C2", ha="center")

    ax.annotate("", xy=(offset, 0.86), xytext=(offset + 1.0, 0.86),
                arrowprops=dict(arrowstyle="<->", color="0.3", lw=1.0))
    ax.text(offset + 0.5, 0.88, r"$P$", ha="center", color="0.3")
    ax.text(offset + 0.05, 0.25, label("触地", "stance"), color="white", fontsize=9)
    ax.text(offset + duty + 0.02, 0.25, label("腾空", "swing"), fontsize=9)
    ax.set_title(label("单腿步态条状图", "single-leg gait bar"), fontsize=10)
    fig.tight_layout()
    savefig(fig, "9_1_single")
    plt.close(fig)


def figure_four_gaits(metrics: list[dict[str, float]]) -> None:
    """Fig. 9-2: the four gaits in a 2x2 panel."""
    fig, axes = plt.subplots(2, 2, figsize=(9.6, 5.6), sharex=True, sharey=True)
    for ax, (name, period, duty, offsets), m in zip(axes.ravel(), GAITS, metrics):
        draw_bars(ax, duty, offsets, f"{name}  r = {duty:.3f}")
        ax.set_title(f"{name}  r = {duty:.3f}\n"
                     + label(f"四足触地 {m['overlap']:.3f}P，四足腾空 {m['flight']:.3f}P",
                             f"overlap {m['overlap']:.3f}P, flight {m['flight']:.3f}P"),
                     fontsize=9)
    fig.supxlabel(label("归一化时间 t/P（黑 = 触地，白 = 腾空）",
                        "normalized time t/P (black = stance, white = swing)"),
                  fontsize=9)
    fig.suptitle(label("四种常用步态", "four common gaits"), fontsize=11)
    fig.tight_layout()
    savefig(fig, "9_1_gaits")
    plt.close(fig)


def main() -> None:
    print("步态参数：步态周期 P = 1.000 s，腿序 0=右前、1=左前、2=右后、3=左后")
    print("步态           r       b/P（四条腿）                   rP/s    (1-r)P/s")
    for name, period, duty, offsets in GAITS:
        print(f"{name:<13s} {duty:5.3f}   "
              + " ".join(f"{x:4.2f}" for x in offsets)
              + f"    {duty * period:5.3f}    {(1 - duty) * period:5.3f}")

    print()
    metrics = [interval_metrics(g[2], g[3]) for g in GAITS]
    print("步态           四足同时触地/s   四足同时腾空/s   最少触地腿数（区间解/采样校验）")
    for (name, period, duty, offsets), m in zip(GAITS, metrics):
        lo, hi = check_with_wave(period, duty, offsets)
        print(f"{name:<13s} {m['overlap'] * period:13.3f}   "
              f"{m['flight'] * period:13.3f}   "
              f"{int(m['min_contact'])} / {lo}（最多 {hi}）")

    c = 0.5 * (2.0 * GAITS[1][2] - 1.0) * GAITS[1][1]
    print()
    print(f"步行对角步态：2c = (2r-1)P = {2 * c:.3f} s，c = {c:.3f} s = 0.125P")
    print("trot 的两段触地首尾相接，重叠恰好为 0；bound 有两段四足腾空。")

    figure_single_leg()
    figure_four_gaits(metrics)


if __name__ == "__main__":
    main()
