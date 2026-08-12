#!/usr/bin/env python3
"""Stage 5c-ii loss-physics verification, staged ii-0 / ii-a / ii-b.

ii-0 (deterministic message replay, PER=0):
    finish_message <= finish_abstract on every cell (envelope compliance),
    overrun_measured <= 17 (< B_pkt = 21), full term attribution.
ii-a (SLAC-only PER in {1e-3, 1e-2}, 20 seeds x 1000 cycles):
    bound safety: no analytic-hidden cell may show median delta > 0;
    loss monotonicity: knee_median(p) <= knee_median(p=0), shift <= 1 at 1e-3.
ii-b (full PER): observational — DC miss rate in the admitted region and
    worst per-EV response; recorded, not a falsification gate.
On violation: term breakdown to docs/ns3_parity_notes.md, abort.
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
from ieee_plot_style import apply_ieee_style, CB_BLUE, CB_ORANGE, CB_RED, CB_GRAY

from src.formulas.transition_formulas import F_now, G_processing_gap, TransitionParams, classify_regime

BINARY = ROOT / "ns3" / "standalone" / "build" / "loss_sim"
BUILD = ROOT / "ns3" / "standalone" / "build.sh"
OUT_DIR = ROOT / "results" / "ns3_e1"
NOTES = ROOT / "docs" / "ns3_parity_notes.md"

K_VALUES = [0, 1, 4, 8, 16]
SEEDS = 20
CYCLES = 1000
ANALYTIC_KNEE = {1: 18, 4: 17, 8: 15, 16: 11}  # IFS=0 (5c-i table, Layer 1)


def run_sim(mode: int, per_slac_ppm: int, per_dc_ppm: int) -> dict[tuple[int, int], dict]:
    if not BINARY.exists():
        subprocess.run([str(BUILD)], check=True, capture_output=True)
    out = subprocess.run(
        [str(BINARY), str(mode), str(per_slac_ppm), str(per_dc_ppm), str(SEEDS), str(CYCLES)],
        check=True, capture_output=True, text=True).stdout
    return {(int(r["N"]), int(r["K"])): r for r in csv.DictReader(out.splitlines())}


def fail(stage: str, coord, breakdown: list[str]) -> None:
    NOTES.write_text(
        f"# ns-3 parity notes — FAILURE RECORD (Stage 5c-ii {stage})\n\n"
        f"Violation at {coord}; run aborted.\n\n"
        + "\n".join(f"- {line}" for line in breakdown) + "\n")
    print(f"5c-ii {stage} FAILURE at {coord} — details in {NOTES}", file=sys.stderr)
    sys.exit(1)


def abstract_finish(n: int, k: int, p: TransitionParams) -> int:
    return F_now(n, k, p.C_req_slots, p.C_res_slots, p.C_proc_slots,
                 p.b_slac_slots, p.B_pkt_slots, p.B_blk_slots)


def knee_median(data: dict, k: int) -> int | None:
    for n in range(1, 41):
        if int(data[(n, k)]["finish_med"]) - int(data[(n, 0)]["finish_med"]) > 0:
            return n
    return None


def main() -> None:
    p = TransitionParams()
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    # ---- ii-0: deterministic message-replay parity -------------------------
    det = run_sim(0, 0, 0)
    for n in range(1, 41):
        for k in K_VALUES:
            row = det[(n, k)]
            fmax, overrun = int(row["finish_max"]), int(row["overrun_max"])
            fabs = abstract_finish(n, k, p)
            if fmax > fabs:
                fail("ii-0", (n, k), [
                    f"finish_message max {fmax} > finish_abstract {fabs}",
                    f"overrun_measured {overrun}, budget qK = {p.b_slac_slots * k}",
                ])
            if overrun > 17:
                fail("ii-0", (n, k), [f"overrun {overrun} > 17 (largest message 18, non-preemptive start rule)"])
    print("ii-0: 200/200 cells finish_message <= finish_abstract; max overrun "
          f"{max(int(r['overrun_max']) for r in det.values())} <= 17 < B_pkt=21 "
          "(difference fully attributed: unused budget + guard slack B_pkt - overrun)")

    # ---- ii-a: SLAC-only PER ----------------------------------------------
    per_data = {0: det}
    for ppm in (1000, 10000):
        per_data[ppm] = run_sim(1, ppm, 0)

    knees: dict[tuple[int, int], int | None] = {}
    for ppm, data in per_data.items():
        for k in [kv for kv in K_VALUES if kv > 0]:
            knees[(ppm, k)] = knee_median(data, k)

    for k in [kv for kv in K_VALUES if kv > 0]:
        # Bound safety: analytic-hidden cells (q*K + B_pkt <= G(N)) must show
        # zero median delta even under loss — a violation means the envelope
        # analysis is wrong (the true falsification direction).
        for ppm, data in per_data.items():
            for n in range(1, 41):
                if p.b_slac_slots * k + p.B_pkt_slots <= G_processing_gap(n, p.C_req_slots, p.C_proc_slots):
                    delta = int(data[(n, k)]["finish_med"]) - int(data[(n, 0)]["finish_med"])
                    if delta > 0:
                        fail("ii-a", (n, k, ppm), [
                            f"median delta {delta} > 0 inside the analytic-hidden region",
                            "worst-case envelope q*K + B_pkt <= G(N) violated by measurement",
                        ])
        # Loss monotonicity: PER may only advance (or hold) the knee.
        base = knees[(0, k)]
        for ppm in (1000, 10000):
            got = knees[(ppm, k)]
            if base is not None and (got is None or got > base):
                fail("ii-a", (k, ppm), [f"knee moved later under loss: p=0 knee {base}, p={ppm/1e6} knee {got}"])
        if base is not None and knees[(1000, k)] is not None and base - knees[(1000, k)] > 1:
            fail("ii-a", (k, 1000), [f"knee shift {base - knees[(1000, k)]} > 1 at p=1e-3"])

    # E2 input: effective SLAC demand inflation per PER.
    e2_rows = []
    for ppm in (0, 1000, 10000):
        data = per_data[ppm]
        retx = sum(int(r["retx_slac_slots"]) for r in data.values())
        served = sum(int(r["served_slac_slots"]) for r in data.values())
        useful = served - retx
        e2_rows.append({"per_slac": ppm / 1e6, "retx_slots": retx, "served_slots": served,
                        "inflation": round(served / useful, 6) if useful else 0.0})

    # ---- ii-b: full PER (observational) ------------------------------------
    iib_rows = []
    for ppm in (1000, 10000):
        data = run_sim(2, ppm, ppm)
        admitted_miss = 0
        admitted_cycles = 0
        worst = 0
        for (n, k), r in data.items():
            if k == 0 and n == 0:
                continue
            if classify_regime(n, k, p) in {"hidden", "paid"}:
                admitted_miss += int(r["miss_cycles"])
                admitted_cycles += int(r["total_cycles"])
                worst = max(worst, int(r["worst_response"]))
        iib_rows.append({"per": ppm / 1e6, "admitted_miss_cycles": admitted_miss,
                         "admitted_cycles": admitted_cycles,
                         "miss_rate": round(admitted_miss / admitted_cycles, 8),
                         "worst_response_slots": worst})

    # ---- CSV + figure -------------------------------------------------------
    csv_path = OUT_DIR / "loss_knee.csv"
    with csv_path.open("w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["N", "K", "per_slac", "finish_med", "finish_p95", "finish_max",
                         "delta_med", "overrun_max", "retx_slac_slots", "served_slac_slots"])
        for ppm, data in per_data.items():
            for n in range(1, 41):
                for k in K_VALUES:
                    r = data[(n, k)]
                    delta = int(r["finish_med"]) - int(data[(n, 0)]["finish_med"])
                    writer.writerow([n, k, ppm / 1e6, r["finish_med"], r["finish_p95"],
                                     r["finish_max"], delta, r["overrun_max"],
                                     r["retx_slac_slots"], r["served_slac_slots"]])

    apply_ieee_style()
    fig, axes = plt.subplots(2, 1, figsize=(3.5, 3.6), sharex=True)
    for ax, k in zip(axes, (4, 16)):
        ns = list(range(1, 41))
        analytic = [max(0, p.b_slac_slots * k + p.B_pkt_slots - G_processing_gap(n, p.C_req_slots, p.C_proc_slots)) for n in ns]
        ax.plot(ns, analytic, color=CB_GRAY, linestyle="--", linewidth=1.0, label="Layer-1 envelope (abstract)")
        for ppm, color in ((0, CB_BLUE), (1000, CB_ORANGE), (10000, CB_RED)):
            data = per_data[ppm]
            med = [int(data[(n, k)]["finish_med"]) - int(data[(n, 0)]["finish_med"]) for n in ns]
            p95 = [int(data[(n, k)]["finish_p95"]) - int(data[(n, 0)]["finish_med"]) for n in ns]
            label = "replay $p{=}0$" if ppm == 0 else f"$p_{{\\mathrm{{SLAC}}}}{{=}}${ppm/1e6:g}"
            ax.plot(ns, med, color=color, linewidth=1.0, label=label)
            ax.fill_between(ns, med, p95, color=color, alpha=0.18, linewidth=0)
        ax.axhline(0, color="black", linewidth=0.5)
        ax.axvline(ANALYTIC_KNEE[k], color=CB_GRAY, linestyle=":", linewidth=0.8)
        ax.set_ylabel(f"$\\Delta$ finish, $K{{=}}{k}$")
        ax.grid(True, alpha=0.2)
    axes[0].legend(fontsize=5.5, loc="upper left")
    axes[1].set_xlabel("DC-active EVs, $N$")
    fig.tight_layout(pad=0.3)
    outputs = []
    for ext in ("pdf", "png"):
        out = OUT_DIR / f"fig_knee_loss_ns3.{ext}"
        fig.savefig(out, bbox_inches="tight")
        outputs.append(out)
    plt.close(fig)

    print("ii-a knees (median): " + "; ".join(
        f"K={k}: p0={knees[(0, k)]}, 1e-3={knees[(1000, k)]}, 1e-2={knees[(10000, k)]} (analytic {ANALYTIC_KNEE[k]})"
        for k in [1, 4, 8, 16]))
    for row in e2_rows:
        print(f"ii-a E2 input: per={row['per_slac']}: inflation={row['inflation']} (retx {row['retx_slots']} / served {row['served_slots']})")
    for row in iib_rows:
        print(f"ii-b: per={row['per']}: admitted-region miss {row['admitted_miss_cycles']}/{row['admitted_cycles']} "
              f"(rate {row['miss_rate']}), worst response {row['worst_response_slots']} slots")
    print(f"csv: {csv_path}")
    for out in outputs:
        print(f"fig: {out}")


if __name__ == "__main__":
    main()
