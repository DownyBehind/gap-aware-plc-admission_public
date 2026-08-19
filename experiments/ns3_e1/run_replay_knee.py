#!/usr/bin/env python3
"""Realized (replay) knee sweep — work-conserving replay occupancy vs. gap.

Runs the standalone `loss_sim` replay engine in mode 2 (full PER with
per-cycle SLAC occupancy columns) over the N = 1..40 x K in
{0, 1, 4, 8, 16} grid at three SLAC frame-error rates, then extracts the
realized knees.

Invocations (exactly the parameters that produced the committed
artefacts; verified by byte-identity against the superseded 20-message
CSVs before the sequence correction):

  loss_sim 2     0 0  1 1000   -> results/ns3_e1/replay_knee_occupancy.csv
  loss_sim 2  1000 0 20 1000   -> results/ns3_e1/replay_knee_per1e3.csv
  loss_sim 2 10000 0 20 1000   -> results/ns3_e1/replay_knee_per1e2.csv

Knee criterion (the convention of the paper's realized-knee figures):
for each K in {1, 4, 8, 16}, the realized knee is the smallest N whose
realized SLAC occupancy exceeds the processing gap available to that
population,

    knee(K) = min { N : occ(N, K) > G'(N) },
    G'(N)   = max(0, C_proc - (N - 1) * C_req) = max(0, 280 - 15 (N-1)),

with occ taken per criterion: `max` uses the occ_max column (worst
per-cycle realized occupancy over all seeds), `median` uses occ_med.
On the superseded 20-message sequence this reproduces the published
sets: max {19, 17, 15, 12}, median {19, 19, 16, 13}.

Outputs:
  results/ns3_e1/replay_knee_occupancy.csv   (PER = 0)
  results/ns3_e1/replay_knee_per1e3.csv      (PER = 1e-3)
  results/ns3_e1/replay_knee_per1e2.csv      (PER = 1e-2)
  results/ns3_e1/replay_knee_summary.csv     (knees per criterion/PER)
"""
import csv
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BINARY = ROOT / "ns3" / "standalone" / "build" / "loss_sim"
BUILD = ROOT / "ns3" / "standalone" / "build.sh"
OUT_DIR = ROOT / "results" / "ns3_e1"

RUNS = [
    ("replay_knee_occupancy.csv", 0, 1, 1000),
    ("replay_knee_per1e3.csv", 1000, 20, 1000),
    ("replay_knee_per1e2.csv", 10000, 20, 1000),
]
K_SET = (1, 4, 8, 16)
C_PROC = 280
C_REQ = 15


def gap(n: int) -> int:
    return max(0, C_PROC - (n - 1) * C_REQ)


def knees(rows: list[dict], crit: str) -> dict[int, int | None]:
    out: dict[int, int | None] = {}
    for k in K_SET:
        cand = [int(r["N"]) for r in rows
                if int(r["K"]) == k and int(r[crit]) > gap(int(r["N"]))]
        out[k] = min(cand) if cand else None
    return out


def main() -> None:
    if not BINARY.exists():
        subprocess.run([str(BUILD)], check=True, capture_output=True)
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    summary = []
    for name, per_ppm, seeds, cycles in RUNS:
        out = subprocess.run(
            [str(BINARY), "2", str(per_ppm), "0", str(seeds), str(cycles)],
            check=True, capture_output=True, text=True).stdout
        path = OUT_DIR / name
        path.write_text(out)
        rows = list(csv.DictReader(out.splitlines()))
        for crit, label in (("occ_max", "max"), ("occ_med", "median")):
            kn = knees(rows, crit)
            summary.append({
                "csv": name, "per_slac": per_ppm / 1e6, "criterion": label,
                **{f"knee_K{k}": kn[k] for k in K_SET},
            })
        print(f"{name}: max {list(knees(rows, 'occ_max').values())} "
              f"median {list(knees(rows, 'occ_med').values())}")
    with (OUT_DIR / "replay_knee_summary.csv").open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(summary[0]))
        w.writeheader()
        w.writerows(summary)
    print(f"csv: {OUT_DIR}/replay_knee_summary.csv")


if __name__ == "__main__":
    main()
