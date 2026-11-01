"""Rigid-body rotations, homogeneous transforms and quaternions.

Conventions
-----------
* Rotation matrices map a vector expressed in the child frame to the parent
  frame: ``p_parent = R_parent_child @ p_child``.
* Elementary rotations follow the right-hand rule.
* Euler angles use the Y-P-R (yaw-pitch-roll) convention:
  ``R = Rz(yaw) @ Ry(pitch) @ Rx(roll)``.  This is the order used by the
  reference book and by the A1 IMU.
* Quaternions are stored as ``[w, x, y, z]`` and are always normalised before
  use.
"""

from __future__ import annotations

import math

import numpy as np


# ---------------------------------------------------------------------------
# Elementary rotations and skew-symmetric matrices
# ---------------------------------------------------------------------------

def Rx(theta: float) -> np.ndarray:
    """Rotation about the x axis by ``theta`` radians."""
    c, s = math.cos(theta), math.sin(theta)
    return np.array([[1.0, 0.0, 0.0],
                     [0.0, c, -s],
                     [0.0, s, c]], dtype=float)


def Ry(theta: float) -> np.ndarray:
    """Rotation about the y axis by ``theta`` radians."""
    c, s = math.cos(theta), math.sin(theta)
    return np.array([[c, 0.0, s],
                     [0.0, 1.0, 0.0],
                     [-s, 0.0, c]], dtype=float)


def Rz(theta: float) -> np.ndarray:
    """Rotation about the z axis by ``theta`` radians."""
    c, s = math.cos(theta), math.sin(theta)
    return np.array([[c, -s, 0.0],
                     [s, c, 0.0],
                     [0.0, 0.0, 1.0]], dtype=float)


def R2(theta: float) -> np.ndarray:
    """Planar rotation matrix (SO(2))."""
    c, s = math.cos(theta), math.sin(theta)
    return np.array([[c, -s], [s, c]], dtype=float)


def skew(v: np.ndarray) -> np.ndarray:
    """Return the matrix ``[v]x`` such that ``[v]x @ w == cross(v, w)``."""
    x, y, z = np.asarray(v, dtype=float).reshape(3)
    return np.array([[0.0, -z, y],
                     [z, 0.0, -x],
                     [-y, x, 0.0]], dtype=float)


def so3_hat(omega: np.ndarray) -> np.ndarray:
    """Alias of :func:`skew`, kept for the Lie-algebra notation."""
    return skew(omega)


def so3_vee(M: np.ndarray) -> np.ndarray:
    """Inverse of :func:`so3_hat` for a 3x3 skew matrix."""
    M = np.asarray(M, dtype=float).reshape(3, 3)
    return np.array([M[2, 1], M[0, 2], M[1, 0]], dtype=float)


def _unit(v: np.ndarray, name: str = "vector") -> np.ndarray:
    v = np.asarray(v, dtype=float).reshape(3)
    n = float(np.linalg.norm(v))
    if n < 1e-15:
        raise ValueError(f"{name} must be non-zero")
    return v / n


# ---------------------------------------------------------------------------
# SO(3) exponential / logarithm
# ---------------------------------------------------------------------------

def so3_exp(omega: np.ndarray) -> np.ndarray:
    """Exponential map from rotation vector to rotation matrix.

    ``omega`` is the axis-angle vector: its direction is the rotation axis and
    its norm is the rotation angle.  Rodrigues' formula is used, with a Taylor
    fallback near zero.
    """
    w = np.asarray(omega, dtype=float).reshape(3)
    theta = float(np.linalg.norm(w))
    K = skew(w)
    if theta < 1e-12:
        # R = I + K + K^2/2 + ...
        return np.eye(3) + K + 0.5 * (K @ K)
    a = w / theta
    K = skew(a)
    s, c = math.sin(theta), math.cos(theta)
    return np.eye(3) + s * K + (1.0 - c) * (K @ K)


def so3_log(R: np.ndarray) -> np.ndarray:
    """Logarithm map from rotation matrix to rotation vector.

    Handles the identity, the neighbourhood of zero and the angle-pi
    singularity.  At exactly pi the sign of the axis is arbitrary; both signs
    describe the same rotation.
    """
    R = np.asarray(R, dtype=float).reshape(3, 3)
    # A light orthogonal projection removes accumulated round-off without
    # changing a valid rotation matrix.
    U, _, Vt = np.linalg.svd(R)
    R = U @ Vt
    if np.linalg.det(R) < 0:
        U[:, -1] *= -1.0
        R = U @ Vt

    tr = float(np.trace(R))
    c = max(-1.0, min(1.0, 0.5 * (tr - 1.0)))
    theta = math.acos(c)
    vee = so3_vee(R - R.T)

    if theta < 1e-8:
        # sin(theta) ~ theta, so vee(R - R^T) / 2 is already the rotation
        # vector to first order.  Add the cubic correction for accuracy.
        return 0.5 * vee * (1.0 + theta * theta / 6.0)

    if math.pi - theta < 1e-6:
        # R + I = 2 a a^T at theta = pi.  Pick the best-conditioned column.
        A = 0.5 * (R + np.eye(3))
        col = int(np.argmax(np.diag(A)))
        axis = A[:, col]
        if np.linalg.norm(axis) < 1e-12:
            # Degenerate fallback: use the largest diagonal of (R + I)/2.
            axis = np.sqrt(np.maximum(np.diag(A), 0.0))
            axis[int(np.argmax(axis))] = max(axis.max(), 1.0)
        axis = axis / np.linalg.norm(axis)
        # Resolve the sign with the antisymmetric part when it is informative.
        if np.linalg.norm(vee) > 1e-8 and np.dot(axis, vee) < 0:
            axis = -axis
        return axis * theta

    axis = vee / (2.0 * math.sin(theta))
    return axis * theta


