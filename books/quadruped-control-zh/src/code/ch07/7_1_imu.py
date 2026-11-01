"""IMU model: mass-spring accelerometer, bias and attitude (Section 7.1).

The static part uses MuJoCo: the A1 is held in ``FixedStand`` for 5 s and its
ideal body acceleration, angular rate and attitude are read from the sensors.
An ``IMU`` model adds a constant bias and white noise.  The measured mean is
used to recover the bias, and two attitude estimates (accelerometer only and a
complementary filter) are compared with the simulator attitude.

The motion part uses a prescribed smooth trajectory, because the point is to
show how linear acceleration contaminates the gravity direction; the geometry
is the same as the MuJoCo sensor model of Eq. (7.6).

expect: 静止 5 s
expect: 加速度计零偏估计
expect: 加速度计倾角误差
expect: 互补滤波倾角误差
expect: 运动段加速度计倾角最大误差
"""

from __future__ import annotations

import logging
import math

import numpy as np
from quadruped import A1Sim, IMU, JointCmd, savefig

import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle
from quadruped.plotting import label
from quadruped.rotation import rpy_to_R

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

G = np.array([0.0, 0.0, -9.81])          # world gravity vector, m/s^2
ACC_BIAS = np.array([0.120, -0.085, 0.150])
GYRO_BIAS = np.array([0.0090, -0.0060, 0.0110])
ACC_STD = 0.080                          # m/s^2
GYRO_STD = 0.0025                        # rad/s
DT = 0.002
SEED = 7


def tilt_from_acc(acc: np.ndarray) -> tuple[float, float]:
    """Roll/pitch from a body-frame proper acceleration (ZYX convention)."""
    ax, ay, az = acc
    roll = math.atan2(ay, az)
    pitch = math.atan2(-ax, math.sqrt(ay * ay + az * az))
    return roll, pitch


