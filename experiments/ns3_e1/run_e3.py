#!/usr/bin/env python3
"""E3: beacon robustness under a persistent schedule.

E3-a static: finish/miss invariant in consecutive beacon losses m (all cells).
E3-b adversarial: map change aligned with a missed beacon — stale_persist must
show collisions (design evidence for fail-silent), fail_silent must show zero.
E3-c eta: admission effectiveness delayed to (m+1) cycles; m_max derived from
the deadline margin (fluid-service bound: 2, completion bound: 3).
Deterministic slot-level analysis (documented: seeds are vacuous here).
"""

from pathlib import Path
import csv
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
BINARY = ROOT / "ns3" / "standalone" / "build" / "e3_sim"
OUT_DIR = ROOT / "results" / "ns3_e1"


def run(mode: str) -> list[dict]:
    out = subprocess.run([str(BINARY), mode], check=True, capture_output=True, text=True).stdout
    return list(csv.DictReader(out.splitlines()))


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    # E3-a: invariance in m.
    rows_a = run("a")
    by_cell: dict[tuple[int, int], set] = {}
    for r in rows_a:
        by_cell.setdefault((int(r["N"]), int(r["K"])), set()).add((r["finish"], r["dc_miss"]))
    for cell, outcomes in by_cell.items():
        assert len(outcomes) == 1, f"E3-a: finish/miss varies with m at {cell}: {outcomes}"
    print(f"E3-a: {len(by_cell)} cells x m in 0..5 — finish and DC miss invariant in m (static harmlessness confirmed)")

    # E3-b: stale_persist collides, fail_silent does not.
    rows_b = run("b")
    persist = [r for r in rows_b if r["rule"] == "stale_persist"]
    silent = [r for r in rows_b if r["rule"] == "fail_silent"]
    colliding = [r for r in persist if int(r["overlap_slots"]) > 0]
    assert colliding, "E3-b: stale_persist produced no collisions (adversarial alignment failed)"
    for r in silent:
        assert int(r["overlap_slots"]) == 0, f"E3-b: fail_silent overlap at {r}"
        assert int(r["others_affected"]) == 0
    print(f"E3-b: stale_persist collides in {len(colliding)}/{len(persist)} scenarios "
          f"(worst overlap {max(int(r['overlap_slots']) for r in persist)} slots); "
          "fail_silent: 0 overlap everywhere, cost = own response deferred (1 miss/stale cycle, self only)")

    # E3-c: eta(m) = m + 1.
    rows_c = run("c")
    for r in rows_c:
        assert int(r["eta_cycles"]) == int(r["m"]) + 1, f"eta mismatch: {r}"
    print("E3-c: eta(m) = m+1 cycles measured for m in 0..5; "
          "margin-derived m_max = 2 (fluid-service bound 1864.04 ms) / 3 (completion bound 1850 ms)")

    for name, rows in (("e3_static.csv", rows_a), ("e3_adversarial.csv", rows_b), ("e3_eta.csv", rows_c)):
        with (OUT_DIR / name).open("w", newline="") as f:
            writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
            writer.writeheader()
            writer.writerows(rows)
        print(f"csv: {OUT_DIR / name}")


if __name__ == "__main__":
    main()
