# `quadruped` 共用代码包

本书第 2–11 章共用的 Python 包。接口按 `notes/outline.md` 0.3 节设计；本文件只记录实现口径与和大纲接口的差异。

## 模块

| 模块 | 内容 |
| --- | --- |
| `types.py` | `LegId`、`MotorMode`、`ControlMode`、`JointState`、`JointCmd`、`BaseState`、`ContactState`、`GaitCommand` |
| `rotation.py` | `Rx/Ry/Rz`、`skew`、`so3_exp/so3_log`、Y-P-R 欧拉角、四元数、`make_T/inv_T/adjoint` |
| `leg.py` | A1 单腿正/逆运动学、雅可比、`jacobian_dot`、`tau=Jᵀf`、`f=J⁻ᵀτ` |
| `robot.py` | `A1Params.from_menagerie()`、`body_equivalent_inertia()` 整机等效惯量、`Quadruped` 四条腿组合、机身/世界坐标变换 |
| `sim.py` | `A1Sim`：MuJoCo A1、混合控制、IMU、足端接触/力、`read_sensors()` 可测量读数、`get_state()` 基座真值、离屏渲染 |
| `motor.py` | 电机数据结构、混合控制律、六种模式、34/78 字节报文、CRC32、`SimMotorBus`/`Rs485MotorBus` |
| `fsm.py` | `State`、`FSM` 两阶段切换、`Passive`、`FixedStand`、`FixedDown` |
| `plotting.py` | `savefig` 统一输出到 `FIG_DIR`（默认 `./figs`）、中文字体检测、绘图小工具 |
| `estimator.py` | `IMU` 噪声模型、18 状态/28 输出离散卡尔曼滤波、`AvgCov` 递推均值/协方差 |
| `balance.py` | 位姿 PD、摩擦四棱锥、OSQP 足端力分配、`τ = -JᵀRᵀf` |
| `gait.py` | `GaitParams`、`WaveGenerator`、`FeetEndCal`、`GaitGenerator` |
| `swing.py` | `SwingTrajectory`：摆线（默认）与五次贝塞尔，位置/速度 |
| `trotting.py` | `FreeStand`、`TrottingController`（接入 `fsm.py` 的状态） |
| `nav.py` | `Lidar2D` 仿真激光、`GridMap` 占据栅格/膨胀、`AStarPlanner`、`PurePursuit`、`NavStack` |

## 主要接口

```python
from quadruped import A1Sim, A1Params, Quadruped, JointCmd, FSM, StateID

sim = A1Sim()                       # 默认 src/code/assets/menagerie/unitree_a1/scene.xml
sensors = sim.read_sensors()        # 实机可测量：IMU、关节 q/dq/tau、接触
base, joints = sim.get_state()      # 真值 + 关节：只用于验收、对照和画图
sim.set_joint_pd(cmd)               # cmd: JointCmd，12 个关节
sim.step()                          # 推进 2 ms
sim.step(cmd, n_substeps=5)         # 同一力矩保持 5 个物理步（10 ms）
sim.apply_torque(tau)               # 纯力矩
contacts = sim.foot_contacts()      # (4,) bool
forces = sim.foot_forces()          # (4,3) 世界系，足端所受合力
image = sim.render()                # 离屏 RGB；用完 sim.close()
```

`read_sensors()` 返回 `SensorReadings`：`quat/rpy/omega_b/acc_b`（IMU）、
`joints`（`JointState`，含 `q/dq/tau`）和 `contact`（四个足端接触开关）。
控制器只能用这个接口加第 7 章估计器；`get_state()` 里的 `base.position`、
`base.velocity` 是 MuJoCo 真值，真实机器人拿不到，**不得进入控制回路**。
`base_state()` 只是把 IMU 读数装进 `BaseState` 的便捷方法，其中的位置、速度
占位为零，估计器初始化时不读取它们。

`JointCmd` 的 `q_des, dq_des, kp, kd, tau_ff` 都是长度 12 的数组（也可传标量自动广播）。
`JointCmd.position/velocity/damping/torque/zero/mixed` 和 `from_mode(ControlMode.*)`
给出式（2.1）的六个特例。

`Leg.fk(q)` 返回足端球心在腿基座系 `{0}` 中的坐标；`Quadruped.fk_all(q)` 返回机身系
（已加各髋安装偏置）。`Leg.ik(p, knee=-1, q_ref=None)` 中 `knee=-1` 是 A1 的膝关节分支；
`q_ref` 用于工作空间边界处两个外摆分支的选取。

