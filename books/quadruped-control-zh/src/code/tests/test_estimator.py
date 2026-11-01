"""Unit tests for the IMU model, recursive covariance and the Kalman filter."""

from __future__ import annotations

import numpy as np

from quadruped.estimator import AvgCov, Estimator, IMU
from quadruped.robot import A1Params, Quadruped


def test_avgcov_matches_batch():
    rng = np.random.default_rng(0)
    data = rng.normal([1.0, -2.0, 0.5], [0.5, 0.2, 0.1], size=(2000, 3))
    ac = AvgCov(3, "test", print_min=False)
    for row in data:
        ac.update(row)
    assert np.allclose(ac.mean, data.mean(axis=0), atol=1e-12)
    assert np.allclose(ac.cov, np.cov(data, rowvar=False), atol=1e-12)
    assert ac.count == 2000


def test_imu_bias_and_world_acceleration():
    imu = IMU(acc_bias=[0.1, -0.2, 0.3], gyro_bias=[0.01, 0.02, 0.03],
              acc_std=0.0, gyro_std=0.0)
    acc, gyro = imu.measure(np.array([0.0, 0.0, 9.81]), np.zeros(3))
    assert np.allclose(acc, [0.1, -0.2, 10.11])
    assert np.allclose(gyro, [0.01, 0.02, 0.03])
    # At rest and level, u = R a + g must be zero.
    u = imu.world_acceleration(np.array([0.0, 0.0, 9.81]), np.eye(3))
    assert np.allclose(u, np.zeros(3), atol=1e-12)


def test_estimator_initialization_and_output_shapes():
    params = A1Params.from_menagerie()
    robot = Quadruped(params)
    est = Estimator(robot=robot, dt=0.002)
    assert est.H.shape == (28, 18)
    assert est.F.shape == (18, 18)
    assert est.G.shape == (18, 3)
    assert np.allclose(est.F, np.eye(18) + 0.002 * est.A)
    assert np.allclose(est.G, 0.002 * est.B)
