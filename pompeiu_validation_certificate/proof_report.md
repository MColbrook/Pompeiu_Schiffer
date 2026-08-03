# Directed-rounding validation of the regular Pompeiu counterexample

> **Manifest-hardened certificate gate (2 August 2026): AUDIT GENERATION
> PASS; CLEAN PACKAGED REPRODUCTION PENDING.** Dawn job `32612504`
> regenerated the 192-bit audit and two byte-identical 256-bit audits from
> the frozen source and shipped inverse, and ended `COMPLETED` with exit code
> `0:0`.
> The resulting audit SHA-256 values are
> `3bf1af9b3334f33c7e0e53f3c3377d1330e6dde0a3a335573442d951c89aeb9a`
> (192 bit) and
> `0012f9e23bb636596719e7e7e10193728950ef84bc4f26a8aa1972a49eee3922`
> (each 256-bit file).  A separately staged archive must still pass a clean
> `THREADS=80 ./reproduce.sh` run before this report may be treated as an
> authenticated theorem claim for this certificate.  The supplied and first
> repaired predecessor certificates support the formulae below, but do not
> discharge that remaining packaging-to-reproduction gate.

## 1. Result and logical status

This computation validates an exact zero, near the supplied hexadecimal
centre, of

\[
F(g,p):=g+|p|^2(1+Kg)=0.
\tag{1}
\]

Here

\[
p(z)=\sum_{j\geq0}p_jz^{10j},\qquad p_0>0,
\tag{2}
\]

and \(K\) is the inverse of the Laplacian with both Dirichlet and radial
Neumann trace clamped to zero. The validated parameters are

\[
(L,S,R,J,\rho)=(60,40,30,30,1.05),\qquad
N=(L+1)S+(J+1)=2471.
\tag{3}
\]

The rigorous 256-bit bounds are strictly inside the simpler rational
majorants

\[
Y\leq 0.000000000159,\qquad Z\leq0.621,\qquad
C_2\leq122,\qquad C_3\leq0.012.
\tag{4}
\]

For the exact decimal radius

\[
r=0.000001
\tag{5}
\]

these majorants give, by exact decimal arithmetic,

\[
Y+Zr+C_2r^2+C_3r^3-r
\leq-0.000000378718999999988<0,
\tag{6}
\]

and

\[
Z+2C_2r+3C_3r^2-1
\leq-0.378755999999964<0.
\tag{7}
\]

Thus the Newton-like map is a strict contraction of its closed radius-\(r\)
ball. The geometry estimates below show that its exact zero defines a
noncircular domain with analytic Jordan boundary. The final implication to
Fourier cancellation is proved in Section 11.

This report is explanatory rather than a trusted executable conclusion.
`certificate.json`, the three compact audit JSON files,
`source_manifest.txt`, and `SHA256SUMS` bind every numerical leaf. The
standalone `verify_certificate.py` authenticates those leaves and recomputes
their support, aggregate bounds, and all final rational inequalities. The
standalone checker does not execute the recurrence engine or mechanically
verify the analytic prose in this report. The separate `reproduce.sh` starts
from an authenticated, immutable archive snapshot, force-builds the frozen
recurrence source in a clean tree, and regenerates the leaves using the
shipped exact binary64 inverse. Agreement with the packaged audits
establishes the source-and-proof-input-to-audit link. These two stages have
deliberately different scopes.

## 2. Basis, symmetry, and norms

For a signed integer angular exponent \(m\), put \(n=|m|\) and

\[
\Phi_{m,s}(r,\theta)
=r^nP_s^{(0,n)}(2r^2-1)e^{im\theta}.
\tag{8}
\]

The production basis is \(Z_{\ell,s}=\Phi_{10\ell,s}\). A real,
reflection-symmetric function is represented by its nonnegative complex
coefficients:

\[
f=\sum_{s\geq0}f_{0,s}Z_{0,s}
+\sum_{\ell>0}\sum_{s\geq0}f_{\ell,s}
\left(Z_{\ell,s}+\overline{Z_{\ell,s}}\right).
\tag{9}
\]

Consequently a positive angular coefficient \(f_{\ell,s}\) is the physical
cosine amplitude \(2f_{\ell,s}\). Let

\[
c_0=1,\qquad c_\ell=2\quad(\ell>0).
\tag{10}
\]

The domain and range norms are

\[
\|(\delta g,\delta p)\|_X
=\sum_{\ell\geq0,\,s\geq1}c_\ell\rho^\ell|\delta g_{\ell,s}|
+\sum_{j\geq0}2\bar p_0\rho^j|\delta p_j|,
\tag{11}
\]

and

\[
\|y\|_Y=\sum_{\ell,s\geq0}c_\ell\rho^\ell|y_{\ell,s}|.
\tag{12}
\]

