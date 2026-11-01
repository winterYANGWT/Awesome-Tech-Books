"""Standing acceptance test: crouch -> FixedStand -> FixedDown."""

from __future__ import annotations

import os
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np

from quadruped.fsm import ControlContext, FSM, StateID
from quadruped.plotting import label, savefig
from quadruped.robot import A1Params
from quadruped.sim import A1Sim
from quadruped.types import JointCmd

ROOT = Path(__file__).resolve().parents[3]


def test_fixed_stand_and_down():
    sim = A1Sim()
    params = A1Params.from_menagerie()

    # Start from a folded pose close to the ground, then settle it briefly.
    q_down = np.array([0.0, 1.2, -2.4] * 4)
    sim.reset(q0=q_down, base_pos=(0.0, 0.0, 0.16))
    settle = JointCmd(q_down.copy(), np.zeros(12),
                      np.array([100.0, 200.0, 250.0] * 4),
                      np.array([2.0, 4.0, 4.0] * 4), np.zeros(12))
    for _ in range(150):
        sim.step(settle)

    context = ControlContext.from_sim(sim, params)
    fsm = FSM(context, initial=StateID.FIXED_STAND)
    assert fsm.state is StateID.FIXED_STAND

    time, height, rpy = [], [], []
    for k in range(2500):                     # 5 s at 2 ms
        fsm.step()
        base = sim.step()
        time.append(sim.data.time)
        height.append(base.position[2])
        rpy.append(base.rpy)

    t = np.asarray(time)
    h = np.asarray(height)
    r = np.asarray(rpy)
    mask = (t >= 2.0) & (t <= 5.0)
    assert mask.sum() > 1000
    mean_2_5 = float(h[mask].mean())
    spread = float(np.ptp(h[mask]))
    max_tilt = float(np.max(np.abs(r[mask])))

    # "Standing height" is the MJCF home height 0.27 m; the compliant
    # contact and PD tracking leave a sub-2 mm offset.
    assert abs(mean_2_5 - params.stand_height) < 0.005, mean_2_5
    assert spread < 0.005, spread
    assert max_tilt < 0.02, max_tilt
    contacts = sim.foot_contacts()
    forces = sim.foot_forces()
    assert np.all(contacts)
    assert abs(float(forces[:, 2].sum()) - params.mass * 9.81) < 2.0

    # Save the height curve required by the acceptance criterion.
    os.environ["FIG_DIR"] = str(ROOT / "build" / "scratch" / "pkg")
    fig, ax = plt.subplots(figsize=(6.4, 3.2))
    ax.plot(t, h * 1000.0, lw=1.4, label=label("机身高度", "base height"))
    ax.axhline(params.stand_height * 1000.0, color="k", ls="--", lw=1.0,
               label=label("目标 0.27 m", "target 0.27 m"))
    ax.axvspan(2.0, 5.0, color="tab:green", alpha=0.08)
    ax.set_xlabel(label("时间 (s)", "time (s)"))
    ax.set_ylabel(label("机身高度 (mm)", "base height (mm)"))
    ax.set_title(label("固定站立：趴下到站起并保持 3 s",
                       "FixedStand: stand up and hold for 3 s"))
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=8)
    path = savefig(fig, "stand")
    plt.close(fig)
    assert path.exists()

    # Transition to FixedDown and check that the body actually lowers.
    fsm.request(StateID.FIXED_DOWN)
    for _ in range(750):                      # 1.5 s
        fsm.step()
        sim.step()
    assert fsm.state is StateID.FIXED_DOWN
    final_height = float(sim.get_state()[0].position[2])
    assert final_height < 0.18, final_height
    assert final_height < mean_2_5 - 0.05, final_height

    print(f"stand height={mean_2_5:.6f} m, spread={spread * 1000:.3f} mm, "
          f"tilt={max_tilt:.5f} rad, down={final_height:.4f} m")
    sim.close()
