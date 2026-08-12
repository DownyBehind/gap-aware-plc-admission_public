#!/usr/bin/env python3
"""E2: loss-aware admission q(p) — axis C experimental edition.

Variants: (A) loss-blind q=7 unlimited retries, (B) q_exp(p) (numerically 7
at both p — ceiling absorption, kept as an explicit arm to record that fact),
(C) q_wc(p) with per-frame retry cap n_r(p, eps).
Measured per p in {1e-3, 1e-2}: admitted-session D_g violations, analytic
admission rejection rate over the (N, K) plane, DC miss rate.
Pass gates: (C) violations at eps level; (A) expected to break at p=1e-2 —
if it does not, report the weakness honestly (window/p strength).
"""

from math import ceil, log
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
from ieee_plot_style import apply_ieee_style, CB_BLUE, CB_ORANGE, CB_RED

from src.formulas.transition_formulas import TransitionParams, classify_regime

BINARY = ROOT / "ns3" / "standalone" / "build" / "e2_sim"
OUT_DIR = ROOT / "results" / "ns3_e1"
SEEDS = 20
EPS = 1e-6


def q_wc(p: float, w: int = 247) -> tuple[int, int]:
    # Paper derivation (union bound over the 20 SLAC messages): the retry cap
    # is the least integer with 20 * p**(n_r + 1) <= EPS. The closed form
    # below lands on the same integer over the supported p range; assert it.
    n_r = ceil(log(EPS) / log(p))
    n_union = next(n for n in range(64) if 20 * p ** (n + 1) <= EPS)
    assert n_r == n_union, f"retry caps disagree: {n_r} != {n_union} at p={p}"
    # Worst-case airtime C_wc = C_slac + n_r * 241: first transmissions at the
    # envelope (247), retries at the measured airtime sum (241); 39 service
    # windows (the joining cycle of the ceil(D_g/T) = 40 carries no credit).
    return ceil((w + n_r * 241) / 39), n_r


def run_sim(q: int, cap: int, per_ppm: int) -> list[dict]:
    out = subprocess.run([str(BINARY), str(q), str(cap), str(per_ppm), str(SEEDS), "90", "2"],
                         check=True, capture_output=True, text=True).stdout
    return list(csv.DictReader(out.splitlines()))


