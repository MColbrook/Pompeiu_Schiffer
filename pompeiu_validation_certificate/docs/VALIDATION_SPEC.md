# Mathematical and computational validation specification

## 1. Target theorem and logical chain

Find an exact zero near the supplied numerical centre of the fixed-disc equation

\[
F(g,p):=g+|p|^2(1+Kg)=0.
\tag{1}
\]

Here `p` is analytic with real 10-fold coefficients,

\[
p(z)=\sum_{j\ge 0}p_jz^{10j},\qquad p_0>0,
\tag{2}
\]

and `K` is the inverse of the Laplacian with both Dirichlet and radial Neumann traces zero. If (1) has a zero, set

\[
v=Kg,\qquad U=1+v,\qquad k=p_0,
\tag{3}
\]

and

\[
\phi(z)=z+\sum_{j\ge1}\frac{p_j}{p_0(10j+1)}z^{10j+1}.
\tag{4}
\]

Then `p=k phi'` and

\[
\Delta U+|p|^2U=0,\qquad U=1,\quad \partial_rU=0
\quad\hbox{on }\partial\mathbb D.
\tag{5}
\]

Provided `phi` is univalent, `u=U\circ\phi^{-1}` satisfies

\[
(\Delta+k^2)u=0\quad\hbox{in }\Omega=\phi(\mathbb D),
\qquad u=1,\quad\partial_\nu u=0\quad\hbox{on }\partial\Omega.
\tag{6}
\]

Green's identity against every plane wave `exp(i k omega dot x)` then yields

\[
\widehat{\mathbf 1_\Omega}(k\omega)=0\qquad(|\omega|=1),
\tag{7}
\]

so `Omega` fails the Pompeiu property. The certificate must also prove that `Omega` is noncircular and has a real-analytic Jordan boundary.

## 2. Basis and exact clamped inverse

The code tracks the nonnegative complex Fourier coefficient of

\[
Z_{\ell,s}(r,\theta)
 =r^{10\ell}P_s^{(0,10\ell)}(2r^2-1)e^{i10\ell\theta}.
\tag{8}
\]

A real reflection-symmetric function is reconstructed as
`f_{0,s} Z_{0,s} + sum_{ell>0} f_{ell,s}(Z_{ell,s}+conj(Z_{ell,s}))`.
Thus the physical nonzero angular mode is `2 f_{ell,s} cos(10 ell theta)`.
This convention explains why the recurrence code does not insert a separate
factor two in the positive Fourier coefficient of `|p|^2`.

The harmonic rows are `s=0`. The unknown `g` has only nonharmonic coefficients `s>=1`. If `n=10 ell`, `s>=1`, and `D=n+2s`, then the clamped inverse is exactly

\[
K\Phi_{\ell,s}
 =\frac{\Phi_{\ell,s-1}}{4D(D+1)}
 -\frac{\Phi_{\ell,s}}{2D(D+2)}
 +\frac{\Phi_{\ell,s+1}}{4(D+1)(D+2)}.
\tag{9}
\]

The multiplication recurrences for `z`, `bar z`, and `r^2` are encoded in `src/recurrence_reference.py` and `src/interval_assemble.cpp`. Before trusting them, the production agent must derive them in the report and test them against exact symbolic polynomial identities for a representative finite set of modes.

## 3. Supplied finite centre

The recommended initial split is

\[
L=60,\qquad S=40,\qquad R=30,\qquad J=30,
\tag{10}
\]

with

\[
N=(L+1)S+(J+1)=2471.
\tag{11}
\]

Finite unknowns are

- `g_{ell,s}` for `0<=ell<=L`, `1<=s<=S`;
- `p_j` for `0<=j<=J`.

Finite residual rows are the matching nonharmonic rows plus harmonic rows `0<=ell<=J`. The centre in `data/center_L60_S40_R30.hex` is exact as a vector of binary64 rationals. Coefficients beyond `p_30` are zero at the validation centre.

The frozen implementation supports only `L>=0`, `S>=1`, `0<=R,J<=L`, cutoffs at most `INT_MAX/128`, consistent signed-index dimensions, and nonempty ASCII-digit precisions in `[64,4096]`. A centre has exactly `R+1+(L+1)S` finite exact-binary64 hexadecimal coefficients and `p0>0`; interval headers contain the derived `N`, and interval and inverse payload sizes are exact. The published split lies strictly inside this low-level contract. The final checker is separately pinned to `L=60,S=40,R=J=30,rho=1.05`; any broader split requires a separately reviewed implementation and checker, and must not be accepted by aliasing out-of-core rows.

## 4. Weighted norm and scaled finite matrix

Start with

\[
\rho=1.05.
\tag{12}
\]

