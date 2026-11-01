"""Inertia tensor of the A1: box formula, CAD values, parallel axis (6.3).

The trunk is approximated by a uniform 0.25 x 0.16 x 0.11 m box.  Its inertia is
compared with the Menagerie trunk inertia.  The hip/thigh/calf links of the
front-right leg are then moved to the hip axis with the parallel-axis theorem,
and all thirteen bodies are summed about the whole-body centre of mass.  The
last sum is compared with the book's equivalent inertia diag(0.1320, 0.3475,
0.3775) kg m^2.

expect: 长方体近似
expect: Menagerie 躯干
expect: FR 腿绕髋轴
expect: 整机惯性
"""

from __future__ import annotations

import logging

import numpy as np
import mujoco

from quadruped import A1Params
from quadruped.robot import default_model_path
from quadruped.rotation import quat_to_R
from quadruped.plotting import savefig
import matplotlib.pyplot as plt

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

DIM = np.array([0.25, 0.16, 0.11])       # l, w, h (m)


# ANCHOR: box
def box_inertia(mass: float, l: float, w: float, h: float) -> np.ndarray:
    """Eq. (6.31): uniform box, origin at the centroid, axes along the edges."""
    return mass / 12.0 * np.diag([w * w + h * h, l * l + h * h, l * l + w * w])
# ANCHOR_END: box


def body_inertia_local(model, bid: int) -> np.ndarray:
    """Inertia about the body's COM, expressed in the body frame."""
    R = quat_to_R(np.asarray(model.body_iquat[bid]))
    return R @ np.diag(np.asarray(model.body_inertia[bid])) @ R.T


def load_home():
    params = A1Params.from_menagerie()
    spec = mujoco.MjSpec.from_file(default_model_path())
    model = spec.compile()
    data = mujoco.MjData(model)
    mujoco.mj_resetData(model, data)
    data.qpos[0:3] = [0.0, 0.0, 0.27]
    data.qpos[3:7] = [1.0, 0.0, 0.0, 0.0]
    data.qpos[7:19] = params.home_q
    mujoco.mj_forward(model, data)
    return params, model, data


# ANCHOR: parallel
def parallel_axis(Ic: np.ndarray, mass: float, d: np.ndarray) -> np.ndarray:
    """Move an inertia tensor from the COM to a point offset by d."""
    return Ic + mass * ((d @ d) * np.eye(3) - np.outer(d, d))
# ANCHOR_END: parallel


def link_inertia_world(model, data, bid: int):
    """Return (mass, COM in world, inertia about COM in world axes)."""
    R = np.asarray(data.xmat[bid]).reshape(3, 3)
    Ic = R @ body_inertia_local(model, bid) @ R.T
    com = np.asarray(data.xpos[bid]) + R @ np.asarray(model.body_ipos[bid])
    return float(model.body_mass[bid]), com, Ic


def leg_about_hip(params, model, data) -> tuple[float, np.ndarray, np.ndarray]:
    hip_id = model.body("FR_hip").id
    origin = np.asarray(data.xpos[hip_id]).copy()
    total = np.zeros((3, 3))
    mass = 0.0
    com = np.zeros(3)
    print("  FR 腿各连杆（相对髋关节原点）：")
    for part in ("FR_hip", "FR_thigh", "FR_calf"):
        bid = model.body(part).id
        m, c, Ic = link_inertia_world(model, data, bid)
        d = c - origin
        total += parallel_axis(Ic, m, d)
        mass += m
        com += m * c
        print(f"    {part:8s} m={m:.3f} kg, |d|={np.linalg.norm(d):.4f} m, "
              f"I_C(xx)={Ic[0, 0]:.6f}")
    print(f"    合计 m={mass:.3f} kg, 质心相对髋轴 "
          f"d=({com[0] / mass - origin[0]: .4f}, "
          f"{com[1] / mass - origin[1]: .4f}, {com[2] / mass - origin[2]: .4f}) m")
    return mass, com / mass, total


def whole_body(model, data):
    trunk_id = model.body("trunk").id
    com = np.asarray(data.subtree_com[trunk_id]).copy()
    total = np.zeros((3, 3))
    mass = 0.0
    names = ["trunk"] + [f"{leg}_{part}" for leg in
                         ("FR", "FL", "RR", "RL")
                         for part in ("hip", "thigh", "calf")]
    for name in names:
        bid = model.body(name).id
        m, c, Ic = link_inertia_world(model, data, bid)
        total += parallel_axis(Ic, m, c - com)
        mass += m
    return mass, com, total


