"""Pack, parse and corrupt one command/state pair (section 2.7).

Both frames use the book package's encoders, which follow T01 and T02 field
by field (for the state frame: LW at bytes 17-20, Pos at 31-34, gyro at 39-44).

expect: 命令帧
expect: 状态帧
expect: CRC 校验通过
expect: 位翻转
"""

from __future__ import annotations

import logging
import math
import struct

from quadruped.motor import (MotorCmd, MotorState, crc32, decode_master_cmd, decode_servo_state, encode_servo_state,
                             encode_master_cmd)
from quadruped.plotting import savefig
import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch, Rectangle

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

TAU_SCALE = 256.0
DQ_SCALE = 128.0
Q_SCALE = 16384.0 / (2.0 * math.pi)
KP_SCALE = 2048.0
KD_SCALE = 1024.0
GYRO_SCALE = (2000.0 / 32768.0) * (math.pi / 180.0)
ACC_SCALE = 8.0 * 9.80665 / 32768.0


def pack_state_t02(s: MotorState) -> bytes:
    """State frame laid out as 原书表 2.2 (T02); the package encoder follows the table field by field."""
    return encode_servo_state(s)


def unpack_state_t02(raw: bytes) -> MotorState:
    """Check head, length and CRC, then read the fields at the T02 offsets."""
    return decode_servo_state(raw)


def hex_dump(raw: bytes) -> None:
    for off in range(0, len(raw), 16):
        chunk = raw[off:off + 16]
        print(f"  {off:02d}: " + " ".join(f"{b:02X}" for b in chunk))


def draw_frames() -> None:
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10.8, 6.3))

    palette = ["#90a4ae", "#bbdefb", "#e1bee7", "#ffcc80", "#a5d6a7",
               "#f48fb1", "#cfd8dc", "#ef9a9a", "#b39ddb"]

    def bar(ax, groups, total, title):
        ax.set_xlim(0, total)
        ax.set_ylim(0, 3.3)
        ax.axis("off")
        y, h = 0.95, 0.55
        for i, (start, end, text) in enumerate(groups):
            ax.add_patch(Rectangle((start, y), end - start, h,
                                   facecolor=palette[i % len(palette)],
                                   edgecolor="white", lw=1.0))
            mid = (start + end) / 2
            ax.plot([mid, mid], [y + h, y + h + 0.12], color="0.4", lw=0.7)
            ax.text(mid, y + h + 0.18 + 0.50 * (i % 2), text, ha="center",
                    va="bottom", fontsize=6.4)
            ax.text(start, y - 0.08, str(start + 1), ha="center", va="top",
                    fontsize=6.2, color="0.4")
        ax.text(total, y - 0.08, str(total), ha="center", va="top", fontsize=6.2,
                color="0.4")
        ax.text(total / 2, 2.95, title, ha="center", fontsize=10)

    bar(ax1, [
        (0, 4, "包头 4"), (4, 8, "mode / 标志 4"), (8, 12, "Modify 4"),
        (12, 14, "T 2"), (14, 16, "W 2"), (16, 20, "Pos 4"),
        (20, 22, "K_P 2"), (22, 24, "K_W 2"), (24, 26, "低频 2"),
        (26, 30, "Res 4"), (30, 34, "CRC 4"),
    ], 34, "命令帧 34 字节（T01）：包头 + 26 字节数据体 + CRC")

    bar(ax2, [
        (0, 4, "包头 4"), (4, 8, "mode/标志 4"), (8, 12, "Read 4"),
        (12, 14, "T 2"), (14, 16, "W 2"), (16, 20, "LW 4"),
        (20, 22, "W2 2"), (22, 26, "LW2 4"), (26, 28, "Acc 2"),
        (28, 30, "OutAcc 2"), (30, 34, "Pos 4"), (34, 38, "Pos2 4"),
        (38, 44, "gyro 6"), (44, 50, "acc 6"), (50, 68, "足端 IMU 18"),
        (68, 74, "足端力/温度 6"), (74, 78, "CRC 4"),
    ], 78, "状态帧 78 字节（T02）：包头 + 70 字节数据体 + CRC")
    fig.tight_layout()
    savefig(fig, "2_7_frames")


