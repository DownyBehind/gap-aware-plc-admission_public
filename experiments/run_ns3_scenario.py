#!/usr/bin/env python3
"""Launch real ns-3 final evaluation scenarios.

The launcher writes raw executable outputs under results/ns3_final/<exp>/raw/
and rejects missing or non-ns-3 summaries.
"""

from __future__ import annotations

import argparse
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parents[0]
NS3_ROOT = REPO / "ns-3-dev"
CONFIG = ROOT / "experiments" / "configs" / "common_nominal.json"
OUT_ROOT = ROOT / "results" / "ns3_final"

SCENARIOS = {
    "exp1_nominal_baseline_comparison": {"experiment": "exp1_burst_sweep", "algorithm": "all_baselines", "N0": 30, "slacBurst": 20, "periods": 40},
    "exp2_fixed_reservation_tradeoff": {"experiment": "exp2_fixed_vs_adaptive", "algorithm": "fixed_vs_adaptive", "N0": 30, "slacBurst": 20, "periods": 40},
    "exp3_condA_vs_condAB": {"experiment": "e3_condA_vs_condAB", "algorithm": "condA_ab_comparison", "N0": 36, "slacBurst": 1, "periods": 40},
    "exp4_three_regime_slack": {"experiment": "e4_three_regime_slack", "algorithm": "proposed_transition_aware", "N0": 18, "slacBurst": 2, "periods": 40},
}


def run_one(name: str, seed: int) -> None:
    scenario = SCENARIOS[name]
    out = OUT_ROOT / name / "raw" / f"seed_{seed}"
    out.mkdir(parents=True, exist_ok=True)
    config = str(CONFIG)
    command_string = (
        f"ev-plc-transition-admission --config={config} --output={out} "
        f"--experiment={scenario['experiment']} --algorithm={scenario['algorithm']} "
        f"--N0={scenario['N0']} --slacBurst={scenario['slacBurst']} "
        f"--seed={seed} --periods={scenario['periods']}"
    )
    cmd = ["./ns3", "run", command_string]
    log = out / "run.log"
    with log.open("w") as handle:
        handle.write(" ".join(cmd) + "\n")
        subprocess.run(cmd, cwd=NS3_ROOT, stdout=handle, stderr=subprocess.STDOUT, check=True)
    required = ["metrics.csv", "events.csv", "summary.json", "formula_vs_simulated.csv"]
    missing = [name for name in required if not (out / name).exists()]
    if missing:
        raise RuntimeError(f"{out} missing required ns-3 outputs: {missing}")
    summary = json.loads((out / "summary.json").read_text())
    if summary.get("result_type") != "actual_ns3_simulation" or summary.get("simulator") != "ns-3" or summary.get("fallback_used") is not False:
        raise RuntimeError(f"{out} is not a validated actual ns-3 result")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--experiments", default="exp1_nominal_baseline_comparison,exp2_fixed_reservation_tradeoff,exp3_condA_vs_condAB,exp4_three_regime_slack")
    parser.add_argument("--seed", type=int, default=1)
    args = parser.parse_args()
    for name in [item.strip() for item in args.experiments.split(",") if item.strip()]:
        if name not in SCENARIOS:
            raise SystemExit(f"unknown scenario {name}")
        run_one(name, args.seed)


if __name__ == "__main__":
    main()
