"""Mass and centre-of-mass identification of the SRB model (Section 8.5).

A torque-controlled stand is simulated for a grid of model masses and lateral
COM offsets.  The measured height and roll errors form two heat maps; a
separate line scan moves the COM fore-aft and records the pitch error.  The
controller never reads the simulator truth: it uses the Chapter 7 estimator.
The truth is used only to score each run.

expect: 质量与重心
expect: 扫描
expect: 最优组合
expect: 理论
expect: 俯仰
"""

from __future__ import annotations

import logging
import os

os.environ.setdefault("MESA_SHADER_CACHE_DISABLE", "true")

import numpy as np
from quadruped.plotting import savefig, label
import matplotlib.pyplot as plt

from quadruped import A1Sim, A1Params, Quadruped
from quadruped.balance import BalanceCtrl
from quadruped.estimator import Estimator
from quadruped.rotation import rpy_to_R
from quadruped.types import JointCmd, MotorMode

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

DT = 0.002
TARGET_H = 0.270
MASSES = np.linspace(10.0, 13.0, 7)
Y_OFFSETS = np.linspace(-0.03, 0.03, 7)
X_OFFSETS = np.linspace(-0.03, 0.03, 5)
KP_LEG = np.array([100.0, 300.0, 400.0] * 4)
KD_LEG = np.array([4.0, 8.0, 8.0] * 4)


# ANCHOR: stand
def stand_once(mass: float, cx: float, cy: float, duration: float = 0.8
               ) -> tuple[float, float, float]:
    """Run a torque-controlled stand and return height, roll, pitch.

    The controller sees only ``read_sensors`` plus the estimator.  The
    returned values are ground truth, used here as the "ruler" of the
    identification experiment.
    """
    params = A1Params.from_menagerie()
    robot = Quadruped(params)
    sim = A1Sim()
    sim.reset(q0=params.home_q, base_pos=(0.0, 0.0, TARGET_H))
    for _ in range(400):
        sim.step(JointCmd(params.home_q, np.zeros(12), KP_LEG, KD_LEG,
                          np.zeros(12)))

    sensors = sim.read_sensors()
    estimator = Estimator(robot=robot, dt=DT)
    estimator.reset(sensors.base_state(), sensors.joints, sensors.contact)
    est = estimator.estimated_base(sensors.base_state())
    p_des = np.array([est.position[0], est.position[1], TARGET_H])
    R_des = rpy_to_R([0.0, 0.0, 0.0])

    ctrl = BalanceCtrl(mass=mass, inertia=params.trunk_inertia,
                       w_dynamics=1000.0, w_force=0.01, w_delta=0.1)
    ctrl.set_robot(robot)
    # Soft pose gains make the model error visible, exactly as the book's
    # tuning procedure asks.  The joint PD is off: the QP torque is the only
    # actuator input, so the mass and COM enter the equilibrium directly.
    ctrl.Kp = np.diag([60.0, 60.0, 120.0])
    ctrl.Kd = np.diag([15.0, 15.0, 22.0])
    ctrl.kp_rot = 35.0
    ctrl.Kd_rot = np.diag([9.0, 9.0, 7.0])

    model_com = params.com_body_home + np.array([cx, cy, 0.0])
    last = None
    for k in range(int(duration / DT)):
        sensors = sim.read_sensors()
        joints = sensors.joints
        est = estimator.step(sensors.base_state(), joints, sensors.contact)
        R = est.rotation()
        feet = est.position + (R @ robot.fk_all(joints.q).T).T
        com = est.position + R @ model_com
        a, alpha = ctrl.desired_acceleration(
            p_des, np.zeros(3), est.position, est.velocity, R_des, R,
            np.zeros(3), sensors.omega_b)
        forces = ctrl.solve_force(a, alpha, R, feet - com,
                                  sensors.contact.astype(bool))
        tau = ctrl.to_joint_torque(joints.q, forces, R, robot)
        base = sim.step(JointCmd(np.zeros(12), np.zeros(12), np.zeros(12),
                                 np.zeros(12), tau,
                                 mode=np.full(12, int(MotorMode.SERVO))))
        if k >= int(duration / DT) - 100:
            last = base
    sim.close()
    return float(last.position[2]), float(last.rpy[0]), float(last.rpy[1])
# ANCHOR_END: stand


