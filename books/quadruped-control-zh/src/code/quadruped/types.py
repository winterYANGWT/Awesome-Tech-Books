"""Shared data structures for the quadruped package.

The package uses SI units and radians throughout.  Leg ids follow the order
used by the A1 motor bus: FR (right front), FL (left front), RR (right rear),
RL (left rear).  Each leg has three joints: abduction, thigh and calf.

The state containers are deliberately plain dataclasses.  They are cheap to
construct and can be copied with :func:`dataclasses.replace`, which keeps the
2 ms control loop free of surprising allocations inside the physics wrapper.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum, IntEnum
from typing import Iterable, Sequence

import numpy as np


class LegId(IntEnum):
    """Leg index and the matching name prefix in the Menagerie MJCF."""

    FR = 0
    FL = 1
    RR = 2
    RL = 3

    @property
    def prefix(self) -> str:
        return self.name

    @classmethod
    def from_name(cls, name: str) -> "LegId":
        key = name.strip().upper()
        if key not in cls.__members__:
            raise ValueError(f"unknown leg id: {name!r}")
        return cls[key]


LEG_NAMES: tuple[str, ...] = tuple(leg.name for leg in LegId)
JOINT_NAMES: tuple[str, ...] = tuple(
    f"{leg.name}_{joint}_joint"
    for leg in LegId
    for joint in ("hip", "thigh", "calf")
)


class MotorMode(IntEnum):
    """Protocol modes from the A1 servo command frame (table T01)."""

    STOP = 0
    OPEN = 5
    SERVO = 10


class ControlMode(str, Enum):
    """The six useful special cases of the hybrid control law.

    ``MIXED`` keeps all five command terms.  The other members are shortcuts
    that set the unused gains/feed-forward terms to zero.
    """

    POSITION = "position"
    VELOCITY = "velocity"
    DAMPING = "damping"
    TORQUE = "torque"
    ZERO = "zero"
    PASSIVE = "passive"
    MIXED = "mixed"


def _as_vector(value, length: int, name: str) -> np.ndarray:
    arr = np.asarray(value, dtype=float).reshape(-1)
    if arr.size == 1:
        arr = np.full(length, float(arr[0]))
    if arr.size != length:
        raise ValueError(f"{name} must have {length} elements, got {arr.size}")
    return arr


@dataclass
class JointState:
    """Measured state of the twelve joints, grouped by leg then joint."""

    q: np.ndarray
    dq: np.ndarray
    tau: np.ndarray | None = None
    temperature: np.ndarray | None = None
    error: np.ndarray | None = None
    mode: np.ndarray | int = MotorMode.SERVO
    timestamp: float = 0.0

    def __post_init__(self) -> None:
        self.q = np.asarray(self.q, dtype=float).reshape(-1)
        n = self.q.size
        self.dq = _as_vector(self.dq, n, "dq")
        if self.tau is None:
            self.tau = np.zeros(n)
        else:
            self.tau = _as_vector(self.tau, n, "tau")
        if self.temperature is None:
            self.temperature = np.full(n, 25.0)
        else:
            self.temperature = _as_vector(self.temperature, n, "temperature")
        if self.error is None:
            self.error = np.zeros(n, dtype=int)
        else:
            self.error = np.asarray(self.error, dtype=int).reshape(-1)
        if np.ndim(self.mode) == 0:
            self.mode = np.full(n, int(self.mode), dtype=int)
        else:
            self.mode = np.asarray(self.mode, dtype=int).reshape(-1)

    def copy(self) -> "JointState":
        return JointState(
            q=self.q.copy(),
            dq=self.dq.copy(),
            tau=self.tau.copy(),
            temperature=self.temperature.copy(),
            error=self.error.copy(),
            mode=self.mode.copy(),
            timestamp=self.timestamp,
        )

    @property
    def n(self) -> int:
        return int(self.q.size)


@dataclass
class JointCmd:
    """The five quantities of the motor hybrid control law.

    ``q_des`` and ``dq_des`` are joint-side (output-side) values for the A1
    simulation, matching :class:`JointState`.  The optional ``mode`` selects a
    protocol mode; in simulation only ``SERVO`` produces feedback torque.
    """

    q_des: np.ndarray
    dq_des: np.ndarray | None = None
    kp: np.ndarray | float | None = None
    kd: np.ndarray | float | None = None
    tau_ff: np.ndarray | float | None = None
    mode: np.ndarray | int = MotorMode.SERVO

    def __post_init__(self) -> None:
        n = 12
        self.q_des = _as_vector(self.q_des, n, "q_des")
        if self.dq_des is None:
            self.dq_des = np.zeros(n)
        else:
            self.dq_des = _as_vector(self.dq_des, n, "dq_des")
        self.kp = _as_vector(0.0 if self.kp is None else self.kp, n, "kp")
        self.kd = _as_vector(0.0 if self.kd is None else self.kd, n, "kd")
        self.tau_ff = _as_vector(0.0 if self.tau_ff is None else self.tau_ff, n, "tau_ff")
        if np.any(self.kp < 0) or np.any(self.kd < 0):
            raise ValueError("kp and kd must be non-negative")
        if np.ndim(self.mode) == 0:
            self.mode = np.full(n, int(self.mode), dtype=int)
        else:
            self.mode = np.asarray(self.mode, dtype=int).reshape(-1)
        if self.mode.size != n:
            raise ValueError("mode must be a scalar or have 12 elements")

    @classmethod
    def zeros(cls, n: int = 12) -> "JointCmd":
        return cls(np.zeros(n), np.zeros(n), np.zeros(n), np.zeros(n), np.zeros(n))

    @classmethod
    def position(cls, q_des: Sequence[float], kp=80.0, kd=2.0,
                 tau_ff=0.0, dq_des=0.0) -> "JointCmd":
        q = np.asarray(q_des, dtype=float)
        return cls(q, np.full(q.size, dq_des), kp, kd, tau_ff)

    @classmethod
    def velocity(cls, dq_des: Sequence[float], kd=2.0,
                 tau_ff=0.0) -> "JointCmd":
        dq = np.asarray(dq_des, dtype=float)
        return cls(np.zeros(dq.size), dq, 0.0, kd, tau_ff)

    @classmethod
    def damping(cls, kd=2.0, n: int = 12) -> "JointCmd":
        return cls(np.zeros(n), np.zeros(n), np.zeros(n), kd, np.zeros(n))

    @classmethod
    def torque(cls, tau_ff: Sequence[float], n: int | None = None) -> "JointCmd":
        tau = np.asarray(tau_ff, dtype=float).reshape(-1)
        n = tau.size if n is None else n
        return cls(np.zeros(n), np.zeros(n), np.zeros(n), np.zeros(n), tau)

    @classmethod
    def zero(cls, n: int = 12) -> "JointCmd":
        return cls.zeros(n)

    @classmethod
    def mixed(cls, q_des, dq_des, kp, kd, tau_ff) -> "JointCmd":
        return cls(q_des, dq_des, kp, kd, tau_ff)

    @classmethod
    def from_mode(cls, mode: ControlMode | str, n: int = 12, *,
                  q_des=None, dq_des=None, kp=80.0, kd=2.0,
                  tau_ff=0.0) -> "JointCmd":
        """Build a command that is the requested special case of Eq. (2.1)."""
        mode = ControlMode(mode)
        q = np.zeros(n) if q_des is None else np.asarray(q_des, dtype=float)
        dq = np.zeros(n) if dq_des is None else np.asarray(dq_des, dtype=float)
        tau = np.zeros(n) if tau_ff is None else np.asarray(tau_ff, dtype=float)
        if mode is ControlMode.POSITION:
            return cls(q, np.zeros(n), kp, kd, np.zeros(n))
        if mode is ControlMode.VELOCITY:
            return cls(np.zeros(n), dq, 0.0, kd, np.zeros(n))
        if mode is ControlMode.DAMPING:
            return cls(np.zeros(n), np.zeros(n), 0.0, kd, np.zeros(n))
        if mode is ControlMode.TORQUE:
            return cls(np.zeros(n), np.zeros(n), 0.0, 0.0, tau)
        if mode in (ControlMode.ZERO, ControlMode.PASSIVE):
            return cls(np.zeros(n), np.zeros(n), 0.0, 0.0, np.zeros(n))
        return cls(q, dq, kp, kd, tau)

    def copy(self) -> "JointCmd":
        return JointCmd(self.q_des.copy(), self.dq_des.copy(), self.kp.copy(),
                        self.kd.copy(), self.tau_ff.copy(), self.mode.copy())

    @property
    def n(self) -> int:
        return int(self.q_des.size)


@dataclass
class BaseState:
    """Base (trunk) state.

    ``position``, ``velocity`` and ``quat`` are expressed in the world frame
    ``{s}``.  ``quat`` is ``[w, x, y, z]``.  ``omega_b`` and ``acc_b`` are the
    IMU readings in the body frame ``{b}``; they come from MuJoCo sensors and
    therefore include the sensor's proper-acceleration convention.
    """

    position: np.ndarray
    velocity: np.ndarray
    quat: np.ndarray
    rpy: np.ndarray | None = None
    omega_b: np.ndarray | None = None
    acc_b: np.ndarray | None = None
    timestamp: float = 0.0

    def __post_init__(self) -> None:
        self.position = np.asarray(self.position, dtype=float).reshape(3)
        self.velocity = np.asarray(self.velocity, dtype=float).reshape(3)
        self.quat = np.asarray(self.quat, dtype=float).reshape(4)
        norm = np.linalg.norm(self.quat)
        if norm > 0:
            self.quat = self.quat / norm
        if self.rpy is None:
            self.rpy = np.zeros(3)
        else:
            self.rpy = np.asarray(self.rpy, dtype=float).reshape(3)
        self.omega_b = (np.zeros(3) if self.omega_b is None
                        else np.asarray(self.omega_b, dtype=float).reshape(3))
        self.acc_b = (np.zeros(3) if self.acc_b is None
                      else np.asarray(self.acc_b, dtype=float).reshape(3))

    @property
    def height(self) -> float:
        return float(self.position[2])

    def rotation(self) -> np.ndarray:
        from .rotation import quat_to_R

        return quat_to_R(self.quat)

    def copy(self) -> "BaseState":
        return BaseState(self.position.copy(), self.velocity.copy(),
                         self.quat.copy(), self.rpy.copy(),
                         self.omega_b.copy(), self.acc_b.copy(),
                         self.timestamp)


@dataclass
class ContactState:
    """Per-foot contact flag and world-frame ground reaction force."""

    contact: np.ndarray
    forces: np.ndarray

    def __post_init__(self) -> None:
        self.contact = np.asarray(self.contact, dtype=bool).reshape(4)
        self.forces = np.asarray(self.forces, dtype=float).reshape(4, 3)

    @property
    def force(self) -> np.ndarray:
        return self.forces


@dataclass
class GaitCommand:
    """High-level velocity command consumed by the gait layer."""

    vx: float = 0.0
    vy: float = 0.0
    vyaw: float = 0.0
    body_height: float = 0.27
    step_height: float = 0.08
    gait_type: str = "trot"

    def as_array(self) -> np.ndarray:
        return np.array([self.vx, self.vy, self.vyaw], dtype=float)


@dataclass
class LegCommand:
    """Cartesian command for one leg (used by swing-leg controllers)."""

    position: np.ndarray
    velocity: np.ndarray = field(default_factory=lambda: np.zeros(3))
    kp: np.ndarray = field(default_factory=lambda: np.full(3, 100.0))
    kd: np.ndarray = field(default_factory=lambda: np.full(3, 2.0))
    tau_ff: np.ndarray = field(default_factory=lambda: np.zeros(3))


def joint_slice(leg: LegId) -> slice:
    """Return the slice of a 12-vector that belongs to ``leg``."""
    i = int(leg)
    return slice(3 * i, 3 * i + 3)


def stack_legs(values: Iterable[Sequence[float]]) -> np.ndarray:
    """Flatten four per-leg vectors into the package's 12-vector layout."""
    return np.concatenate([np.asarray(v, dtype=float).reshape(-1) for v in values])


__all__ = [
    "LegId", "LEG_NAMES", "JOINT_NAMES", "MotorMode", "ControlMode",
    "JointState", "JointCmd", "BaseState", "ContactState", "GaitCommand",
    "LegCommand", "joint_slice", "stack_legs",
]
