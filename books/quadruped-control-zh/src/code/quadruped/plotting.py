"""Consistent matplotlib output for the book's examples.

``savefig`` writes to ``$FIG_DIR`` (default ``./figs``) with a fixed DPI and
tight bounding box.  The module checks whether a Chinese font is available;
if one is present, Chinese labels are enabled, otherwise callers should use
the English strings returned by :func:`label`.
"""

from __future__ import annotations

import os
import tempfile
from pathlib import Path

# Keep matplotlib's cache in a writable place on shared machines.
_cache = Path(tempfile.gettempdir()) / "quadruped-mpl"
os.environ.setdefault("MPLCONFIGDIR", str(_cache))
_cache.mkdir(parents=True, exist_ok=True)

import matplotlib  # noqa: E402

if not os.environ.get("DISPLAY") and not os.environ.get("MPLBACKEND"):
    matplotlib.use("Agg")

import matplotlib.pyplot as plt  # noqa: E402
from matplotlib import font_manager  # noqa: E402

DPI = 150
_CANDIDATES = (
    "Noto Sans CJK SC",
    "Noto Sans SC",
    "Source Han Sans SC",
    "WenQuanYi Zen Hei",
    "WenQuanYi Micro Hei",
    "Microsoft YaHei",
    "SimHei",
    "PingFang SC",
    "Heiti SC",
)


def _pick_chinese_font() -> str | None:
    available = {f.name for f in font_manager.fontManager.ttflist}
    for name in _CANDIDATES:
        if name in available:
            return name
    return None


CHINESE_FONT = _pick_chinese_font()
HAS_CHINESE = CHINESE_FONT is not None

if HAS_CHINESE:
    plt.rcParams["font.sans-serif"] = [CHINESE_FONT, "DejaVu Sans"]
    plt.rcParams["axes.unicode_minus"] = False
else:
    plt.rcParams["font.sans-serif"] = ["DejaVu Sans"]
plt.rcParams["figure.dpi"] = 110
plt.rcParams["savefig.dpi"] = DPI


def label(zh: str, en: str) -> str:
    """Return a Chinese label when a CJK font exists, else the English one."""
    return zh if HAS_CHINESE else en


def fig_dir() -> Path:
    return Path(os.environ.get("FIG_DIR", "figs")).expanduser()


def savefig(fig, name: str, dpi: int = DPI) -> Path:
    """Save ``fig`` to ``$FIG_DIR/name.png`` and return the path."""
    directory = fig_dir()
    directory.mkdir(parents=True, exist_ok=True)
    filename = str(name)
    if not filename.lower().endswith((".png", ".pdf", ".svg")):
        filename += ".png"
    path = directory / filename
    fig.savefig(path, dpi=dpi, bbox_inches="tight")
    return path


def plot_state(time, values, labels=None, title: str | None = None,
               ylabel: str | None = None):
    """Small convenience helper used by the chapter examples."""
    import numpy as np

    t = np.asarray(time, dtype=float)
    y = np.asarray(values, dtype=float)
    fig, ax = plt.subplots(figsize=(6.0, 3.2))
    if y.ndim == 1:
        ax.plot(t, y, lw=1.5)
    else:
        for i in range(y.shape[1]):
            text = None if labels is None else labels[i]
            ax.plot(t, y[:, i], lw=1.3, label=text)
        if labels is not None:
            ax.legend(fontsize=8)
    if title:
        ax.set_title(title)
    if ylabel:
        ax.set_ylabel(ylabel)
    ax.set_xlabel(label("时间 (s)", "time (s)"))
    ax.grid(True, alpha=0.3)
    return fig, ax


def plot_feet(feet, labels=("FR", "FL", "RR", "RL"), title: str | None = None):
    """Plot planar (x-y) foot trajectories."""
    import numpy as np

    feet = np.asarray(feet, dtype=float)
    fig, ax = plt.subplots(figsize=(4.6, 4.2))
    for i in range(feet.shape[1]):
        ax.plot(feet[:, i, 0], feet[:, i, 1], lw=1.3, label=labels[i])
    ax.set_aspect("equal", adjustable="box")
    ax.set_xlabel("x (m)")
    ax.set_ylabel("y (m)")
    ax.legend(fontsize=8)
    ax.grid(True, alpha=0.3)
    if title:
        ax.set_title(title)
    return fig, ax


__all__ = ["savefig", "label", "fig_dir", "DPI", "HAS_CHINESE",
           "CHINESE_FONT", "plot_state", "plot_feet"]