Here and below \((\bar g,\bar p)\) denotes the exact binary64 centre, while
complex conjugation is written explicitly. The finite row and column
scalings are therefore

\[
(D_r)_{\ell,s}=c_\ell\rho^\ell,\qquad
(D_c)_{g_{\ell,s}}=(c_\ell\rho^\ell)^{-1},\qquad
(D_c)_{p_j}=(2\bar p_0\rho^j)^{-1}.
\tag{13}
\]

The factor two in the shape norm is essential. At \(p=\bar p_0\), the
\(p_0\) derivative has coefficient \(2\bar p_0\), while a real \(p_j\),
\(j>0\), produces two conjugate principal coefficients \(\bar p_0\).
Their full symmetric norm is \(2\bar p_0\rho^j\). No additional factor two
is inserted into the positive complex coefficient of \(|p|^2\).

## 3. Exact recurrence and clamped-inverse audit

Let \(d=2s+n+1\). Direct Jacobi identities give

\[
z\Phi_{m,s}=
\begin{cases}
\dfrac{s+n+1}{d}\Phi_{m+1,s}
+\dfrac{s}{d}\Phi_{m+1,s-1},&m\geq0,\\[6pt]
\dfrac{s+1}{d}\Phi_{m+1,s+1}
+\dfrac{s+n}{d}\Phi_{m+1,s},&m<0,
\end{cases}
\tag{14}
\]

and

\[
\bar z\Phi_{m,s}=
\begin{cases}
\dfrac{s+n+1}{d}\Phi_{m-1,s}
+\dfrac{s}{d}\Phi_{m-1,s-1},&m\leq0,\\[6pt]
\dfrac{s+1}{d}\Phi_{m-1,s+1}
+\dfrac{s+n}{d}\Phi_{m-1,s},&m>0.
\end{cases}
\tag{15}
\]

Terms with radial index \(-1\) are zero. Multiplication by
\(r^2=z\bar z\) is

\[
r^2\Phi_{m,s}
=a_s\Phi_{m,s+1}+(1-a_s-c_s)\Phi_{m,s}
+c_s\Phi_{m,s-1},
\tag{16}
\]

where

\[
a_s=\frac{(s+1)(s+n+1)}
{(2s+n+1)(2s+n+2)},\qquad
c_s=\frac{s(s+n)}
{(2s+n)(2s+n+1)},\qquad c_0=0.
\tag{17}
\]

For \(s\geq1\), set \(D=n+2s\). The exact clamped inverse is

\[
K\Phi_{m,s}
=\frac{\Phi_{m,s-1}}{4D(D+1)}
-\frac{\Phi_{m,s}}{2D(D+2)}
+\frac{\Phi_{m,s+1}}{4(D+1)(D+2)}.
\tag{18}
\]

Term-by-term differentiation proves
\(\Delta K\Phi_{m,s}=\Phi_{m,s}\). Since
\(P_s^{(0,n)}(1)=1\), the Dirichlet trace of every \(\Phi_{m,s}\) is one,
while its radial derivative at \(r=1\) is
\(q_{n,s}=n+2s(s+n+1)\). Summing the three coefficients in (18), and then
summing them after multiplication by \(q_{n,s-1},q_{n,s},q_{n,s+1}\),
proves respectively that the Dirichlet and radial Neumann traces vanish.

`tests/symbolic.py` separately expands
\(P_s^{(0,n)}(2z\bar z-1)\) over
\(\mathbb Q[z,\bar z]\). It checks (14)--(18), the Laplacian and both
boundary traces for representative positive, zero, and negative modes. It
also checks the real/complex normalisation, both shape-derivative factors,
and the conjugate-pair form of the far-shape shift. These are symbolic
identities; no floating-point oracle is used.

## 4. Directed intervals and exact input

Every token in `data/center_L60_S40_R30.hex` is parsed by MPFR and is rejected
unless it is an exact finite IEEE-754 binary64 value. Thus the centre is an
exact rational vector, not an uncertainty box. Its SHA-256 is

`a30353f226a96c88a3d7e8b54b9d29d000c884fadb6cc52b47f176a874d73d29`.

The production `Interval` type stores an MPFR lower and upper endpoint.
Addition, subtraction, multiplication, fused multiply-add, division by
positive quantities, and conversion to binary64 name RNDD or RNDU
explicitly. Exact zeros are preserved. The finite residual and derivative
are rebuilt only from (14)--(18); there is no quadrature, collocation, root
computation, or moment-map formulation.

Distinct angular-source tasks and boundary columns are parallelised with
OpenMP. Each recurrence accumulator is thread-local, the schedule is fixed
where ordering matters, and final norm reductions occur in a prescribed
order with upward MPFR rounding. Nested BLAS threading is disabled. Both
the 192-bit and 256-bit production audits used 80 OpenMP threads, MPFR
4.2.1, and GCC 11.5.0.  Before any backend entry point performs proof
arithmetic, it requires `mpfr_buildopt_tls_p()!=0`; otherwise it terminates
nonzero.  Every complete audit binds `mpfr_buildopt_tls_p=true`, and the
exact checker requires the literal Boolean.