# ANCHOR: static_test
def static_test() -> dict:
    """Hold the A1 still for 5 s, recover bias, compare tilt estimates."""
    sim = A1Sim()
    q0 = sim.home_q.copy()
    kp = np.array([100.0, 300.0, 400.0] * 4)
    kd = np.array([4.0, 8.0, 8.0] * 4)
    for _ in range(1000):                 # settle in the home pose
        sim.step(JointCmd(q0, np.zeros(12), kp, kd, np.zeros(12)))
    imu = IMU(ACC_BIAS, GYRO_BIAS, ACC_STD, GYRO_STD, seed=SEED)
    hold = JointCmd(q0, np.zeros(12), kp, kd, np.zeros(12))

    t, acc_meas, acc_ideal, gyro, rpy_true = [], [], [], [], []
    for _ in range(int(5.0 / DT)):
        sim.step(hold)
        sens = sim.read_sensors()
        a_m, w_m = imu.measure(sens.acc_b, sens.omega_b)
        t.append(sens.timestamp - 2.0)
        acc_meas.append(a_m)
        acc_ideal.append(sens.acc_b.copy())
        gyro.append(w_m)
        rpy_true.append(sens.rpy.copy())
    sim.close()
    t = np.asarray(t)
    acc_meas = np.asarray(acc_meas)
    acc_ideal = np.asarray(acc_ideal)
    gyro = np.asarray(gyro)
    rpy_true = np.asarray(rpy_true)

    bias_est = acc_meas.mean(axis=0) - acc_ideal.mean(axis=0)
    gyro_bias_est = gyro.mean(axis=0)
    bias_err = bias_est - ACC_BIAS

    # Accelerometer-only tilt, evaluated sample by sample.
    raw = np.array([tilt_from_acc(a) for a in acc_meas])
    raw_err = raw - rpy_true[:, :2]
    raw_rmse = np.sqrt(np.mean(raw_err ** 2, axis=0))

    # Complementary filter: gyro integration for prediction, accelerometer
    # for the low-frequency correction.  The gyro bias is NOT removed here.
    alpha = 0.98
    est = np.zeros((len(t), 2))
    est[0] = raw[0]
    for k in range(1, len(t)):
        pred = est[k - 1] + DT * gyro[k, :2]
        acc_tilt = raw[k]
        est[k] = alpha * pred + (1.0 - alpha) * acc_tilt
    filt_err = est - rpy_true[:, :2]
    filt_rmse = np.sqrt(np.mean(filt_err ** 2, axis=0))

    # Gyro-only integration, to show the drift caused by the constant bias.
    gyro_only = np.zeros((len(t), 2))
    gyro_only[0] = rpy_true[0, :2]
    for k in range(1, len(t)):
        gyro_only[k] = gyro_only[k - 1] + DT * gyro[k, :2]
    gyro_drift = gyro_only[-1] - rpy_true[-1, :2]

    print("=== 7.1 IMU 静态测试（MuJoCo，静止 5 s） ===")
    print(f"样本数 {len(t)}，采样周期 {DT * 1000:.0f} ms")
    print("设置的零偏：")
    print(f"  加速度计 b_a = {np.array2string(ACC_BIAS, precision=4)} m/s^2")
    print(f"  陀螺仪   b_g = {np.array2string(GYRO_BIAS, precision=4)} rad/s")
    print("加速度计零偏估计（均值法）：")
    print(f"  b_a_hat = {np.array2string(bias_est, precision=4)} m/s^2")
    print(f"  估计误差 = {np.array2string(bias_err, precision=5)} m/s^2")
    print("陀螺仪零偏估计：")
    print(f"  b_g_hat = {np.array2string(gyro_bias_est, precision=5)} rad/s")
    print(f"  估计误差 = {np.array2string(gyro_bias_est - GYRO_BIAS, precision=6)} rad/s")
    print("加速度计白噪声标准差（去均值后）：")
    print(f"  {np.array2string(acc_meas.std(axis=0), precision=4)} m/s^2 "
          f"（设置 {ACC_STD:.4f}）")
    print("加速度计倾角误差（相对真值，rad）：")
    print(f"  roll RMSE {raw_rmse[0]:.5f}，pitch RMSE {raw_rmse[1]:.5f}")
    print("互补滤波倾角误差（alpha=0.98，rad）：")
    print(f"  roll RMSE {filt_rmse[0]:.5f}，pitch RMSE {filt_rmse[1]:.5f}")
    print("纯陀螺积分 5 s 后的漂移（rad）：")
    print(f"  {np.array2string(gyro_drift, precision=5)}")
    print(f"静止时加速度计 z 轴均值 {acc_meas[:, 2].mean():.4f} m/s^2，"
          f"对应 +g 的重力偏置")
    return dict(t=t, acc_meas=acc_meas, acc_ideal=acc_ideal, rpy_true=rpy_true,
                raw=raw, est=est)
# ANCHOR_END: static_test