Let `c_0=1` and `c_ell=2` for `ell>0`; these are the multiplicities of
the full symmetric Laurent sequence. For a perturbation `(delta g, delta p)`,
use

\[
\|(\delta g,\delta p)\|_X
 =\sum_{\ell,s\ge1}c_\ell\rho^\ell|\delta g_{\ell,s}|
  +\sum_{j\ge0}2p_0\rho^j|\delta p_j|.
\tag{13}
\]

For a residual `y`, use the full symmetric Wiener majorant

\[
\|y\|_Y=\sum_{\ell,s\ge0}c_\ell\rho^\ell|y_{\ell,s}|.
\tag{14}
\]

With this normalization the principal shape derivative is the identity for
all `j`: for `j=0` its coefficient is `2p_0`, while for `j>0` its positive
Fourier coefficient is `p_0` and the residual multiplicity is two. The finite
scaling matrices are

\[
(D_r)_{(\ell,s)}=c_\ell\rho^\ell,
\tag{15}
\]

and

\[
(D_c)_{g_{\ell,s}}=(c_\ell\rho^\ell)^{-1},\qquad
(D_c)_{p_j}=(2p_0\rho^j)^{-1}.
\tag{16}
\]

The scaled derivative is

\[
M=D_r\,D F(\bar g,\bar p)\,D_c.
\tag{17}
\]

Select a point approximate inverse `R` of the midpoint of `M`, freeze its binary64 bytes, authenticate them by SHA-256, and include the file in the proof archive. Thereafter `R` is a fixed matrix of exact binary rationals. Authoritative reproduction reuses these bytes and never substitutes regenerated LAPACK output; regenerating `R` with another BLAS/LAPACK is a non-authoritative diagnostic. Do not interval-invert a dense matrix.

## 5. Rigorous finite block

Assemble interval enclosures `[F_c]` and `[M_cc]` with a reviewed directed-rounding backend, preferably MPFR at at least 160 bits. A parallel MPFR run must fail unless `mpfr_buildopt_tls_p()` attests a thread-safe TLS build, and the audit provenance records that attestation. Parallelize over source angular modes or output blocks, with deterministic reductions.

The finite quantities must be bounded by

\[
Y_c=\|R[D_rF_c]\|_1,
\tag{18}
\]

and

\[
Z_{cc}=\|I-R[M_{cc}]\|_1.
\tag{19}
\]

A dense interval matrix product is not required. A binary64 or higher-precision midpoint product may be enclosed using a proved dot-product/GEMM error bound

\[
\gamma_n=\frac{nu}{1-nu},
\tag{20}
\]

combined with `|R| rad(M)`, provided the exact floating-point model, contraction settings, underflow, overflow, and library behavior are audited. Alternatively use MPFR dot products. The same applies to `R F_c`.

The separately coded NumPy recurrence supplies non-rigorous binary64 regression values at test time. They are not admissible upper bounds and no separate metrics file is authenticated.

## 6. Infinite decomposition and approximate inverse

Split both domain and range into

\[
X=X_c\oplus X_t^g\oplus X_t^p,
\qquad
Y=Y_c\oplus Y_t.
\tag{21}
\]

Use `R` on the finite block. On the tails use the identity after the normalization (13)-(16), unless a sharper explicitly verified Neumann preconditioner is helpful. Bound the global defect by **column sums of the full block operator**, rather than by separately optimistic block norms.

Every one of the following must be enclosed:

1. finite columns mapped into finite and omitted rows;
2. omitted `g` columns mapped into finite and tail rows;
3. omitted `p` columns mapped into finite and tail rows;
4. the reverse finite-to-tail output;
5. all radial and angular tails beyond the explicitly enumerated boundary layers.

Because the centre has finite support, all interactions reaching the finite core arise from finitely many boundary-layer columns. Enumerate those rigorously and use monotone analytic bounds only after proving the support cutoff.

## 7. `g`-tail estimate

Let

\[
P_\rho=\sum_{j=0}^{R}|p_j|\rho^j.
\tag{22}
\]

In the complex Fourier coefficient majorant, multiplication by `|p|^2` has norm at most `P_rho^2`. From (9), the absolute column sum of `K` at total index `D` is

\[
\kappa_D=
\frac1{4D(D+1)}+
\frac1{2D(D+2)}+
\frac1{4(D+1)(D+2)}.
\tag{23}
\]

For the first purely radial omitted mode at `ell=0,s=41`, `D=82`, and the float scout gives

\[
P_\rho^2\kappa_{82}\approx0.432.
\tag{24}
\]