All executable entry points that accept dimensions or precision now share a
fail-closed supported-input contract. It requires

\[
L\geq0,\qquad S\geq1,\qquad 0\leq R,J\leq L,
\tag{19a}
\]

each cutoff at most `INT_MAX/128`, and the derived dimension within the
signed-index range. A centre has exactly \(R+1+(L+1)S\) finite exact-binary64
hexadecimal coefficients with \(p_0>0\). Precision tokens are nonempty ASCII
digit strings in \([64,4096]\); interval headers must contain the derived
\(N\); and interval and inverse payloads must have their exact prescribed
sizes. In particular, unsupported cases with \(R>L\) or \(J>L\) are rejected
by the assembler and both verifiers before proof arithmetic.

This contract closes generic defects found outside the authenticated
parameter regime: an out-of-core finite row could formerly be aliased when
\(R>L\) or \(J>L\), the tail precision parser accepted trailing characters,
and tail dimension arithmetic was insufficiently guarded. The repaired
implementation also checks the defensive row maps and derived support sizes.
Regression tests exercise both forbidden dimension orderings, malformed
precision and payloads, and the complete small tail path. The fixed theorem
instance (3) has always satisfied \(R=J=30<L=60\), so the aliasing path was
not reachable in the packaged computation. The final certificate checker is
not a generic checker: it is deliberately pinned to (3), \(\rho=1.05\), and
the authenticated centre hash.

A later independent audit found that the first repaired checker excluded
every regular file whose basename was exactly `SHA256SUMS` when constructing
the actual file set, rather than excluding only the root manifest.  The
authentic first repaired archive contained no such nested file, so this did
not alter a proof source, numerical leaf, or theorem inequality; it did,
however, violate the advertised fail-closed archive contract.  The current
checker compares the root-relative path with `SHA256SUMS`.  Its smoke suite
adds an unlisted `junk/SHA256SUMS` and requires rejection.  The complete
audits were regenerated after this authenticated-source repair.

## 5. Rigorous finite inverse enclosure

Define the scaled finite derivative

\[
M=D_rDF(\bar g,\bar p)D_c.
\tag{19}
\]

LAPACK is used only to choose an approximate inverse \(R\) of the midpoint
of \(M\). Each entry of \(R\) is then frozen as an exact binary64 rational.
The same matrix, with SHA-256

`aa2de444bcae9e0c9bdf8afc8f5290ef5c363d84caf1c1de2df31fe569f9ea1c`,

is used in every rigorous run. Its exact 48,846,752 bytes are shipped as
`data/approx_inverse.bin`; the header is `POMINV1`, version 1, with dimension
2471. These bytes were recovered without alteration from
`pompeiu_regular_validation/work/final_inverse.bin` inside the immutable
original archive `pompeiu_regular_validation.zip`, whose SHA-256 is
`1f1153b6b00b239146f9ce45ce7d092acdb4abb20399da1a63ef445293125845`.
They agree with the inverse hash already bound into all three authenticated
audits. There is no interval inversion. A fresh LAPACK selection may be run
as a non-authoritative reproducibility diagnostic, but its bytes are never
substituted for this frozen proof input.

Here is the enclosure theorem used for the products. Put

\[
u=2^{-53},\qquad
\gamma_n=\frac{nu}{1-nu},\qquad
\beta=2^{-1022},\qquad
\tau_n=\frac{n\beta}{1-nu}.
\tag{20}
\]

For a nonnegative fixed-order FMA sum \(\widehat q\), the code encloses the
corresponding exact sum above by

\[
q\leq\frac{\widehat q+\tau_n}{1-\gamma_n}.
\tag{21}
\]

For a signed midpoint dot product, it separately forms a positive FMA sum of
absolute products, applies (21), and adds
\(\gamma_nq+\tau_n\). For an interval vector
\([x]=x_m+[-x_r,x_r]\), the separately enclosed positive dot
\(|R|x_r\) is added. Consequently every row of \(R[x]\), and hence every
column sum of \(I-R[M]\), is enclosed. The finite run had at most 2471
terms, for which the recorded upper bound was
\(\gamma_{2471}=0.000000000000274336109385\).

The binary64 environment is checked to be little-endian IEEE-754 with a
53-bit significand and round-to-nearest FMA in every worker. Nonzero
subnormal multiplicands in the finite product are rejected. A conservative
normal-underflow reserve using \(\beta\) is included, and an MPFR
precomputation rejects any possible overflow; the recorded overflow-guard
ratio was at most \(10^{-25}\). Tail source intervals whose midpoint or
radius would be subnormal are first enlarged to a normal midpoint-radius
box. The 192-bit tail audit recorded 10,847 such safe enlargements and
their lexicographically first witness.

