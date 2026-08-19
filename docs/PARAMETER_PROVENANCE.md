# Parameter Provenance

This document defines the evaluated reference profile used by the paper and
records the provenance of its numerical inputs.

Each value is classified as one of:
**standard**, **derived**, **measured**, **assumption**,
**architecture parameter (ARCH)** — a design parameter of the
coordinated scheduler itself, in the same category as q, B_pkt and
D_g —, **stress parameter**, or **experiment setting**.

The analytical guarantees in the paper are exact for the stated slot-level
reference profile and MAC assumptions. Values classified as assumptions are
not claimed to be mandated by ISO 15118 or HomePlug Green PHY. Full PHY/EXI/IFS
calibration for a specific charging site is outside the scope of the current
paper.

Reproduction targets for every reported result are indexed in
[`results/RESULTS.md`](../results/RESULTS.md).

## Evaluated reference profile

| Parameter | Value | Role | Status |
|---|---:|---|---|
| Δ | 35.84 µs | slot duration | standard |
| T | 1395 slots (~50 ms) | control period | assumption |
| C_req | 15 slots | DC request airtime | reference-profile assumption |
| C_proc | 280 slots (~10 ms) | response-ready processing latency | assumption |
| C_res | 21 slots | DC response airtime | reference-profile assumption |
| D_g | 40T (~2 s) | admitted-session deadline | design assumption |
| Σℓ_i | 230 slots | sum of the 19-message table (Appendix A of the paper); the demand the replay tiers play | derived from reference sequence |
| C_slac | 247 slots | per-session provisioning budget of the coordinated scheduler (reserved and served per session by the reserving lineage and the ns-3 session model) | architecture parameter (ARCH) |
| B_blk | 21 slots | cycle-head non-preemptive carry-in bound | derived from reference profile |
| B_pkt | 21 slots | in-map non-preemptive packet guard | derived from reference profile |
| B_str | 17 slots | SLAC-only straddling bound | derived |

## 1. Timing primitives

| Symbol | Value | Type | Source | Derivation |
|---|---|---|---|---|
| Δ (slot) | 35.84 µs | standard | HomePlug Green PHY Spec v1.1.1, Sec. 3.6.5 / Fig. 3-22 ("Slot Time") | — |
| T (control period) | 1395 slots | assumption | evaluated reference profile | ⌊50 ms / Δ⌋ = 1395 |
| C_proc | 280 slots | assumption | evaluated reference profile | ⌈10 ms / Δ⌉ = 280 |
| D_g | 40T | design assumption | session deadline | ≈ 2000 ms |

Δ is the priority-resolution slot time, unchanged across HomePlug
generations.

**T** is selected as the control period of the evaluated reference profile;
it is not mandated by ISO 15118. The value is consistent in scale with prior
HPGP use of a 50-ms beacon period on a DC line (Gehrsitz et al., IEEE
VTC2014-Fall) and remains within the relevant ISO 15118 message timing
budget (ISO 15118-2:2014 Sec. 8.8 Table 109,
V2G_EVCC_Msg_Timeout(CurrentDemandReq) = 250 ms).

**C_proc** is selected as a bounded per-EV response-ready latency in the
evaluated profile. It is not a specification constant. The 10-ms value lies
within the ISO 15118-2 CurrentDemandRes response-performance budget used as
a consistency check (ISO 15118-2:2014 Table 109,
V2G_SECC_Msg_Performance_Time(CurrentDemandRes) = 25 ms). Processing is
modeled as off-channel and concurrent across EVs. A dedicated C_proc
sensitivity sweep is not included; the N sweep exercises G(N) from 280 down
to 0.

**D_g** runs from admission; pre-admission wait is reported separately.

## 2. Frame airtimes of the reference profile

The paper evaluates fixed slot-level airtimes
`C_req = 15` and `C_res = 21`.
They are reference-profile inputs, not claimed as uniquely determined HPGP PHY
airtimes.

A provisional byte-to-airtime conversion used during model construction is
recorded below for transparency, but the paper's guarantees apply to the
resulting slot values rather than to this provisional PHY conversion.

### Provisional construction record

```
airtime = frame bytes × 8 / 10 Mbps + 350 µs overhead
header  = Eth 14 + IPv6 40 + TCP 20 + V2GTP 8 = 82 B
C_req   : 82 + EXI 150 = 232 B → 185.6 + 350 = 535.6 µs → 15 slots
C_res   : 82 + EXI 380 = 462 B → 369.6 + 350 = 719.6 µs → 21 slots
```

