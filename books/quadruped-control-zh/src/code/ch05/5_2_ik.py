"""Inverse kinematics of one A1 leg (section 5.2).

The leg is solved by decoupling: the abduction angle follows from the yz
projection, the knee angle from the cosine rule, and the thigh angle from the
remaining planar triangle.  A 20 x 20 x 10 grid is solved and checked by
forward kinematics; the two knee branches and the joint limits are printed.
The figures show the two construction triangles and two workspace slices.

expect: 几何有解
expect: 关节限位内
expect: 无解
expect: 最大 IK
expect: 膝关节两个分支
"""

from __future__ import annotations

import logging
import math

import numpy as np

from quadruped.leg import Leg
from quadruped.plotting import label, savefig
from quadruped.types import LegId
import matplotlib.pyplot as plt

logging.getLogger("matplotlib.font_manager").setLevel(logging.ERROR)

L_THIGH = 0.20
L_CALF = 0.20
D_RIGHT = -0.08505
Q_MIN = np.array([-0.802851, -1.0472, -2.69653])
Q_MAX = np.array([0.802851, 4.18879, -0.916298])


def fk(q: np.ndarray) -> np.ndarray:
    q1, q2, q3 = np.asarray(q, dtype=float)
    s1, c1 = math.sin(q1), math.cos(q1)
    s2, c2 = math.sin(q2), math.cos(q2)
    s23, c23 = math.sin(q2 + q3), math.cos(q2 + q3)
    return np.array([
        -L_CALF * s23 - L_THIGH * s2,
        D_RIGHT * c1 + s1 * (L_THIGH * c2 + L_CALF * c23),
        D_RIGHT * s1 - c1 * (L_THIGH * c2 + L_CALF * c23),
    ])


def _wrap(a: float) -> float:
    return (a + math.pi) % (2.0 * math.pi) - math.pi


def _canonical(value: float, lo: float, hi: float) -> float:
    best, cost = value, None
    for k in (-2, -1, 0, 1, 2):
        v = value + 2.0 * math.pi * k
        c = max(lo - v, 0.0, v - hi)
        key = (c, abs(v))
        if cost is None or key < cost:
            best, cost = v, key
    return best
# ANCHOR: ik


def ik(p: np.ndarray, knee: int = -1, q1_branch: int = 1,
       wrap: bool = True) -> np.ndarray:
    """Closed-form IK; raises ValueError when the point is not reachable.

    ``knee=-1`` is the A1 branch (knee flexed backwards).  ``q1_branch=+1``
    selects the solution of Eq. (5-18), ``-1`` the other one.
    """
    p = np.asarray(p, dtype=float).reshape(3)
    y, z = float(p[1]), float(p[2])
    rho = math.hypot(y, z)
    if rho < abs(D_RIGHT) - 1e-12:
        raise ValueError("目标在髋偏置圆柱内")
    if rho < 1e-15:
        raise ValueError("目标在髋轴线上")
    beta = math.acos(max(-1.0, min(1.0, D_RIGHT / rho)))
    alpha = math.atan2(z, y)
    q1 = _wrap(alpha + q1_branch * beta)

    # Rotate the target into {1}; y becomes D_RIGHT and only x-z remains.
    vx = float(p[0])
    vz = -math.sin(q1) * y + math.cos(q1) * z
    r2 = vx * vx + vz * vz
    c3 = (r2 - L_THIGH ** 2 - L_CALF ** 2) / (2.0 * L_THIGH * L_CALF)
    if c3 < -1.0 - 1e-9 or c3 > 1.0 + 1e-9:
        raise ValueError("目标超出大腿加小腿的长度")
    q3 = knee * math.acos(max(-1.0, min(1.0, c3)))
    gamma = math.atan2(-vx, -vz)
    q2 = gamma - math.atan2(L_CALF * math.sin(q3),
                            L_THIGH + L_CALF * math.cos(q3))
    if not wrap:
        return np.array([q1, q2, q3])
    return np.array([_canonical(q1, Q_MIN[0], Q_MAX[0]),
                     _canonical(q2, Q_MIN[1], Q_MAX[1]),
                     _canonical(q3, Q_MIN[2], Q_MAX[2])])
# ANCHOR_END: ik


