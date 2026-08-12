#!/usr/bin/env python3
"""Policy-comparison figure (the paper's Fig. 4; the filename keeps the
historical fig5 numbering). The gap-aware policy is labeled "Gap-aware"
in the legend (`acbs` in code identifiers and CSV keys). Geometry is
frozen: figsize (5.0, 1.95), two rows x two columns (N0 in {15,30}),
two-line y-labels, legend inside; the admit-wait row is reported in the
paper text, not plotted. Row-wise y-limits are shared across columns
(fixed 0..1 with ticks 0/0.5/1 so 0.82/0.93/0.99 are readable)."""
import csv, sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "figures" / "out"
OUT_DIR.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(ROOT)); sys.path.insert(0, str(ROOT / 'experiments' / 'analysis'))
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from ieee_plot_style import apply_ieee_style, CB_BLUE, CB_ORANGE, CB_GREEN, CB_PURPLE

apply_ieee_style()
ev = list(csv.DictReader(open(ROOT / 'results/ns3_e1/e4_policy_comparison_event.csv')))
lookup = {(r["policy"], int(r["N0"]), int(r["K"])): r for r in ev}
KV = [1, 2, 5, 10, 20, 35]
styles = {"hpgp_csma_ca": ("#999999", ":", "HPGP CSMA/CA"),
          "fixed_10pct": (CB_ORANGE, "--", "Fixed 10%"),
          "fixed_25pct": (CB_GREEN, "--", "Fixed 25%"),
          "fixed_50pct": (CB_PURPLE, "--", "Fixed 50%"),
          "acbs_qwc25": (CB_BLUE, "-", "Gap-aware ($q_{wc}{=}25$)")}
metrics = ["dc_miss_rate", "dg_violation_rate"]
ylabels = ["DC miss\nrate", "SLAC $D_g$\nviol."]
ROW_YLIM = (-0.05, 1.05)   # shared per row, identical across columns
ROW_YTICKS = [0, 0.5, 1]
fig, axes = plt.subplots(2, 2, figsize=(5.0, 1.66), sharex=True)  # height 1.44 with figure-level horizontal legend; smaller heights break ylabel/legend readability (labels are fixed content)
for col, n0 in enumerate((15, 30)):
    for row, metric in enumerate(metrics):
        ax = axes[row][col]
        for name, (color, ls, label) in styles.items():
            ys = [float(lookup[(name, n0, k)][metric]) for k in KV]
            ax.plot(KV, ys, color=color, linestyle=ls, linewidth=1.0,
                    marker="o", markersize=2.2, label=label)
        ax.set_ylim(*ROW_YLIM); ax.set_yticks(ROW_YTICKS)
        ax.set_yticklabels(["0", "0.5", "1"])
        ax.set_xscale("log"); ax.set_xticks(KV)
        ax.set_xticklabels([str(k) for k in KV])
        ax.grid(True, alpha=0.2)
        if row == 0: ax.set_title(f"$N_0 = {n0}$", fontsize=8)
        if col == 0: ax.set_ylabel(ylabels[row])
        if row == 1: ax.set_xlabel("SLAC burst size, $K$")
handles, labels = axes[0][0].get_legend_handles_labels()
fig.legend(handles, labels, loc="upper center", ncol=5, fontsize=4.6,
           frameon=False, bbox_to_anchor=(0.5, 1.02), handlelength=1.2,
           columnspacing=0.9, handletextpad=0.4)
fig.tight_layout(pad=0.3, h_pad=1.2, rect=(0, 0, 1, 0.91))
for ext in ("pdf", "png"):
    fig.savefig(OUT_DIR / f"fig5_policy_comparison.{ext}",
                bbox_inches="tight")
print("fig5 regenerated (Gap-aware legend)")
