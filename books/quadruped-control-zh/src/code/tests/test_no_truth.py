"""Anti-cheat test: the controllers must work without simulator truth.

A thin wrapper replaces the base position/velocity returned by ``get_state``
with NaN and hides the raw ``data``/``model`` objects.  The controllers may
only use :meth:`~quadruped.sim.A1Sim.read_sensors` (IMU + encoders + contact)
and the Chapter 7 estimator.  If any truth value leaks into a controller, the
NaN propagates into the commanded torque and the test fails.
"""

from __future__ import annotations

import numpy as np

from quadruped.fsm import ControlContext
from quadruped.robot import A1Params, Quadruped
from quadruped.sim import A1Sim
from quadruped.trotting import FreeStand, TrottingController
from quadruped.types import JointCmd


class TruthBlindedSim:
    """Expose only the measured interface; truth becomes NaN."""

    def __init__(self, sim: A1Sim) -> None:
        self._sim = sim

    @property
    def timestep(self) -> float:
        return self._sim.timestep

    @property
    def joint_positions(self) -> np.ndarray:
        return self._sim.joint_positions

    @property
    def joint_velocities(self) -> np.ndarray:
        return self._sim.joint_velocities

    def read_sensors(self):
        return self._sim.read_sensors()

    def set_joint_pd(self, cmd: JointCmd) -> None:
        self._sim.set_joint_pd(cmd)

    def get_state(self):
        """Validation-only interface: position/velocity are poisoned."""
        base, joints = self._sim.get_state()
        base.position = np.full(3, np.nan)
        base.velocity = np.full(3, np.nan)
        return base, joints

    @property
    def data(self):
        raise AttributeError("raw MuJoCo data is not available to a controller")

    @property
    def model(self):
        raise AttributeError("raw MuJoCo model is not available to a controller")


def _boot() -> tuple[A1Params, Quadruped, A1Sim]:
    params = A1Params.from_menagerie()
    robot = Quadruped(params)
    sim = A1Sim()
    q0 = params.home_q.copy()
    for _ in range(800):
        sim.step(JointCmd(q0, np.zeros(12),
                          np.array([100.0, 300.0, 400.0] * 4),
                          np.array([4.0, 8.0, 8.0] * 4), np.zeros(12)))
    return params, robot, sim


def _assert_cmd_finite(cmd: JointCmd) -> None:
    for name in ("q_des", "dq_des", "kp", "kd", "tau_ff"):
        assert np.all(np.isfinite(getattr(cmd, name))), name


def _assert_upright(base, min_height: float) -> None:
    assert base.position[2] > min_height, base.position
    assert abs(float(base.rpy[0])) < 0.3, base.rpy
    assert abs(float(base.rpy[1])) < 0.3, base.rpy


def test_free_stand_without_truth():
    params, robot, sim = _boot()
    blind = TruthBlindedSim(sim)
    ctx = ControlContext.from_sim(blind, params)
    state = FreeStand(ctx, robot=robot, params=params)
    try:
        state.enter()
        base, _ = blind.get_state()
        assert np.all(np.isnan(base.position)) and np.all(np.isnan(base.velocity))

        min_h = np.inf
        for _ in range(1500):             # 3 s
            cmd = state.run()
            _assert_cmd_finite(cmd)
            base = sim.step()
            _assert_upright(base, 0.20)
            min_h = min(min_h, float(base.position[2]))
        assert np.all(np.isfinite(sim._tau)), sim._tau
        assert min_h > 0.20, min_h
    finally:
        sim.close()


def test_trotting_without_truth():
    params, robot, sim = _boot()
    blind = TruthBlindedSim(sim)
    ctx = ControlContext.from_sim(blind, params)
    state = TrottingController(ctx, robot=robot, params=params)
    try:
        state.enter()
        state.set_command(vx=0.2)
        min_h = np.inf
        for _ in range(2500):             # 5 s
            cmd = state.run()
            _assert_cmd_finite(cmd)
            base = sim.step()
            _assert_upright(base, 0.18)
            min_h = min(min_h, float(base.position[2]))
        assert np.all(np.isfinite(sim._tau)), sim._tau
        assert min_h > 0.18, min_h
        assert np.all(np.isfinite(state.est_base.position))
        assert np.all(np.isfinite(state.est_base.velocity))
    finally:
        sim.close()
