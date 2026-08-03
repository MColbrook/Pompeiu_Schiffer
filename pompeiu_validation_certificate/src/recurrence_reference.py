"""Non-rigorous NumPy reference for the exact Zernike recurrences.

This module is used only by smoke tests.  It must never be used to certify a
bound: the proof path is src/interval_assemble.cpp plus src/certify.py.
"""
from __future__ import annotations

import pathlib
from typing import Dict, List, Tuple

import numpy as np


C_INT_MAX = int(np.iinfo(np.intc).max)
MAX_CUTOFF = C_INT_MAX // 128


def dimensions(L: int, S: int, R: int, J: int) -> int:
    if (L < 0 or S < 1 or R < 0 or J < 0 or R > L or J > L
            or max(L, S, R, J) > MAX_CUTOFF):
        raise ValueError("invalid or unsupported centre dimensions")
    g_count = (L + 1) * S
    if g_count + J + 1 > C_INT_MAX:
        raise ValueError("centre dimension exceeds signed index range")
    return g_count


def read_center(path: str | pathlib.Path) -> tuple[int, int, int, int, np.ndarray, np.ndarray]:
    tokens = pathlib.Path(path).read_text().split()
    if len(tokens) < 4:
        raise ValueError("truncated centre header")
    L, S, R, J = map(int, tokens[:4])
    g_count = dimensions(L, S, R, J)
    if len(tokens) != 4 + R + 1 + g_count:
        raise ValueError("bad centre coefficient count")
    values = np.array([float.fromhex(x) for x in tokens[4:]], dtype=float)
    p = values[: R + 1]
    g = values[R + 1 :]
    if not np.all(np.isfinite(values)):
        raise ValueError("non-finite centre coefficient")
    if not p[0] > 0:
        raise ValueError("centre p0 is not positive")
    return L, S, R, J, p, g


def x_apply(n: int, matrix: np.ndarray) -> np.ndarray:
    nr, nc = matrix.shape
    s = np.arange(nr, dtype=float)
    a = (s + 1) * (s + n + 1) / ((2 * s + n + 1) * (2 * s + n + 2))
    c = np.zeros(nr)
    c[1:] = s[1:] * (s[1:] + n) / ((2 * s[1:] + n) * (2 * s[1:] + n + 1))
    b = 1 - a - c
    out = np.zeros((nr + 1, nc))
    out[1:] += a[:, None] * matrix
    out[:nr] += b[:, None] * matrix
    out[: nr - 1] += c[1:, None] * matrix[1:]
    return out


def multiply_step(mode: int, matrix: np.ndarray, by_z: bool) -> tuple[int, np.ndarray]:
    nr, nc = matrix.shape
    s = np.arange(nr, dtype=float)
    out = np.zeros((nr + 1, nc))
    if by_z:
        if mode >= 0:
            den = 2 * s + mode + 1
            out[:nr] += ((s + mode + 1) / den)[:, None] * matrix
            out[: nr - 1] += (s[1:] / den[1:])[:, None] * matrix[1:]
        else:
            q = -mode
            den = 2 * s + q + 1
            out[1:] += ((s + 1) / den)[:, None] * matrix
            out[:nr] += ((s + q) / den)[:, None] * matrix
        return mode + 1, out
    if mode <= 0:
        q = -mode
        den = 2 * s + q + 1
        out[:nr] += ((s + q + 1) / den)[:, None] * matrix
        out[: nr - 1] += (s[1:] / den[1:])[:, None] * matrix[1:]
    else:
        q = mode
        den = 2 * s + q + 1
        out[1:] += ((s + 1) / den)[:, None] * matrix
        out[:nr] += ((s + q) / den)[:, None] * matrix
    return mode - 1, out


def shift_mode(mode: int, matrix: np.ndarray, quotient_shift: int) -> tuple[int, np.ndarray]:
    by_z = quotient_shift >= 0
    for _ in range(10 * abs(quotient_shift)):
        mode, matrix = multiply_step(mode, matrix, by_z)
    return mode, matrix


def radial_powers(n: int, input_rows: int, degree: int) -> list[np.ndarray]:
    matrix = np.eye(input_rows)
    result = [matrix]
    for _ in range(degree):
        for _ in range(10):
            matrix = x_apply(n, matrix)
        result.append(matrix)
    return result


