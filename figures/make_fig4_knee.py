#!/usr/bin/env python3
"""Knee verification — knee position N vs active sessions K.

Supplementary figure: it renders the same data the paper reports as
Table I (admission knees vs inter-frame spacing) and is not shipped as a
paper figure. Every plotted value is read from committed
CSVs: the IFS=0 series from results/layer1/knee_verification.csv (first N
with delta > 0 per K) and the IFS in {1,2,3} series from the
knee_empirical column of results/ns3_e1/overhead_knee.csv. The script
asserts the expected knee sets before drawing. Page size is fixed at the
shipped bounding box (259.5 x 165.574 pt)."""
import csv
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "figures" / "out"
sys.path.insert(0, str(ROOT / "experiments" / "analysis"))

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

from ieee_plot_style import apply_ieee_style, CB_BLUE, CB_ORANGE, CB_GREEN, CB_RED

BBOX_PT = (259.5, 165.574)
KS = [1, 4, 8, 16]
EXPECTED = {
    0: {1: 18, 4: 17, 8: 15, 16: 11},
    1: {1: 17, 4: 16, 8: 14, 16: 11},
    2: {1: 16, 4: 15, 8: 13, 16: 10},
    3: {1: 15, 4: 14, 8: 12, 16: 9},
}


def load_knees() -> dict[int, dict[int, int]]:
    knees: dict[int, dict[int, int]] = {0: {}}
    with (ROOT / "results/layer1/knee_verification.csv").open() as f:
        for r in csv.DictReader(f):
            n, k, delta = int(r["N"]), int(r["K"]), int(r["delta"])
            if delta > 0 and (k not in knees[0] or n < knees[0][k]):
                knees[0][k] = n
    with (ROOT / "results/ns3_e1/overhead_knee.csv").open() as f:
        for r in csv.DictReader(f):
            k, ifs, knee = int(r["K"]), int(r["IFS"]), int(r["knee_empirical"])
            if k in KS and knee > 0:
                knees.setdefault(ifs, {})[k] = knee
    return knees


def main() -> None:
    knees = load_knees()
    assert {i: knees[i] for i in EXPECTED} == EXPECTED, f"knee sets changed: {knees}"

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    apply_ieee_style()
    fig, ax = plt.subplots(figsize=(BBOX_PT[0] / 72, BBOX_PT[1] / 72))

    series = [
        (0, "analytic / Layer 1 / event-driven (IFS=0)", CB_BLUE, "o", "-"),
        (1, "measured, IFS=1", CB_ORANGE, "s", "--"),
        (2, "measured, IFS=2", CB_GREEN, "^", "--"),
        (3, "measured, IFS=3", CB_RED, "D", "--"),
    ]
    for ifs, label, color, marker, ls in series:
        ax.plot(KS, [knees[ifs][k] for k in KS], marker=marker, linestyle=ls,
                color=color, label=label, linewidth=1.4, markersize=4.5,
                markeredgecolor=color if ifs else CB_BLUE)

    ax.set_xlabel("Active SLAC sessions, $K$")
    ax.set_ylabel("Knee position, $N$")
    ax.set_xticks(KS)
    ax.grid(True, linewidth=0.4, alpha=0.6)
    ax.legend(loc="upper right", fontsize=5.8, framealpha=0.95,
              handlelength=1.8, borderpad=0.45)
    fig.subplots_adjust(left=0.115, right=0.985, top=0.975, bottom=0.16)

    for ext in ("pdf", "png"):
        fig.savefig(OUT_DIR / f"fig4_knee_verification.{ext}")
    plt.close(fig)
    print(f"fig: {OUT_DIR / 'fig4_knee_verification.pdf'}")


if __name__ == "__main__":
    main()
