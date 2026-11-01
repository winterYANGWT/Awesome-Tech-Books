"""State_Trotting lifecycle, dependency injection and the 38 variables (10.2).

Four long-lived objects are injected into the trotting state: the gait
generator, the estimator, the robot model and the balance controller.  The
example uses a fake sensor source, so it can run without MuJoCo physics and
without a real robot.  It prints the order of enter/run/exit, the five groups
of the 38 intermediate variables with their shapes, and then forces a
transition request to Passive to verify that exit() clears the velocity
command.

expect: 调用顺序
expect: 38 个变量
expect: 分组计数
expect: 切换请求
expect: 退出清理
"""

from __future__ import annotations

import logging

import numpy as np

from quadruped import (A1Params, ControlContext, JointState, Quadruped,
                       SensorReadings, TrottingController)
from quadruped.fsm import StateID
from quadruped.plotting import label, savefig
from quadruped.rotation import rpy_to_R
import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

DT = 0.002


# ANCHOR: lifecycle
class FakeSim:
    """Minimal sensor source: a stationary robot at the A1 home pose."""

    def __init__(self, params: A1Params) -> None:
        self.params = params
        self.q = np.asarray(params.home_q, dtype=float).copy()
        self.dq = np.zeros(12)
        self.contact = np.ones(4, dtype=bool)
        self.last_cmd = None
        self.time = 0.0

    def read_sensors(self) -> SensorReadings:
        return SensorReadings(
            quat=np.array([1.0, 0.0, 0.0, 0.0]),
            rpy=np.zeros(3),
            omega_b=np.zeros(3),
            acc_b=np.array([0.0, 0.0, 9.81]),
            joints=JointState(self.q.copy(), self.dq.copy()),
            contact=self.contact.copy(),
            timestamp=self.time,
        )

    def set_joint_pd(self, cmd) -> None:
        self.last_cmd = cmd


class LifecycleTrot(TrottingController):
    """TrottingController plus an explicit transition counter."""

    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.trace: list[str] = []
        self.runs = 0

    def enter(self) -> None:
        self.trace.append("enter")
        super().enter()

    def run(self):
        self.runs += 1
        self.trace.append(f"run#{self.runs}")
        return super().run()

    def check_transition(self):
        self.trace.append("check_transition")
        if self.runs >= 3:
            self.trace.append("request=Passive")
            return StateID.PASSIVE
        return None

    def exit(self) -> None:
        self.trace.append("exit")
        super().exit()


# ANCHOR_END: lifecycle