## 仿真中力矩怎么施加

Menagerie 的 A1 模型自带 12 个 `position` 执行器。加载时把
`model.actuator_gainprm` 与 `model.actuator_biasprm` 全部置零（执行器变为惰性），
Python 端按

\[
\tau = \tau_{\rm ff} + k_p(q_{\rm des}-q) + k_d(\dot q_{\rm des}-\dot q)
\]

计算 12 个力矩，按 MJCF 的 `forcerange`（A1 为 ±33.5 N·m）截断后写入
`data.qfrc_applied`。因此位置、速度、阻尼、力矩、零力矩和混合模式都由同一个公式产生，
不依赖 MuJoCo 执行器的内部 PD。模型中原来的关节阻尼、电枢和摩擦损失保持不变。

模型没有 IMU sensor；`A1Sim` 在内存中的 `MjSpec` 副本里给 `trunk` 加 `imu` site 和
gyro/accelerometer/framequat 三个 sensor，并加一个名为 `track` 的跟踪相机；不改磁盘上的
MJCF。无显示环境建议 `MUJOCO_GL=egl`。

## 传感器读数与真值边界

控制器的输入链路与实机一致：

```text
IMU(framequat/gyro/accelerometer) + 关节编码器(q,dq) + 足端接触
        -> read_sensors() -> Estimator(第 7 章) -> BalanceCtrl / Trotting
```

`get_state()` 额外返回的机身位置和速度是仿真真值，只允许验收脚本、对照实验和
画图使用（`test_no_truth.py` 会把这两个量换成 NaN，验证控制器没有读它们）。

估计器的初始化也不读真值：水平原点取 0（实机上把进入状态时的位置声明为原点），
速度取 0（站立进入），高度由足端运动学和“接触足在地面”的假设得到。仿真足端球
带有 MJCF 的 1 mm 接触 margin，所以球心静止高度是 `0.021 m`；`A1Params.foot_margin`
从模型读出，估计器用 `foot_radius + foot_margin` 作为接触高度，实机上该 margin 为 0。

## A1 参数来源

`A1Params.from_menagerie(xml_path)` 用 MuJoCo `MjSpec` 只做数据提取：从
`src/code/assets/menagerie/unitree_a1/a1.xml`（或 `scene.xml`）读取
髋/大腿安装位置、`thigh/calf` 长度、足端球半径、关节范围、执行器力矩上限、
躯干质量/惯量和 `home` keyframe；整机质量、各连杆质量/质心/惯量张量及连杆
运动学链也由同一模型得到。运动学、雅可比和静力学全部是 `leg.py`/`robot.py`
的解析公式，不调用 MuJoCo 计算。模型路径可用环境变量 `QUADRUPED_A1_XML` 覆盖。

`A1Params.body_equivalent_inertia(q_stand)` 按第 6 章式（6.33）的平行轴定理，
把躯干和 12 个连杆在给定站立构型下绕整机质心求和，返回机身系中绕质心的
\(3\times3\) 惯量（含非对角元）。`q_stand` 默认取 `home_q`。`trotting.py`
的 `FreeStand`/`TrottingController` 默认调用它；`BalanceCtrl` 在不传
`inertia` 时仍退回原书式（6.32），两者都是整机口径，详见下文。

## 测试

```bash
MUJOCO_GL=egl build/venv/bin/python -m pytest src/code/tests -q
```

- `test_rotation.py`：SO(3) 指数/对数、欧拉角、四元数往返（各 100 组，<1e-9）。
- `test_kinematics.py`：四条腿各 200 组 FK 对 MuJoCo 足端球心（<1e-6 m）、IK 往返
  （<1e-9）、雅可比与中心差分（<1e-6）、`jacobian_dot` 和静力学往返。
- `test_motor.py`：T01/T02 编解码、独立参考实现的 CRC、位翻转检测、混合控制六模式。
- `test_stand.py`：从折叠姿态经 `FixedStand` 站起并保持 3 s，再切 `FixedDown`；
  同时把高度曲线存为 `build/scratch/pkg/stand.png`。

本机验收值：站立高度 0.268432 m（目标 0.27 m，差 1.57 mm），2–5 s 高度波动
0.029 mm，最大姿态角 0.00062 rad，`FixedDown` 后高度 0.1338 m。

## 与大纲接口的差异

