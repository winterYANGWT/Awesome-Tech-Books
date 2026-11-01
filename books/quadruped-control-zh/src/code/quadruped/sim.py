"""MuJoCo wrapper for the Unitree A1.

The simulator exposes the same five quantities as a joint motor:
``q_des, dq_des, kp, kd, tau_ff``.  The mixed-control torque is computed in
Python and applied as a generalized force, so the implementation is identical
for every actuator type in the MJCF.

Implementation choice
----------------------
The Menagerie A1 uses MuJoCo ``position`` actuators.  At load time their gain
and bias parameters are set to zero (the actuators become inert), and the
computed joint torques are written to ``data.qfrc_applied``.  This avoids
depending on actuator order or on MuJoCo's internal PD law, and makes the
``kp=0`` torque-mode case exact.  All torques are clipped to the actuator
force range read from the model (33.5 N m for the A1).

The model has no IMU sensors, so a site named ``imu`` and three sensors
(gyro, accelerometer, frame quaternion) are inserted into an in-memory copy of
the spec before compilation.  The source MJCF on disk is never modified.

Measured vs. true state
-----------------------
:meth:`A1Sim.read_sensors` returns the IMU, joint-encoder and foot-contact
readings available on the robot.  :meth:`A1Sim.get_state` additionally returns
MuJoCo's base position and velocity, which are ground truth and must only be
used by verification/acceptance scripts and plots, never by a controller.
"""

from __future__ import annotations

import os
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

import numpy as np

try:
    import mujoco
except ImportError as exc:  # pragma: no cover - environment error path
    raise ImportError("A1Sim requires the 'mujoco' package") from exc

from .motor import hybrid_torque
from .robot import default_model_path
from .rotation import R_to_rpy, quat_to_R
from .types import (BaseState, JOINT_NAMES, JointCmd, JointState, LegId,
                    MotorMode)


@dataclass
class SensorReadings:
    """Everything a real A1 controller can measure in one cycle.

    The fields come from the IMU (``quat``, ``rpy``, ``omega_b``, ``acc_b``),
    the twelve joint encoders (``joints.q`` / ``joints.dq`` and the commanded
    torque in ``joints.tau``) and the foot contact switches (``contact``).
    There is deliberately no base position or velocity here: those are not
    measured by the robot and must come from the state estimator.

    ``base_state()`` is a convenience for the estimator, which expects a
    :class:`~quadruped.types.BaseState`.  The returned ``position`` and
    ``velocity`` are placeholders (zeros), **not** measurements; the estimator
    initializes them from its own model and the known start pose.
    """

    quat: np.ndarray
    rpy: np.ndarray
    omega_b: np.ndarray
    acc_b: np.ndarray
    joints: JointState
    contact: np.ndarray
    timestamp: float = 0.0

    def __post_init__(self) -> None:
        self.quat = np.asarray(self.quat, dtype=float).reshape(4)
        self.rpy = np.asarray(self.rpy, dtype=float).reshape(3)
        self.omega_b = np.asarray(self.omega_b, dtype=float).reshape(3)
        self.acc_b = np.asarray(self.acc_b, dtype=float).reshape(3)
        self.contact = np.asarray(self.contact, dtype=bool).reshape(4)

    def base_state(self) -> BaseState:
        """IMU attitude/rate/acceleration as a ``BaseState`` placeholder."""
        return BaseState(
            position=np.zeros(3), velocity=np.zeros(3),
            quat=self.quat.copy(), rpy=self.rpy.copy(),
            omega_b=self.omega_b.copy(), acc_b=self.acc_b.copy(),
            timestamp=float(self.timestamp),
        )


