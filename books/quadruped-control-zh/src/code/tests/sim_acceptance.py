"""End-to-end acceptance script for the quadruped control package.

Run from the repository root:

    MUJOCO_GL=egl PYTHONPATH=src/code build/venv/bin/python \
        src/code/tests/sim_acceptance.py

The script exercises FreeStand (pose steps, 30 N push), the estimator and the
trotting controller (vx, -vx, vy, yaw rate, stop and stand).  It prints the
acceptance numbers and saves the figures to ``build/scratch/pkg-control/``.
The control loops only compute; plotting and file output happen afterwards.
"""

from __future__ import annotations

import os
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src" / "code"))
os.environ.setdefault("MPLCONFIGDIR",
                      str(Path(tempfile.gettempdir()) / "quadruped-mpl"))

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

from quadruped.fsm import ControlContext, FSM, FixedStand, StateID
from quadruped.plotting import label, savefig
from quadruped.robot import A1Params, Quadruped
from quadruped.sim import A1Sim
from quadruped.trotting import FreeStand, TrottingController
from quadruped.types import JointCmd

OUT = ROOT / "build" / "scratch" / "pkg-control"
os.environ["FIG_DIR"] = str(OUT)
OUT.mkdir(parents=True, exist_ok=True)

MU = 0.6
F_MAX = 120.0


def boot():
    params = A1Params.from_menagerie()
    robot = Quadruped(params)
    sim = A1Sim()
    q0 = params.home_q.copy()
    for _ in range(700):
        sim.step(JointCmd(q0, np.zeros(12),
                          np.array([100.0, 300.0, 400.0] * 4),
                          np.array([4.0, 8.0, 8.0] * 4), np.zeros(12)))
    ctx = ControlContext.from_sim(sim, params)
    states = [FixedStand(ctx), FreeStand(ctx, robot=robot, params=params),
              TrottingController(ctx, robot=robot, params=params)]
    fsm = FSM(ctx, states=states, initial=StateID.FIXED_STAND)
    for _ in range(700):
        fsm.step()
        sim.step()
    return params, robot, sim, fsm


def friction_ok(forces, contact):
    ok = True
    f = np.asarray(forces)
    for i in range(4):
        if contact[i]:
            ok &= f[i, 2] >= -1e-6
            ok &= abs(f[i, 0]) <= MU * f[i, 2] + 1e-4
            ok &= abs(f[i, 1]) <= MU * f[i, 2] + 1e-4
            ok &= f[i, 2] <= F_MAX + 1e-4
        else:
            ok &= np.allclose(f[i], 0.0, atol=1e-6)
    return bool(ok)


