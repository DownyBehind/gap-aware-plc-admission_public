#!/usr/bin/env python3
"""Create final paper figures from actual ns-3 outputs in results/ns3_final."""

from __future__ import annotations

import csv
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

from ieee_plot_style import apply_ieee_style, set_ieee_size


ROOT = Path(__file__).resolve().parents[2]
FINAL = ROOT / "results" / "ns3_final"
PAPER = ROOT / "results" / "paper_figures"
APP = ROOT / "results" / "appendix_figures"


LABELS = {
    "hpgp_csma_ca_like": "CSMA/CA-like",
    "fixed_reservation": "Fixed reservation",
    "proposed_transition_aware": "Proposed",
    "condA_only": "Cond-A only",
    "condA_and_condB": "Cond-A+B",
    "count_based_proposed": "Count-based",
    "worst_case_proposed": "Worst-case",
    "link_aware_proposed": "Link-aware",
}

AXIS_LABELS = {
    "slac_burst": "SLAC burst size (sessions)",
    "degradation_factor": "PLC degradation factor (x)",
    "completion_index": "SLAC completion index (session)",
    "time_s": "Time (s)",
    "N": "DC-active EVs, N (EVs)",
    "K": "Active SLAC sessions, K (sessions)",
}


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="") as handle:
        return list(csv.DictReader(handle))


def save(fig: plt.Figure, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fig.tight_layout()
    fig.savefig(path.with_suffix(".pdf"))
    fig.savefig(path.with_suffix(".png"))
    plt.close(fig)


def write_plot_log(exp_dir: str, figures: list[Path]) -> None:
    log_path = FINAL / exp_dir / "plot.log"
    log_path.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        f"timestamp_utc={datetime.now(timezone.utc).isoformat()}",
        f"experiment_dir={exp_dir}",
        "status=PASS",
    ]
    for figure in figures:
        lines.append(f"figure_pdf={figure.with_suffix('.pdf').relative_to(ROOT)}")
        lines.append(f"figure_png={figure.with_suffix('.png').relative_to(ROOT)}")
    log_path.write_text("\n".join(lines) + "\n")


def line_plot(path: Path, rows: list[dict[str, str]], x_key: str, y_key: str, group_key: str, ylabel: str, filter_key: str | None = None, filter_value: str | None = None) -> None:
    apply_ieee_style()
    fig, ax = plt.subplots()
    set_ieee_size(fig)
    grouped: dict[str, list[tuple[float, float]]] = defaultdict(list)
    for row in rows:
        if filter_key and row.get(filter_key) != filter_value:
            continue
        grouped[row[group_key]].append((float(row[x_key]), float(row[y_key])))
    for group, values in grouped.items():
        values = sorted(values)
        xs = [v[0] for v in values]
        ys = [v[1] for v in values]
        ax.plot(xs, ys, marker="o", label=LABELS.get(group, group))
    ax.set_xlabel(AXIS_LABELS.get(x_key, x_key.replace("_", " ")))
    ax.set_ylabel(ylabel)
    ax.grid(True, alpha=0.3)
    ax.legend(frameon=False)
    save(fig, path)


def bar_plot(path: Path, labels: list[str], series: dict[str, list[float]], ylabel: str, xlabel: str = "") -> None:
    apply_ieee_style()
    fig, ax = plt.subplots()
    set_ieee_size(fig)
    width = 0.8 / max(1, len(series))
    offsets = [i - (len(series) - 1) / 2 for i in range(len(series))]
    for offset, (name, values) in zip(offsets, series.items()):
        ax.bar([i + offset * width for i in range(len(labels))], values, width, label=LABELS.get(name, name))
    ax.set_xticks(range(len(labels)), labels, rotation=15, ha="right")
    if xlabel:
        ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.grid(axis="y", alpha=0.3)
    ax.legend(frameon=False)
    save(fig, path)


def plot_exp1() -> None:
    rows = read_csv(FINAL / "exp1_nominal_baseline_comparison" / "metrics.csv")
    line_plot(PAPER / "Fig1_dc_miss_vs_burst", rows, "slac_burst", "dc_deadline_miss_ratio", "algorithm", "DC deadline miss ratio (fraction)", "N0", "30")
    line_plot(PAPER / "Fig2_slac_timeout_vs_burst", rows, "slac_burst", "slac_timeout_ratio", "algorithm", "SLAC timeout ratio (fraction)", "N0", "30")
    write_plot_log("exp1_nominal_baseline_comparison", [PAPER / "Fig1_dc_miss_vs_burst", PAPER / "Fig2_slac_timeout_vs_burst"])