def so3_rotation_error(Ra: np.ndarray, Rb: np.ndarray) -> float:
    """Geodesic angle between two rotation matrices, in radians."""
    R = np.asarray(Ra, dtype=float).T @ np.asarray(Rb, dtype=float)
    return float(np.linalg.norm(so3_log(R)))


# ---------------------------------------------------------------------------
# Euler angles (Y-P-R, i.e. R = Rz(yaw) Ry(pitch) Rx(roll))
# ---------------------------------------------------------------------------

def rpy_to_R(rpy: np.ndarray) -> np.ndarray:
    """Convert ``[roll, pitch, yaw]`` to a rotation matrix."""
    roll, pitch, yaw = np.asarray(rpy, dtype=float).reshape(3)
    return Rz(yaw) @ Ry(pitch) @ Rx(roll)


def R_to_rpy(R: np.ndarray) -> np.ndarray:
    """Convert a rotation matrix to ``[roll, pitch, yaw]``.

    The pitch angle is in ``[-pi/2, pi/2]``.  At gimbal lock the roll is set
    to zero and the combined yaw is returned; the caller should use a
    quaternion or a rotation matrix for control near that singularity.
    """
    R = np.asarray(R, dtype=float).reshape(3, 3)
    sp = max(-1.0, min(1.0, -R[2, 0]))
    pitch = math.asin(sp)
    cp = math.cos(pitch)
    if abs(cp) > 1e-9:
        roll = math.atan2(R[2, 1], R[2, 2])
        yaw = math.atan2(R[1, 0], R[0, 0])
    else:
        # R = Rz(y) Ry(+-pi/2) Rx(r).  Only y -+ r is observable.
        roll = 0.0
        if sp > 0:
            # At +pi/2, R = Rz(yaw) Ry(pi/2) Rx(0) with
            # delta = roll - yaw observable; setting roll=0 gives yaw=-delta.
            delta = math.atan2(R[0, 1], R[0, 2])
            yaw = -delta
        else:
            delta = math.atan2(-R[0, 1], R[1, 1])
            yaw = delta
    return np.array([roll, pitch, yaw], dtype=float)


def wrap_angle(angle):
    """Wrap an angle (or array) to ``(-pi, pi]``."""
    a = np.asarray(angle, dtype=float)
    out = (a + math.pi) % (2.0 * math.pi) - math.pi
    out = np.where(np.isclose(out, -math.pi), math.pi, out)
    if np.ndim(angle) == 0:
        return float(out)
    return out


# ---------------------------------------------------------------------------
# Quaternions, [w, x, y, z]
# ---------------------------------------------------------------------------

def quat_mul(q1: np.ndarray, q2: np.ndarray) -> np.ndarray:
    """Hamilton product of two quaternions in ``[w, x, y, z]`` order."""
    w1, x1, y1, z1 = np.asarray(q1, dtype=float).reshape(4)
    w2, x2, y2, z2 = np.asarray(q2, dtype=float).reshape(4)
    return np.array([
        w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2,
        w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2,
        w1 * y2 - x1 * z2 + y1 * w2 + z1 * x2,
        w1 * z2 + x1 * y2 - y1 * x2 + z1 * w2,
    ], dtype=float)


def quat_conj(q: np.ndarray) -> np.ndarray:
    w, x, y, z = np.asarray(q, dtype=float).reshape(4)
    return np.array([w, -x, -y, -z], dtype=float)


def quat_normalize(q: np.ndarray) -> np.ndarray:
    q = np.asarray(q, dtype=float).reshape(4)
    n = float(np.linalg.norm(q))
    if n < 1e-15:
        return np.array([1.0, 0.0, 0.0, 0.0])
    return q / n


