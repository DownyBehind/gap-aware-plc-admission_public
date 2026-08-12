#!/usr/bin/env python3
"""Stage 5b physics-OFF parity harness.

Plays the E1 grid (N = 1..40, K in {0, 1, 4, 8, 16}) through the C++
BeaconMapSlotMachine (via ns3/standalone/build/parity_dump), then checks the
triple identity measured == ComputeFinishTime == cycle_builder.finish on
every cell, compares deltas against Layer 1's knee_verification.csv, and
verifies the replayed phase boundaries against cycle_builder phases.
On any mismatch it writes a term-by-term breakdown to
docs/ns3_parity_notes.md and aborts (§2.2: unexplained differences are a
parity failure).
"""

from pathlib import Path
import csv
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from src.formulas.transition_formulas import F_now, TransitionParams
from src.sim.cycle_builder import build_cycle

BINARY = ROOT / "ns3" / "standalone" / "build" / "parity_dump"
BUILD = ROOT / "ns3" / "standalone" / "build.sh"
LAYER1_CSV = ROOT / "results" / "layer1" / "knee_verification.csv"
OUT_DIR = ROOT / "results" / "ns3_e1"
NOTES = ROOT / "docs" / "ns3_parity_notes.md"
K_VALUES = [0, 1, 4, 8, 16]


def load_machine_rows() -> list[dict]:
    if not BINARY.exists():
        subprocess.run([str(BUILD)], check=True, capture_output=True)
    out = subprocess.run([str(BINARY)], check=True, capture_output=True, text=True).stdout
    return list(csv.DictReader(out.splitlines()))


def load_layer1_deltas() -> dict[tuple[int, int], int]:
    if not LAYER1_CSV.exists():
        subprocess.run([sys.executable, str(ROOT / "experiments" / "layer1" / "verify_knee_e1.py")],
                       check=True, capture_output=True)
    with LAYER1_CSV.open() as f:
        return {(int(r["N"]), int(r["K"])): int(r["delta"]) for r in csv.DictReader(f)}


def fail(coord: tuple[int, int], breakdown: list[str]) -> None:
    NOTES.write_text(
        "# ns-3 parity notes — FAILURE RECORD\n\n"
        f"Physics-OFF parity mismatch at (N, K) = {coord} ("
        "unexplained differences are treated as parity failure; run aborted).\n\n"
        + "\n".join(f"- {line}" for line in breakdown) + "\n"
    )
    print(f"PARITY FAILURE at {coord} — details in {NOTES}", file=sys.stderr)
    sys.exit(1)


def main() -> None:
    params = TransitionParams()
    machine_rows = load_machine_rows()
    layer1 = load_layer1_deltas()
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    assert len(machine_rows) == 40 * len(K_VALUES) == 200
    measured = {(int(r["N"]), int(r["K"])): r for r in machine_rows}

    out_rows = []
    for n in range(1, 41):
        for k in K_VALUES:
            row = measured[(n, k)]
            finish_measured = int(row["finish_measured"])
            finish_formula = F_now(n, k, params.C_req_slots, params.C_res_slots,
                                   params.C_proc_slots, params.b_slac_slots,
                                   params.B_pkt_slots, params.B_blk_slots)
            cycle = build_cycle(n, k, params)

            # Triple identity, zero tolerance.
            if not (finish_measured == int(row["finish_formula"]) == finish_formula == cycle.finish):
                fail((n, k), [
                    f"finish_measured (slot machine) = {finish_measured}",
                    f"finish_formula (C++ ComputeFinishTime) = {row['finish_formula']}",
                    f"finish_formula (Python F_active_state) = {finish_formula}",
                    f"finish (cycle_builder) = {cycle.finish}",
                    f"machine boundaries: req_end={row['req_end']} slac_end={row['slac_end']} "
                    f"guard_end={row['guard_end']} response_start={row['response_start']} "
                    f"last_frame_end={row['last_frame_end']}",
                    f"builder phases: {[(p.name, p.start, p.end) for p in cycle.phases]}",
                ])

            # Replayed phase boundaries must match the builder's phases.
            expect = {p.name: p for p in cycle.phases}
            checks = [("DC_REQ end", int(row["req_end"]), expect["DC_REQ"].end)]
            if k > 0:
                checks += [("SLAC end", int(row["slac_end"]), expect["SLAC_SERVICE"].end),
                           ("guard end", int(row["guard_end"]), expect["PKT_GUARD"].end)]
            checks += [("response start", int(row["response_start"]), expect["DC_RES"].start)]
            for name, got, want in checks:
                if got != want:
                    fail((n, k), [f"phase boundary '{name}': machine={got}, builder={want}"])

            delta_ns3 = finish_measured - int(measured[(n, 0)]["finish_measured"])
            delta_l1 = layer1.get((n, k), 0 if k == 0 else None)
            if delta_l1 is None or delta_ns3 != delta_l1:
                fail((n, k), [f"delta_ns3={delta_ns3} vs delta_layer1={delta_l1}"])

            out_rows.append({
                "N": n, "K": k,
                "finish_measured": finish_measured,
                "finish_formula": finish_formula,
                "finish_cycle_builder": cycle.finish,
                "delta_ns3": delta_ns3,
                "delta_layer1": delta_l1,
                "match": 1,
            })

    csv_path = OUT_DIR / "parity_knee.csv"
    with csv_path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(out_rows[0].keys()))
        writer.writeheader()
        writer.writerows(out_rows)

    print(f"parity: {len(out_rows)}/200 cells, triple identity + phase boundaries + Layer-1 deltas all match")
    print(f"csv:    {csv_path}")


if __name__ == "__main__":
    main()