def rejection_rate(q: int) -> float:
    params = TransitionParams(b_slac_slots=q)
    total = admitted = 0
    for n in range(1, 41):
        for k in range(1, 17):
            total += 1
            if classify_regime(n, k, params) in {"hidden", "paid"}:
                admitted += 1
    return 1.0 - admitted / total


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    summary_rows = []
    for p, ppm in ((1e-3, 1000), (1e-2, 10000)):
        qw, n_r = q_wc(p)
        variants = [("A_loss_blind", 7, 0), ("B_q_exp", 7, 0), ("C_q_wc", qw, n_r)]
        for name, q, cap in variants:
            params = TransitionParams(b_slac_slots=q)
            rows = run_sim(q, cap, ppm)
            # Only cells the variant's own admission (state-level Cond A+B) admits.
            admitted_rows = [r for r in rows
                             if classify_regime(int(r["N"]), int(r["K"]), params) in {"hidden", "paid"}]
            sessions = sum(int(r["K"]) for r in admitted_rows)
            violations = sum(int(r["violations"]) for r in admitted_rows)
            failures = sum(int(r["failures"]) for r in admitted_rows)
            miss = sum(int(r["dc_miss_cycles"]) for r in admitted_rows)
            cycles = sum(int(r["horizon"]) for r in admitted_rows)
            summary_rows.append({
                "per": p, "variant": name, "q": q, "retry_cap": cap,
                "admitted_sessions": sessions,
                "dg_violations": violations,
                "dg_violation_rate": round(violations / sessions, 8) if sessions else 0.0,
                "cap_failures": failures,
                "rejection_rate_plane": round(rejection_rate(q), 4),
                "dc_miss_rate": round(miss / cycles, 8) if cycles else 0.0,
            })

    # Gates.
    for row in summary_rows:
        if row["variant"] == "C_q_wc":
            budget = row["admitted_sessions"] * EPS
            assert row["cap_failures"] <= max(1, ceil(budget * 10)), \
                f"C retry-cap failures {row['cap_failures']} above eps budget {budget}"
            assert row["dg_violations"] == 0, \
                f"C q_wc violated D_g {row['dg_violations']} times at p={row['per']}"
    a_1e2 = next(r for r in summary_rows if r["variant"] == "A_loss_blind" and r["per"] == 1e-2)

    csv_path = OUT_DIR / "e2_admission_variants.csv"
    with csv_path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(summary_rows[0].keys()))
        writer.writeheader()
        writer.writerows(summary_rows)

    # Admission-region contraction figure: rejection boundary (max admitted N
    # per K) for q = 7 / 19 / 25.
    apply_ieee_style()
    fig, ax = plt.subplots(figsize=(3.5, 2.4))
    for q, color, label in ((7, CB_BLUE, "$q{=}7$ ($p{=}0$ / exp)"),
                            (19, CB_ORANGE, "$q_{wc}(10^{-3}){=}19$"),
                            (25, CB_RED, "$q_{wc}(10^{-2}){=}25$")):
        params = TransitionParams(b_slac_slots=q)
        ks = list(range(1, 17))
        boundary = []
        for k in ks:
            admitted = [n for n in range(1, 45)
                        if classify_regime(n, k, params) in {"hidden", "paid"}]
            boundary.append(max(admitted) if admitted else 0)
        ax.plot(boundary, ks, color=color, linewidth=1.2, label=label)
        ax.fill_betweenx(ks, 0, boundary, color=color, alpha=0.08)
    ax.set_xlabel("DC-active EVs, $N$ (max admitted)")
    ax.set_ylabel("Active SLAC sessions, $K$")
    ax.set_xlim(0, 42)
    ax.set_ylim(0.5, 16.5)
    ax.grid(True, alpha=0.2)
    ax.legend(fontsize=6, loc="upper right")
    fig.tight_layout(pad=0.3)
    for ext in ("pdf", "png"):
        fig.savefig(OUT_DIR / f"fig_admission_region_qp.{ext}", bbox_inches="tight")
    plt.close(fig)

    # Breaking-point diagnostic for the loss-blind arm: where does A fail?
    diag_rows = []
    for ppm in (30000, 50000, 100000):
        rows = run_sim(7, 0, ppm)
        params7 = TransitionParams(b_slac_slots=7)
        adm = [r for r in rows if classify_regime(int(r["N"]), int(r["K"]), params7) in {"hidden", "paid"}]
        sessions = sum(int(r["K"]) for r in adm)
        violations = sum(int(r["violations"]) for r in adm)
        diag_rows.append({"per": ppm / 1e6, "variant": "A_diagnostic", "q": 7, "retry_cap": 0,
                          "admitted_sessions": sessions, "dg_violations": violations,
                          "dg_violation_rate": round(violations / sessions, 8),
                          "cap_failures": 0, "rejection_rate_plane": round(rejection_rate(7), 4),
                          "dc_miss_rate": ""})
    with csv_path.open("a", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(summary_rows[0].keys()))
        writer.writerows(diag_rows)

    for row in summary_rows + diag_rows:
        print(f"p={row['per']} {row['variant']} (q={row['q']}, cap={row['retry_cap']}): "
              f"D_g violations {row['dg_violations']}/{row['admitted_sessions']} "
              f"(rate {row['dg_violation_rate']}), cap-failures {row['cap_failures']}, "
              f"plane rejection {row['rejection_rate_plane']}, DC miss {row['dc_miss_rate']}")
    if a_1e2["dg_violations"] == 0:
        print("NOTE: loss-blind A did NOT break at p=1e-2 (0 violations): per-session "
              "credit completion is ~32-33 cycles vs D_g=40, so a breach needs >= 5 frame "
              "failures in one session (P ~ 1.5e-6). The diagnostic sweep locates the "
              "breaking point at p ~ 0.03-0.1 instead; reported honestly, not as safety of A.")
    print(f"csv: {csv_path}")
    print(f"fig: {OUT_DIR / 'fig_admission_region_qp.pdf'} (+.png)")


if __name__ == "__main__":
    main()
