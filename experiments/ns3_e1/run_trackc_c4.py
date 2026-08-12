#!/usr/bin/env python3
"""Track C C4: event-driven E4 (final numbers) + G3 experiments.

E4: scheduled policies must be bit-identical to e4_sim on every cell where
the approximation never missed (no overrun carry); overload cells are carry
cells and must move in the event >= approx direction (same de-approximation
class as C3's backlog finding). CSMA column comes from the C3 event MAC.
G3-(b): count vs link-aware admission under heterogeneous links.
G3-(c): DC-miss burst sensitivity heatmap at constant marginal slot-PER.
G3-(a): q_wc D_g violations on the same G-E plane (discovery recording).
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
from ieee_plot_style import apply_ieee_style, CB_BLUE, CB_ORANGE, CB_GREEN, CB_PURPLE

OUT_DIR = ROOT / "results" / "ns3_e1"
NOTES = ROOT / "docs" / "ns3_parity_notes.md"
APPROX = ROOT / "ns3" / "standalone" / "build" / "e4_sim"
N0_VALUES = [0, 15, 30]
K_VALUES = [1, 2, 5, 10, 20, 35]


def examples_dir() -> Path:
    for line in (ROOT / "ns3" / "module_path.txt").read_text().splitlines():
        if line.startswith("NS3_ROOT="):
            value = line.split("=", 1)[1].strip().strip('"').replace("$ROOT", str(ROOT))
            return Path(value) / "build" / "contrib" / \
                "ev-plc-transition" / "examples"
    raise RuntimeError("NS3_ROOT not found")


def run(cmd: list[str]) -> list[dict]:
    return list(csv.DictReader(
        subprocess.run(cmd, check=True, capture_output=True, text=True).stdout.splitlines()))


def fail(stage: str, detail: list[str]) -> None:
    NOTES.write_text(f"# ns-3 parity notes — FAILURE RECORD (C4 {stage})\n\n"
                     + "\n".join(f"- {line}" for line in detail) + "\n")
    print(f"C4 HALT ({stage}) — {NOTES}", file=sys.stderr)
    sys.exit(1)


def aggregate_cells(rows: list[dict]) -> dict[tuple[int, int], dict]:
    cells: dict[tuple[int, int], dict] = {}
    for r in rows:
        cell = (int(r["N0"]), int(r["K"]))
        d = cells.setdefault(cell, {"misses": 0, "ev": 0, "dg": 0, "adm": 0, "wait": 0.0,
                                    "sessions": 0})
        d["misses"] += int(r["dc_misses"])
        d["ev"] += int(r["dc_ev_cycles"])
        d["dg"] += int(r["dg_violations"])
        d["adm"] += int(r["admitted"])
        d["wait"] += float(r["wait_sum_cycles"])
        d["sessions"] += int(r["admitted"]) + int(r["never_admitted"])
    return cells


def main() -> None:
    ex = examples_dir()
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    # ---- E4 event rerun -----------------------------------------------------
    policies = [("acbs_qwc25", "acbs", 25, 3), ("fixed_10pct", "fixed", 10, 0),
                ("fixed_25pct", "fixed", 25, 0), ("fixed_50pct", "fixed", 50, 0)]
    event_cells: dict[str, dict] = {}
    for name, pol, param, cap in policies:
        approx_rows = run([str(APPROX), pol, str(param), str(cap), "1000", "20", "120", "2"])
        event_rows = run([str(ex / "ns3-dev-trackc-e4-scheduled-optimized"),
                          f"--policy={pol}", f"--param={param}", f"--cap={cap}",
                          "--perPpm=1000", "--seeds=20", "--horizon=120",
                          "--aggCap=1"])
        # Classification: bit parity on carry-free cells; direction on carry cells.
        a_cells = aggregate_cells(approx_rows)
        e_cells = aggregate_cells(event_rows)
        a_by_row = {(r["N0"], r["K"], r["seed"]): r for r in approx_rows}
        for r in event_rows:
            key = (r["N0"], r["K"], r["seed"])
            cell = (int(r["N0"]), int(r["K"]))
            if a_cells[cell]["misses"] == 0:
                if a_by_row[key] != r:
                    fail("e4-bit", [f"{name} carry-free cell {key} differs: {a_by_row[key]} vs {r}"])
        for cell, e in e_cells.items():
            a = a_cells[cell]
            if a["misses"] > 0 and e["misses"] < a["misses"]:
                fail("e4-direction", [f"{name} carry cell {cell}: event misses {e['misses']} < approx {a['misses']}"])
        event_cells[name] = e_cells

    csma_rows = run([str(ex / "ns3-dev-trackc-e4-csma-optimized"),
                     "--perPpm=1000", "--seeds=20", "--horizon=120"])
    csma_cells: dict[tuple[int, int], dict] = {}
    for r in csma_rows:
        cell = (int(r["N0"]), int(r["K"]))
        d = csma_cells.setdefault(cell, {"misses": 0, "ev": 0, "dg": 0, "adm": 0, "wait": 0.0,
                                         "sessions": 0})
        d["misses"] += int(r["dc_misses"])
        d["ev"] += int(r["dc_ev_cycles"])
        d["dg"] += int(r["dg_violations"])
        d["adm"] += int(r["dg_violations"]) + int(r["completed"])
        d["sessions"] += int(r["dg_violations"]) + int(r["completed"])
    event_cells["hpgp_csma_ca"] = csma_cells

    # Final per-cell metrics + figure + worst-cell movement.
    metric_rows = []
    for name, cells in event_cells.items():
        for cell, d in sorted(cells.items()):
            metric_rows.append({
                "policy": name, "N0": cell[0], "K": cell[1],
                "dc_miss_rate": round(d["misses"] / d["ev"], 6) if d["ev"] else 0.0,
                "dg_violation_rate": round(d["dg"] / d["adm"], 6) if d["adm"] else 0.0,
                "admit_wait_cycles": round(d["wait"] / d["sessions"], 3) if d["sessions"] else 0.0,
            })
    csv_path = OUT_DIR / "e4_policy_comparison_event.csv"
    with csv_path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(metric_rows[0].keys()))
        writer.writeheader()
        writer.writerows(metric_rows)

    # Companion raw-count file: integer numerators and
    # denominators behind every rate above. Schema of the rate file is
    # unchanged; this is an additional artifact.
    count_rows = []
    for name, cells in event_cells.items():
        for cell, d in sorted(cells.items()):
            count_rows.append({
                "policy": name, "N0": cell[0], "K": cell[1],
                "dc_misses": d["misses"], "dc_ev_cycles": d["ev"],
                "dg_violations": d["dg"], "admitted_sessions": d["adm"],
                "sessions_total": d["sessions"],
            })
    with (OUT_DIR / "e4_policy_counts.csv").open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(count_rows[0].keys()))
        writer.writeheader()
        writer.writerows(count_rows)

    lookup = {(r["policy"], r["N0"], r["K"]): r for r in metric_rows}
    apply_ieee_style()
    styles = {"hpgp_csma_ca": ("#999999", ":", "HPGP CSMA/CA"),
              "fixed_10pct": (CB_ORANGE, "--", "Fixed 10%"),
              "fixed_25pct": (CB_GREEN, "--", "Fixed 25%"),
              "fixed_50pct": (CB_PURPLE, "--", "Fixed 50%"),
              "acbs_qwc25": (CB_BLUE, "-", "Gap-aware ($q_{wc}{=}25$)")}
    metrics = ["dc_miss_rate", "dg_violation_rate", "admit_wait_cycles"]
    ylabels = ["DC miss rate", "SLAC $D_g$ violation", "Admit wait (cycles)"]
    fig, axes = plt.subplots(3, 3, figsize=(7.0, 4.6), sharex=True)
    for col, n0 in enumerate(N0_VALUES):
        for row, metric in enumerate(metrics):
            ax = axes[row][col]
            for name, (color, ls, label) in styles.items():
                ys = [lookup[(name, n0, k)][metric] for k in K_VALUES]
                ax.plot(K_VALUES, ys, color=color, linestyle=ls, linewidth=1.1, marker="o",
                        markersize=2.4, label=label)
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

    old = {(r["policy"], r["N0"], r["K"]): r
           for r in csv.DictReader((OUT_DIR / "e4_policy_comparison.csv").open())}
    print("E4 event rerun: scheduled carry-free cells bit-identical; carry cells event >= approx.")
    print("worst-cell movement (dc_miss_rate):")
    for name in styles:
        worst_new = max((r for r in metric_rows if r["policy"] == name),
                        key=lambda r: r["dc_miss_rate"])
        old_key = (name, str(worst_new["N0"]), str(worst_new["K"]))
        old_val = float(old[old_key]["dc_miss_rate"]) if old_key in old else float("nan")
        print(f"  {name}: worst cell (N0={worst_new['N0']}, K={worst_new['K']}) "
              f"{old_val:.4f} -> {worst_new['dc_miss_rate']:.4f}")

    # ---- G3-(b) --------------------------------------------------------------
    g3b = run([str(ex / "ns3-dev-trackc-g3-optimized"), "--mode=b", "--seeds=20", "--aggCap=1"])
    print("\nG3-(b) heterogeneous links (N0=10, K=16, half GOOD / half SEVERE, G-E off):")
    for variant in ("count", "link_aware"):
        rows = [r for r in g3b if r["variant"] == variant]
        dg_sev = sum(int(r["dg_severe"]) for r in rows)
        dg_good = sum(int(r["dg_good"]) for r in rows)
        adm_sev = sum(int(r["admitted_severe"]) for r in rows)
        adm_good = sum(int(r["admitted_good"]) for r in rows)
        wait = sum(float(r["wait_sum"]) for r in rows) / max(1, sum(
            int(r["admitted"]) + int(r["never_admitted"]) for r in rows))
        miss = sum(int(r["dc_misses"]) for r in rows) / max(1, sum(int(r["dc_ev_cycles"]) for r in rows))
        print(f"  {variant}: D_g violation SEVERE {dg_sev}/{adm_sev} "
              f"({dg_sev / max(1, adm_sev):.3f}), GOOD {dg_good}/{adm_good} "
              f"({dg_good / max(1, adm_good):.3f}); wait {wait:.2f} cyc; DC miss {miss:.6f}")
    with (OUT_DIR / "g3b_link_aware.csv").open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(g3b[0].keys()))
        writer.writeheader()
        writer.writerows(g3b)

    # ---- G3-(c) --------------------------------------------------------------
    g3c = run([str(ex / "ns3-dev-trackc-g3-optimized"), "--mode=c", "--seeds=20"])
    with (OUT_DIR / "g3c_burst_sensitivity.csv").open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(g3c[0].keys()))
        writer.writeheader()
        writer.writerows(g3c)
    sojourns = [3, 15, 60, 300]
    perbads = [0.05, 0.2, 0.5]
    fig, axes = plt.subplots(1, 3, figsize=(7.0, 2.2))
    for ax, (n, k) in zip(axes, [(10, 4), (25, 4), (37, 1)]):
        grid = [[0.0] * len(sojourns) for _ in perbads]
        for r in g3c:
            if int(r["N"]) == n and int(r["K"]) == k:
                i = perbads.index(float(r["per_bad"]))
                j = sojourns.index(int(float(r["bad_sojourn"])))
                grid[i][j] = int(r["miss_cycles"]) / int(r["total_cycles"])
        im = ax.imshow(grid, aspect="auto", cmap="Blues", origin="lower", vmin=0)
        ax.set_xticks(range(len(sojourns)))
        ax.set_xticklabels(sojourns)
        ax.set_yticks(range(len(perbads)))
        ax.set_yticklabels(perbads)
        ax.set_title(f"$(N,K)=({n},{k})$", fontsize=7)
        ax.set_xlabel("mean bad sojourn (slots)")
        for i in range(len(perbads)):
            for j in range(len(sojourns)):
                ax.text(j, i, f"{grid[i][j]:.4f}", ha="center", va="center", fontsize=5,
                        color="black" if grid[i][j] < 0.5 else "white")
    axes[0].set_ylabel("$p_{bad}$ (slot PER)")
    fig.suptitle("DC miss rate under G-E bursts (marginal slot-PER held at i.i.d. baseline)",
                 fontsize=7)
    fig.tight_layout(pad=0.4)
    for ext in ("pdf", "png"):
        fig.savefig(OUT_DIR / f"fig_g3c_burst_miss.{ext}", bbox_inches="tight")
    plt.close(fig)
    boundary = [(int(r["miss_cycles"]), int(r["total_cycles"]), r) for r in g3c
                if int(r["N"]) == 37]
    worst = max(boundary, key=lambda t: t[0] / t[1])
    print(f"\nG3-(c): boundary cell (37,1) worst miss "
          f"{worst[0] / worst[1]:.5f} at sojourn={worst[2]['bad_sojourn']}, "
          f"per_bad={worst[2]['per_bad']} (same-engine i.i.d. baseline 814/10000, "
          f"g3i_iid_baseline.csv; the once-cited 4.9e-4 is the ii-b admitted-region "
          f"aggregate — different engine and population, see EVENT_COUNTS.csv)")

    # ---- G3-(a) --------------------------------------------------------------
    g3a = run([str(ex / "ns3-dev-trackc-g3-optimized"), "--mode=a", "--seeds=20", "--aggCap=1"])
    with (OUT_DIR / "g3a_qwc_burst.csv").open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(g3a[0].keys()))
        writer.writeheader()
        writer.writerows(g3a)
    print("G3-(a) q_wc=25/cap3 under G-E (N0=15, K=8):")
    for s in sojourns:
        for p in perbads:
            rows = [r for r in g3a if float(r["bad_sojourn"]) == s and float(r["per_bad"]) == p]
            v = sum(int(r["dg_violations"]) for r in rows)
            a = sum(int(r["admitted"]) for r in rows)
            if v:
                print(f"  BREAKS at sojourn={s}, per_bad={p}: {v}/{a} violations")
    total_v = sum(int(r["dg_violations"]) for r in g3a)
    if total_v == 0:
        print("  no D_g violations anywhere on the plane (q_wc holds under these bursts)")
    print(f"\ncsv: {csv_path} + g3b/g3c/g3a CSVs; fig: fig_e4_policy_comparison (event), fig_g3c_burst_miss")


if __name__ == "__main__":
    main()