With

\[
\mathcal A(y_c,y_t)=(Ry_c,y_t),
\tag{22}
\]

the 256-bit finite bounds are

\[
\|R[D_rF_c]\|_1
\leq0.0000000001388812932990041,
\tag{23}
\]

\[
\|I-R[M_{cc}]\|_1
\leq0.0000000084670219820804252,
\tag{24}
\]

and

\[
\max(1,\|R\|_1)
\leq1134.9686631686115561024053023.
\tag{25}
\]

The separately coded, non-rigorous NumPy recurrence in
`src/recurrence_reference.py` supplies smoke-test midpoint regression values
at run time. There is no separately supplied metrics file, and no regression
value enters (23)--(25).

## 6. Complete residual and finite-to-tail block

The finite-support centre implies exact algebraic support. Iterating
(14)--(15) through every monomial \(z^{10j}\bar z^{10k}\) in
\(|p|^2\), including the single \(s\mapsto s\pm1\) enlargement from (18),
gives \(h\leq L+R\), \(s\leq S+10R+1\), and, if \(h>L\),
\(s\leq S+1+10(L+R-h)\). Thus put

\[
s_{\max}=S+10R+1=341,\qquad h_{\max}=L+R=90.
\tag{26}
\]

The omitted residual rows are exactly

\[
\begin{aligned}
&0\leq h\leq60,\quad 41\leq s\leq341,\\
&31\leq h\leq60,\quad s=0,\\
&61\leq h\leq90,\quad
0\leq s\leq41+10(90-h).
\end{aligned}
\tag{27}
\]

These sets contain \(18{,}361+30+5{,}610=24{,}001\) rows. The compact audit
contains one outward interval for every key and the checker recomputes the
key set and its sum. At 256 bits,

\[
Y_t\leq0.0000000000194660826832232,
\tag{28}
\]

so

\[
Y=Y_c+Y_t
\leq0.0000000001583473759822273.
\tag{29}
\]

For every one of the 2471 finite input columns the audit stores three
intervals: its finite defect, its omitted-row image, and an outward
enclosure of their sum. Thus the reverse finite-to-tail block is not
estimated separately and then forgotten. Its largest tail part is

\[
0.0387847225477323115372386
\tag{30}
\]

at finite column 39; the complete sum for that column is

\[
0.0387847225478923340959041.
\tag{31}
\]

## 7. Complete \(g\)-tail columns

For

\[
P_\rho=\sum_{j=0}^{R}|\bar p_j|\rho^j,
\tag{32}
\]

the full symmetric coefficient algebra gives
\(\||\bar p|^2f\|\leq P_\rho^2\|f\|\). From (18), the absolute column sum
of \(K\) at total degree \(D\) is

\[
\kappa_D=
\frac1{4D(D+1)}
+\frac1{2D(D+2)}
+\frac1{4(D+1)(D+2)}.
\tag{33}
\]

Each term decreases with \(D\). Every omitted \(g\)-column that can reach a
finite row is enumerated:

\[
\begin{aligned}
&0\leq\ell\leq60,\quad 41\leq s\leq341
&&(18{,}361\text{ columns}),\\
&61\leq\ell\leq90,\quad
1\leq s\leq41+10(90-\ell)
&&(5{,}580\text{ columns}).
\end{aligned}
\tag{34}
\]

For each of these 23,941 columns, the identity part of
\(DF_g=I+|\bar p|^2K\) cancels against the tail identity in
\(I-\mathcal ADF\). The recurrence computes the remaining
\(|\bar p|^2K\) column exactly as intervals, splits it into finite and tail
rows, applies \(R\) rigorously to the finite part, and adds the two
outward-rounded norms before taking a maximum.

The controlling enumerated column is \((\ell,s)=(0,41)\):

\[
Z_{g,\partial}
\leq0.1683977492491521010808953.
\tag{35}
\]

Its finite and tail contributions are respectively bounded by
0.0566921907723566265158155 and
0.1117055584767954745650798. The largest tail contribution considered
alone occurs at \((0,42)\), but the operator norm is determined from
complete column sums, not from unrelated block maxima.

Outside (34), every column has \(D\geq684\). Therefore monotonicity of
(33) proves the uniform far bound

\[
P_\rho^2\kappa_{684}
\leq0.0063388718907811340136372.
\tag{36}
\]

The checker independently reconstructs \(P_\rho\) as an exact rational from
the hexadecimal centre and verifies that the audit interval in (36)
encloses this formula.

## 8. Complete shape-tail columns

For a real shape perturbation, the derivative is

\[
D_pF(\bar g,\bar p)[\eta]
=(\overline{\bar p}\,\eta+\bar p\,\overline\eta)
(1+K\bar g).
\tag{37}
\]

