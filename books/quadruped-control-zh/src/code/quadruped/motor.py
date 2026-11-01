"""Joint-motor model, hybrid control and the A1 RS-485 frame format.

The first half of this module is hardware independent: :class:`MotorCmd` and
:class:`MotorState` describe one drive, :func:`hybrid_torque` implements
Eq. (2.1), and :class:`Motor` is a small single-joint plant used by the motor
chapter's examples.

The second half implements the 34-byte command frame and the 78-byte state
frame of tables T01/T02, including the fixed-point compression and the
word-oriented CRC32 used by the A1 firmware.  :class:`Rs485MotorBus` only
builds and parses byte strings; opening a real serial port is deliberately
left as an extension point and is not exercised on this machine.
"""

from __future__ import annotations

import math
import struct
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from typing import Iterable, Sequence

import numpy as np

from .types import JointCmd, JointState, MotorMode

# ---------------------------------------------------------------------------
# Frame constants
# ---------------------------------------------------------------------------
CMD_LEN = 34
STATE_LEN = 78
HEAD0 = 0xFE
HEAD1 = 0xEE
BROADCAST_ID = 0xBB
CRC32_POLY = 0x04C11DB7

TAU_SCALE = 256.0
DQ_SCALE = 128.0
Q_SCALE = 16384.0 / (2.0 * math.pi)
KP_SCALE = 2048.0
KD_SCALE = 1024.0
GYRO_SCALE = (2000.0 / 32768.0) * (math.pi / 180.0)
ACC_SCALE = 8.0 * 9.80665 / 32768.0


# ---------------------------------------------------------------------------
# Control law and data structures
# ---------------------------------------------------------------------------

def hybrid_torque(q: np.ndarray, dq: np.ndarray,
                  cmd: JointCmd) -> np.ndarray:
    """Evaluate ``tau = tau_ff + kp (q_des - q) + kd (dq_des - dq)``.

    Only ``SERVO`` mode produces feedback torque.  ``STOP`` and ``OPEN`` are
    represented by zero torque in this software model; the real drive's open
    loop is a firmware function, not a Python control law.
    """
    q = np.asarray(q, dtype=float).reshape(-1)
    dq = np.asarray(dq, dtype=float).reshape(-1)
    tau = (np.asarray(cmd.tau_ff, dtype=float)
           + np.asarray(cmd.kp, dtype=float) * (np.asarray(cmd.q_des) - q)
           + np.asarray(cmd.kd, dtype=float) * (np.asarray(cmd.dq_des) - dq))
    servo = np.asarray(cmd.mode, dtype=int) == int(MotorMode.SERVO)
    return np.where(servo, tau, 0.0)


@dataclass
class MotorCmd:
    """One motor command, with physical (not fixed-point) units."""

    q_des: float = 0.0
    dq_des: float = 0.0
    kp: float = 0.0
    kd: float = 0.0
    tau_ff: float = 0.0
    mode: int = int(MotorMode.SERVO)
    motor_id: int = 0
    modify: int = 0
    modify_bit: int = 0xFF
    read_bit: int = 0xFF
    low_hz_index: int = 0
    low_hz_byte: int = 0
    reserved: bytes = b"\x00\x00\x00\x00"

    def __post_init__(self) -> None:
        self.mode = int(self.mode)
        self.motor_id = int(self.motor_id)
        if not 0 <= self.motor_id <= 255:
            raise ValueError("motor_id must fit in one byte")
        if len(self.reserved) != 4:
            raise ValueError("reserved must be 4 bytes")

    def copy(self) -> "MotorCmd":
        return MotorCmd(**{**self.__dict__})


@dataclass
class MotorState:
    """One motor state, with physical units."""

    q: float = 0.0
    dq: float = 0.0
    tau: float = 0.0
    temperature: float = 25.0
    error: int = 0
    mode: int = int(MotorMode.SERVO)
    motor_id: int = 0
    read_bit: int = 0
    gyro: np.ndarray = field(default_factory=lambda: np.zeros(3))
    acc: np.ndarray = field(default_factory=lambda: np.zeros(3))
    timestamp: float = 0.0

    def __post_init__(self) -> None:
        self.gyro = np.asarray(self.gyro, dtype=float).reshape(3)
        self.acc = np.asarray(self.acc, dtype=float).reshape(3)

    def copy(self) -> "MotorState":
        return MotorState(self.q, self.dq, self.tau, self.temperature,
                          self.error, self.mode, self.motor_id, self.read_bit,
                          self.gyro.copy(), self.acc.copy(), self.timestamp)


