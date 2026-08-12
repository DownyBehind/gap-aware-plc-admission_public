#!/usr/bin/env python3
"""E4 five-policy comparison + E5 adversarial boundaries (final track).

E4: {HPGP CSMA/CA, fixed-10%, fixed-25%, fixed-50%, gap-aware q_wc=25 cap 3} over
burst K x N0 panels at PER = 1e-3, seeds 20. Metrics: DC miss rate, SLAC D_g
violation rate (admitted), admit wait incl. reject-and-retry (censored at the
horizon). Falsification: any cell where another policy dominates gap-aware on all
three metrics (strict on at least one) — reported, never hidden.

E5: deterministic PER=0 replay with the carry-in blocking frame realized at
the cycle head and the B_pkt guard in-map, over every admitted cell of the
full grid (includes the Cond-B equality diagonal N+K = 38). Gate: max per-EV
response end <= T_sched, not a single slot over.
"""

from pathlib import Path
import csv
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

sys.path.insert(0, str(ROOT / "experiments" / "analysis"))
from ieee_plot_style import apply_ieee_style, CB_BLUE, CB_ORANGE, CB_GREEN, CB_RED, CB_PURPLE

BUILD_DIR = ROOT / "ns3" / "standalone" / "build"
OUT_DIR = ROOT / "results" / "ns3_e1"
SEEDS = 20
HORIZON = 120
PER_PPM = 1000

POLICIES = [
    ("hpgp_csma_ca", "csma", 0, 0, CB_GRAY_STANDIN := "#999999", ":"),
    ("fixed_10pct", "fixed", 10, 0, CB_ORANGE, "--"),
    ("fixed_25pct", "fixed", 25, 0, CB_GREEN, "--"),
    ("fixed_50pct", "fixed", 50, 0, CB_PURPLE, "--"),
    ("acbs_qwc25", "acbs", 25, 3, CB_BLUE, "-"),
]
N0_VALUES = [0, 15, 30]
K_VALUES = [1, 2, 5, 10, 20, 35]


