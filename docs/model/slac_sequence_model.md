# SLAC Sequence Model

The paper's SLAC model is the 20-message sequence (241 slots, releases 0..775 ms; see docs/PARAMETER_PROVENANCE.md) provisioned by the session budget `C_slac = 247`. A legacy abstract expansion (PARM request/confirm, START_ATTEN_CHAR, ten MNBC_SOUND indications, ATTEN_CHAR, and MATCH request/confirm; 196 slots) predates it and is kept for regression tests only.

## Message-replay source (`loss_sim`)

The loss-physics replay (`ns3/standalone/loss_sim.cc`) uses the **full 20-message SLAC
sequence** (release times 0..775 ms) rather than the 196-slot abstract sequence above. Two notes
on the sequence sources:

1. `SlacSequenceModel::DefaultSequence()` (C++) totals 196 slots and lacks
   release times — it predates the 20-message table; the replay uses the table.
2. The 20-message sequence sums to **241 slots**
   (11·6 + 12·10 + 18 + 12 + 12 + 13), while the analysis uses the
   provisioned per-session budget `C_slac = 247 ≥ 241`; the budget q=7
   is derived from the 247 envelope, so the replayed demand is strictly
   inside the analyzed envelope.