# ---------------------------------------------------------------------------
# FreeStand
# ---------------------------------------------------------------------------
def run_free_stand():
    params, robot, sim, fsm = boot()
    state = fsm.states[StateID.FREE_STAND]
    fsm.request(StateID.FREE_STAND)
    fsm.step()
    sim.step()
    for _ in range(500):
        fsm.step()
        sim.step()

    names, cmds = [], []
    cases = [
        ("roll +", dict(roll=0.15), 0.15, 0.0, 0.0, 0.27),
        ("roll -", dict(roll=-0.15), -0.15, 0.0, 0.0, 0.27),
        ("pitch +", dict(pitch=0.15), 0.0, 0.15, 0.0, 0.27),
        ("pitch -", dict(pitch=-0.15), 0.0, -0.15, 0.0, 0.27),
        ("yaw +", dict(yaw=0.15), 0.0, 0.0, 0.15, 0.27),
        ("yaw -", dict(yaw=-0.15), 0.0, 0.0, -0.15, 0.27),
        ("height +3 cm", dict(height=0.30), 0.0, 0.0, 0.0, 0.30),
        ("height -3 cm", dict(height=0.24), 0.0, 0.0, 0.0, 0.24),
    ]
    series = {"t": [], "h": [], "rpy": []}
    rows = []
    t = 0.0
    for name, cmd, er, ep, ey, eh in cases:
        state.set_pose(roll=0.0, pitch=0.0, yaw=0.0, height=0.27)
        state.set_pose(**cmd)
        samples = []
        for k in range(500):
            state.run()
            base = sim.step()
            t += 0.002
            series["t"].append(t)
            series["h"].append(base.position[2])
            series["rpy"].append(base.rpy.copy())
            if k >= 400:
                samples.append([*base.rpy, base.position[2]])
        m = np.asarray(samples).mean(axis=0)
        err = [abs(m[0] - er), abs(m[1] - ep), abs(m[2] - ey), abs(m[3] - eh)]
        rows.append((name, *err))
        assert max(err[:3]) < 0.03, (name, err)
        assert err[3] < 0.005, (name, err)
        assert friction_ok(state.forces, np.ones(4, dtype=bool)), name

    # 30 N push for 0.2 s.
    state.set_pose(roll=0.0, pitch=0.0, yaw=0.0, height=0.27)
    for _ in range(500):
        state.run()
        sim.step()
    p_before = sim.get_state()[0].position.copy()
    body_id = sim.model.body("trunk").id
    push = np.array([30.0, 0.0, 0.0])
    push_peak = 0.0
    t_push = 0.0
    push_rec = []

    def _record_push(base):
        push_rec.append([t_push, base.rpy[0], base.rpy[1], base.position[2],
                         base.position[0] - p_before[0],
                         base.position[1] - p_before[1],
                         *state.forces[:, 2]])

    for _ in range(100):                   # 0.2 s push
        sim.data.xfrc_applied[body_id, :3] = push
        state.run()
        base = sim.step()
        t_push += 0.002
        push_peak = max(push_peak, float(state.forces[:, 2].max()))
        _record_push(base)
    sim.data.xfrc_applied[body_id, :3] = 0.0
    for _ in range(1500):                  # 3 s of recovery
        state.run()
        base = sim.step()
        t_push += 0.002
        _record_push(base)
    push_rec = np.asarray(push_rec)
    at_2s = push_rec[(push_rec[:, 0] >= 1.8) & (push_rec[:, 0] <= 2.0)]
    tilt = float(np.abs(push_rec[:, 1:3]).max())
    rec_rpy = float(np.abs(at_2s[:, 1:3]).max())
    rec_h = float(abs(at_2s[:, 3].mean() - 0.27))
    rec_xy = float(np.abs(at_2s[:, 4:6]).max())
    assert tilt < 0.12, tilt
    assert rec_rpy < 0.03, rec_rpy
    assert rec_h < 0.01, rec_h
    assert rec_xy < 0.03, rec_xy
    assert friction_ok(state.forces, np.ones(4, dtype=bool))
    sim.close()

    # Figures
    series["t"] = np.asarray(series["t"])
    series["h"] = np.asarray(series["h"])
    rpy = np.asarray(series["rpy"])
    fig, axes = plt.subplots(2, 1, figsize=(7.2, 5.0), sharex=True)
    axes[0].plot(series["t"], series["h"] * 1000, lw=1.2)
    axes[0].set_ylabel(label("机身高度 (mm)", "base height (mm)"))
    axes[0].set_title(label("FreeStand 位姿阶跃：横滚/俯仰/偏航与高度",
                            "FreeStand pose steps"))
    axes[0].grid(True, alpha=0.3)
    for i, (c, name) in enumerate(zip("rgb", ("roll", "pitch", "yaw"))):
        axes[1].plot(series["t"], rpy[:, i], lw=1.2, color=c, label=name)
    axes[1].set_xlabel(label("时间 (s)", "time (s)"))
    axes[1].set_ylabel(label("姿态角 (rad)", "attitude (rad)"))
    axes[1].grid(True, alpha=0.3)
    axes[1].legend(fontsize=8)
    savefig(fig, "free_pose")
    plt.close(fig)

    fig, axes = plt.subplots(2, 1, figsize=(7.2, 4.8), sharex=True)
    axes[0].plot(push_rec[:, 0], push_rec[:, 1], label="roll")
    axes[0].plot(push_rec[:, 0], push_rec[:, 2], label="pitch")
    axes[0].axvline(0.2, color="k", ls="--", lw=0.8)
    axes[0].set_ylabel(label("姿态角 (rad)", "attitude (rad)"))
    axes[0].legend(fontsize=8)
    axes[0].grid(True, alpha=0.3)
    for i in range(4):
        axes[1].plot(push_rec[:, 0], push_rec[:, 6 + i], lw=1.0,
                     label=("FR", "FL", "RR", "RL")[i])
    axes[1].set_xlabel(label("时间 (s)", "time (s)"))
    axes[1].set_ylabel(label("足端法向力 (N)", "normal force (N)"))
    axes[1].legend(fontsize=8, ncol=4)
    axes[1].grid(True, alpha=0.3)
    savefig(fig, "free_push")
    plt.close(fig)

    return rows, tilt, rec_rpy, rec_h, rec_xy, push_peak


