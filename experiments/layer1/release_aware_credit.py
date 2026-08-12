#!/usr/bin/env python3
"""Release-aware residual-demand credit requirement.

Theorem 2's aggregate-workload argument ignores message release times;
under MAC-contract property (v) unused credit is reclaimed within the
cycle and does not accumulate. This script computes, at every release
point i of the 20-message SLAC sequence,

    q_req(i) = ceil( C_i^rem / nu_i )

where C_i^rem is the remaining worst-case airtime from message i onward
and nu_i is the number of remaining map windows before the session
deadline D_g = 40T.

Window convention (stated explicitly): the deadline spans 40 cycles from
the start of the guarantee; the joining cycle is excluded (the session
joins at the next boundary and receives no service window for it),
leaving 39 service windows. Message i, released at r_i ms, can use the
windows whose start offset satisfies k*T >= r_i:
nu_i = 39 - floor(r_i / T_ms).

Sequence source: the 20-message table of the replay engines
(ns3/standalone/loss_sim.cc PaperSequence(), identical in
ns3/contrib/ev-plc-transition/model/ev-plc-policy-mac.cc), which
docs/model/slac_sequence_model.md designates as the sequence the replay
uses (sum 241 slots, releases 0..775 ms). Values are copied verbatim.

Remaining-demand accounting per setting:
  base   : C_i^rem = sum_{j>=i} c_j                      (lossless)
  n_r = 2: C_i^rem = (C_slac - sum_{j<i} c_j) + 2*sum_{j>=i} c_j
  n_r = 3: C_i^rem = (C_slac - sum_{j<i} c_j) + 3*sum_{j>=i} c_j
i.e., first transmissions are budgeted by the conservative envelope
C_slac = 247 (its remainder after the minimum consumption of earlier
messages) and each remaining message may be retransmitted n_r times at
its actual length. At i = 1 this reproduces the aggregate envelopes
C_wc = 247 + n_r * 241 (729 and 970).
"""
import csv
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "results" / "release_aware_credit.csv"

T_MS = 50
D_G_CYCLES = 40
WINDOWS = D_G_CYCLES - 1  # joining cycle excluded
C_SLAC = 247

# (slots, release_ms, name) — verbatim from the replay table.
SEQ = [
    (11, 0, "SLAC_PARM.REQ"),
    (11, 15, "SLAC_PARM.CNF"),
    (11, 35, "START_ATTEN_CHAR.IND.1"),
    (11, 55, "START_ATTEN_CHAR.IND.2"),
    (11, 75, "START_ATTEN_CHAR.IND.3"),
    (11, 95, "START_ATTEN_CHAR.RSP"),
] + [(12, 105 + 40 * i, f"MNBC_SOUND.IND.{i+1}") for i in range(10)] + [
    (18, 535, "ATTEN_CHAR.IND"),
    (12, 635, "ATTEN_CHAR.RSP"),
    (12, 755, "SLAC_MATCH.REQ"),
    (13, 775, "SLAC_MATCH.CNF"),
]

TOTAL = sum(c for c, _, _ in SEQ)
assert TOTAL == 241, TOTAL
assert len(SEQ) == 20

SETTINGS = [("base", 0), ("nr2", 2), ("nr3", 3)]


def main() -> None:
    rows = []
    results = {}
    for label, nr in SETTINGS:
        best = (0, None)
        consumed = 0
        for i, (c, r, name) in enumerate(SEQ, start=1):
            remaining_actual = TOTAL - consumed
            if nr == 0:
                c_rem = remaining_actual
            else:
                c_rem = (C_SLAC - consumed) + nr * remaining_actual
            nu = WINDOWS - r // T_MS
            q_req = math.ceil(c_rem / nu)
            rows.append({
                "setting": label, "i": i, "message": name, "c_i": c,
                "r_ms": r, "c_rem": c_rem, "nu": nu, "q_req": q_req,
            })
            if q_req > best[0]:
                best = (q_req, i)
            consumed += c
        results[label] = best
    OUT.parent.mkdir(parents=True, exist_ok=True)
    with OUT.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    for label, nr in SETTINGS:
        q, i = results[label]
        c, r, name = SEQ[i - 1]
        print(f"{label} (n_r={nr}): q_req = {q} at release point i={i} "
              f"({name}, r={r} ms)")
    # side checks
    print("--- side checks ---")
    for nr, exp in ((2, 19), (3, 25)):
        v = math.ceil((C_SLAC + nr * TOTAL) / WINDOWS)
        print(f"ceil((247 + {nr}*241)/39) = {v} (expected {exp})")
    for tot, q in ((729, 19), (970, 25), (247, 7)):
        print(f"ceil({tot}/{q}) + 1 = {math.ceil(tot / q) + 1}")
    print(f"csv: {OUT}")


if __name__ == "__main__":
    main()
