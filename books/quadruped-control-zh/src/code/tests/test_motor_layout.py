"""The state frame layout must match 原书表 2.2 (notes/tables/tables.md, T02), field by field."""
import struct

import numpy as np

from quadruped.motor import MotorState, encode_servo_state


def test_state_frame_offsets_match_table_2_2():
    st = MotorState(q=1.25, dq=-3.5, tau=2.0, temperature=40, error=0, mode=10, motor_id=1, read_bit=0,
                    gyro=np.array([0.1, -0.2, 0.3]), acc=np.array([1.0, -2.0, 9.8]))
    raw = encode_servo_state(st)
    # table bytes are 1-based: Pos 31-34, gyro[0] 39-40, acc[0] 45-46, LW 17-20 (float)
    assert abs(struct.unpack_from("<i", raw, 31 - 1)[0] / (16384 / (2 * np.pi)) - 1.25) < 1e-3
    assert abs(struct.unpack_from("<f", raw, 17 - 1)[0] - (-3.5)) < 1e-6
    gyro0 = struct.unpack_from("<h", raw, 39 - 1)[0] * 2000 / 2**15 * 2 * np.pi / 360
    acc2 = struct.unpack_from("<h", raw, 49 - 1)[0] * 8 * 9.80665 / 2**15
    assert abs(gyro0 - 0.1) < 2e-3 and abs(acc2 - 9.8) < 3e-3