After the normalisation (13), one complex half of the defect for the
\(p_j\) column is the shifted coefficient sequence

\[
z^{10j}
\left(
\frac{\overline{\bar p}}{\bar p_0}(1+K\bar g)-1
\right).
\tag{38}
\]

The other half is its conjugate. The signed support of the parenthesised
sequence is \([-90,60]\): \(1+K\bar g\) has support \([-60,60]\), while
\(\overline{\bar p}\) has support \([-30,0]\). Hence a shape column can
reach a finite angular row \(h\leq60\) only through \(j\leq150\).

Every column \(31\leq j\leq150\) is therefore enumerated. Positive and
negative signed coefficients contributing to the same real output mode are
added as intervals before the weighted absolute sum is taken. Its finite
part is passed through \(R\); its tail part is added to form the complete
column. The controlling boundary column is \(j=36\):

\[
Z_{p,\partial}
\leq0.6202382261408257435597591,
\tag{39}
\]

with finite part at most 0.050856827093331113953667 and tail part at most
0.5693813990474946296060921.

At \(j=151\), (38) has only positive output modes
\(61,\ldots,211\), so it has no finite component. Increasing \(j\) by one
multiplies the sequence by \(z^{10}\). On these positive modes, multiplication
by \(z^{10}\) has weighted operator norm at most \(\rho\): each recurrence
column is nonnegative and its coefficients sum to one. The next \(p\)-column's
input normalisation gains the factor \(\rho\). Thus the normalised shift has
operator norm at most one, and tail projection is nonexpansive.
The real-symmetric output consists of the conjugate pair, and the same
factor two occurs in the input normalisation, so this conversion introduces
no missing factor. Hence the 151 explicit weighted mode witnesses at
\(j=151\) uniformly bound every \(j\geq151\) and give

\[
Z_{p,\infty}
\leq0.5997353899953234299039542.
\tag{40}
\]

Equations (31), (35), (36), (39), and (40) cover every input column of
\(X_c\oplus X_t^g\oplus X_t^p\). Since an \(\ell^1\) operator norm is the
supremum of complete column sums,

\[
Z=\max\{
Z_{c,\mathrm{complete}},Z_{g,\partial},
P_\rho^2\kappa_{684},Z_{p,\partial},Z_{p,\infty}\}
\leq0.6202382261408257435597591<1.
\tag{41}
\]

No sum or maximum of disconnected block estimates is substituted for a
complete column.

## 9. Nonlinear remainder

For a perturbation \((h,\eta)\), with \(\delta v=Kh\), the nonlinear
remainder is exactly

\[
N(h,\eta)
=2\operatorname{Re}(\overline{\bar p}\,\eta)Kh
+|\eta|^2(1+K\bar g)
+|\eta|^2Kh.
\tag{42}
\]

There are no higher-order terms. To justify the product constant, embed a
real-symmetric coefficient sequence into its full signed Laurent sequence.
For signed production angular indices \(a,b\in\mathbb Z\) and radial
indices \(s,t\geq0\), the normalised disc polynomials have the finite
linearisation

\[
\Phi_{10a,s}\Phi_{10b,t}
=\sum_q\lambda_q(a,s;b,t)\Phi_{10(a+b),q},\qquad
\lambda_q\geq0,\qquad \sum_q\lambda_q=1.
\]

Indeed, the finite Clebsch--Gordan decomposition identifies the
\(\lambda_q\) with squares of Clebsch--Gordan coefficients. Their sum is
one by evaluating at radial coordinate \(r=1\), where every radial factor
is one and the common angular factor is \(e^{10i(a+b)\theta}\).
Conjugation, \(\overline{\Phi_{10a,s}}=\Phi_{-10a,s}\), covers negative
modes and every conjugated product. Because
\(\rho^{|a+b|}\leq\rho^{|a|}\rho^{|b|}\), the full signed weighted
\(\ell^1_\rho\) norm has product constant one. Finally, the multiplicities
\(c_0=1,c_\ell=2\) identify a real-symmetric sequence isometrically with
its full signed sequence, so the same constant includes every symmetry
factor. This proves the coefficient-algebra constant one used below for all
indices, rather than inferring it from a finite numerical test.

Equation (33) is maximised at the smallest nonharmonic degree \(D=2\), and

\[
\|K\|\leq\kappa_2
=\frac1{24}+\frac1{16}+\frac1{48}
=\frac18.
\tag{43}
\]

Let

\[
b=\bar p_0,\quad
P=\|\bar p\|_\rho,\quad
B=\|1+K\bar g\|_\rho,\quad
\alpha=\max(1,\|R\|_1).
\tag{44}
\]

The 256-bit audit encloses

\[
P\leq54.5376092625986572670626629,\qquad
B\leq31.5906699718702093093725126,
\tag{45}
\]

