"""Rotation, Euler-angle and quaternion consistency tests."""

from __future__ import annotations

import numpy as np
import pytest

from quadruped.rotation import (R_to_quat, R_to_rpy, quat_to_R, rpy_to_R,
                                rpy_to_quat, so3_exp, so3_log, wrap_angle)


def _random_rotation_vectors(rng, n=100):
    out = []
    for _ in range(n):
        axis = rng.normal(size=3)
        axis /= np.linalg.norm(axis)
        theta = rng.uniform(0.0, np.pi)
        out.append(axis * theta)
    return out


def test_so3_exp_log_roundtrip():
    rng = np.random.default_rng(0)
    vectors = _random_rotation_vectors(rng, 100)
    vectors += [np.zeros(3), np.array([1e-10, -2e-10, 3e-10]),
                np.array([0.0, 0.0, np.pi]), np.array([np.pi, 0.0, 0.0])]
    for w in vectors:
        R = so3_exp(w)
        assert np.max(np.abs(so3_exp(so3_log(R)) - R)) < 1e-9
    for w in vectors[:100]:
        R = so3_exp(w)
        assert np.linalg.norm(so3_log(R) - w) < 1e-9


def test_euler_roundtrip():
    rng = np.random.default_rng(1)
    for _ in range(100):
        rpy = np.array([rng.uniform(-np.pi, np.pi),
                        rng.uniform(-1.4, 1.4),
                        rng.uniform(-np.pi, np.pi)])
        R = rpy_to_R(rpy)
        rpy2 = R_to_rpy(R)
        assert np.max(np.abs(rpy_to_R(rpy2) - R)) < 1e-9
        assert np.max(np.abs(wrap_angle(rpy2 - rpy))) < 1e-9


def test_quaternion_consistency():
    rng = np.random.default_rng(2)
    for _ in range(100):
        rpy = np.array([rng.uniform(-np.pi, np.pi),
                        rng.uniform(-1.4, 1.4),
                        rng.uniform(-np.pi, np.pi)])
        R = rpy_to_R(rpy)
        q = R_to_quat(R)
        assert abs(np.linalg.norm(q) - 1.0) < 1e-12
        assert np.max(np.abs(quat_to_R(q) - R)) < 1e-9
        assert np.max(np.abs(R_to_quat(quat_to_R(q)) - q)) < 1e-9
        q2 = rpy_to_quat(rpy)
        # q and -q describe the same rotation.
        assert min(np.max(np.abs(q - q2)), np.max(np.abs(q + q2))) < 1e-9


def test_rotation_matrix_properties():
    rng = np.random.default_rng(3)
    for _ in range(20):
        R = so3_exp(_random_rotation_vectors(rng, 1)[0])
        assert np.max(np.abs(R.T @ R - np.eye(3))) < 1e-9
        assert abs(np.linalg.det(R) - 1.0) < 1e-9
