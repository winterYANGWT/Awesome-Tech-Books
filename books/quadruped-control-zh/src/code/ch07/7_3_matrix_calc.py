"""Matrix calculus used by the Kalman derivation (Section 7.3).

Checks the denominator-layout formulas with central differences:

* d(x^T A x)/dx = x^T (A + A^T)
* d(tr(AB))/dA = B^T
* d(tr(ABA^T))/dA = A (B + B^T), which is 2AB for symmetric B

The finite differences are the reference; no symbolic package is used.  The
gradient-field panel is drawn together with the state-space diagram in
``7_2_state_space.py`` (Figure 7-3), so this example only prints numbers.

expect: 二次型梯度
expect: 迹 tr(AB)
expect: 迹 tr(ABA^T)
expect: 非对称 B
"""

from __future__ import annotations

import logging

import numpy as np

RNG = np.random.default_rng(703)


def central_gradient(f, x: np.ndarray, h: float = 1e-6) -> np.ndarray:
    """Central difference of a scalar function, returned as a row vector."""
    g = np.zeros_like(x)
    for i in range(x.size):
        e = np.zeros_like(x)
        e[i] = h
        g[i] = (f(x + e) - f(x - e)) / (2 * h)
    return g


def matrix_gradient(f, A: np.ndarray, h: float = 1e-6) -> np.ndarray:
    G = np.zeros_like(A)
    for i in range(A.shape[0]):
        for j in range(A.shape[1]):
            E = np.zeros_like(A)
            E[i, j] = h
            G[i, j] = (f(A + E) - f(A - E)) / (2 * h)
    return G


def main() -> None:
    A = RNG.normal(size=(4, 4))
    b = RNG.normal(size=4)
    x = RNG.normal(size=4)
    f = lambda v: float(v @ A @ v + b @ v)
    g_analytic = x @ (A + A.T) + b
    g_fd = central_gradient(f, x)
    e_quad = np.abs(g_analytic - g_fd).max()
    print("=== 7.3 矩阵微积分 ===")
    print("二次型梯度 d(x^T A x + b^T x)/dx：")
    print(f"  解析值 {np.array2string(g_analytic, precision=6)}")
    print(f"  中心差分 {np.array2string(g_fd, precision=6)}")
    print(f"  最大绝对误差 {e_quad:.3e}")

    B = RNG.normal(size=(4, 4))
    f_trace = lambda M: float(np.trace(M @ B))
    g_trace = B.T
    e_trace = np.abs(matrix_gradient(f_trace, A) - g_trace).max()
    print("迹 tr(AB) 对 A 的导数：")
    print(f"  解析 B^T 与中心差分的最大误差 {e_trace:.3e}")
    print(f"  解析 |B^T| = {np.linalg.norm(B.T):.4f}")

    S = RNG.normal(size=(4, 4))
    S = 0.5 * (S + S.T)
    f_sym = lambda M: float(np.trace(M @ S @ M.T))
    g_sym = A @ (S + S.T)
    e_sym = np.abs(matrix_gradient(f_sym, A) - g_sym).max()
    e_2ab = np.abs(matrix_gradient(f_sym, A) - 2 * A @ S).max()
    print("迹 tr(ABA^T) 对 A 的导数（B 对称）：")
    print(f"  A(B+B^T) 与中心差分最大误差 {e_sym:.3e}")
    print(f"  2AB 与中心差分最大误差 {e_2ab:.3e}")

    S2 = RNG.normal(size=(4, 4))
    e_ns = np.abs(matrix_gradient(lambda M: float(np.trace(M @ S2 @ M.T)), A)
                  - A @ (S2 + S2.T)).max()
    e_wrong = np.abs(matrix_gradient(lambda M: float(np.trace(M @ S2 @ M.T)), A)
                     - 2 * A @ S2).max()
    print("非对称 B 的同一公式：")
    print(f"  A(B+B^T) 误差 {e_ns:.3e}；误用 2AB 的误差 {e_wrong:.3e}")

    print("梯度场见图 7-3 右栏：2×2 对称正定 Q，箭头方向为梯度方向")


if __name__ == "__main__":
    main()