This is not by itself the full `g`-tail block bound: boundary-layer columns may also reach finite rows and be amplified by `R`. Enumerate every such column with intervals. Prove that (23) decreases after the enumerated cutoff, including the angular tail `ell>L`.

## 8. `p`-tail estimate and shift argument

For an omitted shape coefficient `p_j`, the normalized derivative has a principal harmonic output equal to the unit vector in mode `j`. The defect can be represented in complex form as a shifted coefficient sequence derived from

\[
z^{10j}\left(\frac{\overline p}{p_0}(1+K\bar g)-1\right).
\tag{25}
\]

The production proof must establish the exact normalization and the real/complex cosine conversion. Multiplication by `z^{10}` divided by the angular weight `rho` is contractive in the weighted complex `ell^1` coefficient norm. Projection onto tail rows is also contractive.

Finite coupling is possible only for a finite range of `j`. With the supplied support, a safe initial range to inspect is `31<=j<=150`; the exact cutoff must be derived from the recurrence support, not assumed. Enclose every boundary column. Beyond that cutoff, prove a uniform shift bound from (25).

Float combined column sums for `j=31,32,35` are approximately `0.797,0.699,0.627`, respectively. These numbers merely indicate that a strict bound below one is plausible.

## 9. Omitted residual

At the finite-support centre, the residual has finite algebraic support. Evaluate every omitted coefficient with outward rounding, including both

- radial rows above `S`;
- angular rows above `L` or harmonic rows above `J`.

Do not infer an omitted-residual bound from sampled quadrature. The exact recurrence must be used. Apply the same scaled approximate inverse/tail map used in the global contraction and include the result in `Y`.

## 10. Nonlinear remainder

For perturbations `(h,eta)`, with `K h` denoted by `delta v`, the exact nonlinear remainder is

\[
N(h,\eta)
=2\operatorname{Re}(\overline{\bar p}\,\eta)Kh
 +|\eta|^2(1+K\bar g)
 +|\eta|^2Kh.
\tag{26}
\]

There are no higher terms. Use a complex symmetric coefficient representation, or prove every factor in the real cosine product majorant, so that no hidden factor of two is lost.

A global elementary bound available from (9) is

\[
\|K\|\le \kappa_2=\frac18
\tag{27}
\]

in the unweighted radial coefficient majorant. Combine this with the angular weight and interval enclosures of `||bar p||_rho` and `||1+K bar g||`. Produce outward-rounded constants `C_2,C_3` such that

\[
\|A N(h,\eta)\|_X
\le C_2\|(h,\eta)\|_X^2+C_3\|(h,\eta)\|_X^3.
\tag{28}
\]

Sharper componentwise constants are encouraged if multiplication by the full finite inverse norm is too pessimistic.

## 11. Radii inequalities

Produce outward-rounded `Y,Z,C_2,C_3` and a positive exact decimal or rational radius `r` satisfying

\[
Y+Zr+C_2r^2+C_3r^3<r,
\tag{29}
\]

and

\[
Z+2C_2r+3C_3r^2<1.
\tag{30}
\]

Here `Y` and `Z` must include all finite, cross, boundary-layer, and infinite-tail contributions. Equations (29)-(30) give an invariant contraction ball and a unique exact zero near the centre.

## 12. Geometry and nontriviality

The candidate satisfies approximately

\[
\sum_{j=1}^{30}|p_j/p_0|=0.6492021720\ldots.
\tag{31}
\]

The certificate must show, throughout the validated ball,

\[
\sum_{j\ge1}|p_j/p_0|<1.
\tag{32}
\]

Then `Re phi'(z)>0` on the closed unit disk, so `phi` is univalent and the boundary is analytic. The weighted norm with `rho>1` gives a complex collar. Also certify that `p_1` cannot vanish in the ball; hence `phi` is not affine and `Omega` is not a disk.

## 13. Audit and reproduction requirements

A result may be called a proof only after all of the following pass:

- symbolic low-mode checks of every recurrence;
- interval/float agreement on small systems;
- an exact-rational small tail oracle covering omitted, cross, boundary, and far-tail families;
- malformed-dimension, precision-token, payload-size, and inverse-size rejection tests;
- same-implementation 192/256-bit cross-precision comparison;
- non-strict nesting of the higher-precision bounds in compatible widened lower-precision bounds, with equality allowed;
- a same-thread deterministic rerun with byte-identical 256-bit audits, explicitly not implementation diversity;
- SHA-256 hashes for inputs, frozen inverse, and sources;
- a standalone authenticated-audit aggregation and final-inequality checker for (29), (30), and (32);
- a frozen-source-and-inverse-to-audit reproducer and a report that distinguishes this from the checker.

If any analytic support or norm claim fails, stop and document the precise failure. Do not replace it with numerical sampling.