and \(\alpha\) by (25). Applying the algebra estimate to the polarised
quadratic and cubic terms in (42), with the allocation of the \(X\)-norm
between \(h\) and \(\eta\), gives the deliberately conservative constants

\[
C_2=
\alpha\max\left\{
\frac{P}{16b},\frac{B}{4b^2}
\right\}
<121.02023842214,
\tag{46}
\]

\[
C_3=\frac{\alpha}{96b^2}
<0.011569342491152.
\tag{47}
\]

Write \(\bar x=(\bar g,\bar p)\) and define the Newton-like map
\(T(x)=x-\mathcal A F(x)\). Equations (29), (41), and (42) imply, for
\(\|h\|_X\leq r\),

\[
\|T(\bar x+h)-\bar x\|_X
\leq Y+Zr+C_2r^2+C_3r^3,
\qquad
\|DT(\bar x+h)\|_X
\leq Z+2C_2r+3C_3r^2.
\]

Moreover, (24) and the Neumann lemma make the square matrix \(RM_{cc}\)
invertible, hence \(R\) is invertible. Therefore the block operator
\(\mathcal A=R\oplus I\) is invertible, and any fixed point of \(T\)
satisfies \(F=0\), not merely \(\mathcal AF=0\).

In particular the coarser exact bounds \(C_2\leq122\) and
\(C_3\leq0.012\) used in (4)--(7) are outward. Equations (6) and (7)
therefore prove both the invariant-ball and derivative contraction
inequalities.

## 10. Univalence, analytic collar, and noncircularity

The exact centre values needed here are

\[
b=31.96700727715839462916846969164907932281494140625,
\tag{48}
\]

\[
\bar p_1
=12.1664288065045713693734796834178268909454345703125,
\tag{49}
\]

and

\[
\sum_{j=1}^{30}|\bar p_j|
=20.7530505576525261086359939852850242499911\ldots.
\tag{50}
\]

The real-symmetry class keeps \(p_0\) real. For any point in the
radius-\(r\) ball,

\[
p_0\geq b-\frac{r}{2b}>0,
\qquad
\sum_{j\geq1}|p_j-\bar p_j|
\leq\frac{r}{2b\rho}.
\tag{51}
\]

At \(r=10^{-6}\), exact rational evaluation of (51) gives

\[
\sum_{j\geq1}\left|\frac{p_j}{p_0}\right|
\leq0.649202172814341302900119697128253\ldots
<0.65<1.
\tag{52}
\]

Thus, on the closed unit disc,

\[
\operatorname{Re}\phi'(z)
=\operatorname{Re}\left(
1+\sum_{j\geq1}\frac{p_j}{p_0}z^{10j}
\right)>0.
\tag{53}
\]

The Noshiro--Warschawski criterion (or its elementary line-segment proof on
the convex disc) makes \(\phi\) univalent. Because \(\rho>1\), the weighted
shape norm gives convergence for \(|z|^{10}<\rho\), hence an analytic collar
strictly beyond the unit circle. The image boundary is therefore a
real-analytic Jordan curve.

The same ball gives

\[
|p_1|\geq|\bar p_1|-\frac{r}{2b\rho}
\geq12.16642879160826055548\ldots>12.16.
\tag{54}
\]

