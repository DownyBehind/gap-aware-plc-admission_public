#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
"$ROOT/tests/run_cpp_tests.sh"
"$ROOT/tests/run_python_tests.sh"
"$ROOT/experiments/run/run_exp1.sh"
python3 "$ROOT/experiments/analysis/plot_exp1.py" "$ROOT/scratch/exp1_csma_vs_scheduled"
python3 -c 'import json, pathlib, sys; s=json.loads(pathlib.Path("'$ROOT'/scratch/exp1_csma_vs_scheduled/summary.json").read_text()); sys.exit(0 if s.get("simulator")=="ns-3" and s.get("fallback_used") is False and s.get("formula_sim_mismatches")==0 else 1)'
