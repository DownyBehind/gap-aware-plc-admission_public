#!/usr/bin/env python3
"""Fig.2 cycle structure — hidden (N=10,K=4) vs paid (N=25,K=4) timelines.

Permanent reconstruction of the shipped figure. All drawn values come from
the deterministic Layer-1 builder (src/sim/cycle_builder.py) at the frozen
baseline parameters; the script asserts the finish/regime values it draws.
Page size is fixed (514.395 x 115.0 pt), so the output does not depend on
font-metric-sensitive tight cropping.

Rendering baseline: matplotlib 3.6.3 (source-built against system
FreeType 2.13.2) per requirements.txt — the shipped copy was re-adopted
from this pinned environment on 2026-08-09; older-generation renders
differ only in hatch/edge encoding (values, text, and geometry data
identical)."""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "figures" / "out"
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(ROOT / "experiments" / "analysis"))

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Patch

from ieee_plot_style import apply_ieee_style
from src.formulas.transition_formulas import TransitionParams
from src.sim.cycle_builder import build_cycle

BBOX_PT = (514.395, 115.0)  # fixed page size (typesetting gate)
STYLE = {
    "DC_REQ": {"facecolor": "#4292c6"},
    "SLAC_SERVICE": {"facecolor": "#9ecae1"},
    "PKT_GUARD": {"facecolor": "#9ecae1", "hatch": "////"},
    "DC_RES": {"facecolor": "#0a3a75"},
}
EXPECTED = {(10, 4): ("hidden", 526, 0), (25, 4): ("paid", 970, 49)}


def draw_case(ax, params, n, k):
    cycle = build_cycle(n, k, params)
    baseline = build_cycle(n, 0, params)
    delta = cycle.finish - baseline.finish
    regime, finish, dexp = EXPECTED[(n, k)]
    assert (cycle.regime, cycle.finish, delta) == (regime, finish, dexp), (
        f"({n},{k}) draws {cycle.regime}/{cycle.finish}/{delta}, expected {regime}/{finish}/{dexp}")
    first_ready = params.C_req_slots + params.C_proc_slots

    # G1: exactly two lanes of identical height; every rectangle in a lane
    # shares one y0/height so the top edges align pixel-exactly.
    LANE_H = 0.62
    CH_Y0 = 1.19           # channel lane (top)
    EV_Y0 = CH_Y0 - 0.6 * LANE_H - LANE_H  # EVSE lane, fixed 0.6h gap
    segs = [(ph.start, ph.length) for ph in cycle.phases if ph.length > 0]
    cols = [STYLE[ph.name]["facecolor"] for ph in cycle.phases if ph.length > 0]
    ax.broken_barh(segs, (CH_Y0, LANE_H), facecolors=cols,
                   edgecolor="black", linewidth=0.5)
    # hatch overlays at the SAME lane coordinates (alignment preserved)
    for ph in cycle.phases:
        if ph.length > 0 and ph.name == "PKT_GUARD":
            ax.broken_barh([(ph.start, ph.length)], (CH_Y0, LANE_H),
                           facecolor="none", edgecolor="black",
                           linewidth=0.5, hatch="////")
    ax.broken_barh([(cycle.finish - params.B_blk_slots, params.B_blk_slots)],
                   (CH_Y0, LANE_H), facecolor="white", edgecolor="black",
                   linewidth=0.5, hatch="xxxx")
    if delta == 0:
        # G2: draw the idle channel span (guard end -> first response) as an
        # explicitly EMPTY stretch of the lane: thin dashed outline only.
        guard_end = max(ph.start + ph.length for ph in cycle.phases
                        if ph.name != "DC_RES" and ph.length > 0)
        ax.broken_barh([(guard_end, first_ready - guard_end)],
                       (CH_Y0, LANE_H), facecolor="none",
                       edgecolor="#888888", linewidth=0.5,
                       linestyle=(0, (2, 2)))

    # G3: EVSE lane starts at C_req = 15 and ends at 295 (both panels)
    ax.broken_barh([(params.C_req_slots, params.C_proc_slots)], (EV_Y0, LANE_H),
                   facecolor="#e0e0e0", edgecolor="black", linewidth=0.5)

    # L1: one dashed marker through both panels; the label lives only in
    # the top panel (drawn above the state-title band).
    ax.axvline(first_ready, color="black", linestyle=":", linewidth=0.9)
    req_end = n * params.C_req_slots
    if delta == 0:
        # M2: short label, pinned RIGHT of the dashed line so it cannot
        # meet the bracket that owns 150..295
        ax.text(first_ready + 14, 2.30, "first result ready (295)",
                fontsize=6.5, va="center")
        # M3: the bracket owns the band above the bar; label above it,
        # no white box
        ax.annotate("", xy=(first_ready, 2.12), xytext=(req_end, 2.12),
                    arrowprops={"arrowstyle": "<->", "linewidth": 0.7})
        # label shifted left of its bracket midpoint so its right edge
        # clears the dotted first-response marker at 295
        ax.text((req_end + first_ready) / 2 - 28, 2.46,
                r"processing gap, $G(10) = 145$",
                fontsize=6.5, ha="center", va="center")
    else:
        ax.text(first_ready + 14, 2.30, "gap closed", fontsize=6.5,
                va="center")

    # M1: panel title lives in the left axis margin (two-line ylabel)
    ax.set_ylabel(f"$N{{=}}{n}$, $K{{=}}{k}$\n{cycle.regime}, "
                  f"$\\Delta F{{=}}{delta}$",
                  rotation=0, ha="right", va="center", fontsize=7,
                  labelpad=4)
    # L6: F value outside the right end of the bar, in the bar's own band
    ax.text(cycle.finish + 14, 1.5, f"$F{{=}}{cycle.finish}$",
            fontsize=7, ha="left", va="center")

    ax.set_ylim(0, 2.75)
    ax.set_yticks([])
    ax.spines[["left", "top", "right"]].set_visible(False)


