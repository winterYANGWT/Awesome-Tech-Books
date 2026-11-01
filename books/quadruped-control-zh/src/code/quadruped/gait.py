"""Gait timing, foothold planning and the walking trajectory generator.

Three classes follow Section 9.4 of the reference book:

* :class:`GaitParams` collects period, duty factor and phase offsets and
  provides the usual trot/walk/pace/bound/pronk presets;
* :class:`WaveGenerator` turns time into four phases and contact flags;
* :class:`FeetEndCal` implements the Raibert-style foothold of Eq. (9.10);
* :class:`GaitGenerator` combines the two and produces world-frame foot
  positions/velocities with a cycloid swing.

The phase convention is the one of Eq. (9.5): for a stance leg ``phase`` is
the fraction of the stance time already elapsed, for a swing leg it is the
fraction of the swing time.  It is therefore in ``[0, 1]`` for both states.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum

import numpy as np

from .swing import SwingTrajectory


# ---------------------------------------------------------------------------
# Gait parameters
# ---------------------------------------------------------------------------
@dataclass
class GaitParams:
    """Timing and foothold parameters of one gait.

    ``offsets`` are the four phase offsets ``b_i / P`` in leg order
    FR, FL, RR, RL.  ``duty`` is the stance fraction ``r``.
    """

    name: str = "trot"
    period: float = 0.36
    duty: float = 0.6
    offsets: np.ndarray = field(
        default_factory=lambda: np.array([0.0, 0.5, 0.5, 0.0]))
    step_height: float = 0.06
    kx: float = 0.06
    ky: float = 0.06
    kyaw: float = 0.08
    ground_z: float = 0.02

    def __post_init__(self) -> None:
        self.period = float(self.period)
        self.duty = float(self.duty)
        self.offsets = np.asarray(self.offsets, dtype=float).reshape(4)
        self.step_height = float(self.step_height)
        self.ground_z = float(self.ground_z)

    @property
    def stance_time(self) -> float:
        return self.duty * self.period

    @property
    def swing_time(self) -> float:
        return (1.0 - self.duty) * self.period

    @classmethod
    def trot(cls, period: float = 0.36, duty: float = 0.6,
             step_height: float = 0.06) -> "GaitParams":
        return cls("trot", period, duty, np.array([0.0, 0.5, 0.5, 0.0]),
                   step_height)

    @classmethod
    def walk(cls, period: float = 0.6, duty: float = 0.75,
             step_height: float = 0.05) -> "GaitParams":
        # Four-beat lateral sequence walk: RR -> FR -> RL -> FL.
        return cls("walk", period, duty,
                   np.array([0.0, 0.5, 0.25, 0.75]), step_height)

    @classmethod
    def pace(cls, period: float = 0.4, duty: float = 0.5,
             step_height: float = 0.05) -> "GaitParams":
        return cls("pace", period, duty, np.array([0.0, 0.5, 0.0, 0.5]),
                   step_height)

    @classmethod
    def bound(cls, period: float = 0.32, duty: float = 0.4,
              step_height: float = 0.06) -> "GaitParams":
        return cls("bound", period, duty, np.array([0.0, 0.0, 0.5, 0.5]),
                   step_height)

    @classmethod
    def pronk(cls, period: float = 0.5, duty: float = 0.7,
              step_height: float = 0.06) -> "GaitParams":
        return cls("pronk", period, duty, np.zeros(4), step_height)

    @classmethod
    def from_name(cls, name: str, **kwargs) -> "GaitParams":
        presets = {"trot": cls.trot, "walk": cls.walk, "pace": cls.pace,
                   "bound": cls.bound, "pronk": cls.pronk}
        key = str(name).strip().lower()
        if key not in presets:
            raise ValueError(f"unknown gait {name!r}; choose from {sorted(presets)}")
        return presets[key](**kwargs)

    def copy(self) -> "GaitParams":
        return GaitParams(self.name, self.period, self.duty, self.offsets.copy(),
                          self.step_height, self.kx, self.ky, self.kyaw,
                          self.ground_z)


class WaveStatus(str, Enum):
    WAVE_ALL = "wave"
    STANCE_ALL = "stance"
    SWING_ALL = "swing"


# ---------------------------------------------------------------------------
# Phase generator
# ---------------------------------------------------------------------------
class WaveGenerator:
    """Time -> phase/contact, with STANCE_ALL and SWING_ALL overrides."""

    def __init__(self, period: float = 0.32, duty: float = 0.5,
                 offsets: np.ndarray | None = None) -> None:
        self.period = float(period)
        self.duty = float(duty)
        self.offsets = (np.zeros(4) if offsets is None
                        else np.asarray(offsets, dtype=float).reshape(4))
        self.status = WaveStatus.STANCE_ALL
        self._t0 = 0.0
        self.phase = np.ones(4)
        self.contact = np.ones(4, dtype=bool)

    @property
    def stance_time(self) -> float:
        return self.duty * self.period

    @property
    def swing_time(self) -> float:
        return (1.0 - self.duty) * self.period

    def set_status(self, status: WaveStatus | str, t: float = 0.0) -> None:
        self.status = WaveStatus(status)
        self._t0 = float(t)
        if self.status is WaveStatus.STANCE_ALL:
            self.phase[:] = 1.0
            self.contact[:] = True
        elif self.status is WaveStatus.SWING_ALL:
            self.phase[:] = 0.0
            self.contact[:] = False

    def step(self, t: float, status: WaveStatus | str | None = None
             ) -> tuple[np.ndarray, np.ndarray]:
        if status is not None:
            self.set_status(status, t)
        if self.status is WaveStatus.STANCE_ALL:
            self.phase[:] = 1.0
            self.contact[:] = True
            return self.phase.copy(), self.contact.copy()
        if self.status is WaveStatus.SWING_ALL:
            self.phase[:] = np.clip((t - self._t0) / max(self.swing_time, 1e-9),
                                    0.0, 1.0)
            self.contact[:] = False
            return self.phase.copy(), self.contact.copy()

        for i in range(4):
            loc = ((t - self._t0) / self.period + self.offsets[i]) % 1.0
            if loc < self.duty:
                self.contact[i] = True
                self.phase[i] = loc / max(self.duty, 1e-9)
            else:
                self.contact[i] = False
                self.phase[i] = (loc - self.duty) / max(1.0 - self.duty, 1e-9)
        return self.phase.copy(), self.contact.copy()

    calc_contact_phase = step

    def get_tstance(self) -> float:
        return self.stance_time

    def get_tswing(self) -> float:
        return self.swing_time

    def get_t(self) -> float:
        return self.period


# ---------------------------------------------------------------------------
# Foothold calculation
# ---------------------------------------------------------------------------
class FeetEndCal:
    """Raibert foothold of Eqs. (9.8)-(9.10), all in the world frame."""

    def __init__(self, robot, params=None, kx: float = 0.06,
                 ky: float = 0.06, kyaw: float = 0.08,
                 ground_z: float = 0.02) -> None:
        self.robot = robot
        self.params = params
        self.kx = float(kx)
        self.ky = float(ky)
        self.kyaw = float(kyaw)
        self.ground_z = float(ground_z)
        q_stand = (np.asarray(params.home_q, dtype=float) if params is not None
                   else np.array([0.0, 0.9, -1.8] * 4))
        self.neutral_body = np.asarray(robot.fk_all(q_stand), dtype=float)
        self.neutral_body[:, 2] = self.ground_z
        # Radius and fixed angle of every leg's neutral point about the body
        # z axis (looking from above, x forward and y left).
        self.radius = np.hypot(self.neutral_body[:, 0], self.neutral_body[:, 1])
        self.theta0 = np.arctan2(self.neutral_body[:, 1],
                                 self.neutral_body[:, 0])

    def neutral_world(self, base_pos: np.ndarray, yaw: float) -> np.ndarray:
        """Neutral points under the body at the current yaw."""
        c, s = np.cos(yaw), np.sin(yaw)
        out = np.zeros((4, 3))
        for i in range(4):
            out[i, 0] = base_pos[0] + c * self.neutral_body[i, 0] - s * self.neutral_body[i, 1]
            out[i, 1] = base_pos[1] + s * self.neutral_body[i, 0] + c * self.neutral_body[i, 1]
            out[i, 2] = self.ground_z
        return out

    def calc(self, leg: int, v_cmd: np.ndarray, w_cmd: float,
             base_pos: np.ndarray, yaw: float, v_cur: np.ndarray,
             w_cur: float, phase: float,
             stance_time: float, swing_time: float) -> np.ndarray:
        """Return the world x-y foothold and the ground z for one leg."""
        i = int(leg)
        p = min(1.0, max(0.0, float(phase)))
        v_cmd = np.asarray(v_cmd, dtype=float).reshape(2)
        v_cur = np.asarray(v_cur, dtype=float).reshape(-1)[:2]
        base = np.asarray(base_pos, dtype=float).reshape(3)
        t_rem = (1.0 - p) * swing_time + 0.5 * stance_time

        theta_f = (yaw + self.theta0[i] + w_cur * t_rem
                   + self.kyaw * (w_cur - float(w_cmd)))
        x = (base[0] + self.radius[i] * np.cos(theta_f)
             + v_cur[0] * t_rem + self.kx * (v_cur[0] - v_cmd[0]))
        y = (base[1] + self.radius[i] * np.sin(theta_f)
             + v_cur[1] * t_rem + self.ky * (v_cur[1] - v_cmd[1]))
        return np.array([x, y, self.ground_z])


# ---------------------------------------------------------------------------
# Complete generator
# ---------------------------------------------------------------------------
class GaitGenerator:
    """Produce four world-frame foot targets and velocities.

    ``update`` is called once per control cycle.  A leg in stance keeps the
    world position at which it landed; a leg in swing follows a cycloid from
    its lift-off point to the continuously updated Raibert foothold.
    """

    def __init__(self, robot, params=None,
                 gait: GaitParams | None = None,
                 ground_z: float = 0.02) -> None:
        self.robot = robot
        self.params = params
        self.gait = GaitParams.trot() if gait is None else gait
        self.ground_z = float(ground_z)
        self.wave = WaveGenerator(self.gait.period, self.gait.duty,
                                  self.gait.offsets)
        self.calc = FeetEndCal(robot, params, self.gait.kx, self.gait.ky,
                               self.gait.kyaw, self.ground_z)
        self.contact = np.ones(4, dtype=bool)
        self.phase = np.ones(4)
        self.position = np.zeros((4, 3))
        self.velocity = np.zeros((4, 3))
        self.stance_pos = np.zeros((4, 3))
        self.swing_start = np.zeros((4, 3))
        self.target = np.zeros((4, 3))
        self.trajectory = [SwingTrajectory(duration=max(self.wave.swing_time, 1e-3))
                           for _ in range(4)]
        self._initialized = False
        self.v_cmd = np.zeros(2)
        self.w_cmd = 0.0
        self.step_height = self.gait.step_height
        self.start_time = 0.0
        self.status = WaveStatus.STANCE_ALL

    # -- configuration ---------------------------------------------------
    def set_gait(self, v_cmd: np.ndarray, w_cmd: float,
                 step_height: float | None = None) -> None:
        self.v_cmd = np.asarray(v_cmd, dtype=float).reshape(2)
        self.w_cmd = float(w_cmd)
        if step_height is not None:
            self.step_height = float(step_height)

    def set_status(self, status: WaveStatus | str, t: float = 0.0) -> None:
        self.wave.set_status(status, t)
        self.status = self.wave.status
        if self.status is WaveStatus.SWING_ALL:
            for i in range(4):
                self.swing_start[i] = self.position[i]

    def reset(self) -> None:
        self._initialized = False

    # -- main update -----------------------------------------------------
    def update(self, t: float, base_pos: np.ndarray, yaw: float,
               v_cur: np.ndarray, w_cur: float,
               feet_current: np.ndarray | None = None,
               status: WaveStatus | str | None = None) -> tuple[np.ndarray, np.ndarray]:
        base = np.asarray(base_pos, dtype=float).reshape(3)
        v_cur = np.asarray(v_cur, dtype=float).reshape(3)
        if status is not None:
            self.set_status(status, t)

        if not self._initialized:
            if feet_current is None:
                feet = self.calc.neutral_world(base, yaw)
                feet[:, 2] = self.ground_z
            else:
                feet = np.asarray(feet_current, dtype=float).reshape(4, 3).copy()
            self.position = feet.copy()
            self.stance_pos = feet.copy()
            self.swing_start = feet.copy()
            self.target = feet.copy()
            self._initialized = True
            self.start_time = float(t)

        phase, contact = self.wave.step(t)
        self.phase = phase

        for i in range(4):
            was_contact = bool(self.contact[i])
            now_contact = bool(contact[i])
            if now_contact:
                if not was_contact:
                    # Touch down where the swing ended.
                    self.stance_pos[i] = self.position[i].copy()
                self.position[i] = self.stance_pos[i]
                self.velocity[i] = 0.0
            else:
                if was_contact:
                    self.swing_start[i] = self.position[i].copy()
                target = self.calc.calc(
                    i, self.v_cmd, self.w_cmd, base, yaw, v_cur, w_cur,
                    phase[i], self.wave.stance_time, self.wave.swing_time)
                self.target[i] = target
                # Re-aim the cycloid at the continuously updated foothold.
                self.trajectory[i].set(
                    self.swing_start[i], self.target[i],
                    self.step_height, max(self.wave.swing_time, 1e-6),
                    "cycloid")
                pos, vel = self.trajectory[i].evaluate(phase[i])
                self.position[i] = pos
                self.velocity[i] = vel
        self.contact = contact.copy()
        return self.position.copy(), self.velocity.copy()

    # -- accessors -------------------------------------------------------
    def foot_target(self, leg: int) -> np.ndarray:
        return self.position[int(leg)].copy()

    def foot_velocity(self, leg: int) -> np.ndarray:
        return self.velocity[int(leg)].copy()

    def all_targets(self) -> np.ndarray:
        return self.position.copy()

    def all_velocities(self) -> np.ndarray:
        return self.velocity.copy()


__all__ = ["GaitParams", "WaveGenerator", "WaveStatus", "FeetEndCal",
           "GaitGenerator"]
