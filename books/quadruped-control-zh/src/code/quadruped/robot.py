"""Whole-robot parameters and the four-leg kinematic model.

``A1Params`` reads the geometry, mass and joint limits from the MuJoCo
Menagerie A1 MJCF at run time.  Nothing in this module is hard-coded for a
particular machine: the values below are only defaults used when a field is
missing from the XML.

``Quadruped`` combines four :class:`~quadruped.leg.Leg` objects.  Its methods
are pure kinematics; the simulator is used only by ``A1Params.from_menagerie``
to read the source data, never to evaluate a controller's kinematics.
"""

from __future__ import annotations

import os
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np

from .leg import Leg
from .rotation import quat_to_R, so3_exp
from .types import JOINT_NAMES, LegId


def default_model_path() -> str:
    """Return the default A1 scene path, honouring ``QUADRUPED_A1_XML``."""
    env = os.environ.get("QUADRUPED_A1_XML")
    if env:
        return env
    root = Path(__file__).resolve().parents[1]
    return str(root / "assets" / "menagerie" / "unitree_a1" / "scene.xml")


def _body_inertia(body) -> np.ndarray:
    """Symmetric 3x3 inertia of an MjsBody about its COM, in the body frame.

    MuJoCo accepts either ``fullinertia`` or ``diaginertia`` plus an inertial
    ``quat``.  The former is already expressed in the body frame; the latter
    has to be rotated from the principal-axis frame.
    """
    full = np.asarray(body.fullinertia, dtype=float).reshape(6)
    if np.all(np.isfinite(full)):
        xx, yy, zz, xy, xz, yz = full
        return np.array([[xx, xy, xz],
                         [xy, yy, yz],
                         [xz, yz, zz]], dtype=float)
    diag = np.asarray(body.inertia, dtype=float).reshape(3)
    R = quat_to_R(np.asarray(body.iquat, dtype=float))
    return R @ np.diag(diag) @ R.T


