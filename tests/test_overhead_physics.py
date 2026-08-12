#!/usr/bin/env python3
"""Runs the Stage 5c-i overhead-physics harness (criteria a-d) as a test."""

from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def main() -> None:
    result = subprocess.run(
        [sys.executable, str(ROOT / "experiments" / "ns3_e1" / "run_overhead.py")],
        capture_output=True, text=True)
    if result.returncode != 0:
        sys.stderr.write(result.stdout + result.stderr)
        raise AssertionError("overhead-physics harness failed")


if __name__ == "__main__":
    main()