1. `A1Params.hip_offset` / `thigh_offset` 存成 `(4,3)` 的逐腿数组（比单个 `(3,)` 更能
   表达左右/前后镜像）；`hip_offsets` 也可从 `hip_offset[i]` 取。
2. `Quadruped.body_jacobian(q, leg, R_sb=None)` 默认返回机体系雅可比；传入 `R_sb`
   才旋转到世界系（大纲只写了“世界系”，但原签名没有 base 参数）。
3. `Leg.ik` 增加可选 `q_ref`：靠近工作空间边界时外摆角有两个解，默认取外摆角最小的
   解；轨迹控制应传入上一时刻关节角以保持分支连续。
4. `A1Sim` 把足端球接触调硬（`solref=[0.004,1]`、`solimp=[0.95,0.99,0.001,0.5,2]`），
   避免 Menagerie 软接触在整机重量下产生约 1 cm 下陷、污染站立高度验收。
5. 命令帧 CRC 覆盖前 28 字节（7 个 32 位字），与 T01 的说明一致；状态帧的
   `crc32_core` 只能按 32 位字取整，故覆盖前 72 字节（18 个字），T02 中 CRC 前的
   最后两个预留字节不参与。编解码两端一致，位翻转测试可检出。
6. `A1Sim.step(cmd, n_substeps)` 的力矩在进入时计算一次，`n_substeps` 个物理步内保持；
   如果需要每个物理步重新算 PD，应在外层循环调用 `step(cmd)`。
7. `plotting.savefig` 在调用时读取 `FIG_DIR`，便于测试脚本临时改输出目录；`FIG_DIR`
   是目录，`name` 不带扩展名时自动加 `.png`。

8. `GaitParams.trot` 默认 0.36 s/0.60，`GaitParams()` 默认同值；`walk` 用四拍偏移 `[0,0.5,0.25,0.75]`、`duty=0.75`，`pace`/`bound`/`pronk` 只改偏移和占空比。
9. `FeetEndCal.calc` 比大纲多了当前速度、偏航角速度、支撑/摆动时长参数；这些量由 `GaitGenerator` 持有，单腿落脚点无法只凭相位算出 Raibert 修正。
10. `Estimator.step` 的输入是 `BaseState + JointState + contact`，而不是裸 IMU 数组；`IMU` 类单独负责噪声模型，便于第 7 章示例替换。
11. `BalanceCtrl.solve_force` 的力约定是“地面对足端”，与 README 中 `sim.foot_forces()` 一致；`to_joint_torque` 已包含式（8.39）的负号，调用者不要再取反。
12. `SwingTrajectory` 增加了 `z1≠z0` 的线性修正（原书式 9.14 只回落到 `z0`）；平地时该项为零。

## 控制模块（第 7–10 章）

### 接口

```python
from quadruped import (IMU, AvgCov, Estimator, BalanceCtrl, GaitParams,
                       WaveGenerator, FeetEndCal, GaitGenerator,
                       SwingTrajectory, FreeStand, TrottingController)
```

- `IMU(acc_bias, gyro_bias, acc_std, gyro_std, seed)`：`measure(acc_b, omega_b)`
  返回带零偏/白噪声的读数；`world_acceleration(acc_meas, R_sb)` 实现式（7.7）。
- `AvgCov(dim, name, print_min, print_period, start_count, scale)`：
  `update(x)`（别名 `measure`）按式（7.71）/（7.76）递推，暴露 `mean`、`cov`、
  `count`、`std`。默认 `print_period=1000`、`start_count=0`、`scale=1`。
- `Estimator(robot, dt=0.002, R0=None, Cu=None, Qdig=None)`：
  `reset(base, joints, contact)` 后每周期调用
  `step(base, joints, contact)`，返回估计后的 `BaseState`；`position`、
  `velocity`、`foot_position(i, base, joints)`、`feet_positions`、
  `feet_velocities` 供控制器使用，另有原书命名的 `getPosition/getVelocity/
  getFootPos/getFeetPos/getFeetVel`。`start_covariance(start_count,
  print_period)`/`stop_covariance()` 用内部 `AvgCov` 复现原书 7.10 的
  `_RCheck`/`_uCheck` 统计，返回测得的 `R` 和 `Cu`。
