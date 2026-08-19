#!/usr/bin/env python3
"""Credit-rule sanity checks (report-only; touches no committed artefact).

Checks three facts used by the provisioning-budget sensitivity note:

  1. Empirical: with q = 6 (cap 0, loss-free, simultaneous cohorts,
     thesis 39-window timeline) the exhaustive K = 1..38 sweep's
     per-session completion maximum and the set of K violating
     D_g = 40 cycles. Uses theorem2_adjudication.run() unmodified.

  2. The C_slac interval on which the credit triple stays (7, 19, 25),
     with the credit rule q(C) = min{q : ceil(C/q) <= 39} (the least
     per-cycle credit whose provisioned airtime fits in the 39 service
     windows before D_g) applied jointly to C_slac, C_slac + 2*230 and
     C_slac + 3*230.

  3. The least C_slac for which the loss-blind credit is >= 7.

  4. Identity: min{q : ceil(C/q) <= 39} == ceil(C/39) for C = 1..4000.
"""
import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from theorem2_adjudication import run, NMSG

SUM_L = 230
DG = 40


def credit(c: int) -> int:
    q = 1
    while math.ceil(c / q) > 39:
        q += 1
    return q


def main() -> None:
    # 1. q = 6 empirical sweep (thesis timeline)
    mx, viol_k = 0, []
    for k in range(1, 39):
        res = run(6, 0, [[0] * NMSG for _ in range(k)], [0] * k,
                  'model', horizon=90)
        elapsed = [cyc for _, cyc, ok in res if ok]
        assert len(elapsed) == k
        m = max(elapsed)
        mx = max(mx, m)
        if m > DG:
            viol_k.append(k)
    print(f"1) q=6 loss-free exhaustive: max {mx} cycles; "
          f"D_g-violating K set: {viol_k[0]}..{viol_k[-1]} "
          f"({len(viol_k)} of 38)" if viol_k else f"max {mx}, no violation")

    # 2/3. credit-preserving interval under the corrected rule
    lo = hi = None
    for c in range(200, 320):
        ok = (credit(c) == 7 and credit(c + 2 * SUM_L) == 19
              and credit(c + 3 * SUM_L) == 25)
        if ok and lo is None:
            lo = c
        if ok:
            hi = c
    min_q7 = next(c for c in range(200, 320) if credit(c) >= 7)
    print(f"2) triple-(7,19,25) C_slac interval: [{lo}, {hi}]")
    print(f"3) least C_slac with q >= 7: {min_q7}")
    # 4. identity with ceil(C/39)
    mism = [c for c in range(1, 4001) if credit(c) != math.ceil(c / 39)]
    print(f"4) identity min-rule == ceil(C/39) on C=1..4000: "
          f"{'holds (0 mismatches)' if not mism else f'FAILS at {mism[:5]}'}")


if __name__ == '__main__':
    main()