# ---------------------------------------------------------------------------
# Trotting and estimator
# ---------------------------------------------------------------------------
def run_trot():
    params, robot, sim, fsm = boot()
    fsm.request(StateID.TROTTING)
    fsm.step()
    sim.step()
    trot = fsm.states[StateID.TROTTING]
    trot.set_command(vx=0.3)
    T = 5000
    log = {k: [] for k in ("t", "h", "rpy", "v", "ev", "eh", "f", "c",
                           "ph", "wz")}
    for k in range(T):
        fsm.step()
        base = sim.step()
        log["t"].append((k + 1) * 0.002)
        log["h"].append(base.position[2])
        log["rpy"].append(base.rpy.copy())
        log["v"].append(base.velocity.copy())
        log["ev"].append(trot.est_base.velocity.copy())
        log["eh"].append(trot.est_base.position[2])
        log["f"].append(trot.forces.copy())
        log["c"].append(trot.gait.contact.copy())
        log["ph"].append(trot.gait.phase.copy())
        log["wz"].append(float((base.rotation() @ base.omega_b)[2]))
    for k in log:
        log[k] = np.asarray(log[k])

    mean_vx = float(log["v"][-1500:, 0].mean())
    min_h = float(log["h"].min())
    dv = log["ev"] - log["v"]
    v_rmse = float(np.sqrt(np.mean(dv ** 2)))
    v_rmse_axes = np.sqrt(np.mean(dv ** 2, axis=0))
    h_err = np.abs(log["eh"] - log["h"])
    h_rmse = float(np.sqrt(np.mean((log["eh"] - log["h"]) ** 2)))
    h_max = float(h_err.max())
    assert min_h > 0.18, min_h
    assert 0.24 < mean_vx < 0.36, mean_vx
    assert v_rmse < 0.1, v_rmse
    assert h_rmse < 0.02, h_rmse

    # Friction cone on the QP solution.
    violations = 0
    for k in range(T):
        if not friction_ok(log["f"][k], log["c"][k]):
            violations += 1
    assert violations == 0, violations

    # Command sweep using the same controller.
    segs = []
    for name, cmd in (("vx=-0.2", (-0.2, 0.0, 0.0)),
                      ("vy=+0.15", (0.0, 0.15, 0.0)),
                      ("yaw=+0.6", (0.0, 0.0, 0.6))):
        trot.set_command(*cmd)
        n = 2000
        rec = {k: [] for k in ("v", "h", "rpy", "wz")}
        for k in range(n):
            fsm.step()
            base = sim.step()
            rec["v"].append(base.velocity.copy())
            rec["h"].append(base.position[2])
            rec["rpy"].append(base.rpy.copy())
            rec["wz"].append(float((base.rotation() @ base.omega_b)[2]))
        rec = {k: np.asarray(v) for k, v in rec.items()}
        if name.startswith("vx"):
            m = float(rec["v"][-1000:, 0].mean())
        elif name.startswith("vy"):
            m = float(rec["v"][-1000:, 1].mean())
        else:
            m = float(rec["wz"][-1000:].mean())
        assert rec["h"].min() > 0.18, (name, rec["h"].min())
        segs.append((name, m, rec))

    # Zero command, then stand.
    trot.set_command(0.0, 0.0, 0.0)
    zero = {k: [] for k in ("v", "h", "rpy")}
    for k in range(1000):
        fsm.step()
        base = sim.step()
        zero["v"].append(base.velocity.copy())
        zero["h"].append(base.position[2])
        zero["rpy"].append(base.rpy.copy())
    zero = {k: np.asarray(v) for k, v in zero.items()}
    assert zero["h"].min() > 0.18
    fsm.request(StateID.FIXED_STAND)
    stand = []
    for k in range(1000):
        fsm.step()
        base = sim.step()
        stand.append(base.position[2])
    stand = np.asarray(stand)
    assert fsm.state is StateID.FIXED_STAND
    assert abs(stand[-1] - params.stand_height) < 0.01
    sim.close()

    # Figures
    t = log["t"]
    fig, axes = plt.subplots(2, 1, figsize=(7.2, 5.0), sharex=True)
    axes[0].plot(t, log["h"] * 1000, lw=1.2)
    axes[0].axhline(180, color="r", ls="--", lw=0.8)
    axes[0].set_ylabel(label("高度 (mm)", "height (mm)"))
    axes[0].set_title(label("Trot vx=0.3 m/s：高度与姿态",
                            "Trot vx=0.3 m/s: height and attitude"))
    axes[0].grid(True, alpha=0.3)
    for i, (c, name) in enumerate(zip("rgb", ("roll", "pitch", "yaw"))):
        axes[1].plot(t, log["rpy"][:, i], lw=1.0, color=c, label=name)
    axes[1].set_xlabel(label("时间 (s)", "time (s)"))
    axes[1].set_ylabel(label("姿态角 (rad)", "attitude (rad)"))
    axes[1].grid(True, alpha=0.3)
    axes[1].legend(fontsize=8)
    savefig(fig, "trot_height_attitude")
    plt.close(fig)

    fig, axes = plt.subplots(3, 1, figsize=(7.2, 6.2), sharex=True)
    axes[0].plot(t, log["v"][:, 0], label="true vx")
    axes[0].plot(t, log["ev"][:, 0], "--", label="estimated vx")
    axes[0].set_ylabel(label("vx (m/s)", "vx (m/s)"))
    axes[0].legend(fontsize=8)
    axes[0].grid(True, alpha=0.3)
    axes[1].plot(t, log["v"][:, 1], label="true vy")
    axes[1].plot(t, log["ev"][:, 1], "--", label="estimated vy")
    axes[1].set_ylabel(label("vy (m/s)", "vy (m/s)"))
    axes[1].legend(fontsize=8)
    axes[1].grid(True, alpha=0.3)
    axes[2].plot(t, (log["eh"] - log["h"]) * 1000, lw=1.0)
    axes[2].set_ylabel(label("高度估计误差 (mm)", "height error (mm)"))
    axes[2].set_xlabel(label("时间 (s)", "time (s)"))
    axes[2].grid(True, alpha=0.3)
    savefig(fig, "estimator")
    plt.close(fig)

    # One second of foot forces, contact and phase.
    sl = slice(4000, 4500)
    tt = t[sl]
    fig, axes = plt.subplots(3, 1, figsize=(7.2, 6.0), sharex=True)
    for i in range(4):
        axes[0].plot(tt, log["f"][sl, i, 2], lw=1.0,
                     label=("FR", "FL", "RR", "RL")[i])
    axes[0].set_ylabel(label("法向力 (N)", "normal force (N)"))
    axes[0].legend(fontsize=8, ncol=4)
    axes[0].grid(True, alpha=0.3)
    for i in range(4):
        axes[1].plot(tt, log["ph"][sl, i] + i * 1.05, lw=0.9,
                     label=("FR", "FL", "RR", "RL")[i])
    axes[1].set_ylabel(label("相位", "phase"))
    axes[1].grid(True, alpha=0.3)
    for i in range(4):
        axes[2].fill_between(tt, i, i + 0.8 * log["c"][sl, i],
                             step="mid", alpha=0.6)
    axes[2].set_yticks([0.4, 1.4, 2.4, 3.4])
    axes[2].set_yticklabels(("FR", "FL", "RR", "RL"))
    axes[2].set_ylabel(label("触地", "contact"))
    axes[2].set_xlabel(label("时间 (s)", "time (s)"))
    axes[2].grid(True, alpha=0.3)
    savefig(fig, "trot_phase_forces")
    plt.close(fig)

    # Command tracking over the sweep.
    fig, axes = plt.subplots(3, 1, figsize=(7.2, 5.8))
    for j, (name, m, rec) in enumerate(segs):
        n = len(rec["v"])
        tt = np.arange(n) * 0.002
        if j == 0:
            axes[0].plot(tt, rec["v"][:, 0], lw=1.1, label=name)
            axes[0].axhline(-0.2, color="k", ls="--", lw=0.7)
        elif j == 1:
            axes[1].plot(tt, rec["v"][:, 1], lw=1.1, label=name)
            axes[1].axhline(0.15, color="k", ls="--", lw=0.7)
        else:
            axes[2].plot(tt, rec["wz"], lw=1.1, label=name)
            axes[2].axhline(0.6, color="k", ls="--", lw=0.7)
    for ax, yl in zip(axes, ("vx (m/s)", "vy (m/s)", "yaw rate (rad/s)")):
        ax.set_ylabel(yl)
        ax.grid(True, alpha=0.3)
        ax.legend(fontsize=8)
    axes[2].set_xlabel(label("时间 (s)", "time (s)"))
    savefig(fig, "trot_commands")
    plt.close(fig)

    return (min_h, mean_vx, v_rmse, v_rmse_axes, h_rmse, h_max, violations,
            segs, zero, stand[-1])