def draw_wiring() -> None:
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10.4, 4.2),
                                   gridspec_kw={"width_ratios": [1.0, 1.2]})
    ax1.set_xlim(0, 6)
    ax1.set_ylim(0, 6)
    ax1.axis("off")
    ax1.add_patch(FancyBboxPatch((0.7, 1.2), 4.6, 3.6,
                                 boxstyle="round,pad=0.08", facecolor="#eceff1",
                                 edgecolor="0.3", lw=1.2))
    ax1.text(3.0, 4.45, "RS-485 插座（局部放大）", ha="center", fontsize=9)
    for x, name, color in ((1.8, "B", "#1565c0"), (3.0, "A", "#c62828"),
                           (4.2, "GND", "0.3")):
        ax1.add_patch(Rectangle((x - 0.25, 2.4), 0.5, 0.9, facecolor=color,
                                edgecolor="0.2"))
        ax1.text(x, 2.05, name, ha="center", fontsize=10)
        ax1.text(x, 3.55, "●", ha="center", fontsize=9, color="0.4")
    ax1.text(3.0, 1.55, "从电机端看：左→右为 B、A、GND", ha="center", fontsize=8,
             color="0.3")

    ax2.set_xlim(0, 10)
    ax2.set_ylim(0, 6)
    ax2.axis("off")
    ax2.add_patch(FancyBboxPatch((0.2, 2.0), 2.3, 1.9,
                                 boxstyle="round,pad=0.05", facecolor="#e3f2fd",
                                 edgecolor="0.25"))
    ax2.text(1.35, 2.95, "上位机 /\nUSB-RS485", ha="center", fontsize=8)
    ax2.add_patch(FancyBboxPatch((6.8, 2.0), 2.6, 1.9,
                                 boxstyle="round,pad=0.05", facecolor="#e8f5e9",
                                 edgecolor="0.25"))
    ax2.text(8.1, 2.95, "关节电机", ha="center", fontsize=9)
    for y, name, color in ((3.55, "A", "#c62828"), (3.1, "B", "#1565c0"),
                           (2.5, "GND", "0.3")):
        ax2.plot([2.5, 6.8], [y, y], color=color, lw=1.2)
        ax2.text(4.6, y + 0.10, f"{name} — {name}", ha="center", fontsize=8,
                 color=color)
    ax2.text(1.35, 1.45, "A 对 A、B 对 B、GND 对 GND", ha="center", fontsize=8,
             color="0.3")
    ax2.text(8.1, 1.45, "屏蔽双绞线，尽量短", ha="center", fontsize=8, color="0.3")
    ax2.set_title("接线：不要按颜色猜 A/B")
    fig.tight_layout()
    savefig(fig, "2_7_wiring")