@dataclass
class A1Params:
    """Geometric, inertial and actuator parameters of the A1 model."""

    mass: float = 12.453
    trunk_mass: float = 4.713
    trunk_inertia: np.ndarray = field(
        default_factory=lambda: np.diag([0.0158533, 0.0377999, 0.0456542]))
    hip_offset: np.ndarray = field(
        default_factory=lambda: np.array([[0.183, -0.047, 0.0],
                                          [0.183, 0.047, 0.0],
                                          [-0.183, -0.047, 0.0],
                                          [-0.183, 0.047, 0.0]]))
    thigh_offset: np.ndarray = field(
        default_factory=lambda: np.array([[0.0, -0.08505, 0.0],
                                          [0.0, 0.08505, 0.0],
                                          [0.0, -0.08505, 0.0],
                                          [0.0, 0.08505, 0.0]]))
    l_thigh: float = 0.2
    l_calf: float = 0.2
    foot_radius: float = 0.02
    foot_margin: float = 0.0
    q_min: np.ndarray = field(default_factory=lambda: np.array(
        [-0.802851, -1.0472, -2.69653] * 4))
    q_max: np.ndarray = field(default_factory=lambda: np.array(
        [0.802851, 4.18879, -0.916298] * 4))
    tau_max: np.ndarray = field(default_factory=lambda: np.full(12, 33.5))
    motor_gear_ratio: float = 9.1
    home_q: np.ndarray = field(
        default_factory=lambda: np.array([0.0, 0.9, -1.8] * 4))
    stand_height: float = 0.27
    com_body_home: np.ndarray = field(
        default_factory=lambda: np.array([0.0, 0.0, 0.0]))
    link_masses: dict[str, float] = field(default_factory=dict)
    link_com: dict[str, np.ndarray] = field(default_factory=dict)
    link_inertia: dict[str, np.ndarray] = field(default_factory=dict)
    link_parent: dict[str, str] = field(default_factory=dict)
    link_pos: dict[str, np.ndarray] = field(default_factory=dict)
    link_quat: dict[str, np.ndarray] = field(default_factory=dict)
    link_axis: dict[str, np.ndarray] = field(default_factory=dict)
    link_joint_pos: dict[str, np.ndarray] = field(default_factory=dict)

    def __post_init__(self) -> None:
        self.hip_offset = np.asarray(self.hip_offset, dtype=float).reshape(4, 3)
        self.thigh_offset = np.asarray(self.thigh_offset, dtype=float).reshape(4, 3)
        self.q_min = np.asarray(self.q_min, dtype=float).reshape(12)
        self.q_max = np.asarray(self.q_max, dtype=float).reshape(12)
        self.tau_max = np.asarray(self.tau_max, dtype=float).reshape(12)
        self.home_q = np.asarray(self.home_q, dtype=float).reshape(12)
        self.trunk_inertia = np.asarray(self.trunk_inertia, dtype=float).reshape(3, 3)
        self.com_body_home = np.asarray(self.com_body_home, dtype=float).reshape(3)

    @classmethod
    def from_menagerie(cls, xml_path: str | None = None) -> "A1Params":
        """Read the A1 parameters from a Menagerie ``a1.xml``/``scene.xml``.

        The parser uses MuJoCo's ``MjSpec`` only for data extraction.  Body
        positions, inertial blocks, joint ranges, foot geometry, actuator
        force limits and the ``home`` keyframe are all taken from the file.
        """
        import mujoco

        path = default_model_path() if xml_path is None else str(xml_path)
        spec = mujoco.MjSpec.from_file(path)

        trunk = spec.body("trunk")
        total_mass = float(sum(b.mass for b in spec.bodies if b.name))

        hip_offsets = []
        thigh_offsets = []
        for leg in LegId:
            hip = spec.body(f"{leg.name}_hip")
            thigh = spec.body(f"{leg.name}_thigh")
            hip_offsets.append(np.asarray(hip.pos, dtype=float))
            thigh_offsets.append(np.asarray(thigh.pos, dtype=float))

        calf = spec.body("FR_calf")
        l_thigh = float(-np.asarray(calf.pos, dtype=float)[2])
        foot_geom = None
        for geom in calf.geoms:
            if int(geom.type) == int(mujoco.mjtGeom.mjGEOM_SPHERE):
                foot_geom = geom
                break
        if foot_geom is None:
            raise ValueError("A1 MJCF has no spherical foot geom on FR_calf")
        l_calf = float(-np.asarray(foot_geom.pos, dtype=float)[2])
        foot_radius = float(np.asarray(foot_geom.size, dtype=float)[0])
        # Contact margin is a modelling parameter: MuJoCo keeps a gap of
        # ``margin`` before a sphere touches the plane, so the sphere centre
        # rests at radius + margin.  A real rubber foot would use margin = 0.
        foot_margin = float(np.asarray(foot_geom.margin, dtype=float))

        q_min, q_max, tau_max = [], [], []
        for name in JOINT_NAMES:
            joint = spec.joint(name)
            q_min.append(float(joint.range[0]))
            q_max.append(float(joint.range[1]))
            actuator = spec.actuator(name.replace("_joint", ""))
            tau_max.append(float(actuator.forcerange[1]))

        home_q = np.array([0.0, 0.9, -1.8] * 4)
        stand_height = 0.27
        if "home" in [k.name for k in spec.keys]:
            key = spec.key("home")
            kq = np.asarray(key.qpos, dtype=float)
            if kq.size >= 19:
                home_q = kq[7:19].copy()
                stand_height = float(kq[2])

        link_masses = {}
        link_com = {}
        link_inertia = {}
        link_parent = {}
        link_pos = {}
        link_quat = {}
        link_axis = {}
        link_joint_pos = {}
        for name, body in (("trunk", trunk),) + tuple(
                (f"{leg.name}_{part}", spec.body(f"{leg.name}_{part}"))
                for leg in LegId for part in ("hip", "thigh", "calf")):
            link_masses[name] = float(body.mass)
            link_com[name] = np.asarray(body.ipos, dtype=float)
            link_inertia[name] = _body_inertia(body)
            if name != "trunk":
                link_parent[name] = body.parent.name
                link_pos[name] = np.asarray(body.pos, dtype=float)
                link_quat[name] = np.asarray(body.quat, dtype=float)
                joint = body.joints[0]
                link_axis[name] = np.asarray(joint.axis, dtype=float)
                link_joint_pos[name] = np.asarray(joint.pos, dtype=float)

        # Centre of mass at the home pose, expressed in the body frame.  This
        # is a parameter, not a controller calculation; a compiled model is
        # the simplest way to combine the four legs correctly.
        com_home = np.zeros(3)
        model = spec.compile()
        data = mujoco.MjData(model)
        mujoco.mj_resetData(model, data)
        data.qpos[0:3] = [0.0, 0.0, stand_height]
        data.qpos[3:7] = [1.0, 0.0, 0.0, 0.0]
        data.qpos[7:19] = home_q
        mujoco.mj_forward(model, data)
        trunk_id = model.body("trunk").id
        com_world = np.array(data.subtree_com[trunk_id], dtype=float)
        R = np.array(data.qpos[3:7], dtype=float)
        from .rotation import quat_to_R as _q2r
        com_home = _q2r(R).T @ (com_world - data.qpos[0:3])

        return cls(
            mass=total_mass,
            trunk_mass=float(trunk.mass),
            trunk_inertia=_body_inertia(trunk),
            hip_offset=np.asarray(hip_offsets),
            thigh_offset=np.asarray(thigh_offsets),
            l_thigh=l_thigh,
            l_calf=l_calf,
            foot_radius=foot_radius,
            foot_margin=foot_margin,
            q_min=np.asarray(q_min),
            q_max=np.asarray(q_max),
            tau_max=np.asarray(tau_max),
            home_q=home_q,
            stand_height=stand_height,
            com_body_home=com_home,
            link_masses=link_masses,
            link_com=link_com,
            link_inertia=link_inertia,
            link_parent=link_parent,
            link_pos=link_pos,
            link_quat=link_quat,
            link_axis=link_axis,
            link_joint_pos=link_joint_pos,
        )

    def body_equivalent_inertia(
            self, q_stand: np.ndarray | None = None) -> np.ndarray:
        """Whole-body inertia about the COM, in the trunk/body frame (6.32).

        Every one of the thirteen rigid bodies is moved to the whole-body
        centre of mass with the parallel-axis theorem, using the link masses,
        COM offsets and inertia tensors read from :meth:`from_menagerie`.
        ``q_stand`` is the 12-joint standing configuration; the default is
        ``home_q``.  The result is the quantity used by the single-rigid-body
        model of Sections 6.2-6.4, not the trunk-only inertia.
        """
        q = (self.home_q if q_stand is None else
             np.asarray(q_stand, dtype=float).reshape(12))
        if not self.link_inertia:
            # A hand-built A1Params has no link data.  Fall back to the
            # whole-body value of Eq. (6.32), never to the trunk tensor.
            return np.diag([0.1320, 0.3475, 0.3775])

        joint_q: dict[str, float] = {}
        for leg in LegId:
            for k, part in enumerate(("hip", "thigh", "calf")):
                joint_q[f"{leg.name}_{part}"] = float(q[3 * int(leg) + k])

        pose_cache: dict[str, tuple[np.ndarray, np.ndarray]] = {
            "trunk": (np.eye(3), np.zeros(3))}

        def link_pose(name: str) -> tuple[np.ndarray, np.ndarray]:
            if name in pose_cache:
                return pose_cache[name]
            R_p, p_p = link_pose(self.link_parent[name])
            R_0 = R_p @ quat_to_R(self.link_quat[name])
            p_0 = p_p + R_p @ self.link_pos[name]
            axis = self.link_axis[name]
            a = self.link_joint_pos[name]
            anchor = p_0 + R_0 @ a
            R = R_0 @ so3_exp(axis * joint_q[name])
            # The joint anchor is fixed to the parent; the body origin moves
            # around it when the joint rotates (joint_pos is zero for the A1).
            p = anchor - R @ a
            pose_cache[name] = (R, p)
            return R, p

        total_mass = 0.0
        first_moment = np.zeros(3)
        moments: list[tuple[float, np.ndarray, np.ndarray]] = []
        for name in self.link_inertia:
            R, p = link_pose(name)
            m = self.link_masses[name]
            c = p + R @ self.link_com[name]
            Ic = R @ self.link_inertia[name] @ R.T
            total_mass += m
            first_moment += m * c
            moments.append((m, c, Ic))
        com = first_moment / total_mass

        inertia = np.zeros((3, 3))
        for m, c, Ic in moments:
            d = c - com
            inertia += Ic + m * ((d @ d) * np.eye(3) - np.outer(d, d))
        return 0.5 * (inertia + inertia.T)

    def leg_limits(self, leg: LegId) -> tuple[np.ndarray, np.ndarray]:
        i = int(leg)
        return self.q_min[3 * i:3 * i + 3], self.q_max[3 * i:3 * i + 3]

    def stand_cmd(self, kp=None, kd=None):
        """Return a position-hold :class:`JointCmd` for the home pose."""
        from .types import JointCmd
        kp = [80.0, 300.0, 400.0] if kp is None else kp
        kd = [2.0, 4.0, 4.0] if kd is None else kd
        return JointCmd(self.home_q.copy(), np.zeros(12),
                        np.tile(kp, 4), np.tile(kd, 4), np.zeros(12))