- `BalanceCtrl(mass, inertia, fric_mu=0.6, f_max=90, w_dynamics=1.0,
  w_force=0.02, w_delta=0.01, S=None, W=None, U=None)`：不传 `inertia`
  时用原书式（6.32）的整机等效值 `diag(0.1320,0.3475,0.3775)`；两个控制
  状态则传入 Menagerie 整机折算值。`w_dynamics` 是
  `S = w_dynamics·I₆` 的标量简写：不传 `S` 时用它组装 P；显式传入 `S`
  时以 `S` 为准（两者都给时 `w_dynamics` 被忽略）。
  `desired_acceleration(...)` 返回 `(a_des, alpha_des)`；
  `solve_force(a_des, alpha_des, R, feet_rel_com, contact)` 返回世界系
  `(4,3)`“地面对足端”的力；`to_joint_torque(q, forces, R, robot)` 返回 12 维
  力矩；`step(...)` 是一次完成上述三步的便捷入口。
- `GaitParams.trot(period=0.36, duty=0.6, step_height=0.06)`（另有
  `.walk/.pace/.bound/.pronk/.from_name`）。`WaveGenerator(period, duty,
  offsets)` 的 `step(t, status)` 返回 `(phase(4), contact(4))`，
  `STANCE_ALL/WAVE_ALL/SWING_ALL` 三种状态。
- `FeetEndCal.calc(leg, v_cmd, w_cmd, base_pos, yaw, v_cur, w_cur, phase,
  stance_time, swing_time)` 返回世界系落点 `(3,)`（z 为 `ground_z`）。
- `GaitGenerator(robot, params, gait)`：`set_gait(v_cmd_world, w_cmd,
  step_height)`、`update(t, base_pos, yaw, v_cur, w_cur,
  feet_current=None, status=None)`、`foot_target(i)`、`foot_velocity(i)`。
- `SwingTrajectory(start, end, height, duration, kind="cycloid")`：
  `position(p)`、`velocity(p)`，`p∈[0,1]`；`kind="bezier"` 用五次贝塞尔。
- `FreeStand(context, robot, params)`：`set_pose(roll, pitch, yaw, height,
  dx, dy)` 只改传入的量；`run()` 做位姿 PD → QP → `τ`，并用期望位姿 +
  固定落脚点做关节 IK 目标。
- `TrottingController(context, robot, params, gait_params=None)`：
  `set_command(vx, vy, vyaw, body_height=None, step_height=None)`，`run()`
  组合估计器、步态、支撑腿 QP、摆动腿笛卡尔 PD，输出 12 维 `JointCmd`。
  `enter()` 先保持 0.2 s 全触地再起步；`exit()` 清零速度命令。

### 实现口径

- 估计器状态顺序为 `[p_b, v_b, p_f0..p_f3]`，输入 `u=R_sb a_b+g`，输出 28 维；
  量测矩阵按式（7.14）直接构造。摆动腿的对应量测块把 `R` 放大到 `1e6`，
  并把该腿的力/力矩当作不可信；支撑腿的足端高度量测取球心高度
  `foot_radius + foot_margin`（仿真 0.021 m，实机 0.02 m），即原书“足端高度为零”
  在球心模型下的写法。`reset()` 只用 IMU 姿态、关节角和接触，不读基座真值。
- `FreeStand` 和 `TrottingController` 都走同一链路：`read_sensors()` 取 IMU 姿态/
  角速度/加速度、关节 q/dq 和接触，`Estimator.step()` 更新机身位置/速度；
  姿态和角速度直接取 IMU。摆动腿的 `p_world -> p_body` 和足端速度补偿都改用
  估计的 `est.position`、`est.velocity`，不再调用 `get_state()`。
- `FreeStand` 的高度用估计值做 PD，并加一个限幅的慢积分修正
  （`z_trim_ki=2`，只改指令高度，最大下修 2 cm），消除腿 PD/接触柔度造成的
  几毫米下垂；横滚/俯仰/偏航仍由 QP + 位姿 PD 控制。水平方向另有一个
  限幅慢积分修正（`xy_trim_ki=2`、每轴最大 8 cm），只改期望水平位置。
  它用来消除调姿/推扰后足端微小滑移留下的几厘米静差；没有真值参与。
- QP 解出后还会把每个支撑足的切向力投影回摩擦棱锥边界。OSQP 的
  1e-5 收敛容差偶尔会留下约 \(10^{-4}\) N 的越界量；投影只改动这个量级，
  保证控制器输出的力严格满足验收用的 `|f_x|,|f_y| <= mu f_z`。