def quat_to_R(q: np.ndarray) -> np.ndarray:
    """Convert a unit quaternion ``[w, x, y, z]`` to a rotation matrix."""
    w, x, y, z = quat_normalize(q)
    return np.array([
        [1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
        [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
        [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)],
    ], dtype=float)


def R_to_quat(R: np.ndarray) -> np.ndarray:
    """Convert a rotation matrix to a unit quaternion ``[w, x, y, z]``.

    The branch with the largest pivot is used for numerical stability.  The
    sign is canonicalised to ``w >= 0`` when possible.
    """
    R = np.asarray(R, dtype=float).reshape(3, 3)
    tr = float(np.trace(R))
    if tr > 0.0:
        s = math.sqrt(tr + 1.0) * 2.0
        w = 0.25 * s
        x = (R[2, 1] - R[1, 2]) / s
        y = (R[0, 2] - R[2, 0]) / s
        z = (R[1, 0] - R[0, 1]) / s
    elif R[0, 0] >= R[1, 1] and R[0, 0] >= R[2, 2]:
        s = math.sqrt(1.0 + R[0, 0] - R[1, 1] - R[2, 2]) * 2.0
        w = (R[2, 1] - R[1, 2]) / s
        x = 0.25 * s
        y = (R[0, 1] + R[1, 0]) / s
        z = (R[0, 2] + R[2, 0]) / s
    elif R[1, 1] >= R[2, 2]:
        s = math.sqrt(1.0 + R[1, 1] - R[0, 0] - R[2, 2]) * 2.0
        w = (R[0, 2] - R[2, 0]) / s
        x = (R[0, 1] + R[1, 0]) / s
        y = 0.25 * s
        z = (R[1, 2] + R[2, 1]) / s
    else:
        s = math.sqrt(1.0 + R[2, 2] - R[0, 0] - R[1, 1]) * 2.0
        w = (R[1, 0] - R[0, 1]) / s
        x = (R[0, 2] + R[2, 0]) / s
        y = (R[1, 2] + R[2, 1]) / s
        z = 0.25 * s
    q = quat_normalize(np.array([w, x, y, z], dtype=float))
    if q[0] < 0.0:
        q = -q
    return q


def quat_to_rpy(q: np.ndarray) -> np.ndarray:
    return R_to_rpy(quat_to_R(q))


def rpy_to_quat(rpy: np.ndarray) -> np.ndarray:
    return R_to_quat(rpy_to_R(rpy))


def quat_rotate(q: np.ndarray, v: np.ndarray) -> np.ndarray:
    """Rotate a 3-vector by a quaternion."""
    return quat_to_R(q) @ np.asarray(v, dtype=float).reshape(3)


# ---------------------------------------------------------------------------
# Homogeneous transforms and the adjoint
# ---------------------------------------------------------------------------

def make_T(R: np.ndarray | None = None, p: np.ndarray | None = None) -> np.ndarray:
    """Build a 4x4 homogeneous transform from a rotation and translation."""
    T = np.eye(4)
    if R is not None:
        T[:3, :3] = np.asarray(R, dtype=float).reshape(3, 3)
    if p is not None:
        T[:3, 3] = np.asarray(p, dtype=float).reshape(3)
    return T


def inv_T(T: np.ndarray) -> np.ndarray:
    """Analytic inverse of a homogeneous transform."""
    T = np.asarray(T, dtype=float).reshape(4, 4)
    R = T[:3, :3]
    p = T[:3, 3]
    out = np.eye(4)
    out[:3, :3] = R.T
    out[:3, 3] = -R.T @ p
    return out


def adjoint(T: np.ndarray) -> np.ndarray:
    """6x6 adjoint of a homogeneous transform.

    The twist ordering is ``[v; omega]``.  For ``T_ab`` the returned matrix
    maps a twist expressed in ``{b}`` to ``{a}``.
    """
    T = np.asarray(T, dtype=float).reshape(4, 4)
    R = T[:3, :3]
    p = T[:3, 3]
    Ad = np.zeros((6, 6))
    Ad[:3, :3] = R
    Ad[:3, 3:] = skew(p) @ R
    Ad[3:, 3:] = R
    return Ad


def transform_point(T: np.ndarray, p: np.ndarray) -> np.ndarray:
    """Apply a homogeneous transform to a point (translation included)."""
    T = np.asarray(T, dtype=float).reshape(4, 4)
    p = np.asarray(p, dtype=float).reshape(3)
    return T[:3, :3] @ p + T[:3, 3]


def transform_vector(T: np.ndarray, v: np.ndarray) -> np.ndarray:
    """Apply only the rotation part of a homogeneous transform to a vector."""
    T = np.asarray(T, dtype=float).reshape(4, 4)
    return T[:3, :3] @ np.asarray(v, dtype=float).reshape(3)


__all__ = [
    "Rx", "Ry", "Rz", "R2", "skew", "so3_hat", "so3_vee", "so3_exp",
    "so3_log", "so3_rotation_error", "rpy_to_R", "R_to_rpy", "wrap_angle",
    "quat_mul", "quat_conj", "quat_normalize", "quat_to_R", "R_to_quat",
    "quat_to_rpy", "rpy_to_quat", "quat_rotate", "make_T", "inv_T",
    "adjoint", "transform_point", "transform_vector",
]
