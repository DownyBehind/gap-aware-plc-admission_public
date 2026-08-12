#!/usr/bin/env python3
"""Stage 5c-i overhead-physics verification (docs/model/physics_rules.md).

Deterministic, zero tolerance:
  (a) every cell: finish_measured == F-tilde (adjusted prediction)
  (b) every (K, IFS): empirical knee == predicted knee
  (c) IFS=0 regression: bit-identical to the 5b parity results
  (d) beacon/PRS cancel in delta: changing c_0 leaves delta unchanged
On mismatch: term-by-term breakdown to docs/ns3_parity_notes.md, abort.
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
from ieee_plot_style import apply_ieee_style, CB_BLUE, CB_ORANGE, CB_GREEN, CB_RED

from src.formulas.transition_formulas import TransitionParams

BINARY = ROOT / "ns3" / "standalone" / "build" / "overhead_dump"
BUILD = ROOT / "ns3" / "standalone" / "build.sh"
PARITY_CSV = ROOT / "results" / "ns3_e1" / "parity_knee.csv"
OUT_DIR = ROOT / "results" / "ns3_e1"
NOTES = ROOT / "docs" / "ns3_parity_notes.md"

K_VALUES = [0, 1, 4, 8, 16]
IFS_VALUES = [1, 2, 3]
BEACON_C0 = 10  # assumed value; spec confirmation deferred to Layer 3 (§3.1)
PRS = 2
IFS_STYLE = {0: (CB_BLUE, "o"), 1: (CB_ORANGE, "s"), 2: (CB_GREEN, "^"), 3: (CB_RED, "D")}


def f_tilde(n: int, k: int, ifs: int, head: int, p: TransitionParams) -> int:
    # docs/model/physics_rules.md, derived from IFS rules 1-8.
    s = head + (ifs if head > 0 else 0)
    channel = n * (p.C_req_slots + ifs) + ((p.b_slac_slots * k + p.B_pkt_slots + ifs) if k > 0 else 0)
    ready = p.C_req_slots + p.C_proc_slots
    return s + max(channel, ready) + n * p.C_res_slots + (n - 1) * ifs + p.B_blk_slots


def g_tilde(n: int, ifs: int, p: TransitionParams) -> int:
    return max(0, p.C_req_slots + p.C_proc_slots - n * (p.C_req_slots + ifs))


def x_tilde(k: int, ifs: int, p: TransitionParams) -> int:
    return p.b_slac_slots * k + p.B_pkt_slots + ifs


def run_dump(ifs: int, beacon: int, prs: int) -> dict[tuple[int, int], int]:
    if not BINARY.exists():
        subprocess.run([str(BUILD)], check=True, capture_output=True)
    out = subprocess.run([str(BINARY), str(ifs), str(beacon), str(prs)],
                         check=True, capture_output=True, text=True).stdout
    return {(int(r["N"]), int(r["K"])): int(r["finish_measured"]) for r in csv.DictReader(out.splitlines())}


def fail(coord, breakdown: list[str]) -> None:
    NOTES.write_text(
        "# ns-3 parity notes — FAILURE RECORD (Stage 5c-i)\n\n"
        f"Overhead-physics mismatch at {coord}; run aborted (§2.2 rule).\n\n"
        + "\n".join(f"- {line}" for line in breakdown) + "\n"
    )
    print(f"OVERHEAD FAILURE at {coord} — details in {NOTES}", file=sys.stderr)
    sys.exit(1)


def main() -> None:
    p = TransitionParams()
    head = BEACON_C0 + PRS
    out_rows = []
    knees: dict[tuple[int, int], tuple[int, int]] = {}  # (K, IFS) -> (empirical, predicted)

    for ifs in IFS_VALUES:
        measured = run_dump(ifs, BEACON_C0, PRS)
        for k in [kv for kv in K_VALUES if kv > 0]:
            knee_pred = next(n for n in range(1, 45) if x_tilde(k, ifs, p) > g_tilde(n, ifs, p))
            knee_emp = next((n for n in range(1, 41)
                             if measured[(n, k)] - measured[(n, 0)] > 0), None)
            if knee_emp != knee_pred:
                fail((k, ifs), [f"(b) empirical knee {knee_emp} != predicted {knee_pred} at K={k}, IFS={ifs}"])
            knees[(k, ifs)] = (knee_emp, knee_pred)
        for n in range(1, 41):
            for k in K_VALUES:
                got = measured[(n, k)]
                pred = f_tilde(n, k, ifs, head, p)
                if got != pred:
                    s = head + ifs
                    fail((n, k, ifs), [
                        f"(a) finish_measured={got}, finish_predicted={pred}",
                        f"S={s}, channel={n * (p.C_req_slots + ifs)} + slac_term, ready={295}",
                        f"terms: N(C_req+IFS)={n * (p.C_req_slots + ifs)}, "
                        f"x_tilde={x_tilde(k, ifs, p) if k else 0}, resp={n * p.C_res_slots + (n - 1) * ifs}, B_blk={p.B_blk_slots}",
                    ])
                delta = got - measured[(n, 0)]
                delta_pred = max(0, x_tilde(k, ifs, p) - g_tilde(n, ifs, p)) if k > 0 else 0
                if delta != delta_pred:
                    fail((n, k, ifs), [f"delta={delta} != delta_pred={delta_pred}"])
                ke, kp = knees.get((k, ifs), (0, 0))
                out_rows.append({"N": n, "K": k, "IFS": ifs, "finish_measured": got,
                                 "finish_predicted": pred, "delta": delta, "delta_pred": delta_pred,
                                 "knee_empirical": ke, "knee_predicted": kp})

    # (c) IFS=0 regression against the 5b parity run.
    baseline = run_dump(0, 0, 0)
    with PARITY_CSV.open() as f:
        parity = {(int(r["N"]), int(r["K"])): int(r["finish_measured"]) for r in csv.DictReader(f)}
    for coord, finish in parity.items():
        if baseline[coord] != finish:
            fail(coord, [f"(c) IFS=0 regression: {baseline[coord]} != 5b parity {finish}"])

    # (d) beacon/PRS cancellation: c_0 = 10 vs 30 must give identical deltas.
    alt = run_dump(2, 30, PRS)
    ref = run_dump(2, BEACON_C0, PRS)
    for n in range(1, 41):
        for k in K_VALUES:
            if alt[(n, k)] - alt[(n, 0)] != ref[(n, k)] - ref[(n, 0)]:
                fail((n, k), ["(d) delta changed under c_0 10 -> 30 (beacon must cancel)"])

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    csv_path = OUT_DIR / "overhead_knee.csv"
    with csv_path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(out_rows[0].keys()))
        writer.writeheader()
        writer.writerows(out_rows)

    # Figure: knee movement per IFS over K, Layer-1 baseline (IFS=0) overlaid.
    apply_ieee_style()
    fig, ax = plt.subplots(figsize=(3.5, 2.4))
    ks = [1, 4, 8, 16]
    layer1 = {1: 18, 4: 17, 8: 15, 16: 11}
    ax.plot(ks, [layer1[k] for k in ks], color=IFS_STYLE[0][0], marker=IFS_STYLE[0][1],
            markersize=3.5, linewidth=1.2, label="IFS=0 (Layer 1 baseline)")
    for ifs in IFS_VALUES:
        color, marker = IFS_STYLE[ifs]
        ax.plot(ks, [knees[(k, ifs)][0] for k in ks], color=color, marker=marker,
                markersize=3, linewidth=1.0, linestyle="--", label=f"IFS={ifs}")
    ax.set_xlabel("Active SLAC sessions, $K$")
    ax.set_ylabel("Knee position, $N$")
    ax.set_xticks(ks)
    ax.grid(True, alpha=0.2)
    ax.legend(fontsize=6, loc="upper right")
    fig.tight_layout(pad=0.3)
    outputs = []
    for ext in ("pdf", "png"):
        out = OUT_DIR / f"fig_knee_overhead_ns3.{ext}"
        fig.savefig(out, bbox_inches="tight")
        outputs.append(out)
    plt.close(fig)

    print(f"cells: {len(out_rows)} (finish and delta exact-match on all; criteria a-d hold)")
    print("knees (empirical == predicted):")
    for ifs in IFS_VALUES:
        print(f"  IFS={ifs}: " + ", ".join(f"K={k}: N={knees[(k, ifs)][0]}" for k in [1, 4, 8, 16]))
    print(f"csv: {csv_path}")
    for out in outputs:
        print(f"fig: {out}")


if __name__ == "__main__":
    main()