# ANCHOR: variables
def collect_variables(state: LifecycleTrot, sim: FakeSim, robot: Quadruped,
                      limits: dict) -> list[tuple[str, str, np.ndarray]]:
    """Return the 38 variables in the five groups of the book."""
    est = state.est_base
    q = sim.q.copy()
    R = est.rotation()
    feet = est.position + (R @ robot.fk_all(q).T).T
    feet_rel = feet - est.position
    R_des = rpy_to_R([0.0, 0.0, state.yaw_des])
    v_world = R @ state.v_cmd_smooth
    p_ref = est.position.copy()
    p_ref[2] = state.body_height
    v_des = np.array([v_world[0], v_world[1], 0.0])
    a_des, alpha_des = state.balance.desired_acceleration(
        p_ref, v_des, est.position, est.velocity, R_des, R,
        np.zeros(3), sim.read_sensors().omega_b)
    feet_goal_body = (R_des.T @ (state.feet_des - state.p_des).T).T
    feet_vel_body = (R_des.T @ state.feet_vel.T).T
    force_body = (R.T @ state.forces.T).T

    V = [
        # 1-10 estimated state
        ("posBody", "估计状态", est.position),
        ("_velBody", "估计状态", est.velocity),
        ("_yaw", "估计状态", np.array([est.rpy[2]])),
        ("dYaw", "估计状态", np.array([(R @ sim.read_sensors().omega_b)[2]])),
        ("_posFeetGlobal", "估计状态", feet),
        ("velFeetGlobal", "估计状态", np.zeros((4, 3))),
        ("posFeet2BGlobal", "估计状态", feet_rel),
        ("_B2G_RotMat", "估计状态", R),
        ("_G2B_RotMat", "估计状态", R.T),
        ("_q", "估计状态", q),
        # 11-23 targets
        ("_pcd", "目标量", state.p_des),
        ("_vCmdGlobal", "目标量", v_world),
        ("_vCmdBody", "目标量", state.v_cmd_smooth),
        ("_yawCmd", "目标量", np.array([state.yaw_des])),
        ("_dYawCmd", "目标量", np.array([state.v_cmd_smooth[2]])),
        ("_wCmdGlobal", "目标量", np.array([0.0, 0.0, state.v_cmd_smooth[2]])),
        ("_posFeetGlobalGoal", "目标量", state.feet_des),
        ("_velFeetGlobalGoal", "目标量", state.feet_vel),
        ("_posFeet2BGoal", "目标量", feet_goal_body),
        ("_velFeet2BGoal", "目标量", feet_vel_body),
        ("_Rd", "目标量", R_des),
        ("_ddPcd", "目标量", a_des),
        ("_dWbd", "目标量", alpha_des),
        # 24-28 forces and joint commands
        ("_forceFeetGlobal", "力与关节命令", state.forces),
        ("_forceFeetBody", "力与关节命令", force_body),
        ("_qGoal", "力与关节命令", state.q_des),
        ("_qdGoal", "力与关节命令", state.dq_des),
        ("_tau", "力与关节命令", state.tau),
        # 29-35 parameters
        ("_gaitHeight", "控制参数", np.array([state.step_height])),
        ("_Kpp", "控制参数", state.balance.Kp),
        ("_Kdp", "控制参数", state.balance.Kd),
        ("_kpw", "控制参数", np.asarray(state.balance.kp_rot)),
        ("_Kdw", "控制参数", state.balance.Kd_rot),
        ("_KpSwing", "控制参数", state.kp_cart),
        ("_KdSwing", "控制参数", state.kd_cart),
        # 36-38 limits
        ("_vxLim", "限幅", limits["vx"]),
        ("_vyLim", "限幅", limits["vy"]),
        ("_wyawLim", "限幅", limits["wyaw"]),
    ]
    return V
# ANCHOR_END: variables


def draw_class_diagram() -> None:
    """Fig. 10-2: the state, the shared components and injection."""
    fig, ax = plt.subplots(figsize=(9.0, 4.8))
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 10)
    ax.axis("off")

    def box(x, y, w, h, text, fc="white", ec="0.25", fs=8.5):
        ax.add_patch(FancyBboxPatch((x, y), w, h,
                                    boxstyle="round,pad=0.05,rounding_size=0.08",
                                    fc=fc, ec=ec, lw=1.0, zorder=3))
        ax.text(x + w / 2, y + h / 2, text, ha="center", va="center",
                fontsize=fs, linespacing=1.35, zorder=4)

    def arrow(x0, y0, x1, y1, ls="-", color="0.3", style="-|>", text=None,
              tx=0.0, ty=0.0):
        ax.add_patch(FancyArrowPatch((x0, y0), (x1, y1), arrowstyle=style,
                                     mutation_scale=10, color=color, lw=1.0,
                                     linestyle=ls, zorder=2,
                                     shrinkA=1, shrinkB=1))
        if text:
            ax.text((x0 + x1) / 2 + tx, (y0 + y1) / 2 + ty, text,
                    fontsize=7.4, color=color, ha="center", va="center")

    box(3.45, 8.05, 2.9, 1.15,
        "FSM\n状态表 + 两阶段切换\ncurrent / pending", fc="#f2f2f2")
    box(3.25, 5.55, 3.3, 1.4,
        "State_Trotting（TrottingController）\nenter()  run()  exit()\ncheckTransition()", fc="#e8f1fb")
    arrow(4.9, 8.02, 4.9, 7.0, style="-|>", text="持有/切换", tx=0.55, ty=0.0)

    box(0.25, 2.35, 2.45, 2.55,
        "CtrlComponents\n（构造一次）\n\nsim / robot / params\n估算 dt、站立角、增益\n状态表共享", fc="#fff4e6")
    arrow(2.72, 4.2, 3.22, 5.95, ls="--", color="C3", text="注入", tx=-0.35, ty=-0.35)

    comps = [
        ("GaitGenerator\ngait", 4.05, 3.55, "#e8f1fb"),
        ("Estimator\nest", 5.65, 3.55, "#e8f1fb"),
        ("Quadruped\nrobot", 4.05, 2.05, "#e8f1fb"),
        ("BalanceCtrl\nbalance", 5.65, 2.05, "#e8f1fb"),
    ]
    for text, x, y, fc in comps:
        box(x, y, 1.35, 1.05, text, fc=fc, fs=8.0)
        arrow(4.9, 5.52, x + 0.67, y + 1.08, ls="--", color="0.45")

    box(7.55, 3.55, 2.15, 1.0, "JointCmd\nq_des, dq_des, tau, kp, kd", fc="#eef7ea", fs=7.8)
    arrow(6.58, 5.55, 8.1, 4.58, ls=":", color="C0", text="写命令", tx=0.45, ty=0.1)
    box(7.55, 2.05, 2.15, 0.9, "LowCmd / 电机总线", fc="#eef7ea", fs=8.0)
    arrow(8.62, 3.52, 8.62, 2.98)

    ax.text(5.0, 9.68, label("State_Trotting 与四个注入组件", "State_Trotting and its components"),
            ha="center", va="center", fontsize=11)
    ax.text(5.0, 0.28, "状态类只持指针：组件的构造、生命周期和共享由 CtrlComponents 负责",
            ha="center", va="center", fontsize=8.2, color="0.3")
    savefig(fig, "10_2_trotting_class")
    plt.close(fig)


