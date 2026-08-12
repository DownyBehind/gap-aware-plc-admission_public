#!/usr/bin/env python3
"""Cycle-structure timeline diagram for the midpoint review (§2 figure).

Renders two cycles from cycle_builder phase output: a hidden case (N=10, K=4)
where the authentication span sits inside the processing gap, and a paid case
(N=25, K=4) where the same span pushes the response phase by auth_span slots.
"""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Patch

sys.path.insert(0, str(ROOT / "experiments" / "analysis"))
from ieee_plot_style import apply_ieee_style

from src.formulas.transition_formulas import TransitionParams
from src.sim.cycle_builder import build_cycle

# Luminance-separated fills (grayscale print); PKT_GUARD adds a hatch.
STYLE = {
    "DC_REQ": {"facecolor": "#4292c6"},
    "SLAC_SERVICE": {"facecolor": "#9ecae1"},
    "PKT_GUARD": {"facecolor": "#9ecae1", "hatch": "////"},
    "DC_RES": {"facecolor": "#0a3a75"},
}
OUT_DIR = ROOT / "results" / "layer1"


def draw_case(ax, params: TransitionParams, n: int, k: int, expected_regime: str) -> None:
    cycle = build_cycle(n, k, params)
    assert cycle.regime == expected_regime, f"({n},{k}) regime {cycle.regime} != {expected_regime}"
    first_ready = params.C_req_slots + params.C_proc_slots

    # Off-channel EVSE processing window of the first request (row 0).
    ax.broken_barh([(params.C_req_slots, params.C_proc_slots)], (0.2, 0.5),
                   facecolor="#e0e0e0", edgecolor="black", linewidth=0.4)
    ax.text(params.C_req_slots + params.C_proc_slots / 2, 0.45,
            "EVSE proc (off-channel)", fontsize=6, ha="center")

    # Channel phases (row 1) + the carry-in envelope B_blk at the tail.
    for ph in cycle.phases:
        if ph.length > 0:
            ax.broken_barh([(ph.start, ph.length)], (1.0, 0.5),
                           edgecolor="black", linewidth=0.4, **STYLE[ph.name])
    ax.broken_barh([(cycle.finish - params.B_blk_slots, params.B_blk_slots)], (1.0, 0.5),
                   facecolor="white", edgecolor="black", linewidth=0.4, hatch="xxxx")

    # First-response readiness and the marginal delay vs the K=0 cycle.
    ax.axvline(first_ready, color="black", linestyle=":", linewidth=0.8)
    ax.text(first_ready + 4, 1.75, r"$C_{\mathrm{req}}{+}C_{\mathrm{proc}}=295$", fontsize=6)
    baseline = build_cycle(n, 0, params)
    delta = cycle.finish - baseline.finish
    note = "inside gap" if delta == 0 else "overflows gap"
    label = f"$N={n}$, $K={k}$: {cycle.regime}, $\\Delta$ = {delta} slots (auth {note})"
    ax.text(0, 2.15, label, fontsize=7)
    ax.annotate(f"finish = {cycle.finish}", xy=(cycle.finish, 1.25), xytext=(cycle.finish + 15, 1.65),
                fontsize=6, arrowprops={"arrowstyle": "-", "linewidth": 0.5})

    ax.set_ylim(0, 2.5)
    ax.set_yticks([])
    ax.spines[["left", "top", "right"]].set_visible(False)


def main() -> None:
    params = TransitionParams()
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    apply_ieee_style()

    fig, axes = plt.subplots(2, 1, figsize=(5.0, 2.9), sharex=True)
    draw_case(axes[0], params, 10, 4, "hidden")
    draw_case(axes[1], params, 25, 4, "paid")
    axes[1].set_xlabel("slots from cycle start")
    axes[1].set_xlim(0, 1050)

    handles = [
        Patch(facecolor="#4292c6", edgecolor="black", linewidth=0.4, label="DC_REQ"),
        Patch(facecolor="#9ecae1", edgecolor="black", linewidth=0.4, label="SLAC_SERVICE"),
        Patch(facecolor="#9ecae1", edgecolor="black", linewidth=0.4, hatch="////", label="PKT_GUARD"),
        Patch(facecolor="#0a3a75", edgecolor="black", linewidth=0.4, label="DC_RES"),
        Patch(facecolor="#e0e0e0", edgecolor="black", linewidth=0.4, label="EVSE proc"),
        Patch(facecolor="white", edgecolor="black", linewidth=0.4, hatch="xxxx", label=r"$B_{\mathrm{blk}}$"),
    ]
    fig.legend(handles=handles, loc="lower center", ncol=6, fontsize=5.5, frameon=False,
               bbox_to_anchor=(0.5, -0.04))
    fig.tight_layout(pad=0.4)

    for ext in ("pdf", "png"):
        out = OUT_DIR / f"fig_cycle_structure.{ext}"
        fig.savefig(out, bbox_inches="tight")
        print(f"fig: {out}")
    plt.close(fig)


if __name__ == "__main__":
    main()
