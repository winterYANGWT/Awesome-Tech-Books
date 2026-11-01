"""Quadratic-programme foot-force allocation (Section 8.3).

The example assembles the 6x12 dynamics matrix, the three cost terms and the
friction-pyramid constraints explicitly, solves the QP with OSQP, checks the
result against scipy's SLSQP and against the shared ``BalanceCtrl``.  The
figure shows the four force vectors, their resultant and the weight
distribution.

expect: 四足力分配
expect: OSQP
expect: SLSQP
expect: 动力学残差
expect: 四足法向力
"""

from __future__ import annotations

import logging
import os

os.environ.setdefault("MESA_SHADER_CACHE_DISABLE", "true")

import numpy as np
from quadruped.plotting import savefig, label
import matplotlib.pyplot as plt
import scipy.sparse as sp
import osqp
from scipy.optimize import minimize
from mpl_toolkits.mplot3d.art3d import Poly3DCollection

from quadruped.balance import BalanceCtrl
from quadruped.rotation import skew

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

MASS = 12.45
INERTIA = np.diag([0.1320, 0.3475, 0.3775])
GRAVITY = np.array([0.0, 0.0, -9.81])
MU = 0.6
F_MAX = 90.0
W_DYN = 1000.0
W_FORCE = 0.001
W_DELTA = 0.02
FEET = np.array([[0.183, -0.132, -0.25],
                 [0.183, 0.132, -0.25],
                 [-0.183, -0.132, -0.25],
                 [-0.183, 0.132, -0.25]])
A_DES = np.array([1.5, -0.8, 0.4])
ALPHA_DES = np.array([1.0, -0.5, 0.3])


# ANCHOR: assemble
def dynamics_matrix(feet_rel_com: np.ndarray) -> np.ndarray:
    A = np.zeros((6, 12))
    A[0:3, :] = np.tile(np.eye(3), (1, 4))
    for i in range(4):
        A[3:6, 3 * i:3 * i + 3] = skew(feet_rel_com[i])
    return A


def dynamics_rhs(a_des: np.ndarray, alpha_des: np.ndarray,
                 R: np.ndarray) -> np.ndarray:
    b = np.zeros(6)
    b[0:3] = MASS * (a_des - GRAVITY)
    b[3:6] = R @ INERTIA @ R.T @ alpha_des
    return b


def constraint_matrix() -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    """Return C, lower and upper bounds for ``l <= C f <= u``."""
    rows, low, high = [], [], []
    friction = np.array([[1.0, 0.0, MU],
                         [-1.0, 0.0, MU],
                         [0.0, 1.0, MU],
                         [0.0, -1.0, MU],
                         [0.0, 0.0, 1.0]])
    for i in range(4):
        block = np.zeros((5, 12))
        block[:, 3 * i:3 * i + 3] = friction
        rows.append(block)
        low.extend([0.0] * 5)
        high.extend([np.inf] * 5)
    for i in range(4):
        block = np.zeros((1, 12))
        block[0, 3 * i + 2] = 1.0
        rows.append(block)
        low.append(-np.inf)
        high.append(F_MAX)
    return np.vstack(rows), np.asarray(low), np.asarray(high)
# ANCHOR_END: assemble