# ANCHOR: motion_test
def motion_test() -> dict:
    """Prescribed motion: gravity projection plus a real linear acceleration."""
    t = np.arange(0.0, 3.0, DT)
    roll = 0.12 * np.sin(2 * np.pi * 0.5 * t)
    pitch = -0.08 * np.sin(2 * np.pi * 0.4 * t)
    yaw = 0.15 * np.sin(2 * np.pi * 0.2 * t)
    a_s = np.column_stack([0.9 * np.sin(2 * np.pi * 0.7 * t),
                           0.5 * np.sin(2 * np.pi * 0.45 * t + 0.6), np.zeros_like(t)])
    rng = np.random.default_rng(SEED)
    acc_meas, acc_ideal, gravity_only, rpy_true, raw = [], [], [], [], []
    for k in range(len(t)):
        R = rpy_to_R([roll[k], pitch[k], yaw[k]])
        a_b = R.T @ (a_s[k] - G) + ACC_BIAS + rng.normal(0.0, ACC_STD, 3)
        g_b = R.T @ (-G)
        acc_meas.append(a_b)
        acc_ideal.append(a_b - ACC_BIAS - rng.normal(0.0, 0.0, 3))
        gravity_only.append(g_b)
        rpy_true.append([roll[k], pitch[k], yaw[k]])
        raw.append(tilt_from_acc(a_b))
    acc_meas = np.asarray(acc_meas)
    gravity_only = np.asarray(gravity_only)
    rpy_true = np.asarray(rpy_true)
    raw = np.asarray(raw)
    raw_err = raw - rpy_true[:, :2]
    print("=== 7.1 IMU 运动测试（解析轨迹，3 s） ===")
    print(f"世界系线加速度峰值 {np.abs(a_s).max():.3f} m/s^2")
    print(f"运动段加速度计倾角误差（rad）：roll RMSE "
          f"{np.sqrt(np.mean(raw_err[:, 0] ** 2)):.5f}，pitch RMSE "
          f"{np.sqrt(np.mean(raw_err[:, 1] ** 2)):.5f}")
    print(f"运动段加速度计倾角最大误差 {np.abs(raw_err).max():.5f} rad")
    return dict(t=t, acc_meas=acc_meas, gravity_only=gravity_only,
                rpy_true=rpy_true, raw=raw, a_s=a_s)
# ANCHOR_END: motion_test



def draw_spring_model() -> None:
    fig, ax = plt.subplots(figsize=(4.6, 4.4))
    ax.set_xlim(-1.1, 1.5)
    ax.set_ylim(-0.5, 4.4)
    ax.axis("off")

    # Fixed base with hatching.
    ax.add_patch(Rectangle((-0.9, -0.25), 1.8, 0.18, fc="0.75", ec="k"))
    for x in np.linspace(-0.85, 0.85, 12):
        ax.plot([x, x - 0.12], [-0.25, -0.45], color="k", lw=0.8)
    ax.plot([-0.9, 0.9], [0, 0], color="k", lw=1.2)

    # Spring: zig-zag from base to mass.
    y0, y1, n = 0.05, 1.75, 7
    ys = np.linspace(y0, y1, 2 * n + 1)
    xs = np.zeros_like(ys)
    xs[1:-1:2] = 0.24
    xs[2:-1:2] = -0.24
    ax.plot(xs, ys, color="C0", lw=1.8)
    ax.text(0.30, 0.95, label("刚度 $k$", "stiffness $k$"), fontsize=11)
    ax.text(-0.72, 0.95, label("弹簧", "spring"), fontsize=10, rotation=90,
            va="center")

    # Two mass positions: equilibrium (dashed) and displaced (solid).
    ax.add_patch(Rectangle((-0.32, 2.35), 0.64, 0.55, fc="none", ec="0.4",
                           ls="--", lw=1.2))
    ax.add_patch(Rectangle((-0.32, 1.75), 0.64, 0.55, fc="C1", ec="k",
                           alpha=0.55))
    ax.plot([0.0, 0.0], [2.35, 1.75], color="k", lw=0.8, ls=":")
    ax.annotate("", xy=(0.42, 1.75), xytext=(0.42, 2.35),
                arrowprops=dict(arrowstyle="<->", lw=1.0))
    ax.text(0.48, 1.98, "$s$", fontsize=13)
    ax.text(0.38, 2.68, label("平衡位置", "rest position"), fontsize=9)
    ax.text(0.38, 1.52, label("加速后", "accelerated"), fontsize=9)
    ax.annotate("", xy=(0.95, 2.95), xytext=(0.95, 1.95),
                arrowprops=dict(arrowstyle="->", lw=1.6, color="C3"))
    ax.text(1.02, 2.40, "$a$", fontsize=13, color="C3")
    ax.annotate("", xy=(0, 3.7), xytext=(0, 3.05),
                arrowprops=dict(arrowstyle="->", lw=1.3))
    ax.text(0.06, 3.30, "$x$", fontsize=12)
    ax.plot([0], [0.02], "o", color="k", ms=3)
    ax.text(-0.18, -0.10, "$O$", fontsize=10)
    ax.set_title(label("加速度计的质量—弹簧模型", "mass-spring accelerometer"))
    savefig(fig, "7_1_imu_spring")
    plt.close(fig)