def in_limits(q: np.ndarray) -> bool:
    return bool(np.all(q >= Q_MIN - 1e-9) and np.all(q <= Q_MAX + 1e-9))


def condition(q: np.ndarray) -> float:
    s = np.linalg.svd(jacobian(q), compute_uv=False)
    return float(s[0] / max(s[-1], 1e-15))


def jacobian(q: np.ndarray) -> np.ndarray:
    """Analytic J = dp/dq, written from the forward-kinematics formula."""
    q1, q2, q3 = np.asarray(q, dtype=float)
    s1, c1 = math.sin(q1), math.cos(q1)
    s2, c2 = math.sin(q2), math.cos(q2)
    s23, c23 = math.sin(q2 + q3), math.cos(q2 + q3)
    B = L_THIGH * c2 + L_CALF * c23
    D = L_THIGH * s2 + L_CALF * s23
    return np.array([
        [0.0, -B, -L_CALF * c23],
        [-D_RIGHT * s1 + c1 * B, -s1 * D, -s1 * L_CALF * s23],
        [D_RIGHT * c1 + s1 * B, c1 * D, c1 * L_CALF * s23],
    ])


def scan_grid() -> dict:
    xs = np.linspace(-0.22, 0.22, 20)
    ys = np.linspace(-0.18, 0.00, 20)
    zs = np.linspace(-0.38, -0.12, 10)
    total = solved = valid = 0
    max_err = 0.0
    max_cond, max_q, max_p = 0.0, None, None
    for x in xs:
        for y in ys:
            for z in zs:
                total += 1
                p = np.array([x, y, z])
                try:
                    q = ik(p)
                except ValueError:
                    continue
                solved += 1
                err = float(np.max(np.abs(fk(q) - p)))
                max_err = max(max_err, err)
                if not in_limits(q):
                    continue
                valid += 1
                c = condition(q)
                if c > max_cond:
                    max_cond, max_q, max_p = c, q, p
    return dict(total=total, solved=solved, valid=valid,
                unsolved=total - solved, out_of_limits=solved - valid,
                max_err=max_err, max_cond=max_cond, max_q=max_q, max_p=max_p)


def compare_package(points: int = 400) -> float:
    leg = Leg(LegId.FR, np.array([0.183, -0.047, 0.0]),
              np.array([0.0, D_RIGHT, 0.0]), L_THIGH, L_CALF, 0.02,
              q_min=Q_MIN, q_max=Q_MAX)
    rng = np.random.default_rng(5)
    worst = 0.0
    for _ in range(points):
        q = rng.uniform(Q_MIN, Q_MAX)
        p = leg.fk(q)
        try:
            q1 = ik(p)
            q2 = leg.ik(p, knee=-1, q_ref=q1)
        except ValueError:
            continue
        worst = max(worst, float(np.max(np.abs(q1 - q2))))
    return worst


