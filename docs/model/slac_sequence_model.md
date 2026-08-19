# SLAC Sequence Model

The canonical SLAC model is the **19-message sequence (230 slots,
releases 0..775 ms**; see docs/PARAMETER_PROVENANCE.md) provisioned by
the session budget `C_slac = 247`. The canonical sequence lives in
`experiments/analysis/theorem2_adjudication.py` (`SEQ`) and is what the
Theorem 2 adjudication, the staggered search, and the release-aware
credit check consume. A legacy abstract expansion (PARM
request/confirm, START_ATTEN_CHAR, ten MNBC_SOUND indications,
ATTEN_CHAR, and MATCH request/confirm; 196 slots) predates it and is
kept for regression tests only.

## Correction: 20 messages → 19 messages

Earlier versions of this model carried a 20-message / 241-slot
sequence containing a `START_ATTEN_CHAR.RSP` message (11 slots,
release 95 ms). Direct comparison against **ISO 15118-3:2015 Annex A**
shows the standard does not define that message:
`CM_START_ATTEN_CHAR.IND` is broadcast three times with **no response
message** (Table A.1 `C_EV_start_atten_char_inds` = 3, the Table A.4
MME set, and the Figure A.1 sequence chart). The corrected
medium-occupying sequence is:

SLAC_PARM.REQ/CNF (2) + START_ATTEN_CHAR.IND ×3 + MNBC_SOUND.IND ×10 +
ATTEN_CHAR.IND/RSP (2) + SLAC_MATCH.REQ/CNF (2) = **19 messages**,
`sum l_i = 5·11 + 10·12 + 18 + 12 + 12 + 13 = 230 slots`.

The two constants play different roles. `Σℓᵢ = 230` is the
**sequence demand** — what the replay tiers actually transmit.
`C_slac = 247` is the **per-session provisioning budget of the
coordinated scheduler** — the amount the reserving lineage and the ns-3
session model (`slac-session.cc`) reserve and serve per session. It is
a scheduler design parameter (ARCH), not a protocol-derived size. The
credits follow the rule `q(C) = min{q : ceil(C/q) <= 39} = ceil(C/39)`;
every budget in [247, 273] yields the same credits (7, 19, 25), and
below 235 the loss-blind credit falls to q = 6, under which the
loss-free completion guarantee fails (45-cycle maximum; see
docs/PARAMETER_PROVENANCE.md and
`experiments/analysis/credit_rule_check.py`).

## Three lineages, not two

The paper's shorthand "bounds use the envelope, replays use the message
table" is incomplete; the artefacts fall into **three** lineages:

1. **Message-table replay (Σℓᵢ = 230)** — the tiers that transmit the
   actual 19-message sequence: the per-link comparison
   (`perlink_*_per_seed.csv`, `g3b_link_aware.csv`), the burst
   provisioning check (`g3a_qwc_burst.csv`), the full policy comparison
   including CSMA (`e4_*`), the loss sweep (`e2_admission_variants.csv`),
   the loss/replay knees (`loss_knee.csv`, `replay_knee_*.csv`), the
   trajectory traces (`thm1_cell_trace*.csv`), and the Python
   adjudication (`theorem2_*.csv`, `release_aware_credit.csv`).
   Retransmissions in this lineage re-consume the actual message
   airtime (`ev-plc-policy-mac.cc`, retry path).
2. **Budget consumption (C_slac = 247, `SlacSession`)** — the tiers
   that serve an opaque per-session budget rather than messages:
   the burst-sensitivity grid of Table II (`g3i_iid_baseline.csv`,
   `g3c_burst_sensitivity.csv`, `g3_burst_vs_iid.csv`) and the parity
   sweep (`parity_knee.csv`); at the tool layer also the
   fixed-reservation baseline and `transition-admission-controller`.
3. **Reservation arithmetic (window = q·K)** — the slot-machine tiers
   that reserve the aggregate window without per-session playback:
   the admission-knee grids of Table I (`knee_verification.csv`,
   `overhead_knee.csv`) and the boundary sweep (`e5_adversarial.csv`).

This mapping also explains why the Table II and parity artefacts were
bit-identical across the 19-message sequence correction: lineage (2)
never consumes the message table.

## Message-replay source (`loss_sim`)

The ns-3 tiers (`ns3/standalone/loss_sim.cc`,
`ns3/contrib/ev-plc-transition`) play the **corrected 19-message /
230-slot sequence** — the same canonical table as the Python analysis
path. The slot-exact cross-check between the Python discipline port
and `e2_sim` capMode 2 holds on the corrected sequence (completion
indices 31/33/34/34 at K = 1/4/8/16, q = 7, PER = 0); three-tier
slot-exact agreement holds on the corrected profile. Additional notes:

1. `SlacSequenceModel::LegacySequence()` (C++) totals 196 slots and
   lacks release times — it predates the message table and is kept for
   regression tests only; the replay uses the canonical table.
2. The replayed demand (230 slots) is strictly inside the analyzed
   `C_slac = 247` envelope, so replay-based observations remain
   within-envelope.