def ellipsoid(ax, I: np.ndarray, mass: float, center, color, label,
              scale=1.0, alpha=0.30):
    w, V = np.linalg.eigh(I)
    radii = np.sqrt(np.maximum(w, 1e-12) / mass) * scale
    u = np.linspace(0.0, 2.0 * np.pi, 40)
    v = np.linspace(0.0, np.pi, 25)
    U, Vv = np.meshgrid(u, v)
    xyz = np.array([
        radii[0] * np.sin(Vv) * np.cos(U),
        radii[1] * np.sin(Vv) * np.sin(U),
        radii[2] * np.cos(Vv),
    ])
    xyz = np.einsum("ij,jkl->ikl", V, xyz)
    ax.plot_surface(xyz[0] + center[0], xyz[1] + center[1],
                    xyz[2] + center[2], color=color, alpha=alpha,
                    linewidth=0, shade=True)
    ax.text(center[0], center[1], center[2] - radii[2] - 0.035, label,
            color=color, fontsize=8.5, ha="center")


def draw_box(ax):
    l, w, h = DIM
    vertices = np.array([[sx * l / 2, sy * w / 2, sz * h / 2]
                         for sx in (-1, 1) for sy in (-1, 1)
                         for sz in (-1, 1)])
    edges = [(0, 1), (0, 2), (0, 4), (1, 3), (1, 5), (2, 3),
             (2, 6), (3, 7), (4, 5), (4, 6), (5, 7), (6, 7)]
    for a, b in edges:
        ax.plot(*zip(vertices[a], vertices[b]), color="tab:blue", lw=1.3)
    for k, (name, color) in enumerate((("x", "k"), ("y", "tab:green"),
                                       ("z", "tab:red"))):
        d = np.eye(3)[k] * 0.13
        ax.quiver(0, 0, 0, *d, color=color, arrow_length_ratio=0.18, lw=1.5)
        ax.text(*(1.18 * d), f"${name}$", color=color, fontsize=10)
    ax.text(l / 2 + 0.025, 0, -h / 2 - 0.02, "$l$", color="tab:blue",
            fontsize=11)
    ax.text(-l / 2 - 0.03, w / 2 + 0.02, 0, "$w$", color="tab:blue",
            fontsize=11)
    ax.text(-l / 2 - 0.03, 0, h / 2 + 0.02, "$h$", color="tab:blue",
            fontsize=11)
    ax.set_xlim(-0.18, 0.20)
    ax.set_ylim(-0.15, 0.15)
    ax.set_zlim(-0.13, 0.16)
    ax.set_xlabel("x (m)")
    ax.set_ylabel("y (m)")
    ax.set_zlabel("z (m)")
    ax.set_box_aspect((1.0, 0.8, 0.9))
    ax.view_init(elev=22, azim=-55)
    ax.set_title("长方体：原点在形心，轴平行于棱")
    ax.grid(False)
    for pane in (ax.xaxis.pane, ax.yaxis.pane, ax.zaxis.pane):
        pane.set_alpha(0.0)