def main() -> None:
    cmd = MotorCmd(mode=10, motor_id=0, q_des=3.0, dq_des=1.2,
                   kp=10.0, kd=1.0, tau_ff=0.5)
    frame = encode_master_cmd(cmd)
    print(f"命令帧（{len(frame)} 字节）：")
    hex_dump(frame)
    print("小端字段：")
    print(f"  T  偏移 12-13: {int.from_bytes(frame[12:14], 'little', signed=True):6d}"
          f"  → {int.from_bytes(frame[12:14], 'little', signed=True) / TAU_SCALE:.6f} N·m")
    print(f"  W  偏移 14-15: {int.from_bytes(frame[14:16], 'little', signed=True):6d}"
          f"  → {int.from_bytes(frame[14:16], 'little', signed=True) / DQ_SCALE:.6f} rad/s")
    print(f"  Pos偏移 16-19: {int.from_bytes(frame[16:20], 'little', signed=True):6d}"
          f"  → {int.from_bytes(frame[16:20], 'little', signed=True) / Q_SCALE:.6f} rad")
    print(f"  K_P偏移 20-21: {int.from_bytes(frame[20:22], 'little', signed=True):6d}"
          f"  → {int.from_bytes(frame[20:22], 'little', signed=True) / KP_SCALE:.6f} N·m/rad")
    print(f"  K_W偏移 22-23: {int.from_bytes(frame[22:24], 'little', signed=True):6d}"
          f"  → {int.from_bytes(frame[22:24], 'little', signed=True) / KD_SCALE:.6f} N·m·s/rad")
    back = decode_master_cmd(frame)
    print(f"解码命令：mode={back.mode}, T={back.tau_ff:.6f}, W={back.dq_des:.6f}, "
          f"Pos={back.q_des:.6f}, K_P={back.kp:.6f}, K_W={back.kd:.6f}")

    over = encode_master_cmd(MotorCmd(mode=10, motor_id=0, kp=20.0))
    raw_kp = int.from_bytes(over[20:22], "little", signed=True)
    print(f"越界演示：K_P=20 乘 2048 得 40960，超出 16 位有符号正数上限，"
          f"帧内饱和为 {raw_kp}（{raw_kp / KP_SCALE:.4f} N·m/rad）")

    state = MotorState(q=1.2345, dq=-2.5, tau=0.75, temperature=42,
                       error=0, mode=10, motor_id=2,
                       gyro=[0.1, -0.2, 0.3], acc=[0.0, 0.0, 9.8])
    raw = pack_state_t02(state)
    print(f"\n状态帧（{len(raw)} 字节）：")
    hex_dump(raw)
    print(f"CRC32 = 0x{struct.unpack_from('<I', raw, 74)[0]:08X}（覆盖前 72 字节）")
    out = unpack_state_t02(raw)
    print("解码状态：")
    print(f"  q={out.q:.6f} rad, dq={out.dq:.6f} rad/s, tau={out.tau:.6f} N·m")
    print(f"  T={out.temperature} °C, MError={out.error}, "
          f"gyro=[{out.gyro[0]:.4f}, {out.gyro[1]:.4f}, {out.gyro[2]:.4f}] rad/s")
    print(f"  acc=[{out.acc[0]:.4f}, {out.acc[1]:.4f}, {out.acc[2]:.4f}] m/s²")
    print("CRC 校验通过")

    print("\n压缩/反压缩误差：")
    for name, original, decoded, res in (
            ("T", cmd.tau_ff, back.tau_ff, 1.0 / TAU_SCALE),
            ("W", cmd.dq_des, back.dq_des, 1.0 / DQ_SCALE),
            ("Pos", cmd.q_des, back.q_des, 2.0 * math.pi / 16384.0),
            ("K_P", cmd.kp, back.kp, 1.0 / KP_SCALE),
            ("K_W", cmd.kd, back.kd, 1.0 / KD_SCALE)):
        print(f"  {name:3s}: {original:.6f} → {decoded:.6f}, "
              f"误差 {decoded - original:+.2e}, 分辨率 {res:.3e}")
    for name, original, decoded in (("q", state.q, out.q), ("dq", state.dq, out.dq),
                                    ("tau", state.tau, out.tau)):
        print(f"  {name:3s}: {original:.6f} → {decoded:.6f}, "
              f"误差 {decoded - original:+.2e}")

    bad = bytearray(raw)
    bad[30] ^= 0x01               # one bit in the Pos payload
    try:
        unpack_state_t02(bytes(bad))
    except ValueError as exc:
        print(f"\n位翻转（状态帧第 31 字节）: {exc}，帧被丢弃")

    bad_cmd = bytearray(frame)
    bad_cmd[12] ^= 0x01            # one bit in T
    try:
        decode_master_cmd(bytes(bad_cmd))
    except ValueError as exc:
        print(f"位翻转（命令帧第 13 字节）: CRC 校验失败（{exc}），命令被丢弃")

    draw_frames()
    draw_wiring()


if __name__ == "__main__":
    main()
