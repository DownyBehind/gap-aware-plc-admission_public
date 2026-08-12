# Equations

## Period Budget

`T_sched = T_ctrl - O_map`

`O_map` is retained as the constant term `c_0` of the beacon overhead model
`C_bcn(N, k) = c_0 + c_e * (N + s(k))` (axis B). The baseline configuration uses `O_map = 0`, so admission is checked against `T = 1395` directly.

## DC Request-Processing-Response Chain

`REQ_i -> C_proc -> RES_i`, with `f_res_i <= r_i + T_ctrl`.

## DC-Only Grant-Map Bound

`F_DC(N) = max(N*C_req_eff, C_req_eff + C_proc) + N*C_res_eff + B_blk`

## Processing Gap

`G(N) = max(0, C_proc - (N - 1)*C_req_eff)`

## Cond A

`max(N*C_req_eff + b_auth*(K + 1) + B_pkt, C_req_eff + C_proc) + N*C_res_eff + B_blk <= T_sched`

### Equivalence with the explicit gap form

The gap-explicit form,
`max(N*C_req, C_req + C_proc) + max(0, x - G(N)) + N*C_res + B <= T`
(with `x = q*k' + B_pkt` when `K' > 0` — the per-period authentication load
including the packetization guard), is algebraically identical to
the folded form above via the identity

`max(a + x, b) = max(a, b) + max(0, x - max(0, b - a))`, for `x >= 0`,

with `a = N*C_req`, `b = C_req + C_proc`, and `max(0, b - a) = G(N)`.
Note the inner `max(0, b - a)` is required: the naive variant
`max(0, x - (b - a))` is wrong whenever `a > b` (i.e. `G(N) = 0`), where it
would under-count the load by the negative gap.

### N = 0 (setup-only)

The process floor `C_req + C_proc` is the earliest start of the first DC
response, so it applies only while a DC stream exists (`N >= 1`). With
`N = 0` the whole period is open to authentication:
`F_active(0, K>0) = q*K + B_pkt + B_blk`, and the empty cycle is defined as
`F_active(0, 0) = 0` — there is no frame for the carry-in envelope `B_blk`
to delay.

## Transition Amplification

`alpha_tr = (C_req_eff + C_res_eff) / b_auth`

## Cond B

`max((N + K + 1)*C_req_eff, C_req_eff + C_proc) + (N + K + 1)*C_res_eff + B_blk <= T_sched`

## Admission Rule

`ADMIT iff CondA && CondB`

## SLAC Completion Bound

`q_slac = ceil(C_slac / b_auth)`

`(q_slac + 1) * T_ctrl <= D_slac`

Semantics: this is an admission-time per-session multiple check, L_init-inclusive —
`ceil(247/7)*T + T = 37T = 1850 ms <= D_slac = 2000 ms` at the baseline.

B_pkt/L_margin removal rationale: the
packetization debt is bounded by a single-frame envelope under debt-neutral
packetization and is absorbed by the +T rounding margin of the completion
bound; a `ceil((W + B_pkt) / q)` form would double-count the same debt.

## Loss-Aware Session Budget q(p) (axis C / E2, 2026-07-14)

Frame error rate `p`, session failure tolerance `eps`. Per-frame retry limit

`n_r(p, eps) = ceil(log eps / log p)`   (so that p^(1+n_r) <= eps... failure
probability of a frame that exhausts all 1+n_r attempts is p^(1+n_r) <= eps).
Over the supported range (p <= 1e-2) this equals the paper's union-bound
minimum — the least n_r with 20 * p^(n_r+1) <= eps, a union bound over the
20 messages; `experiments/ns3_e1/run_e2.py` asserts the equivalence at
runtime.

Worst-case and expected per-session channel demand:

`C_wc(p, eps) = C_slac + n_r * sum_m l_m = 247 + 241 * n_r`   (first
transmissions at the envelope; retries repeat the per-message airtimes, not the
envelope's six-slot excess)
`W_exp(p) = C_slac / (1 - p)`

Budgets over 39 service windows (of the ceil(D_g/T) = 40 cycles a session may
span, the joining cycle carries no credit): `q_wc(p) = ceil(C_wc / 39)`,
`q_exp(p) = ceil(W_exp / 39)`.

| p | n_r (eps=1e-6) | C_wc | q_wc | W_exp | q_exp |
|---|---|---|---|---|---|
| 1e-3 | 2 | 729 | **19** | 247.25 | **7** |
| 1e-2 | 3 | 970 | **25** | 249.49 | **7** |

Completion check under q_wc: `(ceil(729/19)+1)*T = 40 cycles`, `(ceil(970/25)+1)*T = 40 cycles` — both = D_g (= D_slac).

Two structural notes:

1. **Ceiling absorption**: q_exp(p) = 7 = the loss-blind budget at both p —
   expected-demand provisioning changes nothing here; the meaningful knob is
   worst-case provisioning with retry caps (q_wc).
2. **Measured validation**: the measured demand inflation
   (1.001 at p=1e-3, 1.0101 at p=1e-2) matches W_exp/C_slac = 1/(1-p)
   (1.001001, 1.010101) — the expected-demand model is empirically exact.

## Beacon Robustness under a Persistent Schedule (axis B / E3, 2026-07-14)

**Proposition (static harmlessness).** Under a persistent schedule every node
caches the last received map. If membership (N, K) is unchanged, the operative
map of cycle c equals that of cycle c-1, so after m consecutive beacon losses
every node's cached map still equals the operative map — transmissions,
finish, and DC miss are invariant in m, for any m.

**Risk case (map change + missed beacon).** Let the map change take effect at
cycle t0 while node x misses beacons t0 .. t0+m-1. Under *stale-persist*
(x keeps the old map) the response-phase start shifts by

`dS_completion = +C_req - q = +8 slots`  (one session completes: request
phase grows one frame, SLAC window shrinks q)
`dS_admission = +q = +7 slots`           (one session admitted: SLAC window grows q)

Any nonzero shift makes x's old-map non-preemptive frame overlap another
node's new-map slot by `min(C_res, |dS|) > 0` slots — a collision. Under
*fail-silent* (a node that missed the beacon of cycle c does not transmit in
cycle c) no overlap is possible; the cost is x's own frames deferred, at most
one response per stale cycle, and only for x.

**Effectiveness delay eta.** An admission granted in cycle t0 becomes
effective at the first beacon the new node receives: with m consecutive
losses that is cycle t0+m+1, i.e. a wait of `eta = (m+1)` cycles. The nominal
completion bound already contains one beacon wait (L_init), so beacon loss
adds `m*T`. Allowed consecutive losses from the deadline margin:
fluid-service bound 1864.04 ms -> margin 135.96 ms = 2.72T -> **m_max = 2**;
completion bound 1850 ms -> margin 150 ms = 3T -> **m_max = 3**.

## Slack Degradation

Along `(N_i, K_i) = (N0 + i, K0 - i)`, paid-regime per-completion degradation is `DeltaF = C_req_eff + C_res_eff - b_auth` (= 29 slots at the baseline, `b_auth = q = 7`).