def run_policy(binary_policy: str, param: int, cap: int) -> list[dict]:
    out = subprocess.run(
        [str(BUILD_DIR / "e4_sim"), binary_policy, str(param), str(cap), str(PER_PPM),
         str(SEEDS), str(HORIZON), "2"],
        check=True, capture_output=True, text=True).stdout
    return list(csv.DictReader(out.splitlines()))


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    # ---- E4 ----------------------------------------------------------------
    cells: dict[tuple[str, int, int], dict] = {}
    for name, policy, param, cap, _, _ in POLICIES:
        rows = run_policy(policy, param, cap)
        for n0 in N0_VALUES:
            for k in K_VALUES:
                sub = [r for r in rows if int(r["N0"]) == n0 and int(r["K"]) == k]
                admitted = sum(int(r["admitted"]) for r in sub)
                sessions = sum(int(r["admitted"]) + int(r["never_admitted"]) for r in sub)
                misses = sum(int(r["dc_misses"]) for r in sub)
                evcycles = sum(int(r["dc_ev_cycles"]) for r in sub)
                cells[(name, n0, k)] = {
                    "policy": name, "N0": n0, "K": k,
                    "dc_miss_rate": misses / evcycles if evcycles else 0.0,
                    "dg_violation_rate": (sum(int(r["dg_violations"]) for r in sub) / admitted) if admitted else 0.0,
                    "admit_wait_cycles": sum(float(r["wait_sum_cycles"]) for r in sub) / sessions,
                    "never_admitted": sum(int(r["never_admitted"]) for r in sub),
                }

    # Falsification scan: is gap-aware dominated anywhere?
    metrics = ["dc_miss_rate", "dg_violation_rate", "admit_wait_cycles"]
    dominated = []
    for n0 in N0_VALUES:
        for k in K_VALUES:
            ours = cells[("acbs_qwc25", n0, k)]
            for name, *_ in POLICIES:
                if name == "acbs_qwc25":
                    continue
                theirs = cells[(name, n0, k)]
                if all(theirs[m] <= ours[m] + 1e-12 for m in metrics) and \
                   any(theirs[m] < ours[m] - 1e-12 for m in metrics):
                    dominated.append((n0, k, name,
                                      {m: (round(theirs[m], 6), round(ours[m], 6)) for m in metrics}))

    csv_path = OUT_DIR / "e4_policy_comparison.csv"
    with csv_path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(next(iter(cells.values())).keys()))
        writer.writeheader()
        writer.writerows(cells.values())

    # Figure: 3 metric rows x 3 N0 panels.
    apply_ieee_style()
    fig, axes = plt.subplots(3, 3, figsize=(7.0, 4.6), sharex=True)
    labels = {"hpgp_csma_ca": "HPGP CSMA/CA", "fixed_10pct": "Fixed 10%",
              "fixed_25pct": "Fixed 25%", "fixed_50pct": "Fixed 50%",
              "acbs_qwc25": "Gap-aware ($q_{wc}{=}25$)"}
    ylabels = ["DC miss rate", "SLAC $D_g$ violation", "Admit wait (cycles)"]
    for col, n0 in enumerate(N0_VALUES):
        for row, metric in enumerate(metrics):
            ax = axes[row][col]
            for name, _, _, _, color, ls in POLICIES:
                ys = [cells[(name, n0, k)][metric] for k in K_VALUES]
                ax.plot(K_VALUES, ys, color=color, linestyle=ls, linewidth=1.1,
                        marker="o", markersize=2.4, label=labels[name])
            ax.set_xscale("log")
            ax.set_xticks(K_VALUES)
            ax.set_xticklabels([str(k) for k in K_VALUES])
            ax.grid(True, alpha=0.2)
            if row == 0:
                ax.set_title(f"$N_0 = {n0}$", fontsize=8)
            if col == 0:
                ax.set_ylabel(ylabels[row])
            if row == 2:
                ax.set_xlabel("SLAC burst size, $K$")
    handles, lbls = axes[0][0].get_legend_handles_labels()
    fig.legend(handles, lbls, loc="lower center", ncol=5, fontsize=6, frameon=False,
               bbox_to_anchor=(0.5, -0.02))
    fig.tight_layout(pad=0.4, rect=(0, 0.03, 1, 1))
    for ext in ("pdf", "png"):
        fig.savefig(OUT_DIR / f"fig_e4_policy_comparison.{ext}", bbox_inches="tight")
    plt.close(fig)

    # ---- E5 ----------------------------------------------------------------
    out = subprocess.run([str(BUILD_DIR / "e5_sim")], check=True, capture_output=True,
                         text=True).stdout
    e5_rows = list(csv.DictReader(out.splitlines()))
    admitted_cells = [r for r in e5_rows if r["regime"] in {"hidden", "paid"}]
    violations = [r for r in admitted_cells if int(r["max_response_end"]) > int(r["T_sched"])]
    assert not violations, f"E5 falsified: responses over T in {len(violations)} cells, first: {violations[0]}"
    diagonal = [r for r in admitted_cells if int(r["N"]) + int(r["K"]) == 38]
    tightest = max(admitted_cells, key=lambda r: int(r["max_response_end"]))
    with (OUT_DIR / "e5_adversarial.csv").open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(e5_rows[0].keys()))
        writer.writeheader()
        writer.writerows(e5_rows)

    # ---- report -------------------------------------------------------------
    print(f"E4: {len(cells)} policy-cells (5 policies x 18 scenarios, seeds {SEEDS}, PER 1e-3)")
    if dominated:
        print(f"E4 FALSIFICATION: gap-aware dominated in {len(dominated)} cells:")
        for n0, k, name, detail in dominated:
            print(f"  (N0={n0}, K={k}) dominated by {name}: {detail}")
    else:
        print("E4: gap-aware non-dominated in every cell (no falsification)")
    print(f"E5: {len(admitted_cells)} admitted cells under realized carry-in + guard — all responses <= T; "
          f"tightest: (N={tightest['N']}, K={tightest['K']}) max response {tightest['max_response_end']} <= {tightest['T_sched']}; "
          f"Cond-B equality diagonal N+K=38: {len(diagonal)} cells, max {max(int(r['max_response_end']) for r in diagonal)}")
    print(f"csv: {csv_path}")
    print(f"csv: {OUT_DIR / 'e5_adversarial.csv'}")
    print(f"fig: {OUT_DIR / 'fig_e4_policy_comparison.pdf'} (+.png)")


if __name__ == "__main__":
    main()
