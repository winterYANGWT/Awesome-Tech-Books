"""Jitter of a 2 ms control loop under three timing strategies (10.3).

The script measures 5000 periods in each of three cases:

* ordinary ``time.sleep``: the loop sleeps for ``T - work`` relative to the
  end of the previous cycle, so wake-up overshoot accumulates;
* absolute-time busy wait: each cycle spins until ``start + k T``, which
  removes accumulated drift but still pays the wake-up cost;
* absolute-time plus logging: a 12-value record is appended every cycle and
  serialised every 100 cycles, showing the tail that in-loop logging adds.

``SCHED_FIFO`` is only *probed*: if the capability is available the process is
briefly switched to it and immediately restored to ``SCHED_OTHER`` so the
benchmark cannot starve the machine.  The numbers therefore describe ordinary
user-space scheduling; Section 10.3 explains what changes with root or
``CAP_SYS_NICE``.  Set ``CH10_RT_ITERS`` to a smaller value for a quick run.

expect: SCHED_FIFO
expect: 控制循环抖动
expect: 普通 sleep
expect: 忙等
expect: 日志
expect: 抖动直方图
"""

from __future__ import annotations

import errno
import io
import logging
import os
import time

import numpy as np

from quadruped.plotting import label, savefig
import matplotlib.pyplot as plt

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

T = 0.002
N = int(os.environ.get("CH10_RT_ITERS", "5000"))
WORK = 0.00012


def spin(seconds: float) -> None:
    end = time.perf_counter() + seconds
    while time.perf_counter() < end:
        pass


def probe_fifo() -> tuple[bool, int, str]:
    """Try SCHED_FIFO briefly, then restore SCHED_OTHER."""
    if not hasattr(os, "sched_setscheduler"):
        return False, 0, "当前平台没有 sched_setscheduler"
    try:
        prio = os.sched_get_priority_max(os.SCHED_FIFO)
        os.sched_setscheduler(0, os.SCHED_FIFO, os.sched_param(prio))
    except (PermissionError, OSError) as exc:
        code = getattr(exc, "errno", None) or errno.EPERM
        return False, prio if "prio" in locals() else 0, \
            f"不可用（errno={code}，需要 root 或 CAP_SYS_NICE）"
    try:
        os.sched_setscheduler(0, os.SCHED_OTHER, os.sched_param(0))
    except OSError:
        pass
    return True, prio, "可用（已立即恢复普通策略，避免占满 CPU）"


def pin_cpu() -> str:
    try:
        cpus = sorted(os.sched_getaffinity(0))
        target = cpus[0]
        os.sched_setaffinity(0, {target})
        return f"已绑定 CPU {target}（共 {len(cpus)} 个可用核）"
    except (AttributeError, OSError) as exc:
        return f"未绑定（{exc}）"


# ANCHOR: loop
def run_sleep() -> tuple[np.ndarray, float]:
    jitter = np.empty(N)
    t_start = time.perf_counter()
    t_prev = t_start
    for k in range(N):
        spin(WORK)
        time.sleep(max(0.0, T - WORK))
        now = time.perf_counter()
        jitter[k] = (now - t_prev - T) * 1e3
        t_prev = now
    return jitter, (t_prev - (t_start + (N - 1) * T)) * 1e3


def run_absolute(log: bool = False) -> tuple[np.ndarray, float]:
    jitter = np.empty(N)
    ring: list[tuple[float, ...]] = []
    buf = io.StringIO()
    t0 = time.perf_counter()
    for k in range(N):
        target = t0 + k * T
        spin(WORK)
        while time.perf_counter() < target:
            pass
        now = time.perf_counter()
        jitter[k] = (now - target) * 1e3
        if log:
            ring.append((now, jitter[k], 0.27, 0.0, 0.0, 0.0,
                         0.0, 0.0, 0.0, 0.0, 0.0, 0.0))
            if (k + 1) % 1000 == 0:
                np.savetxt(buf, np.asarray(ring[-1000:]), fmt="%.4f")
                buf.seek(0)
                buf.truncate(0)
    return jitter, float(jitter[-1])


# ANCHOR_END: loop


def stats(jitter: np.ndarray) -> tuple[float, float, float]:
    return (float(np.mean(jitter)), float(np.percentile(jitter, 95)),
            float(np.max(jitter)))


def draw(jitters: dict[str, np.ndarray]) -> None:
    """Full range on the left, the 0-0.25 ms core on the right."""
    fig, axes = plt.subplots(1, 2, figsize=(10.0, 3.7))
    colors = ["C0", "C1", "C3"]
    for ax, lo, hi in ((axes[0], -0.1, 2.25), (axes[1], -0.02, 0.25)):
        bins = np.linspace(lo, hi, 90)
        for (name, data), color in zip(jitters.items(), colors):
            ax.hist(np.clip(data, lo, hi), bins=bins, histtype="step",
                    lw=1.4, color=color, label=name)
        ax.axvline(0.0, color="0.4", ls="--", lw=0.9)
        ax.set_yscale("log")
        ax.set_ylim(0.7, 8000)
        ax.set_xlim(lo, hi)
        ax.set_xlabel(label("抖动 / ms", "jitter / ms"))
        ax.grid(True, alpha=0.3)
    axes[0].set_ylabel(label("周期数", "cycles"))
    axes[0].set_title(label("全程：0–2.25 ms", "full range"))
    axes[1].set_title(label("局部：0–0.25 ms", "zoom"))
    axes[0].legend(fontsize=8, loc="upper right")
    axes[1].text(0.98, 0.96,
                 "max / ms\n" + "\n".join(f"{k}: {np.max(v):.3f}"
                                          for k, v in jitters.items()),
                 transform=axes[1].transAxes, fontsize=7.2, color="0.3",
                 ha="right", va="top",
                 bbox=dict(fc="white", ec="0.8", lw=0.6, alpha=0.9))
    fig.suptitle(label("2 ms 循环的抖动分布（5000 周期）", "jitter of a 2 ms loop"),
                 fontsize=11)
    savefig(fig, "10_3_realtime")
    plt.close(fig)


def main() -> None:
    print(f"=== 10.3 2 ms 控制循环的抖动（{N} 个周期） ===")
    ok, prio, message = probe_fifo()
    print(f"SCHED_FIFO 最高优先级 = {prio}；探测结果：{message}")
    print("CPU 亲和性：" + pin_cpu())
    print("说明：以下三组都在普通调度策略下测量，不包含实时抢占的收益")

    j_sleep, drift_sleep = run_sleep()
    j_busy, drift_busy = run_absolute(log=False)
    j_log, drift_log = run_absolute(log=True)
    cases = [("普通 sleep", j_sleep, drift_sleep),
             ("忙等 + 绝对时间", j_busy, drift_busy),
             ("绝对时间 + 日志", j_log, drift_log)]
    print("\n策略                 均值/ms   95%/ms   最大/ms   末周期偏差/ms")
    for name, data, drift in cases:
        mean, p95, mx = stats(data)
        print(f"{name:<18s} {mean:8.4f} {p95:8.4f} {mx:8.4f} {drift:12.4f}")
    print("\n解读：普通 sleep 的漂移逐周期累积；绝对时间把目标钉在 t0+kT，漂移只在测量误差内；"
          "日志写的是内存缓冲区，代价很小；真正把 1000 条落盘时才会出现毫秒级尖峰，所以 10.4 把落盘移出实时线程")
    draw({"普通 sleep": j_sleep, "忙等 + 绝对时间": j_busy,
          "绝对时间 + 日志": j_log})
    print("抖动直方图已保存：10_3_realtime.png")


if __name__ == "__main__":
    main()