- QP 变量是 12 个世界系足端力；代价为
  `AᵀSA + αW + βU` 和 `-bᵀSA - βU f_prev`。默认 `S=I₆`（即
  `w_dynamics=1`）、`W=U=I₁₂`；显式 `S` 优先于 `w_dynamics`。
  每个支撑足 5 行摩擦四棱锥 + 1 行 `f_z≤f_max`；每个摆动足 3 行 `f=0`。
  OSQP 的 `P` 只传上三角，`P/A/l/u` 复用；失败时退回“重力均分”。
- `BalanceCtrl.to_joint_torque` 用 `τ_i=-J_iᵀRᵀf_i`（式 8.39 的符号），
  其中 `J_i` 是机身系雅可比。`solve_force` 的 `feet_rel_com` 是
  `p_foot-p_com`，用 `com_body_home` 作重心偏置。
- 步态相位：支撑腿 `phase` 是支撑相已过时间比例，摆动腿是摆动相比例，
  与式（9.5）一致；`trot` 的默认周期/占空比为 0.36 s/0.60。
- 摆线：`x,y` 用 `(2πp-sin2πp)/2π` 缩放，`z` 用 `(1-cos2πp)h/2`，
  并叠加 `(z1-z0)p` 处理起落高度差；两端速度为 0。
- `TrottingController` 的绝对 x/y 不进入反馈：18 状态滤波器的绝对位置在
  没有外部参考时不可观测，10 s 小跑会漂 ~0.2 m。控制器用估计速度、
  高度和姿态，落点/支撑腿 IK 统一锚在指令轨迹 `p_des` 上；这一点是
  相对原书代码结构的主要改动，见下节。

### 相对原书参数的调整

1. **步态占空比 0.60、周期 0.36 s**（原书 trot 示例 `r≈0.5`）。只有两条
   对角腿支撑时，6 维力/力矩方程对两个点接触的系数矩阵秩为 5（绕两接触点
   连线的力矩不可独立产生）；在 MuJoCo 中 `r=0.5` 时横向速度指令会激发
   逐渐增长的横滚振荡，`r=0.60` 增加双支撑段后消失。仍是 trot 的时序，
   相当于原书的 walking trot 参数。
2. **QP 权重**：`S=I₆`、`w_force=0.02`、`w_delta=0.01`。原书只给公式、没给
   数值；初版按单位权重、`w_delta=0.2` 时“力平滑”项压过动力学残差，足端力
   长期贴在上限。降低 `w_delta`、提高力的正则后，四条腿的力在 60–90 N 内。
3. **`f_max=90 N`**：`f=120 N` 时双支撑段会出现一条腿 120 N、另一条 30 N
   的极端分配；90 N 给 QP 留出重新分配的空间，同时仍大于单腿静态 30.5 N。
4. **单刚体惯量用 Menagerie 整机等效值**：`A1Params.body_equivalent_inertia(
   home_q)` 给出
   \[
   \begin{bmatrix}0.141070&-0.000254&-0.009661\\
   -0.000254&0.367186&-0.000396\\
   -0.009661&-0.000396&0.399641\end{bmatrix}\ \mathrm{kg\,m^2},
   \]
   对角元与原书式（6.32）的 `0.1320/0.3475/0.3775` 分别差 +6.9%/+5.7%/+5.9%。
   选 Menagerie 值而不是式（6.32）的原因：本书其余几何、质量和接触参数都来自
   同一份 Menagerie 模型，第 6.3 节的平行轴求和给出的就是这三个数；式（6.32）
   是原书对 A1 的独立取整，保留作对照。质量取 `A1Params.mass=12.453 kg`。
   不再使用 `trunk_inertia`（约小 8 倍）；仿真里腿的惯量由 MuJoCo 自己积分，
   与这里的整机等效口径一致。
5. **关节/笛卡尔增益**：支撑与摆动腿关节 `kp=[60,150,200]`、`kd=[2,3,3]`，
   摆动腿笛卡尔 `Kp=400I`、`Kd=12I`；`FreeStand` 用同一组关节增益，外加高度
   和水平积分修正。原书未给具体数；这套值在 2 ms 周期下不抖，且足端落地速度
   接近 0。
6. **估计器协方差**：`R=diag(2e-6×12, 5e-4×12, 5e-3×4)`，
   `Cu=0.02I₃`，`Qdig=[1e-8×3, 1e-5×3, 2e-5×12]`。用静止/小跑数据调；
   摆动腿量测块放大到 `1e6`。高度量测用固定球心高度 0.021 m（球半径 + MuJoCo
   接触 margin），而不是原书的 0（原书状态是接触点，本书状态是足端球心）。