def draw_theta1() -> None:
    q = np.array([-0.35, 0.8, -1.5])
    p = fk(q)
    q1 = q[0]
    y, z = p[1], p[2]
    A = D_RIGHT * np.array([math.cos(q1), math.sin(q1)])
    P = np.array([y, z])

    fig, ax = plt.subplots(figsize=(5.6, 5.6))
    ax.axhline(0, color="0.7", lw=0.8)
    ax.axvline(0, color="0.7", lw=0.8)
    ax.annotate("", xy=(0.16, 0), xytext=(0, 0),
                arrowprops=dict(arrowstyle="-|>", color="0.25", lw=1.2))
    ax.annotate("", xy=(0, 0.10), xytext=(0, 0),
                arrowprops=dict(arrowstyle="-|>", color="0.25", lw=1.2))
    ax.text(0.165, -0.018, r"$y_0$", fontsize=10)
    ax.text(-0.020, 0.105, r"$z_0$", fontsize=10)

    # {1} axes: the y1 axis is at angle q1.
    for vec, name, col in ((np.array([math.cos(q1), math.sin(q1)]), r"$y_1$", "tab:red"),
                           (np.array([-math.sin(q1), math.cos(q1)]), r"$z_1$", "tab:red")):
        ax.annotate("", xy=0.15 * vec, xytext=(0, 0),
                    arrowprops=dict(arrowstyle="-|>", color=col, lw=1.0, ls="--"))
        ax.text(*(0.165 * vec), name, color=col, fontsize=10)

    ax.plot([0, A[0]], [0, A[1]], color="tab:green", lw=3.2)
    ax.plot([A[0], P[0]], [A[1], P[1]], color="tab:blue", lw=2.6)
    ax.plot([0, P[0]], [0, P[1]], ls=":", color="0.4", lw=1.2)
    ax.plot(*A, "o", color="tab:green", ms=6)
    ax.plot(*P, "o", color="tab:red", ms=7)
    ax.text(A[0] - 0.018, A[1] - 0.020, "A", fontsize=11)
    ax.text(P[0] - 0.030, P[1] - 0.022, r"P$(y_P,z_P)$", fontsize=10)
    ax.text(0.5 * A[0] - 0.018, 0.5 * A[1] + 0.008, r"$d$", color="tab:green",
            fontsize=11)
    ax.text(0.5 * (A[0] + P[0]) + 0.010, 0.5 * (A[1] + P[1]),
            r"$L$", color="tab:blue", fontsize=11)

    ax.add_patch(plt.matplotlib.patches.Arc(
        (0, 0), 0.12, 0.12, theta1=0 if q1 > 0 else math.degrees(q1),
        theta2=math.degrees(q1) if q1 > 0 else 0,
        color="tab:red", lw=1.4))
    ax.text(0.055, -0.035, r"$\theta_1$", color="tab:red", fontsize=12)
    ax.set_xlim(-0.20, 0.20)
    ax.set_ylim(-0.38, 0.18)
    ax.set_aspect("equal")
    ax.grid(True, alpha=0.2)
    ax.set_xlabel(r"$y_0$ / m")
    ax.set_ylabel(r"$z_0$ / m")
    ax.set_title(label(r"$\theta_1$ 的 $yz$ 投影（右前腿，$d<0$）",
                       r"$\theta_1$ from the yz projection"))
    savefig(fig, "5_2_theta1")
    plt.close(fig)


def draw_theta3() -> None:
    q3 = -1.5
    A = np.array([0.0, 0.0])
    O3 = np.array([0.0, -L_THIGH])
    P = O3 + np.array([-L_CALF * math.sin(q3), -L_CALF * math.cos(q3)])
    interior = math.pi - abs(q3)

    fig, ax = plt.subplots(figsize=(5.2, 5.4))
    ax.plot([A[0], O3[0]], [A[1], O3[1]], color="tab:blue", lw=3.0)
    ax.plot([O3[0], P[0]], [O3[1], P[1]], color="tab:orange", lw=3.0)
    ax.plot([A[0], O3[0]], [A[1], O3[1] - 0.13], color="0.4", lw=1.1, ls="--")
    ax.plot([A[0], P[0]], [A[1], P[1]], color="0.4", lw=1.0, ls=":")
    ax.plot(*A, "o", color="0.2", ms=6)
    ax.plot(*O3, "o", color="0.2", ms=6)
    ax.plot(*P, "o", color="tab:red", ms=7)
    ax.text(A[0] - 0.020, A[1] + 0.012, "A", fontsize=11)
    ax.text(O3[0] + 0.012, O3[1] + 0.010, r"$O_3$", fontsize=11)
    ax.text(P[0] + 0.010, P[1] - 0.025, "P", fontsize=11)

    ax.text(0.008, -0.10, r"$l_{\rm hip}$", color="tab:blue", fontsize=10)
    ax.text(0.095, -0.135, r"$l_{\rm knee}$", color="tab:orange", fontsize=10)
    ax.text(0.115, -0.095, r"$|AP|$", color="0.4", fontsize=10)
    ax.add_patch(plt.matplotlib.patches.Arc(
        O3, 0.10, 0.10, theta1=90 - math.degrees(interior),
        theta2=90, color="0.25", lw=1.2))
    ax.text(O3[0] + 0.030, O3[1] - 0.045, r"$\angle AO_3P$", fontsize=9.5)
    # theta_3 is measured from the downward extension, clockwise negative.
    ax.add_patch(plt.matplotlib.patches.Arc(
        O3, 0.16, 0.16, theta1=270, theta2=270 - math.degrees(q3),
        color="tab:red", lw=1.6))
    ax.text(O3[0] + 0.035, O3[1] - 0.105, r"$\theta_3<0$（顺时针）",
            color="tab:red", fontsize=11)
    ax.set_xlim(-0.13, 0.24)
    ax.set_ylim(-0.37, 0.07)
    ax.set_aspect("equal")
    ax.grid(True, alpha=0.2)
    ax.set_xlabel(label("大腿—小腿平面内的 $x$ / m", "planar x / m"))
    ax.set_ylabel(label("同一平面内的 $z$ / m", "planar z / m"))
    ax.set_title(label("余弦定理求 $\\theta_3$：三角形 $AO_3P$",
                       "cosine rule in triangle AO3P"))
    savefig(fig, "5_2_theta3")
    plt.close(fig)


