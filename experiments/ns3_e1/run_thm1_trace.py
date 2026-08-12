#!/usr/bin/env python3
"""Regenerate and gate the Theorem-1 trajectory traces (trackc-thm1).

All committed traces come from the ns-3 example `trackc-thm1` with the
aggregate SLAC window cap enabled (`--aggCap=1`, the discipline the credit
tier ships with), per-cycle trace output (`--cellCsv=1`), PER = 0, q_wc = 25 /
cap 3, offset sweep 0..40:

  * `results/ns3_e1/thm1_cell_trace.csv`        -- (36,2) start (default n0/K)
  * `results/ns3_e1/thm1_cell_trace_n35k3.csv`  -- (35,3) start
  * `results/ns3_e1/thm1_cell_trace_n34k4.csv`  -- (34,4) start

The runs are deterministic. The main trace is verified byte-identical against
the committed file and never overwritten (mismatch aborts). The two
straddle-cell traces are rewritten in place and gated on the in-cell straddle
maxima that bind the "realized straddle never exceeds 10 slots" claim:
7 at (35,3) and 10 at (34,4), with the sweep-wide maximum across all three
traces equal to 10 (< B_str = 17)."""
import csv
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT_DIR = ROOT / "results" / "ns3_e1"
MAIN_TRACE = OUT_DIR / "thm1_cell_trace.csv"
ARGS = ["--cellCsv=1", "--aggCap=1"]  # q=25, cap=3, maxOffset=40 are defaults
CELLS = [  # (n0, sessions, committed file, expected in-cell straddle max)
    (35, 3, OUT_DIR / "thm1_cell_trace_n35k3.csv", 7),
    (34, 4, OUT_DIR / "thm1_cell_trace_n34k4.csv", 10),
]


def binary() -> Path:
    for line in (ROOT / "ns3" / "module_path.txt").read_text().splitlines():
        if line.startswith("NS3_ROOT="):
            value = line.split("=", 1)[1].strip().strip('"').replace("$ROOT", str(ROOT))
            return Path(value) / "build" / "contrib" / "ev-plc-transition" / \
                "examples" / "ns3-dev-trackc-thm1-optimized"
    raise RuntimeError("NS3_ROOT not found in ns3/module_path.txt")


def trace(extra: list[str]) -> str:
    return subprocess.run([str(binary())] + ARGS + extra,
                          check=True, capture_output=True, text=True).stdout


def straddle_max(text: str, cell: tuple[int, int] | None = None) -> int:
    rows = csv.DictReader(text.splitlines())
    return max(int(r["overrun_vs_qk"]) for r in rows
               if cell is None or (int(r["N"]), int(r["K"])) == cell)


def main() -> None:
    main_out = trace([])
    if main_out != MAIN_TRACE.read_text():
        print(f"FAIL: regenerated main trace differs from {MAIN_TRACE}; "
              "committed file left untouched", file=sys.stderr)
        sys.exit(1)
    print(f"main trace byte-identical: {MAIN_TRACE}")

    sweep_max = straddle_max(main_out)
    for n0, sessions, path, expected in CELLS:
        out = trace([f"--n0={n0}", f"--sessions={sessions}"])
        got = straddle_max(out, (n0, sessions))
        assert got == expected, \
            f"({n0},{sessions}) in-cell straddle max {got} != {expected}"
        sweep_max = max(sweep_max, straddle_max(out))
        path.write_text(out)
        print(f"({n0},{sessions}) straddle max {got}: {path}")
    assert sweep_max == 10, f"sweep-wide straddle max {sweep_max} != 10"
    print("sweep-wide straddle max 10 (< B_str = 17)")


if __name__ == "__main__":
    main()
