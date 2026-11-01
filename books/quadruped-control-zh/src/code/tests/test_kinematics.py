"""Single-leg FK/IK/Jacobian and statics tests against MuJoCo."""

from __future__ import annotations

import numpy as np
import pytest

import mujoco

from quadruped.leg import Leg
from quadruped.robot import A1Params, Quadruped
from quadruped.sim import A1Sim
from quadruped.types import LegId


@pytest.fixture(scope="module")
def rig():
    sim = A1Sim()
    params = A1Params.from_menagerie()
    robot = Quadruped(params)
    yield sim, params, robot
    sim.close()


def _put_pose(sim: A1Sim, q_leg: np.ndarray, base_z: float = 2.0) -> None:
    sim.data.qpos[0:3] = [0.0, 0.0, base_z]
    sim.data.qpos[3:7] = [1.0, 0.0, 0.0, 0.0]
    sim.data.qpos[sim.qpos_adr] = np.tile(q_leg, 4)
    sim.data.qvel[:] = 0.0
    mujoco.mj_forward(sim.model, sim.data)


def test_fk_matches_mujoco(rig):
    sim, params, robot = rig
    rng = np.random.default_rng(10)
    max_err = 0.0
    for leg in robot.legs:
        for _ in range(200):
            q = np.array([rng.uniform(-0.7, 0.7),
                          rng.uniform(0.0, 1.6),
                          rng.uniform(-2.3, -1.2)])
            _put_pose(sim, q)
            foot_mj = sim.foot_positions_leg()[int(leg.leg_id)]
            foot = leg.fk(q)
            max_err = max(max_err, float(np.max(np.abs(foot - foot_mj))))
    assert max_err < 1e-6, max_err


def test_quadruped_fk_matches_body_frame(rig):
    sim, params, robot = rig
    q = np.array([0.1, 0.95, -1.8, -0.05, 0.85, -1.75,
                  0.08, 0.9, -1.82, -0.04, 0.88, -1.79])
    sim.data.qpos[0:3] = [0.0, 0.0, 2.0]
    sim.data.qpos[3:7] = [1.0, 0.0, 0.0, 0.0]
    sim.data.qpos[sim.qpos_adr] = q
    sim.data.qvel[:] = 0.0
    mujoco.mj_forward(sim.model, sim.data)
    assert np.max(np.abs(robot.fk_all(q) - sim.foot_positions_body())) < 1e-6


def test_ik_roundtrip(rig):
    _, _, robot = rig
    rng = np.random.default_rng(11)
    max_err = 0.0
    for leg in robot.legs:
        for _ in range(200):
            q = np.array([rng.uniform(-0.6, 0.6),
                          rng.uniform(0.1, 1.5),
                          rng.uniform(-2.2, -1.3)])
            q2 = leg.ik(leg.fk(q))
            max_err = max(max_err, float(np.max(np.abs(q2 - q))))
    assert max_err < 1e-9, max_err


def test_ik_reference_branch_over_full_range(rig):
    """The optional reference selects the correct branch near the workspace edge."""
    _, params, robot = rig
    rng = np.random.default_rng(12)
    max_err = 0.0
    for leg in robot.legs:
        lo, hi = params.leg_limits(leg.leg_id)
        for _ in range(200):
            q = rng.uniform(lo, hi)
            q2 = leg.ik(leg.fk(q), q_ref=q)
            max_err = max(max_err, float(np.max(np.abs(q2 - q))))
    assert max_err < 1e-9, max_err


def test_jacobian_numerical(rig):
    _, _, robot = rig
    rng = np.random.default_rng(13)
    h = 1e-6
    max_err = 0.0
    for _ in range(40):
        q = np.array([rng.uniform(-0.6, 0.6),
                      rng.uniform(0.1, 1.5),
                      rng.uniform(-2.2, -1.3)])
        leg = robot.legs[0]
        J = leg.jacobian(q)
        Jn = np.zeros((3, 3))
        for k in range(3):
            qp = q.copy()
            qm = q.copy()
            qp[k] += h
            qm[k] -= h
            Jn[:, k] = (leg.fk(qp) - leg.fk(qm)) / (2.0 * h)
        max_err = max(max_err, float(np.max(np.abs(J - Jn))))
    assert max_err < 1e-6, max_err


def test_jacobian_dot_numerical(rig):
    _, _, robot = rig
    leg = robot.legs[1]
    q = np.array([0.2, 0.9, -1.8])
    dq = np.array([0.3, -0.4, 0.5])
    h = 1e-6
    Jd = leg.jacobian_dot(q, dq)
    Jd_num = (leg.jacobian(q + h * dq) - leg.jacobian(q - h * dq)) / (2.0 * h)
    assert np.max(np.abs(Jd - Jd_num)) < 1e-6


def test_statics_roundtrip(rig):
    _, _, robot = rig
    leg = robot.legs[2]
    q = np.array([-0.2, 1.0, -1.9])
    f = np.array([5.0, -3.0, 30.0])
    tau = leg.tau_from_force(q, f)
    f2 = leg.force_from_tau(q, tau)
    assert np.max(np.abs(f - f2)) < 1e-9
