#!/usr/bin/env python3
"""Seed-level statistics for the gap-aware residual DC miss.

Source: results/ns3_e1/e4_slack_occupancy.csv — the per-seed instrumented
output of the same acbs q_wc=25/cap 3 run whose aggregates are committed
in e4_policy_counts.csv (verified to sum identically here). No rerun.

For the worst cell (30,20) and runner-up (15,35): per-seed miss rates,
median and range, and a seed-level bootstrap 95% interval (10,000
resamples of the 20 seeds with replacement; statistic = pooled rate
sum(misses)/sum(EV-cycles); fixed RNG seed for determinism).
"""
import csv
import random
from pathlib import Path
from statistics import median

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "results" / "ns3_e1" / "e4_slack_occupancy.csv"
CHECK = ROOT / "results" / "ns3_e1" / "e4_policy_counts.csv"
OUT = ROOT / "results" / "seed_level_ci.csv"

CELLS = [(30, 20), (15, 35)]
B = 10000
rng = random.Random(41)  # fixed for determinism

rows = list(csv.DictReader(SRC.open()))
counts = {(r["policy"], r["N0"], r["K"]): r for r in csv.DictReader(CHECK.open())}

out = []
for n0, k in CELLS:
    seeds = [(int(r["dc_misses"]), int(r["dc_ev_cycles"])) for r in rows
             if int(r["N0"]) == n0 and int(r["K"]) == k]
    assert len(seeds) == 20, (n0, k, len(seeds))
    agg_m, agg_c = sum(m for m, _ in seeds), sum(c for _, c in seeds)
    ref = counts[("acbs_qwc25", str(n0), str(k))]
    assert (agg_m, agg_c) == (int(ref["dc_misses"]), int(ref["dc_ev_cycles"])), \
        f"({n0},{k}) aggregate mismatch vs e4_policy_counts"
    rates = [m / c for m, c in seeds]
    boots = []
    for _ in range(B):
        sample = [seeds[rng.randrange(20)] for _ in range(20)]
        boots.append(sum(m for m, _ in sample) / sum(c for _, c in sample))
    boots.sort()
    lo, hi = boots[int(0.025 * B)], boots[int(0.975 * B) - 1]
    out.append({
        "cell": f"({n0},{k})", "misses": agg_m, "ev_cycles": agg_c,
        "rate": f"{agg_m / agg_c:.3e}",
        "seed_rate_median": f"{median(rates):.3e}",
        "seed_rate_min": f"{min(rates):.3e}",
        "seed_rate_max": f"{max(rates):.3e}",
        "seeds_with_zero_miss": sum(1 for m, _ in seeds if m == 0),
        "bootstrap95_lo": f"{lo:.3e}", "bootstrap95_hi": f"{hi:.3e}",
        "resamples": B,
    })
    print(out[-1])

with OUT.open("w", newline="") as f:
    w = csv.DictWriter(f, fieldnames=list(out[0].keys()))
    w.writeheader()
    w.writerows(out)
print(f"csv: {OUT}")
