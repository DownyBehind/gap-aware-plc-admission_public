#!/usr/bin/env python3
"""E1 knee verification, Layer-1 edition (Stage 4, §5.2).

Measures delta(N, K) = build_cycle(N, K).finish - build_cycle(N, 0).finish
over N = 1..40 for fixed K in {1, 4, 8, 16} and checks it against the
theory prediction delta_pred = max(0, q*K + B_pkt - G(N)) cell-exactly
(Layer 1 shares the model world with the formulas, so no tolerance).
Falsification condition (E1): empirical knee != analytic knee.
"""

from pathlib import Path
import csv
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

sys.path.insert(0, str(ROOT / "experiments" / "analysis"))
from ieee_plot_style import apply_ieee_style, CB_BLUE, CB_ORANGE, CB_GREEN, CB_PURPLE

from src.formulas.transition_formulas import G_processing_gap, TransitionParams
from src.sim.cycle_builder import build_cycle

K_VALUES = [1, 4, 8, 16]
N_RANGE = range(1, 41)
SERIES_STYLE = {
    1: (CB_BLUE, "o", "-"),
    4: (CB_ORANGE, "s", "--"),
    8: (CB_GREEN, "^", "-."),
    16: (CB_PURPLE, "D", ":"),
}
OUT_DIR = ROOT / "results" / "layer1"


def sweep(params: TransitionParams) -> tuple[list[dict], dict[int, int], dict[int, int]]:
    rows = []
    empirical_knee: dict[int, int] = {}
    analytic_knee: dict[int, int] = {}
    mismatches = []
    for k in K_VALUES:
        x = params.b_slac_slots * k + params.B_pkt_slots
        for n in N_RANGE:
            delta = build_cycle(n, k, params).finish - build_cycle(n, 0, params).finish
            gap = G_processing_gap(n, params.C_req_slots, params.C_proc_slots)
            delta_pred = max(0, x - gap)
            match = delta == delta_pred
            if not match:
                mismatches.append((n, k, delta, delta_pred))
            rows.append({"N": n, "K": k, "delta": delta, "delta_pred": delta_pred, "match": int(match)})
            if delta > 0 and k not in empirical_knee:
                empirical_knee[k] = n
            if x > gap and k not in analytic_knee:
                analytic_knee[k] = n
    if mismatches:
        for n, k, delta, delta_pred in mismatches:
            print(f"MISMATCH at (N={n}, K={k}): delta={delta}, delta_pred={delta_pred}")
        raise AssertionError(f"E1 falsified on {len(mismatches)} cells — stopping")
    return rows, empirical_knee, analytic_knee


def render(rows: list[dict], analytic_knee: dict[int, int]) -> list[Path]:
    apply_ieee_style()
    fig, ax = plt.subplots(figsize=(3.5, 2.4))

    for k in K_VALUES:
        color, marker, linestyle = SERIES_STYLE[k]
        ns = [r["N"] for r in rows if r["K"] == k]
        deltas = [r["delta"] for r in rows if r["K"] == k]
        ax.plot(ns, deltas, color=color, marker=marker, linestyle=linestyle,
                markersize=2.5, markevery=3, linewidth=1.0, label=f"$K={k}$")

    ax.axhline(0, color="black", linewidth=0.6)
    for k in K_VALUES:
        knee = analytic_knee[k]
        color = SERIES_STYLE[k][0]
        ax.plot([knee], [0], marker="v", color=color, markersize=4, clip_on=False, zorder=5)
        y_txt = -14 if k in {1, 8} else -21  # stagger adjacent labels (17 vs 18)
        ax.annotate(str(knee), xy=(knee, 0), xytext=(knee, y_txt), fontsize=6, ha="center", color=color)

    ax.axvline(20, color="black", linestyle=":", linewidth=0.8)
    ax.annotate("gap closure ($G(N){=}0$)", xy=(20.5, 118), fontsize=6, ha="left")

    ax.set_xlabel("DC-active EVs, $N$")
    ax.set_ylabel(r"$\Delta$ finish (slots)")
    ax.set_xlim(0.5, 40.5)
    ax.set_ylim(-25, 145)
    ax.legend(loc="upper left", fontsize=6, framealpha=0.95, handlelength=1.8)
    ax.grid(True, alpha=0.2)
    fig.tight_layout(pad=0.3)

    outputs = []
    for ext in ("pdf", "png"):
        out = OUT_DIR / f"fig_knee_delta_vs_N.{ext}"
        fig.savefig(out, bbox_inches="tight")
        outputs.append(out)
    plt.close(fig)
    return outputs


def main() -> None:
    params = TransitionParams()
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    rows, empirical_knee, analytic_knee = sweep(params)  # criteria (a), (c)

    # Criterion (b): empirical knee == analytic knee for every K.
    for k in K_VALUES:
        assert empirical_knee[k] == analytic_knee[k], (
            f"E1 falsified: K={k} empirical knee {empirical_knee[k]} != analytic {analytic_knee[k]}"
        )
    # Criterion (c) is implied by (a) (delta == delta_pred == 0 in hidden), but
    # assert it independently against the regime classification.
    for r in rows:
        if build_cycle(r["N"], r["K"], params).regime == "hidden":
            assert r["delta"] == 0, f"hidden cell (N={r['N']}, K={r['K']}) has delta={r['delta']}"

    csv_path = OUT_DIR / "knee_verification.csv"
    with csv_path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=["N", "K", "delta", "delta_pred", "match"])
        writer.writeheader()
        writer.writerows(rows)

    outputs = render(rows, analytic_knee)
    for out in outputs:
        assert out.exists() and out.stat().st_size > 0

    print(f"cells: {len(rows)} (delta == delta_pred on all)")
    print(f"knees: empirical == analytic == {dict(sorted(analytic_knee.items()))}")
    print(f"csv:   {csv_path}")
    for out in outputs:
        print(f"fig:   {out}")


if __name__ == "__main__":
    main()