class Motor:
    """A minimal single-joint plant for motor-mode demonstrations."""

    def __init__(self, inertia: float = 0.02, damping: float = 0.2,
                 tau_max: float = 33.5, q: float = 0.0,
                 dq: float = 0.0) -> None:
        self.inertia = float(inertia)
        self.damping = float(damping)
        self.tau_max = float(tau_max)
        self.q = float(q)
        self.dq = float(dq)
        self.tau = 0.0
        self.temperature = 25.0
        self.error = 0
        self._cmd = MotorCmd()

    def apply(self, cmd: MotorCmd) -> float:
        self._cmd = cmd
        if int(cmd.mode) != int(MotorMode.SERVO):
            self.tau = 0.0
        else:
            self.tau = float(cmd.tau_ff + cmd.kp * (cmd.q_des - self.q)
                             + cmd.kd * (cmd.dq_des - self.dq))
        self.tau = max(-self.tau_max, min(self.tau_max, self.tau))
        return self.tau

    def step(self, dt: float = 0.002) -> MotorState:
        load = -self.damping * self.dq
        acc = (self.tau + load) / max(self.inertia, 1e-9)
        self.dq += acc * dt
        self.q += self.dq * dt
        return self.state()

    def state(self) -> MotorState:
        return MotorState(q=self.q, dq=self.dq, tau=self.tau,
                          temperature=self.temperature, error=self.error,
                          mode=self._cmd.mode, motor_id=self._cmd.motor_id)


# ---------------------------------------------------------------------------
# CRC32 (the word-oriented routine used by the A1 firmware)
# ---------------------------------------------------------------------------

def crc32_core(words: Sequence[int] | np.ndarray, length: int | None = None) -> int:
    """CRC32 over 32-bit words, exactly as in the reference description.

    The polynomial is ``0x04C11DB7``, the initial value is ``0xFFFFFFFF`` and
    there is no final XOR.  For each word the 32 bits are processed from the
    most significant bit to the least significant bit.  On a little-endian
    host this means that, for a byte buffer, each four-byte word is consumed
    from its highest-addressed byte down to its lowest-addressed byte.
    """
    if length is None:
        length = len(words)
    crc = 0xFFFFFFFF
    for i in range(int(length)):
        data = int(words[i]) & 0xFFFFFFFF
        xbit = 1 << 31
        for _ in range(32):
            if crc & 0x80000000:
                crc = ((crc << 1) & 0xFFFFFFFF) ^ CRC32_POLY
            else:
                crc = (crc << 1) & 0xFFFFFFFF
            if data & xbit:
                crc ^= CRC32_POLY
            xbit >>= 1
    return crc & 0xFFFFFFFF


def crc32(data: bytes | bytearray | memoryview, nwords: int | None = None) -> int:
    """CRC over the first ``nwords`` little-endian 32-bit words of ``data``.

    Bytes beyond ``nwords * 4`` are ignored.  If fewer bytes are available the
    missing tail is zero-padded, which makes the helper usable for the 72-byte
    state coverage of T02 as well as the 28-byte command coverage.
    """
    raw = bytes(data)
    if nwords is None:
        nwords = len(raw) // 4
    need = int(nwords) * 4
    raw = (raw[:need] + b"\x00" * max(0, need - len(raw)))
    words = np.frombuffer(raw, dtype="<u4", count=int(nwords))
    return crc32_core(words, int(nwords))


# ---------------------------------------------------------------------------
# Fixed-point helpers
# ---------------------------------------------------------------------------

def _to_i16(value: float, scale: float) -> int:
    # The drive truncates toward zero; saturate instead of wrapping.
    raw = int(float(value) * scale)
    return max(-32768, min(32767, raw))


def _to_i32(value: float, scale: float) -> int:
    raw = int(float(value) * scale)
    return max(-2147483648, min(2147483647, raw))


def _u8(value: float) -> int:
    return max(0, min(255, int(value)))


# ---------------------------------------------------------------------------
# Command frame (table T01, 34 bytes)
# ---------------------------------------------------------------------------

def encode_master_cmd(cmd: MotorCmd) -> bytes:
    """Pack one command into the 34-byte frame of T01.

    Multi-byte fields are little-endian.  CRC32 covers bytes 1-28 (the first
    seven 32-bit words); the last two reserved bytes are outside the CRC
    range, matching the reference description.
    """
    buf = bytearray(CMD_LEN)
    buf[0] = HEAD0
    buf[1] = HEAD1
    buf[2] = _u8(cmd.motor_id)
    buf[3] = 0
    buf[4] = _u8(cmd.mode)
    buf[5] = _u8(cmd.modify_bit)
    buf[6] = _u8(cmd.read_bit)
    buf[7] = 0
    struct.pack_into("<I", buf, 8, int(cmd.modify) & 0xFFFFFFFF)
    struct.pack_into("<h", buf, 12, _to_i16(cmd.tau_ff, TAU_SCALE))
    struct.pack_into("<h", buf, 14, _to_i16(cmd.dq_des, DQ_SCALE))
    struct.pack_into("<i", buf, 16, _to_i32(cmd.q_des, Q_SCALE))
    struct.pack_into("<h", buf, 20, _to_i16(cmd.kp, KP_SCALE))
    struct.pack_into("<h", buf, 22, _to_i16(cmd.kd, KD_SCALE))
    buf[24] = _u8(cmd.low_hz_index)
    buf[25] = _u8(cmd.low_hz_byte)
    buf[26:30] = bytes(cmd.reserved)
    struct.pack_into("<I", buf, 30, crc32(buf, 7))
    return bytes(buf)


