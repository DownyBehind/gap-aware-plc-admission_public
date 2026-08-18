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

`C_slac = 247` is retained as the provisioning constant. Its reserve
over the message sum grows from 6 to **17 slots**. Because `C_slac` is
an upper envelope, all envelope-based results are unchanged; note,
however, that the reserve is now **load-bearing** for the published
credits: replacing the budget by the bare sum 230 would give
`q = ceil(230/39) = 6` and `q_wc = 18 / 24` instead of the published
7 / 19 / 25 (see docs/PARAMETER_PROVENANCE.md).

## Message-replay source (`loss_sim`) — divergence note

The ns-3 tiers (`ns3/standalone/loss_sim.cc`,
`ns3/contrib/ev-plc-transition`) **retain the superseded 20-message /
241-slot sequence.** They are not on the Theorem 2 adjudication path,
which is served entirely by the Python discipline port. The slot-exact
cross-check between the port and `e2_sim` capMode 2 (completion
indices 32/34/36/37 at K = 1/4/8/16) was established on the 20-message
sequence and does not hold across the corrected port (19-message
indices: 31/33/34/34). Re-running the ns-3 tiers on the corrected
sequence is deferred. Additional notes:

1. `SlacSequenceModel::DefaultSequence()` (C++) totals 196 slots and
   lacks release times — it predates the message table; the replay
   uses the (20-message) table.
2. The replayed 20-message demand (241 slots) is strictly inside the
   analyzed `C_slac = 247` envelope, so replay-based observations
   remain within-envelope; the corrected 19-message demand (230
   slots) is smaller still.