def main():
    rows, tilt, rec_rpy, rec_h, rec_xy, push_peak = run_free_stand()
    print("FreeStand pose steps (error after 1 s):")
    for name, er, ep, ey, eh in rows:
        print(f"  {name:14s} roll {er:.4f} pitch {ep:.4f} "
              f"yaw {ey:.4f} height {eh * 1000:.2f} mm")
    print(f"FreeStand push: max tilt {tilt:.4f} rad, "
          f"rpy@2s {rec_rpy:.4f} rad, height@2s {rec_h * 1000:.2f} mm, "
          f"xy@2s {rec_xy * 1000:.2f} mm, "
          f"peak normal force during push {push_peak:.1f} N")

    (min_h, mean_vx, v_rmse, v_rmse_axes, h_rmse, h_max, violations,
     segs, zero, final_h) = run_trot()
    print(f"Trot vx=0.3: min height {min_h:.4f} m, mean vx {mean_vx:.4f} m/s")
    print(f"Estimator: velocity RMSE {v_rmse:.4f} m/s "
          f"(axes {v_rmse_axes.round(4)}), height RMSE {h_rmse * 1000:.2f} mm, "
          f"max {h_max * 1000:.2f} mm")
    print(f"QP friction violations: {violations}")
    for name, m, _ in segs:
        print(f"  {name:9s} measured {m:.4f}")
    print(f"Zero command: min height {zero['h'].min():.4f} m, "
          f"mean vx {zero['v'][-500:, 0].mean():.4f} m/s")
    print(f"Back to FixedStand: height {final_h:.4f} m")
    print(f"figures: {OUT}")


if __name__ == "__main__":
    main()
