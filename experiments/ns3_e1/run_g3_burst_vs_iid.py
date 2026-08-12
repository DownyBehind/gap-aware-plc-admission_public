#!/usr/bin/env python3
"""G3 burst-vs-iid full-contrast table.

Runs trackc-g3 --mode=i and --mode=c on the SAME cell set, same marginal
slot-PER (6.7e-5), same accounting (EvPlcMac cycle replay, miss :=
finishSlot > T=1395), same seed list (1..20), and emits one row per
(cell, bad_sojourn, per_bad) with the cell's i.i.d. baseline joined in and
ratio = (miss_burst/total_burst) / (miss_iid/total_iid).

Cell set covers the envelope-slack spectrum, at least three cells per band:
  boundary   slack < 21 slots  (< 1 worst-case retransmission, B_pkt = 21)
  band_1_2   21 <= slack < 42  (1-2 retransmissions)
  band_2_5   42 <= slack < 105 (2-5 retransmissions)
  interior   slack >= 105

Envelope (EvPlcMac replay, K > 0):
  F(N,K) = max(15N + 7K + 21, 15 + 280) + 21N + 21,  slack = 1395 - F.
The max() term is the response gate: DC_RES_0 cannot start before the first
request's processing gate (C_req 15 + C_proc 280 = 295 slots).
"""

from pathlib import Path
import csv
import os
import statistics
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "results" / "ns3_e1" / "g3_burst_vs_iid.csv"

# (N, K) cells by envelope-slack band; at least 3 per band.
CELLS = [
    (37, 1), (37, 2), (36, 6),            # boundary: slack 14, 7, 15
    (36, 4), (35, 9), (36, 3),            # 1-2 retx: slack 29, 30, 36
    (36, 1), (35, 4), (34, 8),            # 2-5 retx: slack 50, 65, 73
    (10, 4), (20, 4), (25, 4), (30, 4),   # interior: slack 869, 605, 425, 245
]
SEEDS = 20
MEAN_SLOT_PER = 6.7e-5
T_SLOTS = 1395
RETX_UNIT = 21  # worst-case single retransmission (C_res_eff = B_pkt = 21)


def envelope_slack(n: int, k: int) -> int:
    pre = 15 * n + (7 * k + 21 if k > 0 else 0)
    finish = max(pre, 15 + 280) + 21 * n + 21
    return T_SLOTS - finish


def band(slack: int) -> str:
    if slack < RETX_UNIT:
        return "boundary"
    if slack < 2 * RETX_UNIT:
        return "band_1_2"
    if slack < 5 * RETX_UNIT:
        return "band_2_5"
    return "interior"


def examples_dir() -> Path:
    override = os.environ.get("NS3_EXAMPLES_DIR")
    if override:
        return Path(override)
    for line in (ROOT / "ns3" / "module_path.txt").read_text().splitlines():
        if line.startswith("NS3_ROOT="):
            value = line.split("=", 1)[1].strip().strip('"').replace("$ROOT", str(ROOT))
            return Path(value) / "build" / "contrib" / "ev-plc-transition" / "examples"
    raise RuntimeError("NS3_ROOT not found")


def run(mode: str) -> list[dict]:
    cells = ",".join(f"{n}:{k}" for n, k in CELLS)
    cmd = [str(examples_dir() / "ns3-dev-trackc-g3-optimized"), f"--mode={mode}",
           f"--seeds={SEEDS}", f"--cells={cells}"]
    return list(csv.DictReader(
        subprocess.run(cmd, check=True, capture_output=True, text=True).stdout.splitlines()))


def main() -> None:
    iid = {(int(r["N"]), int(r["K"])): r for r in run("i")}
    burst = run("c")

    rows = []
    for r in burst:
        cell = (int(r["N"]), int(r["K"]))
        base = iid[cell]
        mi, ti = int(base["miss_cycles"]), int(base["total_cycles"])
        mb, tb = int(r["miss_cycles"]), int(r["total_cycles"])
        if mi > 0:
            ratio = (mb / tb) / (mi / ti)
            ratio_s = f"{ratio:.4f}"
        else:
            ratio_s = "inf" if mb > 0 else "nan"
        slack = envelope_slack(*cell)
        rows.append({
            "N": cell[0], "K": cell[1], "mean_slot_per": MEAN_SLOT_PER,
            "bad_sojourn": r["bad_sojourn"], "per_bad": r["per_bad"],
            "miss_iid": mi, "total_iid": ti,
            "miss_burst": mb, "total_burst": tb,
            "ratio": ratio_s,
            "envelope_slack_slots": slack, "slack_band": band(slack),
        })

    OUT.parent.mkdir(parents=True, exist_ok=True)
    with OUT.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    defined = [(float(r["ratio"]), r) for r in rows if r["ratio"] not in ("nan", "inf")]
    print(f"rows: {len(rows)} ({len(CELLS)} cells x 12 G-E points); csv: {OUT}")
    if defined:
        worst = max(defined, key=lambda t: t[0])
        r = worst[1]
        print(f"max ratio (defined): {worst[0]:.4f} at (N,K)=({r['N']},{r['K']}), "
              f"sojourn={r['bad_sojourn']}, per_bad={r['per_bad']} "
              f"[burst {r['miss_burst']}/{r['total_burst']} vs iid "
              f"{r['miss_iid']}/{r['total_iid']}]")
    inf_rows = [r for r in rows if r["ratio"] == "inf"]
    if inf_rows:
        print(f"inf-ratio rows (iid 0, burst > 0): {len(inf_rows)}")
        for r in inf_rows:
            print(f"  ({r['N']},{r['K']}) sojourn={r['bad_sojourn']} per_bad={r['per_bad']}: "
                  f"burst {r['miss_burst']}/{r['total_burst']}, band={r['slack_band']}")
    for b in ("boundary", "band_1_2", "band_2_5", "interior"):
        vals = [v for v, r in defined if r["slack_band"] == b]
        med = f"{statistics.median(vals):.4f}" if vals else "undefined (iid = 0)"
        n_inf = sum(1 for r in inf_rows if r["slack_band"] == b)
        extra = f" (+{n_inf} inf rows)" if n_inf else ""
        print(f"band {b}: median defined ratio {med}{extra}")


if __name__ == "__main__":
    main()
