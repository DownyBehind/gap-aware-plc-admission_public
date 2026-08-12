#!/usr/bin/env python3
"""Analytic decision check: gap-aware vs gap-oblivious Cond A.

Gap-oblivious variant replaces max(0, qk' + B_pkt - G(N)) with the plain
addend qk' + B_pkt in Cond A. Claim: at q_wc = 25 the admission DECISION
(min of Cond-A and Cond-B maxima) is identical for every N in 0..38,
because Cond B (N + k' <= 38) binds wherever G(N) > 0, and for N >= 20
G(N) = 0 makes the two forms equal. This script tabulates both maxima
and the joint decisions; any differing cell is a hard failure.
"""
import csv
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "results" / "gap_oblivious_check.csv"

C_REQ, C_RES, C_PROC = 15, 21, 280
B_PKT = B_BLK = 21
T = 1395
Q = 25


def g(n: int) -> int:
    return max(0, C_PROC - (n - 1) * C_REQ) if n >= 1 else C_PROC + C_REQ  # n=0: gap = full proc window after first (no requests) — use C_proc (no request); handled below


def cond_a_max_k(n: int, oblivious: bool) -> int:
    head = max(n * C_REQ, C_REQ + C_PROC)
    gap = max(0, C_PROC - (n - 1) * C_REQ) if n >= 1 else C_PROC
    best = -1
    for k in range(0, 60):
        demand = Q * k + B_PKT
        spill = demand if oblivious else max(0, demand - gap)
        lhs = head + spill + n * C_RES + B_BLK
        if lhs <= T:
            best = k
        else:
            break
    return best


def cond_b_max_k(n: int) -> int:
    best = -1
    for k in range(0, 60):
        m = n + k
        f = max(m * C_REQ, C_REQ + C_PROC) + m * C_RES + B_BLK
        if f <= T:
            best = k
        else:
            break
    return best


def main() -> None:
    rows, diff = [], 0
    for n in range(0, 39):
        a_aware = cond_a_max_k(n, oblivious=False)
        a_obl = cond_a_max_k(n, oblivious=True)
        b = cond_b_max_k(n)
        dec_aware = min(a_aware, b)
        dec_obl = min(a_obl, b)
        same = dec_aware == dec_obl
        diff += 0 if same else 1
        rows.append({
            "N": n, "condA_gap_aware_max_k": a_aware,
            "condA_gap_oblivious_max_k": a_obl, "condB_max_k": b,
            "decision_gap_aware": dec_aware, "decision_gap_oblivious": dec_obl,
            "identical": int(same),
        })
    OUT.parent.mkdir(parents=True, exist_ok=True)
    with OUT.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print(f"cells with differing decisions: {diff} / 39")
    if diff:
        for r in rows:
            if not r["identical"]:
                print("  DIFFER:", r)
    print(f"csv: {OUT}")


if __name__ == "__main__":
    main()