class Quadruped:
    """Four A1 legs with the shared 12-joint vector layout."""

    def __init__(self, params: A1Params) -> None:
        self.params = params
        self.legs: list[Leg] = []
        for leg in LegId:
            i = int(leg)
            lo, hi = params.leg_limits(leg)
            self.legs.append(Leg(
                leg, params.hip_offset[i], params.thigh_offset[i],
                params.l_thigh, params.l_calf, params.foot_radius,
                q_min=lo, q_max=hi,
            ))

    # -- forward / inverse kinematics ------------------------------------
    def fk_all(self, q: np.ndarray) -> np.ndarray:
        """Four foot-sphere centres in the body frame, shape ``(4, 3)``."""
        q = np.asarray(q, dtype=float).reshape(12)
        return np.vstack([
            self.params.hip_offset[i] + leg.fk(q[3 * i:3 * i + 3])
            for i, leg in enumerate(self.legs)
        ])

    def ik_all(self, p: np.ndarray, frame: str = "body") -> np.ndarray:
        """Inverse kinematics for four foot points.

        ``frame="body"`` expects points in the body frame; the per-leg hip
        offset is removed before calling :meth:`Leg.ik`.  ``frame="leg"``
        expects points already in each leg's base frame.
        """
        p = np.asarray(p, dtype=float).reshape(4, 3)
        if frame not in ("body", "leg"):
            raise ValueError("frame must be 'body' or 'leg'")
        out = np.zeros((4, 3))
        for i, leg in enumerate(self.legs):
            target = p[i] if frame == "leg" else p[i] - self.params.hip_offset[i]
            out[i] = leg.ik(target, knee=-1)
        return out.reshape(12)

    # -- frames -----------------------------------------------------------
    def feet_in_world(self, q: np.ndarray, base) -> np.ndarray:
        """Foot-sphere centres in the world frame, shape ``(4, 3)``."""
        R = base.rotation() if hasattr(base, "rotation") else quat_to_R(base)
        pos = base.position if hasattr(base, "position") else np.zeros(3)
        return (R @ self.fk_all(q).T).T + np.asarray(pos, dtype=float)

    def body_jacobian(self, q: np.ndarray, leg: int,
                      R_sb: np.ndarray | None = None) -> np.ndarray:
        """Foot Jacobian of one leg.

        With the default ``R_sb=None`` the result is expressed in the body
        frame.  Passing the base rotation ``R_sb`` rotates it to the world
        frame, matching the ``J_s = R_sb J_b`` relation of Chapter 6.
        """
        q = np.asarray(q, dtype=float).reshape(12)
        i = int(leg)
        J = self.legs[i].jacobian(q[3 * i:3 * i + 3])
        if R_sb is None:
            return J
        return np.asarray(R_sb, dtype=float).reshape(3, 3) @ J

    def leg_jacobian(self, q: np.ndarray, leg: int) -> np.ndarray:
        q = np.asarray(q, dtype=float).reshape(12)
        i = int(leg)
        return self.legs[i].jacobian(q[3 * i:3 * i + 3])

    def com_body(self) -> np.ndarray:
        """Centre of mass at the home pose, in the body frame."""
        return self.params.com_body_home.copy()

    def home_cmd(self, kp=None, kd=None):
        return self.params.stand_cmd(kp, kd)


__all__ = ["A1Params", "Quadruped", "default_model_path"]
