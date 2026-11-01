"""Gait timing, foothold and swing-trajectory unit tests (no simulation)."""

from __future__ import annotations

import numpy as np

from quadruped.gait import (FeetEndCal, GaitGenerator, GaitParams,
                            WaveGenerator, WaveStatus)
from quadruped.robot import A1Params, Quadruped
from quadruped.swing import SwingTrajectory


def test_trot_contact_pattern():
    g = GaitParams.trot(period=0.4, duty=0.5)
    w = WaveGenerator(g.period, g.duty, g.offsets)
    w.set_status(WaveStatus.WAVE_ALL)
    contacts = []
    for k in range(400):
        _, c = w.step(k * 0.002)
        contacts.append(c.copy())
    contacts = np.array(contacts)
    # Two diagonal legs at any time and both diagonals used.
    assert np.all(contacts.sum(axis=1) == 2)
    assert np.any(contacts[:, 0] & contacts[:, 3])
    assert np.any(contacts[:, 1] & contacts[:, 2])
    # Half a period later the pattern is swapped.
    i = 50
    j = 50 + 100
    assert np.array_equal(contacts[i], ~contacts[j])


def test_other_gait_presets():
    for name, n_contact in (("pace", 2), ("bound", 2), ("pronk", 4)):
        g = GaitParams.from_name(name)
        w = WaveGenerator(g.period, g.duty, g.offsets)
        w.set_status(WaveStatus.WAVE_ALL)
        c = np.array([w.step(k * 0.01)[1] for k in range(100)])
        assert c.sum(axis=1).max() >= n_contact
    # The walk has a four-beat section and a duty factor above 0.5.
    walk = GaitParams.walk()
    assert walk.duty > 0.5
    assert len(np.unique(walk.offsets)) == 4


def test_stance_and_swing_overrides():
    g = GaitParams.trot()
    w = WaveGenerator(g.period, g.duty, g.offsets)
    w.set_status(WaveStatus.STANCE_ALL)
    _, c = w.step(0.0)
    assert c.all()
    w.set_status(WaveStatus.SWING_ALL)
    _, c = w.step(0.1)
    assert not c.any()
    assert w.stance_time == g.period * g.duty
    assert w.swing_time == g.period * (1.0 - g.duty)


def test_foothold_raibert_terms():
    params = A1Params.from_menagerie()
    robot = Quadruped(params)
    g = GaitParams.trot(period=0.4, duty=0.5)
    calc = FeetEndCal(robot, params, kx=0.1, ky=0.1, kyaw=0.1)
    base = np.array([0.0, 0.0, 0.27])
    # At p=1, zero velocity error: the foot lands half a stance ahead.
    vx = 0.4
    target = calc.calc(0, [vx, 0.0], 0.0, base, 0.0, [vx, 0.0], 0.0,
                       1.0, g.stance_time, g.swing_time)
    neutral = calc.neutral_world(base, 0.0)[0]
    assert abs((target[0] - neutral[0]) - 0.5 * vx * g.stance_time) < 1e-9
    # Speed error shifts the foothold by kx * dv at the same current speed.
    ref = calc.calc(0, [0.5, 0.0], 0.0, base, 0.0, [0.5, 0.0], 0.0,
                    1.0, g.stance_time, g.swing_time)
    fast = calc.calc(0, [0.0, 0.0], 0.0, base, 0.0, [0.5, 0.0], 0.0,
                     1.0, g.stance_time, g.swing_time)
    assert abs((fast[0] - ref[0]) - 0.1 * 0.5) < 1e-9


def test_cycloid_endpoints_and_velocity():
    tr = SwingTrajectory(np.array([0.0, 0.0, 0.02]),
                         np.array([0.12, 0.0, 0.02]), 0.06, 0.2)
    assert np.allclose(tr.position(0.0), [0.0, 0.0, 0.02])
    assert np.allclose(tr.position(1.0), [0.12, 0.0, 0.02])
    assert np.allclose(tr.velocity(0.0), 0.0, atol=1e-12)
    assert np.allclose(tr.velocity(1.0), 0.0, atol=1e-12)
    assert abs(tr.position(0.5)[2] - 0.08) < 1e-12
    # Bezier variant also starts and ends at the requested points.
    tr2 = SwingTrajectory(np.array([0.0, 0.0, 0.02]),
                          np.array([0.12, 0.0, 0.02]), 0.06, 0.2, "bezier")
    assert np.allclose(tr2.position(0.0), [0.0, 0.0, 0.02])
    assert np.allclose(tr2.position(1.0), [0.12, 0.0, 0.02])


def test_gait_generator_swing_and_stance():
    params = A1Params.from_menagerie()
    robot = Quadruped(params)
    g = GaitParams.trot(period=0.4, duty=0.5)
    gen = GaitGenerator(robot, params, g)
    feet = robot.fk_all(params.home_q) + np.array([0.0, 0.0, 0.27])
    gen.set_status(WaveStatus.WAVE_ALL, 0.0)
    gen.set_gait([0.3, 0.0], 0.0, 0.06)
    positions = []
    for k in range(201):
        pos, vel = gen.update(k * 0.002, np.zeros(3), 0.0, [0.3, 0.0, 0.0],
                              0.0, feet_current=feet)
        positions.append(pos.copy())
    positions = np.array(positions)
    assert positions.shape == (201, 4, 3)
    assert np.isfinite(positions).all()
    # Swing feet lift by about step_height; stance feet stay at ground z.
    swing_lift = positions[:, :, 2].max(axis=0) - g.ground_z
    assert swing_lift.min() > 0.7 * g.step_height
    assert swing_lift.max() < 1.05 * g.step_height + 1e-9