7. **不含绝对位置反馈**：见“实现口径”最后一条。估计器仍输出绝对位置，
   但 10 s 小跑 x 漂移约 0.21 m（无外部观测时的固有限制）；控制器的
   位置环只保留高度，x/y 用速度环。`FreeStand` 以进入时的估计位置为锚，
   只做局部位姿保持。
8. **控制器不再使用仿真真值**：`FreeStand` 和 `TrottingController` 都只读
   `read_sensors()` 和估计器；`get_state()` 仅用于验收/画图。`test_no_truth.py`
   在真值位置/速度被替换为 NaN 时跑 3 s 站立和 5 s 小跑，输出力矩仍有限且
   机身高度保持。
9. **姿态增益按轴分开，偏航环刻意软**：整机惯量是躯干值的约 8–9 倍，若继续用
   原来的标量 `kp_rot=150`，同样的姿态误差会命令约 8 倍的偏航力矩；A1 足端是
   球形点接触，偏航力矩只能靠四个切向摩擦力的力偶提供，结果足端打滑、偏航
   残差反而变大。现在 `Kp_rot=diag(150,150,50)`、`Kd_rot=diag(18,18,12)`：
   横滚/俯仰保持原刚度，偏航降到 50，阻尼比约 0.85。这个改动配合水平积分
   修正后，FreeStand 的八个位姿阶跃和 30 N 推扰全部回到原判据内。

### 测试与验收

```bash
MUJOCO_GL=egl build/venv/bin/python -m pytest src/code/tests -q
MUJOCO_GL=egl PYTHONPATH=src/code build/venv/bin/python src/code/tests/sim_acceptance.py
```

新增测试：`test_estimator.py`（IMU、`AvgCov`、KF 维数）、`test_gait.py`
（步态相位/接触、Raibert 落点、摆线端点）、`test_balance.py`（FreeStand
八组位姿阶跃、30 N 推扰、摩擦锥）、`test_trotting.py`（10 s 前进、反向、
侧移、偏航、归零与切回站立）、`test_inertia.py`（整机等效惯量对第 6.3 节的
平行轴求和、对称正定、随站立构型变化）、`test_no_truth.py`（真值位置/速度
置 NaN 后跑 FreeStand 3 s 和 Trot 5 s）。`sim_acceptance.py` 生成
`build/scratch/pkg-control/` 下的 `free_pose.png`、`free_push.png`、
`estimator.png`、`trot_height_attitude.png`、`trot_phase_forces.png`、
`trot_commands.png`。

本机实测（`build/venv` Python 3.12、MuJoCo 3.17、OSQP 1.1.3）：

| 项目 | 判据 | 实测 |
| --- | --- | --- |
| FreeStand 横滚/俯仰/偏航 ±0.15 rad | 1 s 内误差 <0.03 rad | 最大 0.0227 rad（yaw+ 阶跃）；yaw− 0.0041 rad；height+ 时 yaw 0.0027 rad，height− 0.0041 rad；roll/pitch 各阶跃最大 0.0195 rad |
| FreeStand 高度 ±3 cm | 1 s 内误差 <5 mm | +3 cm 2.75 mm，−3 cm 2.87 mm |
| 30 N 水平推力 0.2 s | 不摔、2 s 回原位姿 | 最大倾角 0.0235 rad，2 s 时姿态误差 0.0179 rad、高度差 0.08 mm、xy 0.18 mm |
| QP 摩擦锥 | 每个支撑足满足 | 0 次违反（`test_trotting`/验收脚本逐周期检查） |
| 估计器速度 RMSE | <0.1 m/s | 0.0173 m/s（轴 RMSE 0.0235/0.0068/0.0171） |
| 估计器高度误差 | <2 cm | RMSE 12.25 mm，最大 13.87 mm |
| 小跑 vx=0.3 m/s 10 s | 高度 >0.18 m、平均 vx 0.24–0.36 | 最低 0.2687 m，平均 0.3235 m/s |
| vx=−0.2 m/s | 跟踪 | −0.2167 m/s |
| vy=0.15 m/s | 跟踪 | 0.1612 m/s |
| 原地 yaw 0.6 rad/s | 角速度误差 <30% | 0.5967 rad/s（−0.55%） |
| 归零 + 切回站立 | 稳定踏步、站高接近 0.27 | 归零 2 s 最低 0.2722 m、平均 vx −0.0032；切回后 0.2706 m |
| 防作弊（真值 NaN） | 力矩无 NaN、不摔 | FreeStand 3 s、Trot 5 s 均通过（`test_no_truth.py`） |