def _slice(kind: str):
    if kind == "yz":
        a = np.linspace(-0.24, 0.10, 130)
        b = np.linspace(-0.44, 0.06, 130)
        # Build explicitly: horizontal y, vertical z, x fixed at 0.
        ys, zs = np.meshgrid(a, b, indexing="xy")
        state = np.zeros_like(ys, dtype=int)
        cond = np.full_like(ys, np.nan)
        for iz in range(zs.shape[0]):
            for iy in range(zs.shape[1]):
                p = np.array([0.0, ys[iz, iy], zs[iz, iy]])
                try:
                    q = ik(p)
                except ValueError:
                    continue
                state[iz, iy] = 2 if in_limits(q) else 1
                if state[iz, iy] == 2:
                    cond[iz, iy] = condition(q)
        return a, b, state, cond, "y", "z"
    a = np.linspace(-0.30, 0.30, 150)
    b = np.linspace(-0.44, 0.06, 150)
    xs, zs = np.meshgrid(a, b, indexing="xy")
    state = np.zeros_like(xs, dtype=int)
    cond = np.full_like(xs, np.nan)
    for iz in range(zs.shape[0]):
        for ix in range(zs.shape[1]):
            p = np.array([xs[iz, ix], D_RIGHT, zs[iz, ix]])
            try:
                q = ik(p)
            except ValueError:
                continue
            state[iz, ix] = 2 if in_limits(q) else 1
            if state[iz, ix] == 2:
                cond[iz, ix] = condition(q)
    return a, b, state, cond, "x", "z"


def draw_workspace() -> None:
    fig, axs = plt.subplots(1, 2, figsize=(10.4, 4.8))
    for ax, kind in zip(axs, ("yz", "xz")):
        a, b, state, cond, xname, zname = _slice(kind)
        # Background: 0 unreachable, 1 outside joint limits, 2 valid.
        cmap = plt.matplotlib.colors.ListedColormap(
            ["#e8e8e8", "#f4c27a", "#cfe8d8"])
        ax.imshow(state, origin="lower", extent=[a[0], a[-1], b[0], b[-1]],
                  aspect="auto", cmap=cmap, vmin=0, vmax=2, zorder=1)
        # One condition-number contour is enough to show where the leg
        # becomes anisotropic; a dense set of labels would cover the map.
        lc = np.log10(np.where(np.isfinite(cond), cond, np.nan))
        ax.contour(a, b, lc, levels=[np.log10(30.0)], colors=["#2f7d4f"],
                   linewidths=[1.2], zorder=3)

        if kind == "yz":
            ax.plot(-0.08505, -0.24864, "o", color="tab:red", ms=6, zorder=5)
            ax.text(-0.080, -0.262, "home P", fontsize=8, color="tab:red")
            ax.set_xlabel(r"$y_0$ / m")
            ax.set_ylabel(r"$z_0$ / m")
            ax.set_title(label("$x=0$ 的 $yz$ 截面", "$yz$ slice at $x=0$"))
        else:
            # In the xz plane through the thigh joint A, the leg is a 2-link
            # chain of radius l_thigh + l_calf; the joint limit stops short.
            rr = np.linspace(math.pi, 2 * math.pi, 240)
            ax.plot(0.4 * np.cos(rr), 0.4 * np.sin(rr), ls="--", color="#a11",
                    lw=1.2, zorder=4)
            rmax = 2 * L_THIGH * math.cos(abs(Q_MAX[2]) / 2)
            ax.plot(rmax * np.cos(rr), rmax * np.sin(rr), ls=":",
                    color="#2f7d4f", lw=1.3, zorder=4)
            ax.plot(0.0, -0.24864, "o", color="tab:red", ms=6, zorder=5)
            ax.text(0.02, -0.262, "home P", fontsize=8, color="tab:red")
            ax.text(-0.10, -0.425, r"$r=0.4$ 几何奇异圆（不可达）", fontsize=8,
                    color="#a11")
            ax.text(-0.28, -0.33, r"$r=0.359$ 膝限位边界", fontsize=8,
                    color="#2f7d4f")
            ax.set_xlabel(r"$x_0$ / m")
            ax.set_ylabel(r"$z_0$ / m")
            ax.set_title(label("$y=d$ 的 $xz$ 截面", "$xz$ slice at $y=d$"))
        ax.set_xlim(a[0], a[-1])
        ax.set_ylim(b[0], b[-1])
        ax.set_aspect("equal")
        ax.tick_params(labelsize=8)
    from matplotlib.lines import Line2D
    from matplotlib.patches import Patch
    handles = [
        Patch(facecolor="#cfe8d8", edgecolor="none", label="可达（限位内）"),
        Patch(facecolor="#f4c27a", edgecolor="none", label="几何有解、超出限位"),
        Patch(facecolor="#e8e8e8", edgecolor="none", label="无解"),
        Line2D([0], [0], color="#2f7d4f", lw=1.2, label="条件数 30 等值线"),
        Line2D([0], [0], color="#a11", lw=1.2, ls="--", label=r"$r=0.4$ 奇异圆"),
        Line2D([0], [0], color="#2f7d4f", lw=1.2, ls=":", label=r"$r=0.359$ 膝限位"),
    ]
    fig.legend(handles=handles, loc="lower center", ncol=3, fontsize=8,
               frameon=False)
    fig.subplots_adjust(bottom=0.20)
    savefig(fig, "5_2_workspace")
    plt.close(fig)


