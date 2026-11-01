"""Motor law and RS-485 frame tests."""

from __future__ import annotations

import math

import numpy as np
import pytest

from quadruped.motor import (Motor, MotorCmd, MotorState, Rs485MotorBus,
                             crc32, decode_master_cmd, decode_servo_state,
                             encode_master_cmd, encode_servo_state,
                             hybrid_torque)
from quadruped.types import ControlMode, JointCmd, MotorMode


def _reference_crc(words, nwords):
    """Bit-by-bit transcription of the reference routine (independent copy)."""
    crc = 0xFFFFFFFF
    poly = 0x04C11DB7
    for i in range(nwords):
        data = int(words[i]) & 0xFFFFFFFF
        xbit = 1 << 31
        for _ in range(32):
            if crc & 0x80000000:
                crc = ((crc << 1) & 0xFFFFFFFF) ^ poly
            else:
                crc = (crc << 1) & 0xFFFFFFFF
            if data & xbit:
                crc ^= poly
            xbit >>= 1
    return crc & 0xFFFFFFFF


@pytest.mark.parametrize("nwords", [7, 18])
def test_crc_matches_reference(nwords):
    rng = np.random.default_rng(20)
    raw = rng.integers(0, 256, size=nwords * 4, dtype=np.uint8).tobytes()
    words = np.frombuffer(raw, dtype="<u4")
    assert crc32(raw, nwords) == _reference_crc(words, nwords)


def test_command_roundtrip_and_layout():
    cmd = MotorCmd(q_des=math.pi / 4, dq_des=1.0, kp=10.0, kd=1.0,
                   tau_ff=1.5, mode=int(MotorMode.SERVO), motor_id=1)
    raw = encode_master_cmd(cmd)
    assert len(raw) == 34
    assert raw[:4] == bytes([0xFE, 0xEE, 1, 0])
    assert raw[4] == 10
    assert raw[5] == 0xFF and raw[6] == 0xFF
    # Little-endian fields at the offsets of table T01.
    assert int.from_bytes(raw[12:14], "little", signed=True) == 384
    assert int.from_bytes(raw[14:16], "little", signed=True) == 128
    assert int.from_bytes(raw[16:20], "little", signed=True) == 2048
    assert int.from_bytes(raw[20:22], "little", signed=True) == 20480
    assert int.from_bytes(raw[22:24], "little", signed=True) == 1024
    out = decode_master_cmd(raw)
    assert out.motor_id == 1
    assert out.mode == 10
    assert out.q_des == pytest.approx(math.pi / 4, abs=1e-12)
    assert out.dq_des == pytest.approx(1.0)
    assert out.kp == pytest.approx(10.0)
    assert out.kd == pytest.approx(1.0)
    assert out.tau_ff == pytest.approx(1.5)


def test_command_crc_rejects_bit_flip():
    raw = bytearray(encode_master_cmd(MotorCmd(q_des=0.2, kp=5.0, kd=1.0)))
    raw[16] ^= 0x01
    with pytest.raises(ValueError, match="CRC"):
        decode_master_cmd(raw)


def test_state_roundtrip_and_crc():
    state = MotorState(q=math.pi / 4, dq=0.5, tau=1.0, temperature=30,
                       error=0, mode=int(MotorMode.SERVO), motor_id=2,
                       gyro=[0.1, -0.2, 0.3], acc=[0.0, 0.0, 9.8])
    raw = encode_servo_state(state)
    assert len(raw) == 78
    assert raw[:4] == bytes([0xFE, 0xEE, 2, 0])
    out = decode_servo_state(raw)
    assert out.motor_id == 2
    assert out.mode == 10
    assert out.temperature == 30
    assert out.q == pytest.approx(math.pi / 4, abs=1e-12)
    assert out.dq == pytest.approx(0.5)
    assert out.tau == pytest.approx(1.0)
    assert np.max(np.abs(out.gyro - state.gyro)) < 2e-3
    assert np.max(np.abs(out.acc - state.acc)) < 2e-2
    bad = bytearray(raw)
    bad[28] ^= 0x01
    with pytest.raises(ValueError, match="CRC"):
        decode_servo_state(bad)


def test_hybrid_law_and_modes():
    q = np.zeros(12)
    dq = np.zeros(12)
    cmd = JointCmd(np.full(12, 0.5), np.full(12, 0.2),
                   np.full(12, 10.0), np.full(12, 2.0), np.full(12, 1.0))
    tau = hybrid_torque(q, dq, cmd)
    assert np.allclose(tau, 1.0 + 10.0 * 0.5 + 2.0 * 0.2)

    pos = JointCmd.from_mode(ControlMode.POSITION, q_des=np.ones(12),
                             kp=20.0, kd=3.0)
    assert np.all(pos.dq_des == 0.0)
    assert np.all(pos.tau_ff == 0.0)
    damp = JointCmd.from_mode(ControlMode.DAMPING, kd=2.5)
    assert np.all(damp.kp == 0.0)
    tq = JointCmd.from_mode(ControlMode.TORQUE, tau_ff=2.0)
    assert np.all(tq.kp == 0.0) and np.all(tq.kd == 0.0)
    zero = JointCmd.from_mode(ControlMode.ZERO)
    assert np.all(zero.tau_ff == 0.0)

    stop = JointCmd(np.zeros(12), np.zeros(12), 10.0, 1.0, np.zeros(12),
                    mode=int(MotorMode.STOP))
    assert np.all(hybrid_torque(np.zeros(12), np.zeros(12), stop) == 0.0)


def test_motor_plant_and_rs485_wrapper():
    m = Motor(inertia=0.02, damping=0.1)
    m.apply(MotorCmd(q_des=0.1, kp=10.0, kd=1.0))
    s0 = m.state()
    assert s0.tau > 0.0
    m.step(0.002)
    assert m.q > 0.0

    bus = Rs485MotorBus()
    cmd = MotorCmd(q_des=0.1, kp=5.0, motor_id=0)
    raw = bus.encode(cmd)
    out = decode_master_cmd(raw)
    assert out.q_des == pytest.approx(0.1, abs=1e-3)
    with pytest.raises(RuntimeError):
        bus.send(cmd)