def plot_static_and_motion(st: dict, mo: dict) -> None:
    fig, ax = plt.subplots(2, 2, figsize=(8.4, 6.0))
    ax[0, 0].plot(st["t"], st["acc_meas"][:, 2], lw=0.8, color="C0",
                  label=label("测量 $a_z$", "measured $a_z$"))
    ax[0, 0].plot(st["t"], st["acc_ideal"][:, 2], lw=1.2, color="C3",
                  label=label("理想值", "ideal"))
    ax[0, 0].axhline(9.81, color="k", ls="--", lw=0.8,
                     label=label("$+g$", "$+g$"))
    ax[0, 0].set_ylabel("$a_z$ (m/s$^2$)")
    ax[0, 0].set_title(label("静止：重力偏置与零偏", "static: gravity + bias"))
    ax[0, 0].legend(fontsize=7, loc="lower right")

    ax[0, 1].plot(st["t"], st["raw"][:, 0] * 180 / np.pi, lw=0.9,
                  color="C1", label=label("加速度计 roll", "accel roll"))
    ax[0, 1].plot(st["t"], st["est"][:, 0] * 180 / np.pi, lw=1.2,
                  color="C0", label=label("互补滤波 roll", "complementary"))
    ax[0, 1].plot(st["t"], st["rpy_true"][:, 0] * 180 / np.pi, lw=1.0,
                  color="k", ls="--", label=label("真值", "truth"))
    ax[0, 1].set_ylabel("roll (deg)")
    ax[0, 1].set_title(label("静止：两种倾角估计", "static: tilt estimates"))
    ax[0, 1].legend(fontsize=7)

    for i, name in enumerate(["$a_x$", "$a_y$", "$a_z$"]):
        ax[1, 0].plot(mo["t"], mo["acc_meas"][:, i], lw=0.9,
                      label=name + label(" 测量", " measured"))
        ax[1, 0].plot(mo["t"], mo["gravity_only"][:, i], lw=0.9, ls="--",
                      label=name + label(" 仅重力", " gravity only"))
    ax[1, 0].set_xlabel(label("时间 (s)", "time (s)"))
    ax[1, 0].set_ylabel("$a_b$ (m/s$^2$)")
    ax[1, 0].set_title(label("运动：读数 = 重力投影 + 线加速度",
                             "motion: gravity + linear acceleration"))
    ax[1, 0].legend(fontsize=6, ncol=2)

    err = (mo["raw"] - mo["rpy_true"][:, :2]) * 180 / np.pi
    ax[1, 1].plot(mo["t"], err[:, 0], lw=1.0, label="roll")
    ax[1, 1].plot(mo["t"], err[:, 1], lw=1.0, label="pitch")
    ax[1, 1].axhline(0, color="k", lw=0.6)
    ax[1, 1].set_xlabel(label("时间 (s)", "time (s)"))
    ax[1, 1].set_ylabel(label("倾角误差 (deg)", "tilt error (deg)"))
    ax[1, 1].set_title(label("运动：线加速度污染倾角",
                             "motion: acceleration corrupts tilt"))
    ax[1, 1].legend(fontsize=8)
    for a in ax.ravel():
        a.grid(True, alpha=0.3)
    fig.tight_layout()
    savefig(fig, "7_1_imu")
    plt.close(fig)


def main() -> None:
    draw_spring_model()
    static = static_test()
    motion = motion_test()
    plot_static_and_motion(static, motion)


if __name__ == "__main__":
    main()
