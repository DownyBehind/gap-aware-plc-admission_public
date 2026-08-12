#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main() -> None:
    runner = (ROOT / "experiments" / "run_final_all.sh").read_text()
    assert "generate_final_eval_data.py" not in runner
    assert "run_ns3_scenario.py" in runner
    assert "results/ns3_final" in runner
    assert (ROOT / "experiments" / "run_ns3_scenario.py").exists()
    assert (ROOT / "experiments" / "analysis" / "aggregate_ns3_final_results.py").exists()


if __name__ == "__main__":
    main()