def main() -> None:
    params = A1Params.from_menagerie()
    com_home = params.com_body_home
    print("=== 8.5 单刚体模型的质量与重心整定 ===")
    print(f"整机质量（CAD/称重）m_w = {params.mass:.4f} kg，"
          f"整机重心 p_g = [{com_home[0]:+.4f} {com_home[1]:+.4f} "
          f"{com_home[2]:+.4f}] m")
    print(f"扫描 m ∈ [{MASSES[0]:.2f}, {MASSES[-1]:.2f}] kg，"
          f"p_g,y ∈ [{com_home[1] + Y_OFFSETS[0]:+.4f}, "
          f"{com_home[1] + Y_OFFSETS[-1]:+.4f}] m，每组 0.8 s")

    height = np.zeros((len(MASSES), len(Y_OFFSETS)))
    roll = np.zeros_like(height)
    for i, mass in enumerate(MASSES):
        for j, dy in enumerate(Y_OFFSETS):
            h, r, _ = stand_once(mass, 0.0, dy)
            height[i, j] = h
            roll[i, j] = np.degrees(r)

    # Score in physical units: 1 mm of height and 0.5 deg of roll.
    cost = ((height - TARGET_H) / 1e-3) ** 2 + (roll / 0.5) ** 2
    ii, jj = np.unravel_index(np.argmin(cost), cost.shape)
    best_m = MASSES[ii]
    best_y = com_home[1] + Y_OFFSETS[jj]
    print(f"最优组合：m = {best_m:.3f} kg，p_g,y = {best_y:+.4f} m；"
          f"高度 = {height[ii, jj]:.4f} m，roll = {roll[ii, jj]:+.4f} deg，"
          f"代价 = {cost[ii, jj]:.2f}")
    print(f"理论对比：整机质量 {params.mass:.4f} kg，"
          f"整机重心 y = {com_home[1]:+.4f} m；"
          f"等效质量比整机低 {100.0 * (1.0 - best_m / params.mass):.2f} %")

    pitch_com = []
    pitch = []
    for dx in X_OFFSETS:
        _, _, p = stand_once(best_m, dx, Y_OFFSETS[jj])
        pitch_com.append(com_home[0] + dx)
        pitch.append(np.degrees(p))
    print("俯仰扫描（m 取最优值）：")
    for x, p in zip(pitch_com, pitch):
        print(f"  p_g,x = {x:+.4f} m → pitch = {p:+.4f} deg")
    print("p_g,y 是横向偏移，主要激起 roll；前后偏移 p_g,x 才对应俯仰。")

    # Figure 8-6: height/roll heat maps and the fore-aft pitch scan.
    fig, axes = plt.subplots(1, 3, figsize=(13.4, 4.0))
    extent = [com_home[1] + Y_OFFSETS[0], com_home[1] + Y_OFFSETS[-1],
              MASSES[0], MASSES[-1]]
    im = axes[0].imshow(height * 1000.0, origin="lower", aspect="auto",
                        extent=extent, cmap="viridis")
    cs = axes[0].contour(np.linspace(extent[0], extent[1], len(Y_OFFSETS)),
                         MASSES, height, levels=[TARGET_H], colors="w",
                         linewidths=1.2)
    axes[0].clabel(cs, fmt="270 mm", fontsize=8)
    axes[0].plot(best_y, best_m, "wx", ms=9, mew=2.0)
    axes[0].set_xlabel("$p_{g,y}$ (m)")
    axes[0].set_ylabel("$m$ (kg)")
    axes[0].set_title(label("站立高度 (mm)", "standing height (mm)"))
    fig.colorbar(im, ax=axes[0], fraction=0.046, pad=0.03)

    vmax = np.abs(roll).max()
    im = axes[1].imshow(roll, origin="lower", aspect="auto", extent=extent,
                        cmap="coolwarm", vmin=-vmax, vmax=vmax)
    axes[1].contour(np.linspace(extent[0], extent[1], len(Y_OFFSETS)),
                    MASSES, roll, levels=[0.0], colors="k",
                    linewidths=1.0)
    axes[1].plot(best_y, best_m, "kx", ms=9, mew=2.0)
    axes[1].set_xlabel("$p_{g,y}$ (m)")
    axes[1].set_ylabel("$m$ (kg)")
    axes[1].set_title(label("横滚角 (deg)", "roll (deg)"))
    fig.colorbar(im, ax=axes[1], fraction=0.046, pad=0.03)

    axes[2].plot(pitch_com, pitch, "o-", color="#1565c0", lw=1.5)
    axes[2].axhline(0.0, color="0.6", ls="--", lw=0.9)
    axes[2].axvline(com_home[0], color="0.6", ls=":", lw=0.9)
    axes[2].set_xlabel("$p_{g,x}$ (m)")
    axes[2].set_ylabel(label("俯仰角 (deg)", "pitch (deg)"))
    axes[2].set_title(label("前后重心偏移与俯仰", "fore-aft COM and pitch"))
    axes[2].grid(True, alpha=0.3)
    fig.tight_layout()
    savefig(fig, "8_6_mass_id")
    plt.close(fig)


if __name__ == "__main__":
    main()