### Calibration limits

- The EXI payload sizes 150 B and 380 B are model assumptions; no measured
  encoding-size trace is claimed.
- The 10-Mbps rate is a rounded construction value. Using the exact
  HPGP HS-ROBO_AV rate of 9.8452 Mbps (HPGP Spec Table 3-13) can move the
  request across a slot boundary: it gives C_req = 16 (538.5 µs, 0.9 µs past
  the 15-slot boundary; the current value sits on a 2.0 µs margin), while
  C_res stays 21. Numerical knees and capacities should therefore be
  interpreted as properties of the stated reference slot profile.
- The 350-µs fixed frame overhead is a construction assumption rather than a
  claim of one unique HPGP interframe-space convention (HomePlug 1.0/AV
  values give ≈369.4 µs; IEEE 1901 defaults give ≈547.6 µs).
- The paper does not pin a complete HPGP PHY mode / coding / IFS stack
  (under STD-ROBO_AV, 4.9226 Mbps, C_req becomes 21).
- Site-specific PHY/EXI calibration is future work.

## 3. SLAC reference sequence and session budget

The paper uses a 19-message SLAC reference sequence with modeled airtimes
of 11–18 slots and total modeled demand of 230 slots. The sequence structure
and release ordering follow ISO 15118-3; the individual slot airtimes are
reference-profile inputs rather than measured PHY airtimes.

| Symbol | Value | Type | Source | Derivation |
|---|---|---|---|---|
| ℓ_i (19 messages) | 11–18 slots | reference-profile constants | `experiments/analysis/theorem2_adjudication.py` (`SEQ`, the canonical sequence table); message set fixed by direct comparison against ISO 15118-3:2015 Annex A (Table A.1 `C_EV_start_atten_char_inds` = 3, Table A.4 MME set, Figure A.1) | fixed model constants; not claimed as measured PHY airtimes |
| Σ ℓ_i | 230 slots | derived | — | 5×11 + 10×12 + 18 + 12 + 12 + 13 |
| C_slac | 247 slots | architecture parameter (ARCH) | scheduler design parameter — see note | per-session provisioning budget reserved and served by the coordinated scheduler (`ns3/contrib/ev-plc-transition/model/slac-session.cc`); not a protocol-derived size, not standards-derived |
| sequence structure (19 msgs) | — | standard | ISO 15118-3 Annex A.9 (matching process) | — |
| release schedule (0–775 ms) | — | model constants consistent with ISO 15118-3 Annex A Table A.1 timers | see release note | — |
| B_str | 17 slots | derived | — | max ℓ_i − 1 = 18 − 1 |

C_slac note: `C_slac = 247` is the per-session provisioning budget of
the coordinated scheduler — the amount the reserving lineage and the
ns-3 session model actually reserve and serve per session — not a claim
about ISO message sizes. The budget is load-bearing but its admissible
range is bounded. The
credit rule is q = min{q : ceil(C/q) <= 39}, the least per-cycle credit
whose provisioned airtime fits in the 39 service windows a session has
before D_g; this is exactly ceil(C/39). Every budget in [247, 273]
yields the same credits (7, 19, 25). Below 235 the loss-blind credit
falls to q = 6, under which exhaustive execution of the loss-free
cohorts reaches 45 cycles and violates D_g for every K >= 4; the
completion guarantee therefore requires q >= 7. Above 273 the loss-blind
credit rises to q = 8 and moves the admission boundary. What is not
derived is the position of 247 within [247, 273]
(verification: `experiments/analysis/credit_rule_check.py`). In the
evaluated grid, realized demand did not exceed the provisioned budget.

ℓ_i note: `CM_ATTEN_CHAR.IND` has a variable-length attenuation-profile
payload in the specification. The 18-slot value is therefore part of the
evaluated reference profile rather than a claimed universal protocol
maximum.

Release map (canonical table in
`experiments/analysis/theorem2_adjudication.py` `SEQ`; Theorem 2's
release-aware check iterates these 19 release points directly. The
former row `START_ATTEN_CHAR.RSP` (11 slots, 95 ms) was removed by the
ISO 15118-3 Annex A comparison — the standard defines no such
message):