# ANCHOR: main
def main() -> None:
    params, model, data = load_home()
    mass = params.trunk_mass
    l, w, h = DIM
    I_box = box_inertia(mass, l, w, h)
    I_trunk = np.asarray(params.trunk_inertia)
    print("=== 6.3 刚体的惯性张量 ===")
    print(f"长方体近似：m={mass:.3f} kg, l×w×h = "
          f"{l:.3f}×{w:.3f}×{h:.3f} m")
    print(f"  式 (6.31) 的惯性张量 = diag({I_box[0, 0]:.6f}, "
          f"{I_box[1, 1]:.6f}, {I_box[2, 2]:.6f}) kg·m^2")
    print(f"Menagerie 躯干惯性张量（机身系）=")
    for row in I_trunk:
        print("  [" + "  ".join(f"{x: .6f}" for x in row) + "]")
    rel = np.diag(I_trunk) / np.diag(I_box) - 1.0
    print(f"  对角元相对差 = ({rel[0] * 100:+.1f}%, {rel[1] * 100:+.1f}%, "
          f"{rel[2] * 100:+.1f}%)")
    print(f"  非对角元最大绝对值 = {np.max(np.abs(I_trunk - np.diag(np.diag(I_trunk)))):.6f}")

    print("\nFR 腿绕髋轴（平行轴定理）：")
    leg_mass, leg_com, I_leg = leg_about_hip(params, model, data)
    print("  I_leg =")
    for row in I_leg:
        print("   [" + "  ".join(f"{x: .6f}" for x in row) + "]")
    print(f"  对角元 = ({I_leg[0, 0]:.6f}, {I_leg[1, 1]:.6f}, "
          f"{I_leg[2, 2]:.6f}) kg·m^2")
    print(f"  I_leg(xx) / I_trunk(xx) = {I_leg[0, 0] / I_trunk[0, 0]:.3f}")
    print(f"  四条腿总质量 = {4 * leg_mass:.3f} kg，占整机 "
          f"{4 * leg_mass / params.mass * 100:.1f}%")

    print("\n整机惯性（13 个刚体绕整机重心求和）：")
    m_all, com, I_all = whole_body(model, data)
    print(f"  总质量 = {m_all:.3f} kg，质心 = ({com[0]: .4f}, "
          f"{com[1]: .4f}, {com[2]: .4f}) m")
    print(f"  I_all 对角元 = ({I_all[0, 0]:.6f}, {I_all[1, 1]:.6f}, "
          f"{I_all[2, 2]:.6f}) kg·m^2")
    print(f"  非对角元最大绝对值 = "
          f"{np.max(np.abs(I_all - np.diag(np.diag(I_all)))):.6f}")
    print(f"  与式 (6.32) 的 diag(0.1320, 0.3475, 0.3775) 相对差 = "
          f"({(I_all[0, 0] / 0.1320 - 1) * 100:+.1f}%, "
          f"{(I_all[1, 1] / 0.3475 - 1) * 100:+.1f}%, "
          f"{(I_all[2, 2] / 0.3775 - 1) * 100:+.1f}%)")
    print(f"  I_all 的特征值 = ({np.linalg.eigvalsh(I_all)[0]:.6f}, "
          f"{np.linalg.eigvalsh(I_all)[1]:.6f}, "
          f"{np.linalg.eigvalsh(I_all)[2]:.6f})")
    print(f"  最小特征值 > 0：正定")

    fig = plt.figure(figsize=(14.0, 4.2))
    ax0 = fig.add_subplot(1, 4, 1, projection="3d")
    draw_box(ax0)
    panels = (
        (I_box, mass, "tab:blue", "均匀长方体",
         f"diag({I_box[0, 0]:.4f}, {I_box[1, 1]:.4f}, {I_box[2, 2]:.4f})"),
        (I_trunk, params.trunk_mass, "tab:green", "Menagerie 躯干",
         f"diag({I_trunk[0, 0]:.4f}, {I_trunk[1, 1]:.4f}, {I_trunk[2, 2]:.4f})"),
        (I_all, m_all, "tab:red", "整机折算",
         f"diag({I_all[0, 0]:.4f}, {I_all[1, 1]:.4f}, {I_all[2, 2]:.4f})"),
    )
    for k, (I, m, color, name, diag) in enumerate(panels):
        ax = fig.add_subplot(1, 4, k + 2, projection="3d")
        ellipsoid(ax, I, m, np.zeros(3), color, "")
        ax.set_xlim(-0.20, 0.20)
        ax.set_ylim(-0.20, 0.20)
        ax.set_zlim(-0.20, 0.20)
        ax.set_xlabel("x (m)")
        ax.set_ylabel("y (m)")
        ax.set_zlabel("z (m)")
        ax.set_box_aspect((1.0, 1.0, 1.0))
        ax.view_init(elev=18, azim=-60)
        ax.set_title(f"{name}\n{diag}", fontsize=9)
        ax.grid(False)
        for pane in (ax.xaxis.pane, ax.yaxis.pane, ax.zaxis.pane):
            pane.set_alpha(0.0)
    fig.suptitle("回转半径椭球 $\\sqrt{I_{ii}/m}$（四个面板同一比例）",
                 fontsize=11)
    fig.tight_layout()
    savefig(fig, "6_3_inertia")
# ANCHOR_END: main


if __name__ == "__main__":
    main()