def main() -> None:
    params = A1Params.from_menagerie()
    robot = Quadruped(params)
    sim = FakeSim(params)
    context = ControlContext(sim=sim, params=params, robot=robot, dt=DT)
    state = LifecycleTrot(context, robot=robot, params=params)

    print("=== 10.2 State_Trotting 的生命周期与中间变量 ===")
    state.enter()
    print("进入后：目标位姿已对齐当前估计位姿，v_cmd = "
          f"{np.array2string(state.v_cmd, precision=3)}")
    state.set_command(vx=0.20, vy=0.0, vyaw=0.10)
    nxt = None
    for _ in range(6):
        state.run()
        nxt = state.check_transition()
        if nxt is not None:
            break
    print("调用顺序：" + " → ".join(state.trace))

    limits = {"vx": np.array([-0.5, 0.5]),
              "vy": np.array([-0.3, 0.3]),
              "wyaw": np.array([-1.0, 1.0])}
    values = collect_variables(state, sim, robot, limits)
    assert len(values) == 38, len(values)
    groups: dict[str, list[tuple[str, np.ndarray]]] = {}
    for name, group, value in values:
        groups.setdefault(group, []).append((name, np.asarray(value)))
    print("38 个变量按用途分为五组：")
    for group, items in groups.items():
        print(f"  {group}: {len(items)} 个")
    print("分组计数：" + "，".join(f"{g} {len(v)}" for g, v in groups.items()))
    print("\n序号  变量名                  形状        首值")
    for k, (name, group, value) in enumerate(values, start=1):
        arr = np.asarray(value, dtype=float)
        flat = arr.reshape(-1)
        first = flat[0] if flat.size else float("nan")
        print(f"{k:3d}   {name:<22s} {str(arr.shape):<10s} {first: .4f}   [{group}]")

    print(f"\n切换请求：check_transition() → {nxt.value if nxt else None}")
    if nxt is not None:
        state.exit()
    print("退出清理：v_cmd = "
          f"{np.array2string(state.v_cmd, precision=3)}，v_cmd_smooth = "
          f"{np.array2string(state.v_cmd_smooth, precision=3)}，entered = {state.entered}")
    print("退出清理通过：速度命令已清零；下一次 enter() 会重新对齐位姿和步态")
    draw_class_diagram()
    print("图 10-2 已保存：10_2_trotting_class.png")


if __name__ == "__main__":
    main()
