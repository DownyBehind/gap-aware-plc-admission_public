# RESULTS.md — numeric claims and their producing runs

Input-constant provenance (types, sources, derivations, and limits) is recorded in [`docs/PARAMETER_PROVENANCE.md`](../docs/PARAMETER_PROVENANCE.md).

Every number cited in the paper, with the committed result file and the
script/configuration that produces it. All runs are deterministic
(fixed-seed mt19937); seed counts are listed per row.

| # | Claim | Value | Result file | Producing script | Configuration |
|---|---|---|---|---|---|
| 1 | Layer-1 knee positions (Table I, IFS=0 row) | {18, 17, 15, 11} for K ∈ {1, 4, 8, 16}, zero tolerance | `results/layer1/knee_verification.csv` | `experiments/layer1/verify_knee_e1.py` | N = 1..40 |
| 2 | Contention baseline DC deadline miss at (N₀=30, K=0) | 93.08% (67,018 / 72,000) | `results/ns3_e1/e4_csma_single_cell_30_0.csv` | `trackc-e4-csma --singleN0=30 --singleK=0 --perPpm=1000 --seeds=20 --horizon=120` | 20 seeds, 30 EVs × 120 cycles |
| 3 | Burst effect on DC miss (Table II; unified accounting, slack-resolved) | boundary (37,1): i.i.d. 814/10,000 = 8.14e-2, G-E worst 779/10,000 = 7.79e-2 (ratio 0.96); maximum defined ratio 81.5× at (36,1), sojourn 300, per_bad 0.05 (163/10,000 vs 2/10,000); some cells have i.i.d. 0 but burst > 0 | `results/ns3_e1/g3_burst_vs_iid.csv`, `results/ns3_e1/g3i_iid_baseline.csv`, `results/ns3_e1/g3c_burst_sensitivity.csv` | `experiments/ns3_e1/run_g3_burst_vs_iid.py` (`trackc-g3 --mode=i/c --cells=…`) | 13 cells over the envelope-slack spectrum × 12 G-E points, marginal slot-PER 6.7e-5 held, 20 seeds |
| 4 | Link-aware admission cuts SEVERE-link deadline violations | 7.1× (50 → 7 of 160) | `results/ns3_e1/g3b_link_aware.csv` | `experiments/ns3_e1/run_trackc_c4.py` | N₀=10, K=16, half GOOD / half SEVERE, SEVERE PER 1e-2, 20 seeds |
| 5 | SEVERE-link session violation share under count-based admission | 31.25% (50 / 160) | `results/ns3_e1/g3b_link_aware.csv` | same as #4 | same as #4 |
| 6 | Tightest admitted state finish | 1389 / 1395 slots (slack 6) at (38, 0) | `results/ns3_e1/e5_adversarial.csv`, `results/layer1/admission_region.csv` | `experiments/ns3_e1/run_e4e5.py` (e5), `experiments/layer1/sweep_admission_region.py` | adversarial carry-in realization |
| 7 | Naive-bound counterexample finish | 1402 slots at (37, 4) — analytic value, not a measurement | formula | `src/formulas/transition_formulas.py` (`F_active_state`) | max(555+28+21, 295) + 777 + 21 |
| 8 | Admitted-region size | 588 admitted = 295 hidden + 293 paid | `results/layer1/admission_region.csv` | `experiments/layer1/sweep_admission_region.py` | 966-cell grid |
| 9 | Loss outcomes (§V-C) | design points (q_wc=19/cap 2 at p=1e-3, 25/cap 3 at p=1e-2): 0/1440 violations; loss-blind q=7: 3/1440 at p=1e-2, 28/86/280 at p=0.03/0.05/0.1 | `results/ns3_e1/e2_admission_variants.csv` | `experiments/ns3_e1/run_e2.py` | 20 seeds, 1440 admitted sessions per row |
| 10 | Packet-aware bound decomposition at (37,1), q_wc=25 | 555 + (25 + 17) + 777 + B_blk = 1395 = T (equality); at (37,1) the realized maximum is 1386 envelope-inclusive (channel 1365); the trajectory maximum 1389 (channel 1368) occurs at the terminal state (38,0); measured straddle overrun ≤ 17 = max message 18 − 1 | `results/ns3_e1/loss_knee.csv` (`overrun_max` column) | `experiments/ns3_e1/run_loss.py`; trajectory adjudication: `ns3/contrib/ev-plc-transition/examples/trackc-thm1.cc` | q ∈ {19, 25}, offset sweep 0..40, PER=0 |
| 11 | Effective-parameter knee shifts under IFS (Table I, IFS 1–3 rows) | {17,16,14,11}, {16,15,13,10}, {15,14,12,9} | `results/ns3_e1/overhead_knee.csv` | `experiments/ns3_e1/run_overhead.py` | IFS ∈ {1,2,3}, 600 cells, zero error |
| 12 | Loss-completion ratio | 1/(1−p) to four digits (1.001, 1.0101) | `results/ns3_e1/loss_knee.csv` | `experiments/ns3_e1/run_loss.py` | p ∈ {1e-3, 1e-2} |
| 13 | Contention baseline with authenticating arrivals | 96.7% DC miss at (30,1), saturating to 0.989 at (30,35) | `results/ns3_e1/e4_policy_comparison_event.csv` | `experiments/ns3_e1/run_trackc_c4.py` | `hpgp_csma_ca` rows, 20 seeds |
| 14 | None of the three evaluated fixed-reservation fractions protects both deadlines across the tested grid | 10%: 100% session-deadline violations at K=35; 25/50%: DC miss 0.82–1.00 at high load | `results/ns3_e1/e4_policy_comparison_event.csv` | `experiments/ns3_e1/run_trackc_c4.py` | `fixed_10pct/25pct/50pct` rows |
| 15 | Cond-B ablation (Cond A only) | DC population reaches 40 at N₀=30 (47 at (15,35)); F_DC(40) = 1461 > 1395; ≈94% DC miss (95,750 / 101,920, as reported in the paper) at (15,35) with zero D_g violations | `results/ns3_e1/e4_ablation_condA_only.csv` | `trackc-e4-scheduled --condB=0` (same invocation as `run_trackc_c4.py`, Cond B flag off) | same grid/seeds/RNG as the policy comparison |
| 16 | Admission cost at (30,35) | 8/35 candidates per seed admitted (160/700), all at the first boundary (wait 0); mean admit wait 92.6 cycles is horizon-censored (27/35 right-censored at the 120-cycle horizon) | `results/ns3_e1/e4_policy_counts.csv`, `results/ns3_e1/e4_policy_comparison_event.csv` (`admit_wait_cycles`) | `experiments/ns3_e1/run_trackc_c4.py` | `acbs_qwc25`, 20 seeds |
| 17 | Provisioned credit under bursts | 0 / 1,920 admitted sessions violate D_g for bad-state sojourns {3, 15, 60, 300} slots | `results/ns3_e1/g3a_qwc_burst.csv` | `experiments/ns3_e1/run_trackc_c4.py` (`trackc-g3`, q_wc=25 cap 3) | 20 seeds × 4 sojourns × 3 per_bad |
| 18 | Link-aware experiment DC misses | 0 / 102,802 DC EV-cycles (both variants combined) | `results/ns3_e1/g3b_link_aware.csv` | same as #4 | same as #4 |
| 19 | Design corrections (honest findings) | round-robin window sharing starves late sessions at p=0 (design note in `ns3/standalone/e2_sim.cc`); a stale map kept across a membership change collides in 12 of 18 `stale_persist` alignments; backlog-free slot approximation optimistic in overload by up to +75 pp (fixed, (30,10)) / +11.1 pp (CSMA, (30,1)) | `results/ns3_e1/e3_adversarial.csv`, `results/ns3_e1/c3_csma_comparison.csv`, `results/ns3_e1/e4_policy_comparison.csv` vs `e4_policy_comparison_event.csv` | `experiments/ns3_e1/run_e3.py`, `run_trackc_c3.py`, `run_e4e5.py` + `run_trackc_c4.py` | Fig. 4 reports the corrected (event-driven) numbers |
| 20 | Loss-free cohort completion (Theorem 2 adjudication) | exhaustive K = 1..38 simultaneous cohorts, q = 7: per-session max elapsed 40 cycles = D_g exactly (zero margin) on the paper's 39-window timeline, first attained at K = 26; 39 cycles on the engine timeline; 0 violations — deterministic loss-free dynamics make the sweep a complete case analysis | `results/theorem2_completion.csv` | `experiments/analysis/run_theorem2_adjudication.py` (discipline port: `theorem2_adjudication.py`, verified against `e2_sim` capMode 2: completion indices 32/34/36/37 at K = 1/4/8/16) | q = 7, cap 0, all admitted at cycle 0 |
| 21 | Loss-aware credits are provisioning values, not a worst-case completion guarantee | staggered lossless admission (1/cycle, K = 15): 42 cycles (39-window timeline) / 41 (engine) > D_g — the cohort premise is necessary; q_wc = 19/cap 2 within-cap cohort counterexample (one session cap-maxed, demand 723 ≤ 729): 41 cycles > D_g on both timelines; q_wc = 25/cap 3: no violation in the 994-configuration cohort-scoped search, maxima 40 (39-window) / 39 (engine) — a search result, not a proof | `results/theorem2_adversarial.csv`, `results/theorem2_counterexample_traces.txt` | `experiments/analysis/run_theorem2_adjudication.py` | fixed failure families + `random.Random(7)` sweep (40 patterns/K, K ∈ {1..16, 20, 24, 30, 35}) |