| i | message | ℓ_i (slots) | r_i (ms) |
|---|---|---|---|
| 1 | SLAC_PARM.REQ | 11 | 0 |
| 2 | SLAC_PARM.CNF | 11 | 15 |
| 3–5 | START_ATTEN_CHAR.IND ×3 | 11 | 35 / 55 / 75 |
| 6–15 | MNBC_SOUND.IND ×10 | 12 | 105..465 (step 40) |
| 16 | ATTEN_CHAR.IND | 18 | 535 |
| 17 | ATTEN_CHAR.RSP | 12 | 635 |
| 18 | SLAC_MATCH.REQ | 12 | 755 |
| 19 | SLAC_MATCH.CNF | 13 | 775 |

Release note: the implemented release instants are model constants chosen
to be consistent with the ISO 15118-3 Table A.1 timing structure. A
one-to-one derivation from each timer bound is not claimed.

## 4. Non-preemption envelopes

| Symbol | Value | Type | Derivation | Notes |
|---|---|---|---|---|
| B_blk | 21 slots | derived | max(C_req, C_res, max ℓ_i) = max(15, 21, 18) | equality with C_res is not a coincidence: the max is attained at C_res |
| B_pkt | 21 slots | derived | same (medium-wide guard) | trajectory analysis uses the SLAC-only B_str = 17 |

B_pkt note: B_pkt is applied to the SLAC setup window only
(charged when K > 0); its value is the medium-wide non-preemptive
frame bound (21 slots), not the tight SLAC straddling bound. Lemma 1
uses the tighter B_str = 17 because only SLAC frames straddle an
aggregate window. The two differ by design: B_pkt is conservative in
value, B_str is tight in scope.

## 5. Credit parameters

| Symbol | Value | Type | Derivation |
|---|---|---|---|
| q | 7 | derived | min{q : ⌈C_slac/q⌉ ≤ 39} = ⌈C_slac/39⌉ — least credit whose provisioned airtime fits in the 39 service windows before D_g; q = 6 would need 42 windows |
| 39 (service windows) | — | derived | ⌈D_g/T⌉ − 1 = 40 − 1 (the joining cycle carries no credit) |
| ε | 10⁻⁶ | design target | target retry-cap-exceedance probability under the independent-attempt union-bound model |
| n_r (retry cap) | 2 / 3 | derived | least integer with 19·p^(n_r+1) ≤ ε at p = 10⁻³ / 10⁻² |
| C_wc | 707 / 937 | derived | C_slac + n_r · Σℓ_i |
| q_wc | 19 / 25 | derived | min{q : ⌈C_wc/q⌉ ≤ 39} = ⌈C_wc/39⌉ — 19 at cap 2 (18 would need 40 windows), 25 at cap 3 (24 would need 40) |

## 6. Channel / loss parameters

The Gilbert–Elliott grid is an equal-mean stress sweep, not a calibrated
statistical model of one charging site.

| Symbol | Value | Type | Source |
|---|---|---|---|
| mean slot error rate | 6.7×10⁻⁵ | derived | `ns3/contrib/ev-plc-transition/examples/trackc-g3.cc:28` — frame error rate 10⁻³ over 15-slot frames |
| G-E bad-sojourn grid | {3, 15, 60} slots | literature-scaled stress points | `experiments/ns3_e1/run_trackc_c4.py:222` |
| G-E bad-sojourn extension | 300 slots | extended stress point | same sweep; probes the q_wc absorption margin |
| G-E bad-state error prob. | {0.05, 0.2, 0.5} | stress parameter | `experiments/ns3_e1/run_trackc_c4.py:223` |
| link classes | GOOD 10⁻⁶ / SEVERE 10⁻² (unit: per-slot Bernoulli) | experiment setting | heterogeneous-link run (`trackc-g3 --mode=b`) |

The 3–60-slot bad-state range is scaled to the impulse-duration range
reported in PLC noise literature (Zimmermann & Dostert, IEEE Trans. EMC
2002: impulse widths from microseconds to a few milliseconds, inter-arrival
times milliseconds to seconds — motivating bad-state sojourns of 1–60 slots,
0.036–2.15 ms, and good-state sojourns of 10 ms–1 s; no point estimates are
claimed). The 300-slot (10.75 ms) point intentionally extends beyond that
range to probe the scheduler's residual-slack absorption limit. The
bad-state error probabilities are sweep parameters; the literature does not
supply values of that kind. Three of the four burst values reported in the
paper's Table II — the 81.5× ratio at (36,1), 24 at (30,4), and 2 at (10,4)
— come from the 300-slot extension point. Restricted to the
literature-scaled range (bad sojourn ≤ 60 slots), the same data give:

| (N,K) | i.i.d. | bursty worst (all 12 points) | bursty worst (sojourn ≤ 60) |
|---|---|---|---|
| (37,1) | 814/10,000 | 779/10,000 (0.96) | 779/10,000 (0.96, unchanged) |
| (36,1) | 2/10,000 | 163/10,000 (81.5×) | 103/10,000 (51.5×) |
| (30,4) | 0/10,000 | 24/10,000 | 1/10,000 |
| (10,4) | 0/10,000 | 2/10,000 | 0/10,000 |

Source: `results/ns3_e1/g3_burst_vs_iid.csv` (per-point rows).

The qualitative conclusion does not depend on the 300-slot extension:
within the literature-scaled range, bursty loss still produces a 51.5×
amplification at (36,1). The 81.5× value reported by the paper is the
maximum over the full 12-point stress grid.

G-E construction (as implemented in
`ns3/contrib/ev-plc-transition/examples/trackc-g3.cc:28-42` and
`ns3/contrib/ev-plc-transition/model/plc-shared-channel.cc`):

```
target       = 6.7e-5                 # marginal slot error rate (held)
duty         = target / per_bad       # bad-state fraction
pi_bg        = 1 / bad_sojourn        # bad->good, per slot
good_sojourn = bad_sojourn * (1/duty - 1)
pi_gb        = 1 / good_sojourn       # good->bad, per slot
PER_good     = 0
PER_bad      = per_bad
initial state = GOOD
sojourns      : geometric, s = 1 + floor(ln u / ln(1 - p_leave))
frame failure : P_fail = 1 - (1-PER_good)^L_good * (1-PER_bad)^L_bad
```

The equal-mean property is an identity, not an approximation:
stationary mean = duty · PER_bad + (1 − duty) · 0 = target, exactly,
at all 12 points.

Because the marginal slot error rate is held fixed by construction, some
stress-grid points imply long GOOD-state sojourns. These points are
retained for controlled equal-mean comparison, not as site-calibrated
point estimates. Implied good-state sojourns (physical time) across the
grid:

| per_bad \ bad sojourn | 3 (107 µs) | 15 (538 µs) | 60 (2.15 ms) | 300 (10.75 ms) |
|---|---|---|---|---|
| 0.05 | 80 ms | 401 ms | 1.60 s | 8.0 s |
| 0.2 | 321 ms | 1.60 s | 6.4 s | 32 s |
| 0.5 | 802 ms | 4.01 s | 16 s | 80 s |

6.7e-5 calibration:
linear approximation 1e-3 / 15 = 6.667e-5; exact for a 15-slot frame
1 − (1−p)^15 = 1e-3 → p = 6.6698e-5; adopted 6.7e-5 (0.45% above
exact; conservative direction). Frames are 15, 18, or 21 slots; the
calibration uses the 15-slot DC request as the reference length and
applies one common slot error process to all frames.

Two probability domains are used intentionally:

- Sec. III-C provisioning uses per-attempt **frame** error probability p
  (10⁻³, 10⁻²) — one Bernoulli draw per message attempt.
- Sec. V-C/V-D channel experiments use per-**slot** error probabilities
  (6.7×10⁻⁵; GOOD 10⁻⁶ / SEVERE 10⁻²) and compose frame failure over
  frame length as 1 − (1 − p_slot)^L.

The 6.7e-5 slot value is chosen so a 15-slot request has approximately
1e-3 frame error probability, providing a common comparison scale.

## 7. Experiment settings

| Setting | Value | Where |
|---|---|---|
| seeds | 20 | all runners (`--seeds=20`); `results/RESULTS.md` |
| horizon | 120 cycles | policy comparison (`--horizon=120`) |
| burst-cell cycles | 10,000 = 20 seeds × 500 | `results/RESULTS.md` (accounting note) |
| admitted sessions per loss row | 1,440 | `results/RESULTS.md` entry 9 |
| G-E sweep session denominator | 1,920 | reported denominator (0/1920) |
| knee grid | 600 cells | `experiments/ns3_e1/run_overhead.py` |
| admitted region | 588 states (output, not input) | `results/layer1/admission_region.csv` |
| cycle-head sweep | 12-slot head | head sweep runner |
| IFS sweep | {0, 1, 2, 3} slots | `experiments/ns3_e1/run_overhead.py` |
| K sweep | {1, 2, 5, 10, 20, 35} | `experiments/ns3_e1/run_trackc_c4.py:33` |
| N₀ sweep | {0, 15, 30} | policy comparison |