def main() -> None:
    print("=== 5.2 A1 右前腿的逆运动学 ===")
    print(f"关节限位: {Q_MIN.round(4)} ... {Q_MAX.round(4)} rad")
    print("网格: 20 x 20 x 10 = 4000 点, "
          "x∈[-0.22,0.22], y∈[-0.18,0.00], z∈[-0.38,-0.12] m")
    stats = scan_grid()
    print(f"几何有解 = {stats['solved']} / {stats['total']} "
          f"({100 * stats['solved'] / stats['total']:.2f}%)")
    print(f"关节限位内 = {stats['valid']} / {stats['total']} "
          f"({100 * stats['valid'] / stats['total']:.2f}%)")
    print(f"无解 = {stats['unsolved']} / {stats['total']} "
          f"({100 * stats['unsolved'] / stats['total']:.2f}%)")
    print(f"限位外 = {stats['out_of_limits']} / {stats['total']} "
          f"({100 * stats['out_of_limits'] / stats['total']:.2f}%)")
    print(f"最大 IK→FK 往返误差 = {stats['max_err']:.2e} m")
    print(f"与 Leg.ik 的最大差 = {compare_package():.2e} rad")
    print(f"网格内最大条件数 = {stats['max_cond']:.2f} "
          f"(q = {stats['max_q'].round(4)}, p = {stats['max_p'].round(4)})")

    q_ref = np.array([0.15, 0.65, -1.50])
    p = fk(q_ref)
    print(f"\n膝关节两个分支 (目标 P = {p.round(5)} m):")
    print(" 分支       q1       q2       q3     限位内")
    for knee in (-1, 1):
        q = ik(p, knee=knee, wrap=False)
        print(f" knee={knee:+d}  {q[0]:8.4f} {q[1]:8.4f} {q[2]:8.4f}   "
              f"{'是' if in_limits(q) else '否'}")
    print(f" 原始角度     {q_ref[0]:8.4f} {q_ref[1]:8.4f} {q_ref[2]:8.4f}   "
          f"{'是' if in_limits(q_ref) else '否'}")
    print("\n膝分支说明: A1 的 q3 限位为 [-2.6965, -0.9163] rad，"
          "只接受 knee=-1；knee=+1 的解在限位外。")

    draw_theta1()
    draw_theta3()
    draw_workspace()


if __name__ == "__main__":
    main()