## Release-aware credit requirement (`release_aware_credit.csv`)

Theorem 2's aggregate-workload argument is refined by a release-aware
check: at every release point i of the 20-message sequence (verbatim from
the replay table; sum 241 slots, releases 0..775 ms),
q_req(i) = ceil(C_i^rem / nu_i), where C_i^rem is the remaining
worst-case airtime (envelope remainder plus n_r retransmissions of the
remaining messages) and nu_i = 39 - floor(r_i / 50 ms) is the number of
remaining service windows before D_g = 40T (40 cycles minus the excluded
joining cycle). Results (`experiments/layer1/release_aware_credit.py`):
q_req = max_i q_req(i) = **7** (lossless), **19** (n_r = 2), **25**
(n_r = 3), each attained at the first release point (SLAC_PARM.REQ,
r = 0) — the release structure does not raise the credit requirement
beyond the aggregate values. Side identities: ceil((247 + 2*241)/39) = 19,
ceil((247 + 3*241)/39) = 25. The cycle counts ceil(729/19)+1 = 40,
ceil(970/25)+1 = 40, ceil(247/7)+1 = 37 are entitlement-coverage
arithmetic (service windows needed plus the joining cycle), not
completion guarantees; adjudicated completion behavior is recorded in
`theorem2_completion.csv` / `theorem2_adversarial.csv` (claims #20–21).

## Theorem 2 completion adjudication (`theorem2_completion.csv`, `theorem2_adversarial.csv`)

Deterministic adjudication of the setup-completion guarantee against the
exact committed credit discipline (capMode 2: per-session signed credit,
aggregate allowance = q·K_active − debt, non-preemptive straddle → debt,
persistent service pointer = first blocked session). The checker
(`experiments/analysis/theorem2_adjudication.py`) is an RNG-free port
verified against the committed `e2_sim` engine (completion cycle indices
32/34/36/37 at K = 1/4/8/16, q = 7, PER = 0, capMode 2); the adversary
chooses every retry outcome (≤ cap per message), admission offsets, and
session order. Two timeline conventions are reported: **39-window** (the
paper's credit derivation — no credit or service in the joining cycle)
and **engine** (credit begins in the admission cycle; one cycle earlier).

Findings (`experiments/analysis/run_theorem2_adjudication.py`):

1. **Loss-free simultaneous cohorts complete** (claim #20): exhaustive
   execution of all cohort sizes K = 1..38 — loss-free dynamics are
   deterministic per K, so the sweep is a complete case analysis —
   bounds per-session completion at **40 cycles = D_g exactly** (zero
   margin, first attained at K = 26) on the 39-window timeline, 39 on
   the engine timeline, with zero violations.
2. **The cohort premise is necessary**: staggered admission (one
   session per cycle, K = 15) violates D_g with **no losses at all**
   (42 / 41 cycles). Mechanism: as earlier sessions complete, the
   aggregate window quota q·K_active contracts, so service deferred
   during the crowded phase can lack the supply to be recovered.
3. **Entitlement coverage is not a worst-case service guarantee**
   (claim #21): at q_wc = 19/cap 2 a within-cap simultaneous-cohort
   pattern (one session cap-maxed, demand 723 ≤ C_wc = 729) completes
   at 41 cycles > D_g; at q_wc = 25/cap 3 the cohort-scoped search
   finds no violation (maxima 40 / 39) — a search result, not a proof.
4. All committed experiment observations are unaffected: the evaluated
   workloads admit sessions in simultaneous bursts, and their recorded
   zero-violation counts (claims #9, #17) remain exact observations.

The committed e2 CSVs carry no completion-cycle column and the engine's
violation counter tests only elapsed ≥ 40 cycles, so sub-deadline
completion margins were not previously recorded; the adjudication
artifacts add that visibility.

## Cond-A-only ablation (`ns3_e1/e4_ablation_condA_only.csv`)

Same engine, grid, seeds, and provisioning as the policy comparison
(N0 in {0,15,30} x K in {1,2,5,10,20,35}, 20 seeds, 120 cycles,
p = 1e-3, q_wc = 25/cap 3), with **Cond B disabled** in the admission
test (Cond A and all other code paths, including RNG draws, unchanged;
default-flag output remains byte-identical to the committed run —
gated). Purpose: isolate the contribution of the post-transition check
against completion-created load. Per-seed rows carry admitted /
never_admitted / wait / D_g violations / DC misses / EV-cycles and the
largest DC population reached (max_n).

## Gap-oblivious decision check (`gap_oblivious_check.csv`)

Analytic (no simulation): for N = 0..38 the maximum admissible k' under
gap-aware Cond A, gap-oblivious Cond A (the max(0, x - G(N)) hinge
replaced by the plain addend), and Cond B. At q_wc = 25 the joint
admission decision is **identical in all 39 rows** (Cond B binds wherever
G(N) > 0; for N >= 20, G(N) = 0 makes the two Cond-A forms equal), so a
gap-oblivious ablation cannot change admission behavior at these
parameters — recorded as an analytic result
(`experiments/layer1/gap_oblivious_check.py`).

## Seed-level statistics for the residual DC miss (`seed_level_ci.csv`)

From the committed per-seed instrumentation
(`ns3_e1/e4_slack_occupancy.csv`; aggregates verified identical to
`e4_policy_counts.csv`): worst cell (30,20) = 8/88,479 with per-seed
median 0 (15/20 seeds miss-free), range 0..9.0e-4, seed-level bootstrap
95% interval [2.3e-5, 1.9e-4]; runner-up (15,35) = 7/83,380, median 0
(14/20 miss-free), range 0..4.8e-4, bootstrap [2.4e-5, 1.4e-4]
(`experiments/analysis/seed_level_ci.py`, 10,000 resamples, fixed RNG).

## Burst-vs-i.i.d. contrast table (`ns3_e1/g3_burst_vs_iid.csv`)

One row per (N, K, bad_sojourn, per_bad): the cell's i.i.d. baseline
(mode=i, uniform slot-PER 6.7e-5) joined against the G-E run (mode=c, same
marginal slot-PER, same engine, same accounting `miss := finishSlot > T`,
same seeds 1..20, 500 cycles each → 10000 cycles/cell). `ratio` =
(miss_burst/total)/(miss_iid/total); `nan` when both are 0, `inf` when the
i.i.d. count is 0 but the burst count is not. `envelope_slack_slots` =
1395 − [max(15N + 7K + 21, 295) + 21N + 21]; bands use the worst-case
single-retransmission unit of 21 slots. Cross-check: at (37,1) the i.i.d.
measurement 8.14e-2 agrees with the analytic single-error envelope
1 − (1−6.7e-5)^1332 ≈ 8.5e-2 (1332 = exposed REQ+RES slots).

## q settings per experiment (`Q_SETTINGS.csv`)

Measured q (per-session SLAC budget, slots/cycle) and retry cap for every
experiment, with the source path and quoted constant — no inference. Headline: every boundary/burst artifact (E5, G1, G3c/G3i/
G3-burst-vs-iid, loss ii-b) runs at the loss-blind default **q = 7**
(`EvPlcParams::m_bAuthSlots`, `TransitionParams.b_slac_slots`); q_wc = 25
(cap 3) appears only in the policy experiments (E2 variant C, the E4
gap-aware policy, G3a, Thm1). At q_wc = 25 the (37,1) message-blind envelope is
max(555+25+21, 295) + 777 + 21 = **1399 > T = 1395** — unreachable/every-
cycle-miss under full-window replay accounting; it is admitted only via
the packet-aware bound 555 + (25+17) + 777 + 21 = 1395 = T (claim #10),
with realized trajectory max 1389 < T. The (37,1) boundary state of the
burst experiments (slack 14) therefore exists **only at q = 7**.

## Slack-occupancy of the policy sweep (`ns3_e1/e4_slack_occupancy.csv`)

Per-cycle instrumentation of the E4 gap-aware run (q_wc=25 cap 3, frame
PER 1e-3, 20 seeds × 120 cycles): each cycle is classified by the
deterministic plan finish F0 = max(15N + S_plan, 295) + 21N (raw channel
convention, same as the engine's miss test finish > T = 1395), where
S_plan is the SLAC work the credit/release state would play at PER = 0.
Occupancy of slack < 21 (one worst-case retx) is **0 cycles in every
cell**; the sweep's tightest state is the post-transition DC steady state
N = 38 with **plan slack 27** (= 1368 vs 1395). All 21 residual DC misses
(cells (15,35) 7, (30,10) 2, (30,20) 8, (30,35) 4) fall in slack < 42
cycles (occupancy 2,060/2,400 cycles = 85.8%, 78,280/88,480 EV-cycles):
r* = ⌊27/21⌋ + 1 = 2 stacked retransmissions, the same slack-quantized
mechanism as the fixed boundary cell (37,1) at q=7 (slack 14, r* = 1).
Instrumentation is counter-only (`--slackCsv=1`); the default output is
byte-identical (gated).

## Accounting convention for finish-time columns

All analytic and slot-machine finish values in this repository include the
carry-in envelope B_blk = 21; where a number is a raw channel finish
(no envelope), it is labeled as such. Per column:

| File / column | Convention |
|---|---|
| `layer1/admission_region.csv` — `finish`, `slack` | envelope-inclusive: finish = last frame end + B_blk (carry-in not realized); slack = T − finish |
| `layer1/knee_verification.csv` — `delta`, `delta_pred` | difference of two finishes; B_blk cancels |
| `ns3_e1/parity_knee.csv` — `finish_measured`, `finish_formula`, `finish_cycle_builder` | envelope-inclusive (all three, same convention — that is what parity compares) |
| `ns3_e1/overhead_knee.csv` — `finish_measured`, `finish_predicted` | envelope-inclusive; beacon head (c₀ + PRS) played as airtime per the IFS rules |
| `ns3_e1/loss_knee.csv` — `finish_med/p95/max` | envelope-inclusive (`lastEnd + B_blk`); `overrun_max` is a raw slot count |
| `ns3_e1/e5_adversarial.csv` — `max_response_end`, `last_frame_end` | carry-in **realized** as a played head block of B_blk slots; no trailing envelope added (same worst case expressed as airtime — (38,0) gives 1389 either way) |
| `ns3_e1/e3_static.csv` — `finish` | envelope-inclusive |
| Event-engine trajectory numbers (claim #10) | channel finish (no envelope) unless stated: 1368 channel = 1389 envelope-inclusive |

Labels only — no committed value was changed.

## Theorem 1 trajectory reobservation (per-cycle trace, claim #10 revision)

`ns3_e1/thm1_cell_trace.csv` (4,920 rows) is the per-cycle trace of the
`trackc-thm1` sweep (q_wc = 25, cap 3, offsets 0..40, PER = 0), produced
by `trackc-thm1 --cellCsv=1`. Columns: q, offset, cycle, N, K,
chan_finish (channel basis, no B_blk), finish_bblk (= chan_finish + 21),
slac_played (slots actually played), overrun_vs_qk, resp_start. The
instrumentation (`PolicyCycleTraceRow`, `EnableCycleTrace`) is dump-only:
the default aggregate output and RNG draw order are unchanged, and the
aggregate regression (max channel finish 1368 at (38,0), over_T = 0)
reproduces.

Per-cell maxima over the sweep:

| cell | max chan_finish | + B_blk | window occupancy (overrun vs qK) | resp_start |
|---|---|---|---|---|
| (37,1) | 1365 | 1386 | 33 (+8) | 588 |
| (38,0) | 1368 | 1389 | 0 | 570 |
| (36,2) | 1354 | 1375 | 58 (+8) | 606 |
| (36,1) | 1329 | 1350 | 33 (+8) | 573 |

Correction to the earlier claim-#10 record: the previously reported
"(37,1) finish 1383 with window occupancy 30" is not reproduced by this
trace. 1383 was the (36,2) maximum of the pre-cap trace (1375 under the
aggregate window cap below); 30 was a back-derivation of 1383 into
the (37,1) structure (1383 - 555 - 777 - 21 = 30) and does not occur in
the (37,1) occupancy distribution {0, 12, 18, 22, 24, 25, 33}. The
(37,1) maximum is 1386 (envelope-inclusive), occupancy 33, nine slots
below the packet-aware bound 1395 (occupancy bound 42). All trajectory
safety claims are unaffected (every finish <= T, over_T = 0).

## Realized-knee sweep under work-conserving replay

`ns3_e1/replay_knee_occupancy.csv` (deterministic, PER = 0, 1 seed,
1,000 cycles), `replay_knee_per1e3.csv` and `replay_knee_per1e2.csv`
(SLAC-only PER 1e-3 / 1e-2, 20 seeds, 1,000 cycles) are full-grid
`loss_sim` runs with two added columns: occ_med / occ_max, the per-cycle
SLAC occupancy (slots actually played, credit- and release-gated). The
instrumentation is additive: the pre-existing columns, seeds, and RNG
draw order are unchanged, and the regression cells reproduce ((18,1)
delta 0, (19,1) median delta 1, (38,0) finish 1389).

Realized knees (first N with delta = finish(N,K) - finish(N,0) > 0),
q = 7, K in {1, 4, 8, 16}:

| criterion | PER = 0 | 1e-3 | 1e-2 | prediction |
|---|---|---|---|---|
| median cycle | {19, 19, 16, 13} | same | same | later than both envelopes |
| any cycle (max; the paper's maximum-delay criterion) | {19, 17, 15, 12} | same | same | = qK + B_str cap {19, 17, 15, 12} |

Both criteria are loss-invariant at matched horizons; the admission
(envelope) knees remain {18, 17, 15, 11}. An earlier 200-cycle
deterministic probe showed K = 8 max-knee 16 — an undersampling
artifact (mode 0 fixes the horizon at 200 cycles); at 1,000 cycles the
occupancy realizes enough of the cap to move the cell.

(37,4) under replay (naive-bound counterexample state): occ_max = 43,
finish_max = 1396 > T = 1395 — the naive-admitted state produces a
realized deadline violation in the reclaiming tier as well (the full
28 + 17 = 45 occupancy and its 1398 finish remain unobserved). The
reserving tier finishes at 1402 exactly (`parity_dump`). D_g accounting
anchor: session clocks start at admittedCycle
(`ev-plc-policy-mac.cc`, release gating and the D_g check) — admission
waiting is a separate metric (never-admitted counted apart).

## Aggregate SLAC window cap (v4.14) — service discipline of the credit tier

All credit-tier engines (`e2_sim`, `e4_sim`, the event-driven
`EvPlcPolicyMac`) enforce an aggregate per-cycle SLAC window on top of the
unchanged per-session non-transferable credits: quota = qK (count-based;
B_fix for fixed reservations; sum of link-inflated q_i for link-aware),
carried debt `debt = max(0, debt + consumed - quota)`, a message may START
only while consumed < allowance = quota - debt (started messages are
non-preemptive and may straddle), and service resumes each cycle from the
first session blocked in the previous cycle (persistent pointer — no
starvation). Runners pass the cap explicitly (`capMode=2` /
`--aggCap=1`); mode 0 preserves the pre-cap path bit-exactly. Assertion
A1 (consumed <= ceil(quota) + 17) is active in every capped run and holds
with zero violations across all committed results.

Rationale: without the cap the credit tier realizes cycle occupancies
above the aggregate bound qK + B_str on synchronized K >= 3 states —
measured +24 slots at (35,3) and +32 at (34,4) (trackc-thm1
--sessions=3/4) — contradicting the single-window model. With the cap the
realized straddle over the full offset sweep is at most **10 slots**
((37,1) 8, (36,2) 8, (35,3) 7, (34,4) 10), strictly inside B_str = 17
(committed per-cycle traces: `ns3_e1/thm1_cell_trace.csv` for the (36,2)
start, `thm1_cell_trace_n35k3.csv` / `thm1_cell_trace_n34k4.csv` for the
binding cells; regenerate and gate via
`experiments/ns3_e1/run_thm1_trace.py`).
The replay tier (`loss_sim`) already used this aggregate-debt discipline
and is byte-identical before/after (regression-checked); the reserving
and analytic tiers are untouched (e5, knee, admission-region CSVs
byte-identical).