So \(\phi'\) is not constant. The 10-fold equivariance of \(\phi\) forces
any disc image to be centred at the origin. A conformal map from the unit
disc onto a centred disc that fixes the origin is linear; the normalisation
\(\phi'(0)=1\) would then make it the identity. Equation (54) excludes this,
and the validated domain is not a disc.

## 11. PDE transfer and Fourier cancellation

At the exact zero of (1), put

\[
v=Kg,\qquad U=1+v,\qquad k=p_0.
\tag{55}
\]

The analytic shape series also defines

\[
\phi(z)=z+\sum_{j\geq1}
\frac{p_j}{p_0(10j+1)}z^{10j+1}.
\tag{56}
\]

Here is the regularity passage needed to interpret the coefficient
identities as a PDE with boundary traces. The normalised disc polynomial
\(\Phi_{m,s}\) is a diagonal matrix coefficient of a unitary
\(\mathrm{SU}(2)\) representation, the same realisation used in the
Clebsch--Gordan argument above. Hence

\[
\sup_{\overline{\mathbb D}}|\Phi_{m,s}|\leq1.
\]

Let \(g_N\) be finite coefficient truncations and put \(v_N=Kg_N\). The
weighted coefficient norm and this supremum bound give \(g_N\to g\)
uniformly. The coefficient bound \(\|K\|\leq1/8\) gives \(v_N\to v\)
uniformly. For every \(N\), the polynomial identities already proved give

\[
\Delta v_N=g_N,\qquad
v_N|_{\partial\mathbb D}=0,\qquad
\partial_rv_N|_{\partial\mathbb D}=0.
\]

Fix any \(q>2\). Regard \(v_N-v_M\) as the zero-Dirichlet solution with
right-hand side \(g_N-g_M\). The standard Dirichlet estimate on the smooth
disc gives

\[
\|v_N-v_M\|_{W^{2,q}(\mathbb D)}
\leq C_q\|g_N-g_M\|_{L^q(\mathbb D)}.
\]

Thus \(v_N\) converges in \(W^{2,q}\); its uniform limit identifies the
limit as the coefficient-defined \(v=Kg\). The distributional Laplacian
and the continuous Dirichlet and normal trace maps therefore pass to the
limit:

\[
\Delta v=g,\qquad v|_{\partial\mathbb D}=0,\qquad
\partial_rv|_{\partial\mathbb D}=0.
\]

Since \(p=k\phi'\), equation (1) now implies in the weak
\(W^{2,q}\) sense

\[
\Delta U+|p|^2U=0,\qquad
U=1,\quad\partial_rU=0
\quad\text{on }\partial\mathbb D.
\tag{57}
\]

Let \(\Omega=\phi(\mathbb D)\) and \(u=U\circ\phi^{-1}\). The analytic
collar makes \(\phi\) a smooth diffeomorphism through the boundary, so the
weak equation and both traces pass under composition. Conformal covariance
of the planar Laplacian,

\[
\Delta(U)=|\phi'|^2(\Delta u)\circ\phi,
\tag{58}
\]

turns (57) into

\[
(\Delta+k^2)u=0\quad\text{in }\Omega,\qquad
u=1,\quad\partial_\nu u=0
\quad\text{on }\partial\Omega.
\tag{59}
\]

For any \(|\omega|=1\), the plane wave
\(w(x)=e^{ik\omega\cdot x}\) also satisfies
\((\Delta+k^2)w=0\). Because \(q>2\), the weak Green identity for
\(u\in W^{2,q}(\Omega)\) and smooth \(w\) has the displayed boundary
traces. Applying it and then the boundary data in (59) gives

\[
0=\int_{\partial\Omega}
(u\partial_\nu w-w\partial_\nu u)
=\int_{\partial\Omega}\partial_\nu w.
\tag{60}
\]

The divergence theorem and \(\Delta w=-k^2w\) now give

\[
0=-k^2\int_\Omega e^{ik\omega\cdot x}\,dx.
\tag{61}
\]

Because \(k=p_0>0\),

\[
\widehat{\mathbf1_\Omega}(k\omega)=0
\qquad(|\omega|=1).
\tag{62}
\]

Together with Sections 10 and 11, this proves the claimed regular
noncircular counterexample.

## 12. Precision, determinism, and standalone checking

The regenerated 192-bit audit is `audit_mpfr_192.json`, with SHA-256

`3bf1af9b3334f33c7e0e53f3c3377d1330e6dde0a3a335573442d951c89aeb9a`.

It gives

\[
Y\leq0.0000000002,\qquad
Z\leq0.63.
\tag{63}
\]

The first regenerated 256-bit audit is `audit_mpfr_256a.json`, with SHA-256

`0012f9e23bb636596719e7e7e10193728950ef84bc4f26a8aa1972a49eee3922`.

It gives the sharper values (29) and (41). The deterministic same-code rerun
is `audit_mpfr_256b.json`. The certificate records the hashes of both
256-bit files, and the checker requires their complete byte strings to be
identical. After removing only the precision label, it recursively checks
non-strict containment of every 256-bit interval in its 192-bit counterpart;
equality is allowed for exact values. During canonical audit serialisation,
every positive exported 192-bit bound is deliberately enlarged outward by a
factor \(1.01\). The nesting test is consequently a coarse precision
consistency check, not an implementation-diversity check.

All three audits bind the same exact input, source manifest, and binary64
inverse. The final source-manifest SHA-256 is recorded in

`certificate.json` and `SHA256SUMS`; it is

`2057deab6a10db09c8df250205f6a52807b5cde5ffba899a9bdbac7be3749ed5`.

The manifest-hardened audit-generation stage was run as Slurm job `32612504`
(`pompeiu-hardened-generate`) on `pvc-s-247.data.cluster`. The allocation was
one exclusive Dawn `pvc9` node: 96 physical Intel Xeon Platinum 8468 cores,
1,027,200 MB of Slurm memory, no swap, and four partition-required GPUs that
were not used by this CPU/MPFR computation. The validator used 80 OpenMP
threads. Slurm recorded a wall time of 39 minutes 20 seconds and a batch-step
maximum RSS of 203,606,608 K. The largest individually timed certification
step used 202,449,084 KiB; every timed command reported zero swaps and exit
status zero.

The force-built assembler used `/usr/bin/g++`, GCC 11.5.0, with MPFR 4.2.1
and GMP 6.3.0; the driver used Python 3.9.25 and NumPy 1.23.5. The node was
x86-64, little-endian, with two 48-core sockets and one hardware thread per
core. The environment fixed `OMP_NUM_THREADS=80`, disabled dynamic and
nested OpenMP execution, and fixed BLAS-family thread counts at one. Because
the exact shipped inverse was reused, this audit generation did not
invoke LAPACK to select a replacement inverse. MPFR reported
`mpfr_buildopt_tls_p=1`, and every complete audit records the literal Boolean
`provenance.mpfr_buildopt_tls_p=true`. The built assembler had SHA-256
`839e714dff2a00f5740c587a97a9bdf983fecab47ea9bab3cffa045cf1e63ce4`;
the 192- and 256-bit interval payloads had SHA-256 values
`17980dacd8e237c230c979a2a033a5be6a4ea6e3d4a55bc90af5a8241467f3dc`
and
`5ab2f7297b52811bd78187f074ff9c28950216fdfbbfeba3c5264459915b5439`,
respectively. The two 256-bit interval payloads had the same SHA-256; the
two audit JSON files were byte-identical by direct comparison. Relative to
the first repaired certificate, recursive parsed comparison found
exactly three structural changes in each audit: the two source-manifest hash
values changed, and `provenance.mpfr_buildopt_tls_p=true` was added. Every
numerical interval, support entry, witness, aggregate, and other metadata
leaf is unchanged.

These facts close the manifest-hardened source-to-audit generation stage
only. At
the time this report was updated, the fresh `SHA256SUMS`, final certificate
ZIP identity, and a clean reproduction from an extraction of that ZIP were
still pending. Failure of that clean packaged reproduction must leave the
manifest-hardened certificate unproved.

For a `PROVED` archive, the exact allowlist is 23 regular files recorded in
`SHA256SUMS`, plus `SHA256SUMS` itself and no additional member.

The standalone checker treats the authenticated interval leaves as inputs;
it does not rerun the expensive recurrence engine. It does not trust the
prose or aggregate numbers in the certificate, and it does not certify the
analytic derivations merely by parsing this report. It:

1. verifies the exact archive allowlist, `SHA256SUMS`, canonical JSON, safe
   relative paths, the exact source manifest, the frozen inverse, the ordered
   parameter trials, and the exact hexadecimal centre;
2. derives all 2471 finite keys, 23,941 \(g\)-boundary keys, 120 enumerated
   shape keys, 24,001 omitted residual keys, and 151 far-shape witness keys;
3. recomputes each complete column as finite plus tail and then recomputes
   \(Y\) and \(Z\);
4. reconstructs \(P\), \(B\), and \(P^2\kappa_{684}\) with Python
   `Fraction` arithmetic directly from the exact centre;
5. recomputes \(C_2,C_3\), both radii inequalities, (52), and (54) with
   exact rational arithmetic; and
6. rejects unless the 256-bit rerun is byte-identical and nested inside the
   192-bit enclosure.

`make symbolic` checks exact representative recurrences. `make smoke` checks
the MPFR backend against a separately coded, non-rigorous small float
recurrence and also runs rigorous finite and complete-tail paths, exact-
rational small-tail oracles, determinism checks, and malformed-input
rejections. Those tests check machinery rather than the theorem instance.
`make stats` reports 2,961 nonblank, noncomment lines across the six counted
assembler, certification-driver, reference, checker, and test files, and
enforces the transparently raised 3,000-line compactness
cap; that cap is code-hygiene evidence, not proof evidence.

`reproduce.sh` first copies the authenticated package to an immutable
reference snapshot and runs the standalone checker there. It then derives a
separate build tree, removes copied build products and Python caches, verifies
the source manifest, and force-builds the interval assembler. All three
audits are regenerated from the exact centre and shipped frozen inverse. The
two 256-bit audits must be byte-identical. With the recorded thread count,
all three regenerated audits must match their packaged byte strings; at a
different permitted thread count, the comparison may remove only the integer
`provenance.threads` field. Every other JSON value must agree. Finally, the
same immutable reference snapshot is checked again. Optional inverse
regeneration is labelled as a non-authoritative selector diagnostic and never
changes the proof input. Thus the checker establishes the authenticated
audit-to-theorem arithmetic, while the full reproducer establishes the
frozen-source-and-proof-input-to-audit computation.

The 192-bit run and two 256-bit runs use the same source, formulas, compiler
path, MPFR backend and frozen inverse. Their precision nesting and byte
determinism are useful consistency evidence, but they are not an independent
validation implementation and do not protect against a systematic defect
shared by that implementation.

Therefore the proof rests on exact identities, outward-rounded enclosures,
exhaustive finite support, monotone analytic tails, and exact final
arithmetic—not on numerical residuals alone.