def main() -> None:
    params = TransitionParams()
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    apply_ieee_style()

    fig, axes = plt.subplots(2, 1, figsize=(BBOX_PT[0] / 72, BBOX_PT[1] / 72),
                             sharex=True)
    draw_case(axes[0], params, 10, 4)
    draw_case(axes[1], params, 25, 4)
    axes[1].set_xlabel(
        r"slots from cycle start ($\Delta = 35.84\,\mu$s, $T = 1395$)", fontsize=8)
    axes[1].set_xlim(0, 1040)

    handles = [
        Patch(facecolor="#4292c6", edgecolor="black", linewidth=0.5, label="DC requests"),
        Patch(facecolor="#9ecae1", edgecolor="black", linewidth=0.5, label="SLAC window ($qK$)"),
        Patch(facecolor="#9ecae1", edgecolor="black", linewidth=0.5, hatch="////",
              label=r"$B_{\mathrm{pkt}}$ guard"),
        Patch(facecolor="#0a3a75", edgecolor="black", linewidth=0.5, label="DC responses"),
        Patch(facecolor="#e0e0e0", edgecolor="black", linewidth=0.5, label="EVSE processing"),
        Patch(facecolor="white", edgecolor="black", linewidth=0.5, hatch="xxxx",
              label=r"$B_{\mathrm{blk}}$ carry-in"),
    ]
    fig.legend(handles=handles, loc="lower center", ncol=6, fontsize=6.5,
               frameon=False, bbox_to_anchor=(0.5, 0.0), columnspacing=1.4,
               handlelength=1.4)
    fig.subplots_adjust(left=0.108, right=0.995, top=0.97, bottom=0.33, hspace=0.75)

    for ext in ("pdf", "png"):
        fig.savefig(OUT_DIR / f"fig2_cycle_structure.{ext}")
    plt.close(fig)
    print(f"fig: {OUT_DIR / 'fig2_cycle_structure.pdf'}")


if __name__ == "__main__":
    main()
