#!/usr/bin/env python3
"""Full-grid Python/C++ regime semantics parity (Stage 5a).

Runs the standalone-built C++ regime_grid dumper (real GrantMapScheduler::
ClassifyRegime, not a transcription) and compares every (N, K) cell against
src/formulas/transition_formulas.classify_regime. Builds the standalone
targets on first use.
"""

from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from src.formulas.transition_formulas import TransitionParams, classify_regime

BINARY = ROOT / "ns3" / "standalone" / "build" / "regime_grid"
BUILD = ROOT / "ns3" / "standalone" / "build.sh"


def main() -> None:
    if not BINARY.exists():
        subprocess.run([str(BUILD)], check=True, capture_output=True)
    out = subprocess.run([str(BINARY)], check=True, capture_output=True, text=True).stdout
    p = TransitionParams()
    lines = out.strip().splitlines()
    assert lines[0] == "N,K,regime"
    rows = [line.split(",") for line in lines[1:]]
    assert len(rows) == 46 * 21
    for n_str, k_str, cpp_regime in rows:
        n, k = int(n_str), int(k_str)
        py_regime = classify_regime(n, k, p)
        assert cpp_regime == py_regime, f"regime mismatch at (N={n}, K={k}): C++={cpp_regime}, Python={py_regime}"


if __name__ == "__main__":
    main()
