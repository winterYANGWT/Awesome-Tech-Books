"""Shared code package for the quadruped control book.

Modules
-------
``types``      shared dataclasses and enums
``rotation``   SO(3), SE(3), Euler angles and quaternions
``leg``        one-leg FK/IK/Jacobian/statics
``robot``      A1 parameters and the four-leg model
``sim``        MuJoCo A1 wrapper with a motor-style interface
``motor``      hybrid control, motor model and RS-485 frames
``fsm``        finite-state machine and the stand/down states
``plotting``   consistent figure output
``estimator``  IMU model, 18-state Kalman filter and covariance statistics
``balance``    pose PD, friction pyramid and QP foot-force allocation
``gait``       gait phases, footholds and the walking generator
``swing``      cycloid/Bezier swing trajectories
``trotting``   FreeStand and TrottingController states
``nav``        2D lidar, occupancy grid, A*, pure pursuit and NavStack
"""

from .types import (BaseState, ContactState, ControlMode, GaitCommand,
                    JointCmd, JointState, LegCommand, LegId, MotorMode)
from .rotation import (R2, Rx, Ry, Rz, adjoint, inv_T, make_T, quat_to_R,
                       R_to_quat, R_to_rpy, rpy_to_R, skew, so3_exp,
                       so3_log)
from .leg import Leg
from .robot import A1Params, Quadruped, default_model_path
from .motor import (Motor, MotorBus, MotorCmd, MotorState, Rs485MotorBus,
                    SimMotorBus, crc32, crc32_core, decode_master_cmd,
                    decode_servo_state, encode_master_cmd,
                    encode_servo_state, hybrid_torque)
from .fsm import (ControlContext, FSM, FixedDown, FixedStand, Passive, State,
                  StateID)
from .plotting import savefig
from .sim import A1Sim, SensorReadings
from .estimator import AvgCov, Estimator, IMU
from .balance import BalanceCtrl
from .gait import FeetEndCal, GaitGenerator, GaitParams, WaveGenerator, WaveStatus
from .swing import SwingTrajectory
from .trotting import FreeStand, TrottingController
from .nav import AStarPlanner, GridMap, Lidar2D, NavStack, NavStats, PurePursuit

__all__ = [
    "BaseState", "ContactState", "ControlMode", "GaitCommand", "JointCmd",
    "JointState", "LegCommand", "LegId", "MotorMode", "R2", "Rx", "Ry", "Rz",
    "adjoint", "inv_T", "make_T", "quat_to_R", "R_to_quat", "R_to_rpy",
    "rpy_to_R", "skew", "so3_exp", "so3_log", "Leg", "A1Params",
    "Quadruped", "default_model_path", "Motor", "MotorBus", "MotorCmd",
    "MotorState", "Rs485MotorBus", "SimMotorBus", "crc32", "crc32_core",
    "decode_master_cmd", "decode_servo_state", "encode_master_cmd",
    "encode_servo_state", "hybrid_torque", "ControlContext", "FSM",
    "FixedDown", "FixedStand", "Passive", "State", "StateID", "savefig",
    "A1Sim", "SensorReadings", "IMU", "AvgCov", "Estimator", "BalanceCtrl", "GaitParams",
    "WaveGenerator", "WaveStatus", "FeetEndCal", "GaitGenerator",
    "SwingTrajectory", "FreeStand", "TrottingController", "Lidar2D",
    "GridMap", "AStarPlanner", "PurePursuit", "NavStack", "NavStats",
]