`FreeStand` 推扰时 QP 的峰值法向力 90.0 N（达到 `f_max`）；小跑双支撑段单足峰值约 90 N。


## 导航模块（第 11 章）

### 接口

```python
from quadruped import (Lidar2D, GridMap, AStarPlanner, PurePursuit,
                       NavStack, NavStats)

lidar = Lidar2D(sim, n_beams=360, max_range=6.0,
                angle_min=-np.pi, angle_max=np.pi, noise_std=0.0)
ranges = lidar.scan(est.position, yaw)       # (360,), inf = 无回波
grid = GridMap(x_min=-1.0, y_min=-2.5, x_max=5.0, y_max=2.5,
               resolution=0.05, robot_half_width=0.16, safety_margin=0.06)
# 也可以从已知占据栅格直接建图（occupancy[iy, ix]，origin 为左下角）：
grid = GridMap.from_occupancy(occ, 0.05, origin=(x_min, y_min),
                              robot_half_width=0.16, safety_margin=0.06)
grid.update_scan(ranges, lidar.last_origin, yaw,
                 angles=lidar.angles, max_range=6.0)
planner = AStarPlanner(grid)
path = planner.plan(start_xy, goal_xy)       # (N,2)；失败时 N=0
tracker = PurePursuit(lookahead=0.45, v_max=0.32, w_max=1.0)
vx, wz, arrived = tracker.compute(position_xy, yaw, path)
nav = NavStack(sim, trot, goal_xy, grid=grid, scan_hz=10.0, plan_hz=2.0)
nav.reset(keep_map=True)      # 保留静态地图；默认 reset() 会清空
nav.step()                    # 更新地图/路径并给 trot 写速度
cmd = trot.run()
sim.step(cmd)
```

- `Lidar2D` 用 `mujoco.mj_multiRay`。默认 360 束、1°、量程 6 m、最小 0.05 m，无回波返回 `inf`；
  `noise_std` 给量程加高斯噪声，`mount` 是机体系安装位置（默认机身上方 0.08 m）。
  默认 `geomgroup=(1,1,0,0,1,1)`：A1 的视觉/碰撞几何在组 2/3，因此不会打到自身；环境几何在组 0/1。
  扫描位姿是**估计器的 `position` + IMU 的 `rpy[2]`**，不接受仿真真值。
- `GridMap` 是 `[iy, ix]` 布局的 log-odds 栅格：命中 `+0.85`、穿过 `-0.40`，截断到 `[-3.5, 3.5]`；
  占据阈值 0.60、空闲阈值 0.40。`update_ray` 用整数 Bresenham 精确标记一条射线；
  `update_scan` 默认把 360 条射线按栅格步长采样后一次向量化更新（360 束约 0.4–0.7 ms），
  明显快于逐格 Python 循环。未知格默认可通行，已见障碍按
  `robot_half_width + safety_margin = 0.22 m` 做圆盘膨胀；`traversable()` 给出规划掩码。
  `GridMap.from_occupancy(occupancy, resolution, origin=(x_min, y_min), ...)`
  从已知栅格建图：数组布局 `[iy, ix]`，`origin` 是 `(0,0)` 格左下角的世界坐标，
  所有格标为已观测，并立即完成膨胀；无需再写 `log_odds`、`observed`
  或清理私有缓存。
- `AStarPlanner` 是 8 邻域 A*：octile 启发、禁止斜穿障碍角、失败/超时返回空路径；
  `plan()` 先做视线捷径再用移动平均平滑，并保留首尾精确端点。`last_expanded`、`last_time`、
  `last_raw_length`、`last_length` 给出扩展节点数、耗时和路径长度。
- `PurePursuit` 先把当前位置投影到折线，再取前视距离 `lookahead` 处的点：
  `alpha = wrap(atan2(dy,dx)-yaw)`，`wz = vx·2sin(alpha)/Ld`，并限幅到
  `[0,v_max]`、`[-w_max,w_max]`；接近目标减速，航向误差大时原地转。
  `goal_tolerance` 内返回 `arrived=True` 和零速度。
