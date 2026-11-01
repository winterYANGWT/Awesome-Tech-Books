"""Finite-state machine and the standing states.

The design follows the three-function state interface of Chapter 3:
``enter()`` runs once, ``run()`` runs every 2 ms cycle, ``exit()`` runs once
when leaving, and ``check_transition()`` proposes the next state.  A
two-phase switch is used: the request is registered at the end of one cycle,
and the old state's ``exit`` / new state's ``enter`` happen at the beginning
of the next cycle.  This guarantees that a state always gets a chance to
finish its own bookkeeping.

The states here are deliberately small.  ``Passive`` is the damping/safe
state, ``FixedStand`` interpolates from the current joint angles to the A1
home pose, and ``FixedDown`` interpolates to a folded, low pose.  Later
chapters add ``FreeStand`` and ``Trotting`` without changing this interface.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum

import numpy as np

from .types import JointCmd, MotorMode


class StateID(str, Enum):
    PASSIVE = "Passive"
    FIXED_STAND = "FixedStand"
    FIXED_DOWN = "FixedDown"
    FREE_STAND = "FreeStand"
    TROTTING = "Trotting"
    MOVE_BASE = "move_base"

    @classmethod
    def coerce(cls, value) -> "StateID":
        if isinstance(value, cls):
            return value
        if isinstance(value, str):
            for member in cls:
                if value == member.value or value == member.name:
                    return member
        raise ValueError(f"unknown state: {value!r}")


@dataclass
class ControlContext:
    """Objects shared by all states (the book's ``CtrlComponents``)."""

    sim: object
    params: object = None
    robot: object = None
    dt: float = 0.002
    q_stand: np.ndarray | None = None
    q_down: np.ndarray | None = None
    kp_stand: np.ndarray = field(
        default_factory=lambda: np.array([100.0, 800.0, 1000.0] * 4))
    kd_stand: np.ndarray = field(
        default_factory=lambda: np.array([4.0, 10.0, 10.0] * 4))
    kp_down: np.ndarray = field(
        default_factory=lambda: np.array([80.0, 400.0, 500.0] * 4))
    kd_down: np.ndarray = field(
        default_factory=lambda: np.array([4.0, 8.0, 8.0] * 4))
    passive_kd: np.ndarray = field(
        default_factory=lambda: np.array([1.0, 2.0, 2.0] * 4))

    def __post_init__(self) -> None:
        if self.q_stand is None:
            if self.params is not None and hasattr(self.params, "home_q"):
                self.q_stand = np.asarray(self.params.home_q, dtype=float).copy()
            else:
                self.q_stand = np.array([0.0, 0.9, -1.8] * 4)
        self.q_stand = np.asarray(self.q_stand, dtype=float).reshape(12)
        if self.q_down is None:
            self.q_down = np.array([0.0, 1.35, -2.55] * 4)
        self.q_down = np.asarray(self.q_down, dtype=float).reshape(12)
        self.kp_stand = np.asarray(self.kp_stand, dtype=float).reshape(12)
        self.kd_stand = np.asarray(self.kd_stand, dtype=float).reshape(12)
        self.kp_down = np.asarray(self.kp_down, dtype=float).reshape(12)
        self.kd_down = np.asarray(self.kd_down, dtype=float).reshape(12)
        self.passive_kd = np.asarray(self.passive_kd, dtype=float).reshape(12)

    @classmethod
    def from_sim(cls, sim, params=None, **kwargs) -> "ControlContext":
        return cls(sim=sim, params=params, dt=float(getattr(sim, "timestep", 0.002)),
                   **kwargs)


class State:
    """Base class for every controller state."""

    id: StateID

    def __init__(self, context: ControlContext,
                 state_id: StateID | str = StateID.PASSIVE) -> None:
        self.context = context
        self.id = StateID.coerce(state_id)
        self.last_cmd: JointCmd | None = None
        self.entered = False

    @property
    def sim(self):
        return self.context.sim

    def enter(self) -> None:
        self.entered = True
        self.last_cmd = None

    def run(self) -> JointCmd:
        raise NotImplementedError

    def exit(self) -> None:
        self.entered = False

    def check_transition(self) -> StateID | None:
        return None

    def _send(self, cmd: JointCmd) -> JointCmd:
        self.last_cmd = cmd
        self.sim.set_joint_pd(cmd)
        return cmd

    def __repr__(self) -> str:
        return f"<{type(self).__name__} {self.id.value}>"


class Passive(State):
    """Damping mode: zero feed-forward and zero position gain."""

    def __init__(self, context: ControlContext) -> None:
        super().__init__(context, StateID.PASSIVE)

    def run(self) -> JointCmd:
        cmd = JointCmd(np.zeros(12), np.zeros(12), np.zeros(12),
                       self.context.passive_kd, np.zeros(12),
                       mode=np.full(12, int(MotorMode.SERVO)))
        return self._send(cmd)


class _InterpolatingState(State):
    """Shared linear interpolation helper for stand/down transitions."""

    duration: float = 1.5
    target: np.ndarray
    kp: np.ndarray
    kd: np.ndarray

    def __init__(self, context: ControlContext, state_id: StateID,
                 target: np.ndarray | None = None, duration: float | None = None,
                 kp: np.ndarray | None = None, kd: np.ndarray | None = None) -> None:
        super().__init__(context, state_id)
        self._target_arg = None if target is None else np.asarray(
            target, dtype=float).reshape(12)
        self.duration = float(self.duration if duration is None else duration)
        self._kp_arg = None if kp is None else np.asarray(kp, dtype=float).reshape(12)
        self._kd_arg = None if kd is None else np.asarray(kd, dtype=float).reshape(12)
        self.q_start = np.zeros(12)
        self.q_des = np.zeros(12)
        self.elapsed = 0.0
        self.done = False

    def enter(self) -> None:
        super().enter()
        self.q_start = np.asarray(self.sim.joint_positions, dtype=float).copy()
        self.q_des = self.q_start.copy()
        self.elapsed = 0.0
        self.done = False

    @property
    def q_target(self) -> np.ndarray:
        raise NotImplementedError

    @property
    def gains(self) -> tuple[np.ndarray, np.ndarray]:
        raise NotImplementedError

    def run(self) -> JointCmd:
        self.elapsed += self.context.dt
        if self.duration <= 0:
            alpha = 1.0
        else:
            alpha = min(1.0, self.elapsed / self.duration)
        self.q_des = (1.0 - alpha) * self.q_start + alpha * self.q_target
        self.done = alpha >= 1.0
        kp, kd = self.gains
        cmd = JointCmd(self.q_des.copy(), np.zeros(12), kp, kd, np.zeros(12),
                       mode=np.full(12, int(MotorMode.SERVO)))
        return self._send(cmd)


class FixedStand(_InterpolatingState):
    """Interpolate to the A1 home pose and hold it.

    The state records the joint angles at entry and linearly interpolates to
    the stand target.  It can be left as soon as the interpolation has
    finished; there is no automatic transition, so the caller keeps control.
    """

    duration = 1.5
    target = None

    def __init__(self, context: ControlContext, target: np.ndarray | None = None,
                 duration: float = 1.5, kp=None, kd=None) -> None:
        super().__init__(context, StateID.FIXED_STAND, target, duration, kp, kd)

    @property
    def q_target(self) -> np.ndarray:
        if self._target_arg is not None:
            return self._target_arg
        return self.context.q_stand

    @property
    def gains(self) -> tuple[np.ndarray, np.ndarray]:
        kp = self.context.kp_stand if self._kp_arg is None else self._kp_arg
        kd = self.context.kd_stand if self._kd_arg is None else self._kd_arg
        return kp, kd

    def check_transition(self) -> StateID | None:
        # The FSM's external request can still switch earlier.  This hook is
        # useful for chapter examples that want to leave only after settling.
        return None


class FixedDown(_InterpolatingState):
    """Interpolate to a folded, low pose (the ``FixedDown`` state)."""

    duration = 1.2

    def __init__(self, context: ControlContext, target: np.ndarray | None = None,
                 duration: float = 1.2, kp=None, kd=None) -> None:
        super().__init__(context, StateID.FIXED_DOWN, target, duration, kp, kd)

    @property
    def q_target(self) -> np.ndarray:
        if self._target_arg is not None:
            return self._target_arg
        return self.context.q_down

    @property
    def gains(self) -> tuple[np.ndarray, np.ndarray]:
        kp = self.context.kp_down if self._kp_arg is None else self._kp_arg
        kd = self.context.kd_down if self._kd_arg is None else self._kd_arg
        return kp, kd


class FSM:
    """State container with two-phase transitions."""

    def __init__(self, context: ControlContext, states=None,
                 initial: StateID | str = StateID.PASSIVE) -> None:
        self.context = context
        if states is None:
            states = [Passive(context), FixedStand(context), FixedDown(context)]
        self.states: dict[StateID, State] = {}
        for state in states:
            self.states[state.id] = state
        self.current = StateID.coerce(initial)
        if self.current not in self.states:
            raise KeyError(f"initial state {self.current} is not registered")
        self._pending: StateID | None = None
        self.states[self.current].enter()
        self.cycle = 0

    @classmethod
    def default(cls, sim, params=None, initial: StateID | str = StateID.PASSIVE,
                **kwargs) -> "FSM":
        context = ControlContext.from_sim(sim, params=params, **kwargs)
        return cls(context, initial=initial)

    @property
    def state(self) -> StateID:
        return self.current

    @property
    def current_state(self) -> State:
        return self.states[self.current]

    def add(self, state: State) -> None:
        self.states[state.id] = state

    def request(self, target: StateID | str) -> None:
        """Register a transition for the next control cycle."""
        target = StateID.coerce(target)
        if target not in self.states:
            raise KeyError(f"state {target} is not registered")
        self._pending = target

    switch_to = request

    def _switch(self, target: StateID) -> None:
        self.states[self.current].exit()
        self.current = target
        self.states[self.current].enter()

    def step(self) -> JointCmd:
        """Run one 2 ms control cycle.

        The returned command has already been handed to the simulator through
        ``sim.set_joint_pd``; the caller advances physics with ``sim.step()``.
        """
        if self._pending is not None:
            target = self._pending
            self._pending = None
            if target != self.current:
                self._switch(target)
        cmd = self.states[self.current].run()
        nxt = self.states[self.current].check_transition()
        if nxt is not None:
            nxt = StateID.coerce(nxt)
            if nxt != self.current:
                self._pending = nxt
        self.cycle += 1
        return cmd


__all__ = [
    "StateID", "ControlContext", "State", "Passive", "FixedStand",
    "FixedDown", "FSM",
]