def plot_exp2() -> None:
    rows = read_csv(FINAL / "exp2_fixed_reservation_tradeoff" / "metrics.csv")
    subset = [r for r in rows if r["N0"] == "30" and r["slac_burst"] == "20"]
    labels = [r["B_fix"] for r in subset]
    bar_plot(PAPER / "Fig3_fixed_reservation_tradeoff", labels, {"timeout": [float(r["slac_timeout_ratio"]) for r in subset], "idle/512": [float(r["reserved_but_idle_slots"]) / 512 for r in subset]}, "Normalized value (fraction)", "Fixed SLAC reservation, B_fix (slots)")
    write_plot_log("exp2_fixed_reservation_tradeoff", [PAPER / "Fig3_fixed_reservation_tradeoff"])


def plot_exp3() -> None:
    rows = read_csv(FINAL / "exp3_condA_vs_condAB" / "metrics.csv")
    by_n: dict[str, dict[str, int]] = defaultdict(lambda: {"Cond-A only": 0, "Cond-A+B": 0})
    for row in rows:
        by_n[row["N"]]["Cond-A only"] += int(row["CondA_only"] == "admit")
        by_n[row["N"]]["Cond-A+B"] += int(row["CondA_B"] == "admit")
    labels = sorted(by_n, key=int)
    bar_plot(PAPER / "Fig4_condA_vs_condAB", labels, {"Cond-A only": [by_n[n]["Cond-A only"] for n in labels], "Cond-A+B": [by_n[n]["Cond-A+B"] for n in labels]}, "Admitted cases (count)", "DC-active EVs, N (EVs)")
    write_plot_log("exp3_condA_vs_condAB", [PAPER / "Fig4_condA_vs_condAB"])


def plot_exp4() -> None:
    rows = read_csv(FINAL / "exp4_three_regime_slack" / "metrics.csv")
    apply_ieee_style()
    fig, ax = plt.subplots()
    set_ieee_size(fig)
    colors = {"hidden": 0, "paid": 1, "rejected": 2}
    xs = [int(r["N"]) for r in rows]
    ys = [int(r["K"]) for r in rows]
    cs = [colors[r["regime"]] for r in rows]
    sc = ax.scatter(xs, ys, c=cs, s=5)
    ax.set_xlabel("DC-active EVs, N (EVs)")
    ax.set_ylabel("Active SLAC sessions, K (sessions)")
    ax.grid(True, alpha=0.2)
    save(fig, PAPER / "Fig5_three_regime_heatmap")
    deg = read_csv(FINAL / "exp4_three_regime_slack" / "slack_degradation.csv")
    line_plot(PAPER / "Fig6_slack_degradation", deg, "completion_index", "delta_finish", "N0", "Finish increase (slots)")
    write_plot_log("exp4_three_regime_slack", [PAPER / "Fig5_three_regime_heatmap", PAPER / "Fig6_slack_degradation"])


def plot_exp5() -> None:
    rows = read_csv(FINAL / "exp5_plc_degradation" / "metrics.csv")
    line_plot(PAPER / "Fig7_dc_miss_under_plc_degradation", rows, "degradation_factor", "dc_deadline_miss_ratio", "algorithm", "DC deadline miss ratio (fraction)", "N0", "30")
    line_plot(PAPER / "Fig8_slac_timeout_under_plc_degradation", rows, "degradation_factor", "slac_timeout_ratio", "algorithm", "SLAC timeout ratio (fraction)", "N0", "30")
    write_plot_log("exp5_plc_degradation", [PAPER / "Fig7_dc_miss_under_plc_degradation", PAPER / "Fig8_slac_timeout_under_plc_degradation"])


def plot_exp6() -> None:
    rows = read_csv(FINAL / "exp6_heterogeneous_link_mix" / "metrics.csv")
    subset = [r for r in rows if r["N0"] == "35" and r["slac_burst"] == "15"]
    labels = sorted({r["link_mix"] for r in subset})
    series = {}
    for alg in ["count_based_proposed", "worst_case_proposed", "link_aware_proposed"]:
        series[alg] = [sum(int(r["unsafe_admission_count"]) + int(r["safe_rejection_count"]) for r in subset if r["algorithm"] == alg and r["link_mix"] == label) for label in labels]
    bar_plot(PAPER / "Fig9_heterogeneous_link_mix", labels, series, "Unsafe + safe rejection (count)", "PLC link mix (profile set)")
    write_plot_log("exp6_heterogeneous_link_mix", [PAPER / "Fig9_heterogeneous_link_mix"])


def plot_exp7() -> None:
    rows = read_csv(FINAL / "exp7_dynamic_arrival" / "metrics.csv")
    line_plot(APP / "App_dynamic_state_evolution", rows, "time_s", "N_t", "arrival_model", "DC-active EVs, N(t) (EVs)")
    line_plot(APP / "App_dynamic_arrival_summary", rows, "time_s", "channel_utilization", "arrival_model", "Channel utilization (fraction)")
    write_plot_log("exp7_dynamic_arrival", [APP / "App_dynamic_state_evolution", APP / "App_dynamic_arrival_summary"])


def main() -> None:
    for fn in [plot_exp1, plot_exp2, plot_exp3, plot_exp4]:
        fn()


if __name__ == "__main__":
    main()
