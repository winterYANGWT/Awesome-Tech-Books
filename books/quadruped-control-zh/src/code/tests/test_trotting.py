"""Trotting acceptance: velocity commands, height, estimator and stand return."""

from __future__ import annotations

import numpy as np

from quadruped.fsm import ControlContext, FSM, FixedStand, StateID
from quadruped.robot import A1Params, Quadruped
from quadruped.sim import A1Sim
from quadruped.trotting import FreeStand, TrottingController
from quadruped.types import JointCmd


def _boot():
    params = A1Params.from_menagerie()
    robot = Quadruped(params)
    sim = A1Sim()
    q0 = params.home_q.copy()
    for _ in range(700):
        sim.step(JointCmd(q0, np.zeros(12), np.array([100.0, 300.0, 400.0] * 4),
                          np.array([4.0, 8.0, 8.0] * 4), np.zeros(12)))
    ctx = ControlContext.from_sim(sim, params)
    states = [FixedStand(ctx), FreeStand(ctx, robot=robot, params=params),
              TrottingController(ctx, robot=robot, params=params)]
    fsm = FSM(ctx, states=states, initial=StateID.FIXED_STAND)
    return params, robot, sim, fsm


def _record(fsm, sim, n, trot=None):
    vel, height, rpy, est_v, est_h, forces, contact = [], [], [], [], [], [], []
    for _ in range(n):
        fsm.step()
        base = sim.step()
        vel.append(base.velocity.copy())
        height.append(base.position[2])
        rpy.append(base.rpy.copy())
        if trot is not None:
            est_v.append(trot.est_base.velocity.copy())
            est_h.append(trot.est_base.position[2])
            forces.append(trot.forces.copy())
            contact.append(trot.gait.contact.copy())
    out = dict(vel=np.array(vel), height=np.array(height), rpy=np.array(rpy),
               est_v=np.array(est_v), est_h=np.array(est_h),
               forces=np.array(forces), contact=np.array(contact))
    return out


def test_trot_forward_and_estimator():
    params, robot, sim, fsm = _boot()
    try:
        for _ in range(700):
            fsm.step()
            sim.step()
        fsm.request(StateID.TROTTING)
        fsm.step()
        sim.step()
        trot = fsm.states[StateID.TROTTING]
        trot.set_command(vx=0.3)
        rec = _record(fsm, sim, 5000, trot)

        assert rec["height"].min() > 0.18
        mean_vx = float(rec["vel"][-1500:, 0].mean())
        assert 0.24 < mean_vx < 0.36, mean_vx
        assert abs(rec["vel"][-1500:, 1].mean()) < 0.08
        assert abs(rec["vel"][-1500:, 2].mean()) < 0.08

        dv = rec["est_v"] - rec["vel"]
        v_rmse = float(np.sqrt(np.mean(dv ** 2)))
        h_err = np.abs(rec["est_h"] - rec["height"])
        assert v_rmse < 0.1, v_rmse
        assert float(h_err.mean()) < 0.02, h_err.mean()
        assert float(h_err.max()) < 0.05

        # Friction pyramid for the QP solution on every stance foot.
        f = rec["forces"]
        c = rec["contact"]
        for k in range(len(f)):
            for i in range(4):
                if c[k, i]:
                    assert f[k, i, 2] >= -1e-6
                    assert abs(f[k, i, 0]) <= 0.6 * f[k, i, 2] + 1e-4
                    assert abs(f[k, i, 1]) <= 0.6 * f[k, i, 2] + 1e-4
                else:
                    assert np.allclose(f[k, i], 0.0, atol=1e-6)
        print(f"trot vx={mean_vx:.4f} m/s, v_rmse={v_rmse:.4f} m/s, "
              f"h_err={h_err.mean() * 1000:.2f} mm")
    finally:
        sim.close()


def test_other_commands_and_stand_return():
    params, robot, sim, fsm = _boot()
    try:
        for _ in range(700):
            fsm.step()
            sim.step()
        fsm.request(StateID.TROTTING)
        fsm.step()
        sim.step()
        trot = fsm.states[StateID.TROTTING]

        # Backwards.
        trot.set_command(vx=-0.2)
        rec = _record(fsm, sim, 2000, trot)
        assert rec["height"].min() > 0.18
        assert abs(float(rec["vel"][-1000:, 0].mean()) + 0.2) < 0.08

        # Lateral.
        trot.set_command(vx=0.0, vy=0.15)
        rec = _record(fsm, sim, 2000, trot)
        assert rec["height"].min() > 0.18
        assert abs(float(rec["vel"][-1000:, 1].mean()) - 0.15) < 0.07

        # Yaw rate.
        trot.set_command(vx=0.0, vy=0.0, vyaw=0.6)
        rec = _record(fsm, sim, 2000, trot)
        assert rec["height"].min() > 0.18
        # Use the slope of the unwrapped yaw as the actual world yaw rate.
        yaw = np.unwrap(rec["rpy"][:, 2])
        wz = np.gradient(yaw) / 0.002
        assert abs(float(wz[-800:].mean()) - 0.6) < 0.18, wz[-800:].mean()

        # Zero command: trot in place, then switch back to FixedStand.
        trot.set_command(0.0, 0.0, 0.0)
        rec = _record(fsm, sim, 1000, trot)
        assert rec["height"].min() > 0.18
        assert abs(float(rec["vel"][-500:, 0].mean())) < 0.06
        fsm.request(StateID.FIXED_STAND)
        rec = _record(fsm, sim, 1000)
        assert fsm.state is StateID.FIXED_STAND
        assert rec["height"][-1] > 0.25
        assert abs(rec["height"][-1] - params.stand_height) < 0.01
    finally:
        sim.close()