class A1Sim:
    """MuJoCo A1 with a motor-like hybrid-control interface."""

    def __init__(self, model_path: str | None = None, timestep: float = 0.002,
                 render_size: tuple[int, int] = (640, 480),
                 stiff_foot_contact: bool = True) -> None:
        self.model_path = str(model_path) if model_path else default_model_path()
        if not Path(self.model_path).exists():
            raise FileNotFoundError(
                f"A1 model not found: {self.model_path}; "
                "run src/code/fetch_a1.sh or set QUADRUPED_A1_XML")
        self.timestep = float(timestep)
        self.render_size = (int(render_size[0]), int(render_size[1]))

        spec = mujoco.MjSpec.from_file(self.model_path)
        self._add_sensors_and_camera(spec)
        model = spec.compile()

        model.opt.timestep = self.timestep
        # Turn the position servos into inert actuators; torque goes through
        # qfrc_applied instead (documented in README.md).
        model.actuator_gainprm[:, :] = 0.0
        model.actuator_biasprm[:, :] = 0.0

        self.model = model
        self.data = mujoco.MjData(model)

        self.joint_names = list(JOINT_NAMES)
        self.joint_ids = np.array(
            [model.joint(name).id for name in self.joint_names], dtype=int)
        self.qpos_adr = np.array(
            [model.jnt_qposadr[j] for j in self.joint_ids], dtype=int)
        self.dof_adr = np.array(
            [model.jnt_dofadr[j] for j in self.joint_ids], dtype=int)
        self.actuator_ids = np.array(
            [model.actuator(name.replace("_joint", "")).id
             for name in self.joint_names], dtype=int)
        self.tau_max = np.abs(
            np.asarray(model.actuator_forcerange[self.actuator_ids, 1],
                       dtype=float))

        self.foot_geom_ids = self._find_foot_geoms()
        self.foot_body_ids = np.array(
            [model.geom_bodyid[g] for g in self.foot_geom_ids], dtype=int)
        self.hip_body_ids = np.array(
            [model.body(f"{leg.name}_hip").id for leg in LegId], dtype=int)

        self._sensor_adr = self._sensor_addresses()
        self.home_q, self.home_height = self._read_home()
        if stiff_foot_contact:
            self._stiffen_feet()

        self._tau = np.zeros(12)
        self._cmd: JointCmd | None = None
        self._renderer = None
        self._renderer_size: tuple[int, int] | None = None
        self._closed = False
        # Start from the model's home keyframe rather than MuJoCo's raw
        # qpos0 (which has all joints at zero and the base at 0.43 m).
        self.reset()

    # ------------------------------------------------------------------
    # Construction helpers
    # ------------------------------------------------------------------
    @staticmethod
    def _add_sensors_and_camera(spec) -> None:
        trunk = spec.body("trunk")
        site_names = [s.name for s in trunk.sites]
        if "imu" not in site_names:
            trunk.add_site(name="imu", pos=[0.0, 0.0, 0.0],
                           size=[0.005, 0.005, 0.005])
        sensor_names = {s.name for s in spec.sensors}
        definitions = (
            ("imu_gyro", mujoco.mjtSensor.mjSENS_GYRO),
            ("imu_acc", mujoco.mjtSensor.mjSENS_ACCELEROMETER),
            ("imu_quat", mujoco.mjtSensor.mjSENS_FRAMEQUAT),
        )
        for name, stype in definitions:
            if name not in sensor_names:
                spec.add_sensor(name=name, type=stype,
                                objtype=mujoco.mjtObj.mjOBJ_SITE,
                                objname="imu")
        camera_names = {c.name for c in spec.worldbody.cameras}
        if "track" not in camera_names:
            spec.worldbody.add_camera(
                # TARGETBODY keeps the camera pointed at the trunk; plain TRACK only follows its
                # position and keeps the default orientation, which looks straight down.
                name="track", mode=mujoco.mjtCamLight.mjCAMLIGHT_TARGETBODY,
                targetbody="trunk", pos=[0.75, -0.75, 0.45], fovy=45.0)

    def _find_foot_geoms(self) -> np.ndarray:
        """Return the four foot-sphere geom ids in leg order."""
        found: dict[int, int] = {}
        for gid in range(self.model.ngeom):
            if int(self.model.geom_type[gid]) != int(mujoco.mjtGeom.mjGEOM_SPHERE):
                continue
            body_name = self.model.body(self.model.geom_bodyid[gid]).name
            for leg in LegId:
                if body_name == f"{leg.name}_calf":
                    found[int(leg)] = gid
        if len(found) != 4:
            raise RuntimeError("could not identify the four A1 foot spheres")
        return np.array([found[i] for i in range(4)], dtype=int)

    def _sensor_addresses(self) -> dict[str, int]:
        out = {}
        for name in ("imu_gyro", "imu_acc", "imu_quat"):
            sid = self.model.sensor(name).id
            out[name] = int(self.model.sensor_adr[sid])
        return out

    def _read_home(self) -> tuple[np.ndarray, float]:
        for i in range(int(self.model.nkey)):
            k = self.model.key(i)
            if k.name == "home":
                q = np.asarray(k.qpos, dtype=float)
                if q.size >= 19:
                    return q[7:19].copy(), float(q[2])
        return np.array([0.0, 0.9, -1.8] * 4), 0.27

    def _stiffen_feet(self) -> None:
        """Make the rubber foot contact stiff enough for standing tests.

        The Menagerie foot uses a soft contact (about 1 cm penetration at the
        A1 weight), which would make the measured standing height depend on
        the contact solver.  A stiffer solref/solimp keeps the penetration
        below 2 mm while preserving the sphere geometry and friction.
        """
        for gid in self.foot_geom_ids:
            self.model.geom_solref[gid] = [0.004, 1.0]
            self.model.geom_solimp[gid] = [0.95, 0.99, 0.001, 0.5, 2.0]

    # ------------------------------------------------------------------
    # State and command
    # ------------------------------------------------------------------
    @property
    def joint_positions(self) -> np.ndarray:
        return np.asarray(self.data.qpos[self.qpos_adr], dtype=float).copy()

    @property
    def joint_velocities(self) -> np.ndarray:
        return np.asarray(self.data.qvel[self.dof_adr], dtype=float).copy()

    def reset(self, q0: np.ndarray | None = None,
              base_pos: tuple[float, float, float] = (0.0, 0.0, 0.27),
              base_quat: np.ndarray | None = None) -> BaseState:
        """Reset the simulation and return the new base **truth** state."""
        mujoco.mj_resetData(self.model, self.data)
        q = self.home_q if q0 is None else np.asarray(q0, dtype=float).reshape(12)
        self.data.qpos[0:3] = np.asarray(base_pos, dtype=float).reshape(3)
        if base_quat is None:
            self.data.qpos[3:7] = [1.0, 0.0, 0.0, 0.0]
        else:
            qq = np.asarray(base_quat, dtype=float).reshape(4)
            self.data.qpos[3:7] = qq / np.linalg.norm(qq)
        self.data.qpos[self.qpos_adr] = q
        self.data.qvel[:] = 0.0
        self.data.qfrc_applied[:] = 0.0
        self._tau = np.zeros(12)
        self._cmd = None
        mujoco.mj_forward(self.model, self.data)
        return self.get_state()[0]

    def set_joint_pd(self, cmd: JointCmd | np.ndarray) -> None:
        """Compute and store the hybrid-control torque from ``cmd``."""
        cmd = self._coerce_cmd(cmd)
        tau = hybrid_torque(self.joint_positions, self.joint_velocities, cmd)
        self._cmd = cmd
        self._tau = np.clip(tau, -self.tau_max, self.tau_max)

    def apply_torque(self, tau: np.ndarray) -> None:
        """Apply a joint torque vector directly, with actuator-limit clipping."""
        tau = np.asarray(tau, dtype=float).reshape(12)
        self._cmd = None
        self._tau = np.clip(tau, -self.tau_max, self.tau_max)

    def step(self, cmd: JointCmd | np.ndarray | None = None,
             n_substeps: int = 1) -> BaseState:
        """Advance ``n_substeps`` physics steps holding the computed torque.

        The returned :class:`BaseState` is ground truth; it is meant for the
        caller's verification/plotting, not as a controller input.
        """
        if cmd is not None:
            self.set_joint_pd(cmd)
        self.data.qfrc_applied[:] = 0.0
        self.data.qfrc_applied[self.dof_adr] = self._tau
        for _ in range(int(n_substeps)):
            mujoco.mj_step(self.model, self.data)
        return self.get_state()[0]

    def _coerce_cmd(self, cmd: JointCmd | np.ndarray) -> JointCmd:
        if isinstance(cmd, JointCmd):
            return cmd
        arr = np.asarray(cmd, dtype=float)
        if arr.shape == (12,):
            # Treat a bare vector as a position command with a modest gain.
            # This is a convenience; explicit JointCmd is preferred.
            return JointCmd(arr, np.zeros(12), 80.0, 2.0, np.zeros(12))
        if arr.shape == (5, 12):
            return JointCmd(arr[0], arr[1], arr[2], arr[3], arr[4])
        if arr.shape == (12, 5):
            return JointCmd(arr[:, 0], arr[:, 1], arr[:, 2], arr[:, 3],
                            arr[:, 4])
        raise ValueError("array command must have shape (12,), (5,12) or (12,5)")

    def _joint_state(self) -> JointState:
        return JointState(
            q=self.joint_positions,
            dq=self.joint_velocities,
            tau=self._tau.copy(),
            temperature=np.full(12, 25.0),
            error=np.zeros(12, dtype=int),
            mode=np.full(12, int(MotorMode.SERVO), dtype=int),
            timestamp=float(self.data.time),
        )

    def _imu_reading(self) -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
        """Read the three IMU sensors: quaternion, gyro and accelerometer."""
        d = self.data
        quat = np.asarray(d.sensordata[
            self._sensor_adr["imu_quat"]:self._sensor_adr["imu_quat"] + 4],
            dtype=float).copy()
        omega = np.asarray(d.sensordata[
            self._sensor_adr["imu_gyro"]:self._sensor_adr["imu_gyro"] + 3],
            dtype=float).copy()
        acc = np.asarray(d.sensordata[
            self._sensor_adr["imu_acc"]:self._sensor_adr["imu_acc"] + 3],
            dtype=float).copy()
        if not np.all(np.isfinite(quat)) or np.linalg.norm(quat) < 1e-12:
            quat = np.array([1.0, 0.0, 0.0, 0.0])
        quat = quat / np.linalg.norm(quat)
        rpy = R_to_rpy(quat_to_R(quat))
        return quat, rpy, omega, acc

    def read_sensors(self) -> SensorReadings:
        """Return only the quantities a physical A1 can measure.

        IMU attitude/angular rate/acceleration, joint encoders and the four
        foot contact switches.  Controllers must use this interface (plus the
        state estimator); :meth:`get_state` is for validation and plotting.
        """
        quat, rpy, omega, acc = self._imu_reading()
        return SensorReadings(
            quat=quat, rpy=rpy, omega_b=omega, acc_b=acc,
            joints=self._joint_state(), contact=self.foot_contacts(),
            timestamp=float(self.data.time),
        )

    def get_state(self) -> tuple[BaseState, JointState]:
        """Return base ground truth and joint measurements.

        .. warning::
           ``BaseState.position`` and ``BaseState.velocity`` are MuJoCo's
           simulation truth.  They are intended **only** for verification,
           acceptance metrics and plotting.  A controller must never read
           them; use :meth:`read_sensors` and the Chapter 7 estimator instead.
        """
        quat, rpy, omega, acc = self._imu_reading()
        d = self.data
        base = BaseState(
            position=np.asarray(d.qpos[0:3], dtype=float).copy(),
            velocity=np.asarray(d.qvel[0:3], dtype=float).copy(),
            quat=quat,
            rpy=rpy,
            omega_b=omega,
            acc_b=acc,
            timestamp=float(d.time),
        )
        return base, self._joint_state()

    # ------------------------------------------------------------------
    # Contacts, rendering, cleanup
    # ------------------------------------------------------------------
    def foot_contacts(self) -> np.ndarray:
        """Boolean contact flag for each foot sphere, shape ``(4,)``."""
        flags = np.zeros(4, dtype=bool)
        for i, gid in enumerate(self.foot_geom_ids):
            for c in range(self.data.ncon):
                con = self.data.contact[c]
                if con.geom1 == gid or con.geom2 == gid:
                    flags[i] = True
                    break
        return flags

    def foot_forces(self) -> np.ndarray:
        """World-frame contact force on each foot body, shape ``(4, 3)``."""
        # MuJoCo's cfrc_ext layout is [torque(3), force(3)]; the force part
        # is the one needed here.
        forces = np.asarray(self.data.cfrc_ext[self.foot_body_ids, 3:6],
                            dtype=float).copy()
        # cfrc_ext also contains tiny solver noise when a foot is in free
        # flight; zero it when the corresponding foot sphere is not touching.
        contacts = self.foot_contacts()
        forces[~contacts] = 0.0
        return forces

    def foot_positions_world(self) -> np.ndarray:
        """Foot-sphere centres computed by MuJoCo, world frame."""
        return np.asarray(self.data.geom_xpos[self.foot_geom_ids],
                          dtype=float).copy()

    def foot_positions_body(self) -> np.ndarray:
        """Foot-sphere centres expressed in the body frame, shape ``(4,3)``."""
        R = quat_to_R(np.asarray(self.data.qpos[3:7], dtype=float))
        feet = self.foot_positions_world()
        return (R.T @ (feet - np.asarray(self.data.qpos[0:3])).T).T

    def foot_positions_leg(self) -> np.ndarray:
        """Foot centres in each leg base frame ``{0}``, shape ``(4,3)``."""
        R = quat_to_R(np.asarray(self.data.qpos[3:7], dtype=float))
        hip = np.asarray(self.data.xpos[self.hip_body_ids], dtype=float)
        feet = self.foot_positions_world()
        return (R.T @ (feet - hip).T).T

    def hip_positions_world(self) -> np.ndarray:
        return np.asarray(self.data.xpos[self.hip_body_ids], dtype=float).copy()

    def render(self, camera: str = "track", width: int = 640,
               height: int = 480) -> np.ndarray:
        """Render one RGB frame off-screen."""
        size = (int(width), int(height))
        if self._renderer is None or self._renderer_size != size:
            if self._renderer is not None:
                self._renderer.close()
            # A larger shadow map removes the shadow acne the default 4096 leaves on figures.
            self.model.vis.quality.shadowsize = 8192
            self._renderer = mujoco.Renderer(self.model, size[1], size[0])
            self._renderer_size = size
        self._renderer.update_scene(self.data, camera=camera)
        return np.asarray(self._renderer.render()).copy()

    def close(self) -> None:
        if getattr(self, "_renderer", None) is not None:
            self._renderer.close()
            self._renderer = None
            self._renderer_size = None
        self._closed = True

    def __enter__(self) -> "A1Sim":
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        self.close()

    def __del__(self):
        try:
            self.close()
        except Exception:
            pass


__all__ = ["A1Sim", "SensorReadings"]