def clamped_inverse_block(quotient_mode: int, radial_count: int) -> np.ndarray:
    n = 10 * quotient_mode
    K = np.zeros((radial_count + 2, radial_count))
    s = np.arange(1, radial_count + 1, dtype=float)
    D = n + 2 * s
    K[np.arange(radial_count), np.arange(radial_count)] = 1 / (4 * D * (D + 1))
    K[np.arange(1, radial_count + 1), np.arange(radial_count)] = -1 / (2 * D * (D + 2))
    K[np.arange(2, radial_count + 2), np.arange(radial_count)] = 1 / (4 * (D + 1) * (D + 2))
    return K


def potential_blocks(p: np.ndarray, L: int, S: int, R: int) -> list[list[np.ndarray]]:
    input_rows = S + 2
    output_rows = S + 1
    blocks = [[np.zeros((output_rows, input_rows)) for _ in range(L + 1)] for _ in range(L + 1)]
    for l in range(L + 1):
        powers = radial_powers(10 * l, input_rows, R)
        differences = []
        for d in range(R + 1):
            nr = powers[R - d].shape[0]
            total = np.zeros((nr, input_rows))
            for r in range(R - d + 1):
                total[: powers[r].shape[0]] += p[r + d] * p[r] * powers[r]
            differences.append(total)
        for d in range(max(-R, -l), min(R, L - l) + 1):
            h = l + d
            mode, matrix = shift_mode(10 * l, differences[abs(d)], d)
            assert mode == 10 * h
            blocks[h][l] += matrix[:output_rows]
        if l > 0:
            for h in range(min(L, R - l) + 1):
                d = h + l
                mode, matrix = shift_mode(-10 * l, differences[d], d)
                assert mode == 10 * h
                blocks[h][l] += matrix[:output_rows]
    return blocks


def assemble_float(path: str | pathlib.Path) -> tuple[np.ndarray, np.ndarray]:
    L, S, R, Jmax, p, g = read_center(path)
    blocks = potential_blocks(p, L, S, R)
    g_count = (L + 1) * S
    N = g_count + Jmax + 1
    full_rows = (L + 1) * (S + 1)
    A = np.zeros((full_rows, g_count))
    b = np.zeros(full_rows)
    for h in range(L + 1):
        row = slice(h * (S + 1), (h + 1) * (S + 1))
        b[row] = blocks[h][0][:, 0]
        for l in range(L + 1):
            col = slice(l * S, (l + 1) * S)
            A[row, col] = blocks[h][l] @ clamped_inverse_block(l, S)
    U = []
    for l in range(L + 1):
        u = clamped_inverse_block(l, S) @ g[l * S : (l + 1) * S]
        if l == 0:
            u[0] += 1.0
        U.append(u)
    C = np.zeros((full_rows, Jmax + 1))
    for l in range(L + 1):
        powers = radial_powers(10 * l, S + 2, R)
        nr = max(x.shape[0] for x in powers)
        P = np.zeros((nr, R + 1))
        for r, matrix in enumerate(powers):
            P[: matrix.shape[0], r] = matrix @ U[l]
        shape_shift = max(R, Jmax)
        for h in range(max(0, l - shape_shift), min(L, l + shape_shift) + 1):
            d = abs(h - l)
            _, V = shift_mode(10 * l, P, h - l)
            for r in range(R + 1):
                if r + d <= Jmax:
                    C[h * (S + 1) : (h + 1) * (S + 1), r + d] += p[r] * V[: S + 1, r]
                if r <= Jmax and r + d <= R:
                    C[h * (S + 1) : (h + 1) * (S + 1), r] += p[r + d] * V[: S + 1, r]
        if l > 0:
            for h in range(max(0, shape_shift - l) + 1):
                d = h + l
                _, V = shift_mode(-10 * l, P, d)
                for r in range(R + 1):
                    if r + d <= Jmax:
                        C[h * (S + 1) : (h + 1) * (S + 1), r + d] += p[r] * V[: S + 1, r]
                    if r <= Jmax and r + d <= R:
                        C[h * (S + 1) : (h + 1) * (S + 1), r] += p[r + d] * V[: S + 1, r]
    selected = []
    for h in range(L + 1):
        selected.extend(h * (S + 1) + s for s in range(1, S + 1))
    selected.extend(h * (S + 1) for h in range(Jmax + 1))
    selected = np.asarray(selected, dtype=int)
    F = b + A @ g
    Jg = A.copy()
    for l in range(L + 1):
        for s in range(1, S + 1):
            F[l * (S + 1) + s] += g[l * S + s - 1]
            Jg[l * (S + 1) + s, l * S + s - 1] += 1.0
    return F[selected], np.column_stack((Jg[selected], C[selected]))
