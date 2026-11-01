"""Unconstrained quadratic programming: scalar and vector cases (Section 7.4).

For a random symmetric positive definite Q and a random c, the minimiser of
J(x) = 0.5 x^T Q x + c^T x is computed three ways: the closed form
x = -0.5 Q^{-1} c, a linear solve with numpy, and OSQP as a black-box QP.
All three should agree to solver tolerance.  The residual of the gradient is
printed instead of a wall-clock time, because the latter is not reproducible.

expect: 标量二次函数
expect: 向量二次型
expect: 三种解法的偏差
expect: OSQP
"""

from __future__ import annotations

import logging

import numpy as np
import osqp
from scipy import sparse

from quadruped import savefig

import matplotlib.pyplot as plt

plt.rcParams["mathtext.fontset"] = "dejavusans"
logging.getLogger("matplotlib").setLevel(logging.ERROR)
RNG = np.random.default_rng(704)


def scalar_case(a: float, b: float, c: float) -> float:
    print("=== 7.4 二次规划 ===")
    print(f"标量二次函数 f(x) = {a:.2f} x^2 + {b:.2f} x + {c:.2f}")
    xstar = -b / (2 * a)
    print(f"  驻点 x = -b/(2a) = {xstar:.6f}")
    print(f"  二阶导 2a = {2 * a:.2f} > 0，是全局极小")
    return xstar


# ANCHOR: vector_case
def vector_case(n: int = 5) -> tuple[np.ndarray, np.ndarray]:
    M = RNG.normal(size=(n, n))
    Q = M.T @ M + n * np.eye(n)          # symmetric positive definite
    c = RNG.normal(size=n) * 2.0
    x_closed = -np.linalg.solve(Q, c)
    x_solve = np.linalg.solve(Q, -c)
    P = sparse.csc_matrix(Q)
    solver = osqp.OSQP()
    solver.setup(P=P, q=c, A=sparse.csc_matrix(np.zeros((1, n))),
                 l=np.array([0.0]), u=np.array([0.0]), verbose=False,
                 eps_abs=1e-10, eps_rel=1e-10, polish=True)
    result = solver.solve()
    x_osqp = np.asarray(result.x)

    print(f"向量二次型：x ∈ R^{n}，Q 对称正定，最小特征值 "
          f"{np.linalg.eigvalsh(Q).min():.4f}")
    print("三种解法的偏差：")
    print(f"  ||x_closed - x_solve||_inf = "
          f"{np.abs(x_closed - x_solve).max():.3e}")
    print(f"  ||x_osqp  - x_solve||_inf = "
          f"{np.abs(x_osqp - x_solve).max():.3e}")
    print(f"OSQP 状态 {result.info.status}，迭代 {result.info.iter} 次")
    for name, x in (("解析", x_closed), ("numpy", x_solve), ("OSQP", x_osqp)):
        print(f"  {name:5s} 梯度残差 ||Qx+c||_2 = "
              f"{np.linalg.norm(Q @ x + c):.3e}，目标值 = "
              f"{0.5 * x @ Q @ x + c @ x:.8f}")
    return Q, c
# ANCHOR_END: vector_case



def plot_cases(x_scalar: float, Q: np.ndarray, c: np.ndarray) -> None:
    fig, ax = plt.subplots(1, 2, figsize=(8.4, 3.6))
    x = np.linspace(-3.0, 3.0, 400)
    a, b, cc = 1.3, -2.2, 0.8
    ax[0].plot(x, a * x ** 2 + b * x + cc, lw=1.8)
    ax[0].axvline(x_scalar, color="C1", ls="--", lw=1.0)
    ax[0].plot([x_scalar], [a * x_scalar ** 2 + b * x_scalar + cc], "o",
               color="C1", ms=6)
    ax[0].set_xlabel("$x$")
    ax[0].set_ylabel("$f(x)$")
    ax[0].set_title("标量：$f(x)=ax^2+bx+c$，$a>0$")
    ax[0].grid(True, alpha=0.3)

    # Two-dimensional projection of the vector problem.
    Q2 = Q[:2, :2]
    c2 = c[:2]
    x1, x2 = np.meshgrid(np.linspace(-2.0, 2.0, 80), np.linspace(-2.0, 2.0, 80))
    J = 0.5 * (Q2[0, 0] * x1 ** 2 + 2 * Q2[0, 1] * x1 * x2
               + Q2[1, 1] * x2 ** 2) + c2[0] * x1 + c2[1] * x2
    cs = ax[1].contour(x1, x2, J, levels=14, cmap="viridis", linewidths=0.8)
    xs = -0.5 * np.linalg.solve(Q2, c2)
    ax[1].plot(*xs, "r*", ms=13, label="最优点")
    ax[1].set_xlabel("$x_1$")
    ax[1].set_ylabel("$x_2$")
    ax[1].set_title("二维投影：等高线与最优点")
    ax[1].legend(fontsize=8)
    ax[1].grid(True, alpha=0.25)
    fig.colorbar(cs, ax=ax[1], shrink=0.85, label="$J$")
    fig.tight_layout()
    savefig(fig, "7_4_qp")
    plt.close(fig)


def main() -> None:
    xs = scalar_case(1.3, -2.2, 0.8)
    Q, c = vector_case()
    plot_cases(xs, Q, c)


if __name__ == "__main__":
    main()