def decode_master_cmd(data: bytes | bytearray | memoryview) -> MotorCmd:
    """Parse and validate a 34-byte command frame."""
    raw = bytes(data)
    if len(raw) != CMD_LEN:
        raise ValueError(f"command frame must be {CMD_LEN} bytes, got {len(raw)}")
    if raw[0] != HEAD0 or raw[1] != HEAD1:
        raise ValueError("bad command frame header")
    stored = struct.unpack_from("<I", raw, 30)[0]
    if crc32(raw, 7) != stored:
        raise ValueError("command frame CRC mismatch")
    modify = struct.unpack_from("<I", raw, 8)[0]
    return MotorCmd(
        q_des=struct.unpack_from("<i", raw, 16)[0] / Q_SCALE,
        dq_des=struct.unpack_from("<h", raw, 14)[0] / DQ_SCALE,
        kp=struct.unpack_from("<h", raw, 20)[0] / KP_SCALE,
        kd=struct.unpack_from("<h", raw, 22)[0] / KD_SCALE,
        tau_ff=struct.unpack_from("<h", raw, 12)[0] / TAU_SCALE,
        mode=raw[4],
        motor_id=raw[2],
        modify=modify,
        modify_bit=raw[5],
        read_bit=raw[6],
        low_hz_index=raw[24],
        low_hz_byte=raw[25],
        reserved=raw[26:30],
    )


# ---------------------------------------------------------------------------
# State frame (table T02, 78 bytes)
# ---------------------------------------------------------------------------

def encode_servo_state(state: MotorState) -> bytes:
    """Pack one state into the 78-byte frame of T02.

    The word-oriented CRC covers the first 72 bytes (18 words), i.e. through
    the end of the first reserved field.  The two remaining bytes before the
    CRC (the tail of ``Res[0]`` and padding) are not covered by the 32-bit
    word routine.
    """
    buf = bytearray(STATE_LEN)
    buf[0] = HEAD0
    buf[1] = HEAD1
    buf[2] = _u8(state.motor_id)
    buf[3] = 0
    buf[4] = _u8(state.mode)
    buf[5] = _u8(state.read_bit)
    buf[6] = _u8(state.temperature)
    buf[7] = _u8(state.error)
    struct.pack_into("<i", buf, 8, 0)            # Read
    struct.pack_into("<h", buf, 12, _to_i16(state.tau, TAU_SCALE))
    struct.pack_into("<h", buf, 14, _to_i16(state.dq, DQ_SCALE))
    # Offsets follow 原书表 2.2 (bytes 1-based there): LW is a 4-byte float at 17-20, so
    # everything after it sits 2 bytes later than a 2-byte LW would put it.
    struct.pack_into("<f", buf, 16, float(state.dq))  # LW (filtered speed, float)
    struct.pack_into("<h", buf, 20, 0)           # W2
    struct.pack_into("<f", buf, 22, 0.0)         # LW2
    struct.pack_into("<h", buf, 26, 0)           # Acc
    struct.pack_into("<h", buf, 28, 0)           # OutAcc
    struct.pack_into("<i", buf, 30, _to_i32(state.q, Q_SCALE))
    struct.pack_into("<i", buf, 34, 0)           # Pos2
    for i in range(3):
        struct.pack_into("<h", buf, 38 + 2 * i,
                         _to_i16(state.gyro[i], 1.0 / GYRO_SCALE))
        struct.pack_into("<h", buf, 44 + 2 * i,
                         _to_i16(state.acc[i], 1.0 / ACC_SCALE))
    # Fgyro, Facc, Fmag, Ftemp, Force16, Force8, FError, Res stay zero.
    buf[71] = 0
    struct.pack_into("<I", buf, 74, crc32(buf, 18))
    return bytes(buf)


