"""The smallest useful motor program, run against a simulated bus (2.5).

Stage 1: mode=5, open-loop slow rotation for 1 s.
Stage 2: mode=10, hold the current rotor angle with a small PD gain.
Stage 3: mode=0, stop.

Every command is packed into the 34-byte frame and parsed back; every state
is packed into the 78-byte frame and parsed back, so the flow is the same as
with a real adapter.  ``--real --port /dev/ttyUSB0`` is the hardware path.

expect: 阶段 mode=5
expect: 阶段 mode=10
expect: 阶段 mode=0
expect: 实机 mode=5
"""

from __future__ import annotations

import argparse
import logging

import numpy as np

from quadruped.motor import (Motor, MotorBusConfig, MotorCmd, Rs485MotorBus,
                             decode_master_cmd, decode_servo_state,
                             encode_master_cmd, encode_servo_state)
from quadruped.plotting import savefig
import matplotlib.pyplot as plt

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

DT = 0.002
OPEN_LOOP_SPEED = 1.2       # rotor rad/s, the firmware's slow open-loop turn
LOAD = 0.4                  # gravity-like constant load, N m


class LoadMotor(Motor):
    """Motor with a constant opposing load torque."""

    def __init__(self, load: float = LOAD, **kwargs) -> None:
        super().__init__(**kwargs)
        self.load = float(load)

    def step(self, dt: float = DT):
        acc = (self.tau - self.load - self.damping * self.dq) / self.inertia
        self.dq += acc * dt
        self.q += self.dq * dt
        self.temperature += (2.0 * self.tau ** 2
                             - 0.5 * (self.temperature - 25.0)) * dt
        return self.state()


class SimMotorBus:
    """A one-motor bus that still exercises the frame encoder/decoder."""

    def __init__(self, motor: LoadMotor, config: MotorBusConfig) -> None:
        self.motor = motor
        self.config = config
        self.mode = 0
        self.frames_sent = 0

    def send(self, cmd: MotorCmd) -> None:
        raw = encode_master_cmd(cmd)             # 34 bytes on the wire
        parsed = decode_master_cmd(raw)
        self.mode = parsed.mode
        self.frames_sent += 1
        if parsed.mode == 10:
            self.motor.apply(parsed)
        elif parsed.mode == 0:
            self.motor.tau = 0.0

    def step(self) -> None:
        if self.mode == 5:
            self.motor.q += OPEN_LOOP_SPEED * DT
            self.motor.dq = OPEN_LOOP_SPEED
            self.motor.tau = 0.0
            self.motor.temperature += -0.5 * (self.motor.temperature - 25.0) * DT
        elif self.mode == 0:
            self.motor.dq = 0.0
            self.motor.tau = 0.0
        else:
            self.motor.step(DT)

    def recv(self):
        raw = encode_servo_state(self.motor.state())   # 78 bytes on the wire
        return decode_servo_state(raw)


def run(port: str, real: bool):
    if real:
        bus = Rs485MotorBus(MotorBusConfig(port=port))
        try:
            bus.open()
        except NotImplementedError as exc:
            raise SystemExit(f"真实串口尚未在本机接通（{exc}）；请按 2.7 节接好收发后端") from exc
        raise SystemExit("真实串口路径需要在实机上运行，本书示例默认使用 SimMotorBus")

    motor = LoadMotor(inertia=0.02, damping=0.1, tau_max=33.5)
    bus = SimMotorBus(motor, MotorBusConfig(port=port))

    log = {"t": [], "q": [], "dq": [], "tau": [], "temp": []}
    t = 0.0

    def drive_stage(mode: int, duration: float, cmd: MotorCmd, name: str):
        nonlocal t
        n = int(round(duration / DT))
        for _ in range(n):
            bus.send(cmd)
            bus.step()
            state = bus.recv()
            t += DT
            log["t"].append(t)
            log["q"].append(state.q)
            log["dq"].append(state.dq)
            log["tau"].append(state.tau)
            log["temp"].append(state.temperature)
        if mode == 5:
            print("  实机 mode=5 返回全 0；仿真返回真实状态，便于画 q(t)。")
        print(f"阶段 mode={mode}（{name}）: q={state.q:.4f} rad, dq={state.dq:.4f} rad/s, "
              f"tau={state.tau:.4f} N·m, T={motor.temperature:.2f} °C")

    print(f"打开 {port}（模拟）：4.8 Mbit/s, 8N1")
    drive_stage(5, 1.0, MotorCmd(mode=5, motor_id=0), "开环慢转")
    q_hold = motor.q
    drive_stage(10, 1.0,
                MotorCmd(mode=10, motor_id=0, q_des=q_hold, dq_des=0.0,
                         kp=8.0, kd=0.5, tau_ff=0.0),
                "位置保持")
    drive_stage(0, 0.2, MotorCmd(mode=0, motor_id=0), "停转")
    print(f"共发送 {bus.frames_sent} 条 34 字节命令，每条都回读 78 字节状态；帧内温度按 1 °C 取整。")
    return log


def plot(log: dict) -> None:
    t = np.array(log["t"])
    q = np.array(log["q"])
    tau = np.array(log["tau"])
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(8.2, 5.6), sharex=True)
    ax1.plot(t, q, color="#1f77b4", lw=1.8)
    ax1.set_ylabel("转子位置 q (rad)")
    ax1.set_title("开环慢转 → 伺服保持 → 停转")
    ax1.grid(True, alpha=0.3)
    ax2.plot(t, tau, color="#d62728", lw=1.8)
    ax2.set_ylabel(r"力矩 $\tau$ (N·m)")
    ax2.set_xlabel("时间 (s)")
    ax2.grid(True, alpha=0.3)
    for ax in (ax1, ax2):
        ax.axvspan(0.0, 1.0, color="#e3f2fd", alpha=0.6)
        ax.axvspan(1.0, 2.0, color="#e8f5e9", alpha=0.6)
        ax.axvspan(2.0, 2.2, color="#ffebee", alpha=0.6)
    ax1.text(0.50, ax1.get_ylim()[1] * 0.92, "mode=5", ha="center", fontsize=9)
    ax1.text(1.50, ax1.get_ylim()[1] * 0.92, "mode=10", ha="center", fontsize=9)
    ax1.text(2.10, ax1.get_ylim()[1] * 0.92, "mode=0", ha="center", fontsize=8)
    fig.tight_layout()
    savefig(fig, "2_5_motor_python")


def main() -> None:
    parser = argparse.ArgumentParser(description="A1 motor Python example")
    parser.add_argument("--port", default="/dev/ttyUSB0")
    parser.add_argument("--real", action="store_true",
                        help="use a real RS-485 adapter (hardware required)")
    args = parser.parse_args()
    log = run(args.port, args.real)
    plot(log)


if __name__ == "__main__":
    main()
