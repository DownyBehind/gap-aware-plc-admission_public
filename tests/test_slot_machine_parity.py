#!/usr/bin/env python3
"""Runs the Stage 5b physics-OFF parity harness as a test (200-cell grid)."""

from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def main() -> None:
    result = subprocess.run(
        [sys.executable, str(ROOT / "experiments" / "ns3_e1" / "run_parity.py")],
        capture_output=True, text=True)
    if result.returncode != 0:
        sys.stderr.write(result.stdout + result.stderr)
        raise AssertionError("physics-off parity harness failed")


if __name__ == "__main__":
    main()