def main() -> None:
    print("=== 8.3 四足力分配 QP ===")
    print(f"m = {MASS:.3f} kg, I = diag({INERTIA[0, 0]:.4f}, "
          f"{INERTIA[1, 1]:.4f}, {INERTIA[2, 2]:.4f}) kg m^2")
    print(f"a_des = [{A_DES[0]:.3f} {A_DES[1]:.3f} {A_DES[2]:.3f}] m/s^2, "
          f"alpha_des = [{ALPHA_DES[0]:.3f} {ALPHA_DES[1]:.3f} "
          f"{ALPHA_DES[2]:.3f}] rad/s^2")
    print(f"权重：S = {W_DYN:.0f} I6, w_x = {W_FORCE:.4f}, "
          f"w_delta = {W_DELTA:.3f}")

    R = np.eye(3)
    A = dynamics_matrix(FEET)
    b = dynamics_rhs(A_DES, ALPHA_DES, R)
    P = A.T @ (W_DYN * np.eye(6)) @ A + W_FORCE * np.eye(12) \
        + W_DELTA * np.eye(12)
    q = -A.T @ (W_DYN * np.eye(6)) @ b
    C, low, high = constraint_matrix()
    n_fric = 20

    # OSQP works on 0.5 x'Px + q'x with l <= Cx <= u.  Scale the objective
    # so that the solver sees numbers of order one; scaling does not move x*.
    # ANCHOR: solve
    scale = float(np.max(np.abs(P)))
    P_s, q_s = P / scale, q / scale
    solver = osqp.OSQP()
    solver.setup(sp.triu(sp.csc_matrix(P_s)).tocsc(), q_s,
                 sp.csc_matrix(C), low, high, verbose=False,
                 eps_abs=1e-10, eps_rel=1e-10, max_iter=20000,
                 polishing=False)
    result = solver.solve()
    x = np.asarray(result.x, dtype=float)
    residual = float(np.max(np.abs(A @ x - b)))
    friction_viol = float(max(0.0, -(C[:n_fric] @ x - low[:n_fric]).min()))
    upper_viol = float(max(0.0, (C[n_fric:] @ x - high[n_fric:]).max()))
    objective = float(0.5 * x @ P @ x + q @ x)
    # ANCHOR_END: solve
    print(f"OSQP: 状态 = {result.info.status}, 迭代 = {result.info.iter}, "
          f"目标 = {objective:.4f}, 动力学残差 = {residual:.3e}, "
          f"约束违反 = {max(friction_viol, upper_viol):.3e}")

    # The same problem through a general nonlinear solver.
    def fun(y):
        return 0.5 * y @ P_s @ y + q_s @ y

    def jac(y):
        return P_s @ y + q_s

    cons = [{"type": "ineq", "fun": lambda y: C[:n_fric] @ y - low[:n_fric],
             "jac": lambda y: C[:n_fric]}]
    for i in range(4):
        row = np.zeros(12)
        row[3 * i + 2] = 1.0
        cons.append({"type": "ineq",
                     "fun": lambda y, row=row: F_MAX - row @ y,
                     "jac": lambda y, row=row: -row})
    x0 = np.zeros(12)
    x0[2::3] = MASS * 9.81 / 4.0
    slsqp = minimize(fun, x0, jac=jac, constraints=cons, method="SLSQP",
                     options={"ftol": 1e-12, "maxiter": 1000})
    diff = float(np.max(np.abs(x - slsqp.x)))
    print(f"SLSQP: 成功 = {slsqp.success}, 迭代 = {slsqp.nit}, "
          f"max|f_OSQP - f_SLSQP| = {diff:.3e}")

    # Cross-check against the shared package, which uses the same equations.
    ctrl = BalanceCtrl(mass=MASS, inertia=INERTIA, fric_mu=MU, f_max=F_MAX,
                       w_dynamics=W_DYN, w_force=W_FORCE,
                       w_delta=W_DELTA)
    f_pkg = ctrl.solve_force(A_DES, ALPHA_DES, R, FEET, np.ones(4, bool))
    print(f"BalanceCtrl: 状态 = {ctrl.last_status}, "
          f"max|f_QP - f_pkg| = "
          f"{np.max(np.abs(x.reshape(4, 3) - f_pkg)):.3e}")

    forces = x.reshape(4, 3)
    total_force = forces.sum(axis=0)
    total_moment = np.cross(FEET, forces).sum(axis=0)
    print("四足法向力 = [" + ", ".join(f"{v:.4f}" for v in forces[:, 2]) + "] N")
    print(f"总力 = [{total_force[0]:.4f} {total_force[1]:.4f} "
          f"{total_force[2]:.4f}] N")
    print(f"总力矩（对重心）= [{total_moment[0]:.4f} {total_moment[1]:.4f} "
          f"{total_moment[2]:.4f}] N·m")
    print(f"需求力 = [{b[0]:.4f} {b[1]:.4f} {b[2]:.4f}] N, "
          f"需求力矩 = [{b[3]:.4f} {b[4]:.4f} {b[5]:.4f}] N·m")

    # Figure 8-4: force vectors and the per-foot component chart.
    fig = plt.figure(figsize=(11.2, 4.8))
    ax = fig.add_subplot(1, 2, 1, projection="3d")
    ax.scatter(FEET[:, 0], FEET[:, 1], FEET[:, 2], color="k", s=18, zorder=5)
    for i, p in enumerate(FEET):
        v = forces[i] * 0.0022
        ax.quiver(*p, *v, color="#1565c0", lw=1.8, arrow_length_ratio=0.16)
        ax.text(*(p + v * 1.05), f"$f_{i}$", color="#1565c0", fontsize=9)
    ax.quiver(0, 0, 0, *(total_force * 0.0012), color="#c62828", lw=2.2,
              arrow_length_ratio=0.12)
    ax.text(*(total_force * 0.0012 * 1.04), "合力", color="#c62828",
            fontsize=9)
    ax.scatter([0.0], [0.0], [0.0], color="#c62828", s=30)
    ax.text(0.0, 0.0, 0.012, "重心", color="#c62828", fontsize=9)
    box = np.array([[0.15, -0.10, 0.03], [0.15, 0.10, 0.03],
                    [-0.15, 0.10, 0.03], [-0.15, -0.10, 0.03],
                    [0.15, -0.10, -0.03], [0.15, 0.10, -0.03],
                    [-0.15, 0.10, -0.03], [-0.15, -0.10, -0.03]])
    faces = [[0, 1, 2, 3], [4, 5, 6, 7], [0, 1, 5, 4],
             [1, 2, 6, 5], [2, 3, 7, 6], [3, 0, 4, 7]]
    ax.add_collection3d(Poly3DCollection(box[faces], alpha=0.08,
                                         facecolor="0.4", edgecolor="0.5"))
    ax.set_xlabel("$x$ (m)")
    ax.set_ylabel("$y$ (m)")
    ax.set_zlabel("$z$ (m)")
    ax.set_xlim(-0.28, 0.28)
    ax.set_ylim(-0.28, 0.28)
    ax.set_zlim(-0.30, 0.10)
    ax.set_title(label("四个足端力与合力", "foot forces and resultant"))
    ax.view_init(elev=18, azim=-58)

    ax = fig.add_subplot(1, 2, 2)
    width = 0.22
    idx = np.arange(4)
    for j, (comp, color, tag) in enumerate(zip(
            (0, 1, 2), ("#ef6c00", "#2e7d32", "#1565c0"),
            ("$f_x$", "$f_y$", "$f_z$"))):
        ax.bar(idx + (j - 1) * width, forces[:, comp], width,
               color=color, label=tag)
    ax.axhline(0.0, color="k", lw=0.6)
    ax.set_xticks(idx)
    ax.set_xticklabels(["FR", "FL", "RR", "RL"])
    ax.set_ylabel(label("力 (N)", "force (N)"))
    ax.set_title(label("各足端的三维力分量", "force components"))
    ax.grid(True, axis="y", alpha=0.3)
    ax.legend(fontsize=8)
    fig.tight_layout()
    savefig(fig, "8_4_qp_force")
    plt.close(fig)


if __name__ == "__main__":
    main()
