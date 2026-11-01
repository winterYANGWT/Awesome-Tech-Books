"""FreeStand acceptance: pose steps, 30 N push and QP friction check."""

from __future__ import annotations

import numpy as np

from quadruped.balance import BalanceCtrl
from quadruped.fsm import ControlContext
from quadruped.robot import A1Params, Quadruped
from quadruped.sim import A1Sim
from quadruped.trotting import FreeStand
from quadruped.types import JointCmd

FEET = np.array([[0.183, -0.132, -0.25],
                 [0.183, 0.132, -0.25],
                 [-0.183, -0.132, -0.25],
                 [-0.183, 0.132, -0.25]])
A_DES = np.array([1.5, -0.8, 0.4])
ALPHA_DES = np.array([1.0, -0.5, 0.3])


def _solve_default(ctrl: BalanceCtrl) -> np.ndarray:
    return ctrl.solve_force(A_DES, ALPHA_DES, np.eye(3), FEET,
                            np.ones(4, dtype=bool))


def test_default_qp_forces_are_unchanged():
    """The default w_dynamics=1 must reproduce the old S=I6 result."""
    expected = np.array([[4.4823018583, -2.2473630835, 22.3966041590],
                         [4.3543421775, -2.2473630835, 28.9321665762],
                         [4.4823018583, -2.4247617318, 31.8718036093],
                         [4.3543421775, -2.4247617318, 38.4073660265]])
    ctrl = BalanceCtrl()
    assert np.allclose(ctrl.S, np.eye(6))
    np.testing.assert_allclose(_solve_default(ctrl), expected,
                               rtol=0.0, atol=1e-6)
    explicit = BalanceCtrl(S=np.eye(6))
    np.testing.assert_allclose(expected, _solve_default(explicit),
                               rtol=0.0, atol=1e-9)


def test_w_dynamics_scales_the_dynamics_term():
    """w_dynamics is the shorthand for S = w_dynamics * I6."""
    scaled = _solve_default(BalanceCtrl(w_dynamics=100.0))
    explicit = _solve_default(BalanceCtrl(S=100.0 * np.eye(6)))
    np.testing.assert_allclose(scaled, explicit, rtol=0.0, atol=1e-9)
    baseline = _solve_default(BalanceCtrl())
    assert not np.allclose(scaled, baseline, atol=1e-3)


def _make_standing():
    params = A1Params.from_menagerie()
    robot = Quadruped(params)
    sim = A1Sim()
    q0 = params.home_q.copy()
    for _ in range(800):
        sim.step(JointCmd(q0, np.zeros(12), np.array([100.0, 300.0, 400.0] * 4),
                          np.array([4.0, 8.0, 8.0] * 4), np.zeros(12)))
    ctx = ControlContext.from_sim(sim, params)
    state = FreeStand(ctx, robot=robot, params=params)
    state.enter()
    for _ in range(500):
        state.run()
        sim.step()
    return params, robot, sim, state


def _friction_ok(forces, mu=0.6, f_max=120.0):
    f = np.asarray(forces)
    return bool(np.all(f[:, 2] >= -1e-6)
                and np.all(np.abs(f[:, 0]) <= mu * f[:, 2] + 1e-4)
                and np.all(np.abs(f[:, 1]) <= mu * f[:, 2] + 1e-4)
                and np.all(f[:, 2] <= f_max + 1e-4))


def test_free_stand_pose_tracking_and_friction():
    params, robot, sim, state = _make_standing()
    try:
        cases = [
            ("roll+", dict(roll=0.15), 0.15, 0.0, 0.0, 0.27),
            ("roll-", dict(roll=-0.15), -0.15, 0.0, 0.0, 0.27),
            ("pitch+", dict(pitch=0.15), 0.0, 0.15, 0.0, 0.27),
            ("pitch-", dict(pitch=-0.15), 0.0, -0.15, 0.0, 0.27),
            ("yaw+", dict(yaw=0.15), 0.0, 0.0, 0.15, 0.27),
            ("yaw-", dict(yaw=-0.15), 0.0, 0.0, -0.15, 0.27),
            ("height+", dict(height=0.30), 0.0, 0.0, 0.0, 0.30),
            ("height-", dict(height=0.24), 0.0, 0.0, 0.0, 0.24),
        ]
        for name, cmd, er, ep, ey, eh in cases:
            state.set_pose(roll=0.0, pitch=0.0, yaw=0.0, height=0.27)
            state.set_pose(**cmd)
            samples = []
            for k in range(500):            # 1 s per step
                state.run()
                base = sim.step()
                if k >= 400:                # last 0.2 s
                    samples.append([*base.rpy, base.position[2]])
            mean = np.asarray(samples).mean(axis=0)
            assert abs(mean[0] - er) < 0.03, (name, mean)
            assert abs(mean[1] - ep) < 0.03, (name, mean)
            assert abs(mean[2] - ey) < 0.03, (name, mean)
            assert abs(mean[3] - eh) < 0.005, (name, mean)
            assert _friction_ok(state.forces), (name, state.forces)

        # 30 N horizontal push for 0.2 s, then recover.
        state.set_pose(roll=0.0, pitch=0.0, yaw=0.0, height=0.27)
        for _ in range(500):
            state.run()
            sim.step()
        body_id = sim.model.body("trunk").id
        push = np.array([30.0, 0.0, 0.0])
        for _ in range(100):
            sim.data.xfrc_applied[body_id, :3] = push
            state.run()
            sim.step()
        sim.data.xfrc_applied[body_id, :3] = 0.0
        rec = []
        for _ in range(1000):               # 2 s
            state.run()
            base = sim.step()
            rec.append([*base.rpy, base.position[2], *base.position[:2]])
        rec = np.asarray(rec)
        assert abs(rec[:, 2].max()) < 0.10
        # After two seconds the pose is back within the tracking tolerance.
        final = rec[-100:]
        assert np.all(np.abs(final[:, :3]) < 0.03), final[:, :3].mean(axis=0)
        assert abs(final[:, 3].mean() - 0.27) < 0.01
        assert np.all(np.abs(final[:, 4:6]) < 0.03)
        assert _friction_ok(state.forces)
    finally:
        sim.close()