def decode_servo_state(data: bytes | bytearray | memoryview) -> MotorState:
    """Parse and validate a 78-byte state frame."""
    raw = bytes(data)
    if len(raw) != STATE_LEN:
        raise ValueError(f"state frame must be {STATE_LEN} bytes, got {len(raw)}")
    if raw[0] != HEAD0 or raw[1] != HEAD1:
        raise ValueError("bad state frame header")
    stored = struct.unpack_from("<I", raw, 74)[0]
    if crc32(raw, 18) != stored:
        raise ValueError("state frame CRC mismatch")
    gyro = np.array([struct.unpack_from("<h", raw, 38 + 2 * i)[0] * GYRO_SCALE
                     for i in range(3)])
    acc = np.array([struct.unpack_from("<h", raw, 44 + 2 * i)[0] * ACC_SCALE
                    for i in range(3)])
    return MotorState(
        q=struct.unpack_from("<i", raw, 30)[0] / Q_SCALE,
        dq=struct.unpack_from("<h", raw, 14)[0] / DQ_SCALE,
        tau=struct.unpack_from("<h", raw, 12)[0] / TAU_SCALE,
        temperature=raw[6],
        error=raw[7],
        mode=raw[4],
        motor_id=raw[2],
        read_bit=raw[5],
        gyro=gyro,
        acc=acc,
    )


# ---------------------------------------------------------------------------
# Buses
# ---------------------------------------------------------------------------

class MotorBus(ABC):
    """Common interface for simulation and RS-485 motor backends."""

    @abstractmethod
    def send(self, cmd) -> None:
        """Send one 12-joint command."""

    @abstractmethod
    def recv(self):
        """Return the latest 12-joint state."""

    def close(self) -> None:
        pass


class SimMotorBus(MotorBus):
    """Adapter from the motor interface to :class:`quadruped.sim.A1Sim`."""

    def __init__(self, sim) -> None:
        self.sim = sim

    def send(self, cmd) -> None:
        self.sim.set_joint_pd(cmd)

    def recv(self) -> JointState:
        return self.sim.read_sensors().joints

    def close(self) -> None:
        pass


@dataclass
class MotorBusConfig:
    port: str = "/dev/ttyUSB0"
    baudrate: int = 4_800_000
    bytesize: int = 8
    parity: str = "N"
    stopbits: int = 1
    timeout: float = 0.01
    motor_ids: tuple[int, ...] = (0, 1, 2)


class Rs485MotorBus(MotorBus):
    """Frame builder/parser for a real A1 bus.

    ``encode_*``/``decode_*`` are complete and unit-testable without hardware.
    ``open``/``send``/``recv`` are the extension points for a serial backend;
    they are intentionally not exercised on this machine.
    """

    def __init__(self, config: MotorBusConfig | None = None) -> None:
        self.config = config or MotorBusConfig()
        self._serial = None

    # -- pure functions ---------------------------------------------------
    @staticmethod
    def encode(cmd: MotorCmd) -> bytes:
        return encode_master_cmd(cmd)

    @staticmethod
    def decode(data: bytes | bytearray | memoryview) -> MotorState:
        return decode_servo_state(data)

    def encode_many(self, cmds: Iterable[MotorCmd]) -> list[bytes]:
        return [encode_master_cmd(c) for c in cmds]

    def decode_many(self, frames: Iterable[bytes]) -> list[MotorState]:
        return [decode_servo_state(f) for f in frames]

    # -- hardware extension points ---------------------------------------
    def open(self) -> None:
        raise NotImplementedError(
            "opening an RS-485 port is a hardware step; use encode/decode "
            "or a platform-specific serial implementation")

    def send(self, cmd) -> None:
        if self._serial is None:
            raise RuntimeError("RS-485 backend is not open; use encode_master_cmd")
        if isinstance(cmd, MotorCmd):
            self._serial.write(encode_master_cmd(cmd))
        else:
            for c in cmd:
                self._serial.write(encode_master_cmd(c))

    def recv(self):
        if self._serial is None:
            raise RuntimeError("RS-485 backend is not open; use decode_servo_state")
        data = self._serial.read(STATE_LEN)
        return decode_servo_state(data)

    def close(self) -> None:
        if self._serial is not None:
            self._serial.close()
            self._serial = None


def joint_cmd_to_motor_cmds(cmd: JointCmd) -> list[MotorCmd]:
    """Split a 12-joint command into twelve :class:`MotorCmd` objects."""
    out = []
    for i in range(12):
        mode = int(np.asarray(cmd.mode).reshape(-1)[i])
        out.append(MotorCmd(
            q_des=float(cmd.q_des[i]), dq_des=float(cmd.dq_des[i]),
            kp=float(cmd.kp[i]), kd=float(cmd.kd[i]),
            tau_ff=float(cmd.tau_ff[i]), mode=mode, motor_id=i,
        ))
    return out


__all__ = [
    "CMD_LEN", "STATE_LEN", "CRC32_POLY", "MotorCmd", "MotorState", "Motor",
    "MotorBus", "SimMotorBus", "Rs485MotorBus", "MotorBusConfig",
    "hybrid_torque", "crc32_core", "crc32", "encode_master_cmd",
    "decode_master_cmd", "encode_servo_state", "decode_servo_state",
    "joint_cmd_to_motor_cmds",
]