- `NavStack` 每 2 ms 只做投影/纯跟踪和 `controller.set_command()`；扫描默认 10 Hz，规划默认 2 Hz。
  `reset(keep_map=False)` 默认清空地图；`reset(keep_map=True)` 只重置计时器、路径和统计，
  保留已建好的静态/障碍层（例如 `from_occupancy` 的结果）。
  地图、路径历史、扫描历史、`NavStats`（扫描/规划次数、失败次数、耗时）都可读。
  控制回路中不使用 `get_state()`；验收脚本里的真值只用于判分和画图。

### 场景

`src/code/assets/nav_scene.xml` 用 MJCF `<include file="menagerie/unitree_a1/a1.xml"/>`
引用 Menagerie 模型，不复制任何模型文件；场景里加了三块静态箱子（`crate_mid`、
`crate_left`、`crate_right`，几何组 1）。加载方式：

```python
sim = A1Sim(model_path="src/code/assets/nav_scene.xml")
# 或
# QUADRUPED_A1_XML=src/code/assets/nav_scene.xml
```

`a1.xml` 的 `meshdir="assets"` 是相对其自身目录解析的，而 `nav_scene.xml` 在上一级
`assets/` 目录，所以场景在 `<include>` 之后写了一个相对 `meshdir` 覆盖；这样不依赖绝对路径。
环境里的箱子都在雷达量程内，A* 在第一次扫描后就能看到中间箱子并规划绕行。

### 测试与验收

```bash
MUJOCO_GL=egl build/venv/bin/python -m pytest src/code/tests -q
MUJOCO_GL=egl PYTHONPATH=src/code build/venv/bin/python src/code/tests/nav_acceptance.py
```

本机全量 `pytest` 为 48 passed（其中 `test_nav.py` 13 项）。`test_nav.py` 覆盖：

- 已知单条射线的 free/occupied/unknown 结果、无回波射线的清空、膨胀半径；
- A* 在带缺口墙上与独立 Dijkstra 的最短路长度逐位一致（三组起终点）、开阔图直线化、
  封闭目标返回空路径；
- 纯跟踪在直线/左弧/右弧上的速度与角速度符号、限幅、到点判断和原地转；
- `nav_scene.xml` 经 `A1Sim` 加载、正前方 1.05 m 命中中间箱子、雷达不命中自身，
  以及 `QUADRUPED_A1_XML` 环境变量加载。

`nav_acceptance.py` 从 `home` 站立开始，先保持 1 s，再进入 `TrottingController` 小跑；
目标为 `(3.0, 0.0)`，中间三块箱子迫使机器人绕到 `y≈1.6 m`。60 s 上限内以真值判断。
本机实测（`build/venv` Python 3.12、MuJoCo 3.17）：

| 项目 | 判据 | 实测 |
| --- | --- | --- |
| 到达时间（真值首次 <0.3 m） | <60 s | 16.68 s |
| `NavStack` 判定到达 | — | 17.32 s |
| 终点误差（真值） | <0.30 m | 0.2710 m |
| 定位漂移（末值/RMSE/最大） | 记录 | 0.2337 / 0.1862 / 0.2410 m |
| 偏航估计 RMSE | 记录 | 0.0006 rad |
| 与箱子最近距离（机身中心） | 不碰 | 0.2468 m，接触 0 次 |
| 摔倒 | 无 | 无（最低机身高度约 0.272 m） |
| 实际路径长度 | 记录 | 5.060 m |
| 扫描/规划次数 | 10 Hz / 2 Hz | 184 / 37，失败 0 |
| 单次扫描 / 规划耗时 | 2 ms 周期内只做轻量计算 | 0.69 / 0.33 ms（多次 0.65–0.69 / 0.29–0.39 ms） |

`build/scratch/pkg-nav/` 下的图：`nav_map.png`（占据栅格、膨胀层、规划路径、实际轨迹、目标）、
`nav_lidar.png`（起点 360 束扫描）、`nav_tracking.png`（估计/真值、漂移、速度指令）、
`nav_scene.png`（终点离屏渲染），以及 `summary.txt` 和 `pytest.log`。

漂移来源与第 7、10 章一致：没有外部绝对位置观测时，足端里程计的 x/y 会随打滑缓慢漂移；
本次 17 s 导航末值约 0.23 m。若实机需要更高到点精度，应加激光/视觉地图匹配或外部定位，
而不是把仿真真值接进控制器。
