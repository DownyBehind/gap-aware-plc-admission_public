# Physics rules enforced by the simulator

Normative summary of the overhead/loss physics the code implements
(`ns3/contrib/ev-plc-transition/model/beacon-map-slot-machine.cc` and the
event-driven MAC). `experiments/ns3_e1/run_overhead.py` asserts all of it
with zero tolerance.

**IFS placement.** One IFS after a non-empty head block (beacon `c_0` +
PRS, contiguous); between consecutive DC requests (N−1); between the
request block and the next block (if N > 0); after the B_pkt guard
(if K > 0); between consecutive DC responses (N−1). No IFS inside the
SLAC budget block, and none before B_blk (an accounting envelope, not a
frame). PRS appears once per cycle, directly after the beacon.

**Adjusted finish and knee.** Channel term N·(C_req+IFS) + [K>0]·(q·K +
B_pkt + IFS) vs. readiness floor C_req + C_proc; effective gap
G̃(N) = max(0, C_proc − (N−1)·C_req − N·IFS), so the knee
min{N : q·K + B_pkt + IFS > G̃(N)} moves earlier as IFS grows:
IFS 0/1/2/3 → knees {18,17,15,11} / {17,16,14,11} / {16,15,13,10} /
{15,14,12,9} for K ∈ {1,4,8,16}. The beacon head cancels out of
delta(N,K) = F̃(N,K) − F̃(N,0).

**Credit and loss.** Each session accrues q slots of credit per cycle and
may start its next released message while its credit is positive; a
non-preemptive start may overdraw (measured overrun ≤ 17 = max message
18 − 1 < B_pkt = 21), and the debt is repaid from later accrual. A failed
frame still occupies the channel and re-queues against the remaining
budget; DC frames retransmit immediately. Loss runs use IFS = c_0 =
PRS = 0 to isolate effects. Gilbert–Elliott failure is integrated per
slot: P_fail = 1 − (1−p_g)^{L_g}·(1−p_b)^{L_b}; the i.i.d. mode
calibrates as p_frame = 1 − (1−p_slot)^L.

**RNG.** Exactly one roll per frame attempt (frame-error stream);
dedicated streams for G–E dwell and CSMA backoff. Deterministic (PER=0)
runs must be bit-identical between the event build and the serialized
reference.
