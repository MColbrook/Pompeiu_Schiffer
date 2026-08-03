#!/usr/bin/env python3
"""Exact rational-polynomial checks for Zernike identities and symmetry."""
from fractions import Fraction as Q
from math import comb


def clean(p):
    return {k: v for k, v in p.items() if v}


def add(*polys):
    out = {}
    for p in polys:
        for monomial, value in p.items():
            out[monomial] = out.get(monomial, Q(0)) + value
    return clean(out)


def scale(c, p):
    return clean({m: c * v for m, v in p.items()})


def multiply(p, q):
    out = {}
    for (a, b), x in p.items():
        for (c, d), y in q.items():
            key = (a + c, b + d)
            out[key] = out.get(key, Q(0)) + x * y
    return clean(out)


def basis(mode, s):
    """Phi_(mode,s) as a polynomial in independent variables z and zbar."""
    n = abs(mode)
    out = {}
    # P_s^(0,n)(2t-1) = sum_k C(s,k) C(s+n,s-k)(t-1)^(s-k)t^k.
    for k in range(s + 1):
        outer = Q(comb(s, k) * comb(s + n, s - k))
        for u in range(s - k + 1):
            power = k + u
            value = outer * comb(s - k, u) * (-1) ** (s - k - u)
            a = power + (n if mode >= 0 else 0)
            b = power + (n if mode < 0 else 0)
            out[(a, b)] = out.get((a, b), Q(0)) + value
    return clean(out)


def laplacian(p):
    out = {}
    for (a, b), value in p.items():
        if a and b:
            out[(a - 1, b - 1)] = out.get((a - 1, b - 1), Q(0)) + 4 * a * b * value
    return clean(out)


def trace(p, derivative=False):
    out = {}
    for (a, b), value in p.items():
        mode = a - b
        factor = a + b if derivative else 1
        out[mode] = out.get(mode, Q(0)) + factor * value
    return clean(out)


def shifted(p, da, db):
    return {(a + da, b + db): value for (a, b), value in p.items()}


def check_multiplication_recurrences():
    t = {(1, 1): Q(1)}
    for mode in (-20, -3, -1, 0, 1, 4, 20):
        n = abs(mode)
        for s in range(7):
            phi = basis(mode, s)
            den = 2 * s + n + 1
            if mode >= 0:
                rhs_z = scale(Q(s + n + 1, den), basis(mode + 1, s))
                if s:
                    rhs_z = add(rhs_z, scale(Q(s, den), basis(mode + 1, s - 1)))
            else:
                rhs_z = add(scale(Q(s + 1, den), basis(mode + 1, s + 1)),
                            scale(Q(s + n, den), basis(mode + 1, s)))
            assert shifted(phi, 1, 0) == rhs_z, ("z", mode, s)

            if mode <= 0:
                rhs_bz = scale(Q(s + n + 1, den), basis(mode - 1, s))
                if s:
                    rhs_bz = add(rhs_bz, scale(Q(s, den), basis(mode - 1, s - 1)))
            else:
                rhs_bz = add(scale(Q(s + 1, den), basis(mode - 1, s + 1)),
                             scale(Q(s + n, den), basis(mode - 1, s)))
            assert shifted(phi, 0, 1) == rhs_bz, ("zbar", mode, s)

            a = Q((s + 1) * (s + n + 1), (2 * s + n + 1) * (2 * s + n + 2))
            c = Q(0) if s == 0 else Q(s * (s + n), (2 * s + n) * (2 * s + n + 1))
            rhs_r2 = add(scale(a, basis(mode, s + 1)), scale(1 - a - c, phi))
            if s:
                rhs_r2 = add(rhs_r2, scale(c, basis(mode, s - 1)))
            assert multiply(t, phi) == rhs_r2, ("r2", mode, s)


def check_clamped_inverse():
    for mode in (-20, -10, 0, 10, 20):
        n = abs(mode)
        for s in range(1, 7):
            d = n + 2 * s
            kphi = add(
                scale(Q(1, 4 * d * (d + 1)), basis(mode, s - 1)),
                scale(Q(-1, 2 * d * (d + 2)), basis(mode, s)),
                scale(Q(1, 4 * (d + 1) * (d + 2)), basis(mode, s + 1)),
            )
            assert laplacian(kphi) == basis(mode, s), (mode, s)
            assert trace(kphi) == {}, ("Dirichlet", mode, s)
            assert trace(kphi, derivative=True) == {}, ("Neumann", mode, s)


def laurent_product(p, q):
    out = {}
    for a, x in p.items():
        for b, y in q.items():
            out[a + b] = out.get(a + b, Q(0)) + x * y
    return clean(out)


def lshift(p, amount):
    return {k + amount: value for k, value in p.items()}


def conjugate(p):
    return {-k: value for k, value in p.items()}


def check_symmetry_normalization():
    a, b = Q(7), Q(3)
    p = {0: a, 1: b}
    pbar = conjugate(p)
    squared = laurent_product(p, pbar)
    assert squared[0] == a * a + b * b
    assert squared[1] == a * b  # physical cosine amplitude is 2*a*b

    principal_p = {0: a}
    dp0 = add(laurent_product({0: Q(1)}, principal_p),
              laurent_product(principal_p, {0: Q(1)}))
    dp1 = add(laurent_product({1: Q(1)}, principal_p),
              laurent_product(principal_p, {-1: Q(1)}))
    assert dp0 == {0: 2 * a}
    assert dp1 == {-1: a, 1: a}

    # Exact angular form of (25): the two real-symmetry halves are conjugate.
    u = {-2: Q(1, 5), -1: Q(-1, 4), 0: Q(6, 5), 1: Q(-1, 4), 2: Q(1, 5)}
    j = 4
    derivative = add(lshift(laurent_product(pbar, u), j),
                     lshift(laurent_product(p, u), -j))
    principal = {j: a, -j: a}
    base = add(scale(Q(1, a), laurent_product(pbar, u)), {0: Q(-1)})
    shifted_defect = scale(a, add(lshift(base, j), conjugate(lshift(base, j))))
    assert add(derivative, scale(-1, principal)) == shifted_defect

    rho = Q(21, 20)
    full_principal_norm = sum(rho ** abs(k) * abs(v) for k, v in principal.items())
    assert full_principal_norm == 2 * a * rho**j


def main():
    check_multiplication_recurrences()
    check_clamped_inverse()
    check_symmetry_normalization()
    print("symbolic PASS: exact recurrences, clamped inverse, and symmetry normalization")


if __name__ == "__main__":
    main()
