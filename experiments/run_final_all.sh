#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODE="${1:---core}"
CONTINUE_ON_FAIL=0

if [ "${2:-}" = "--continue-on-fail" ] || [ "${1:-}" = "--continue-on-fail" ]; then
  CONTINUE_ON_FAIL=1
fi

case "$MODE" in
  --clean|--core|--full|--figures-only|--continue-on-fail) ;;
  *) echo "usage: $0 [--clean|--core|--full|--figures-only] [--continue-on-fail]" >&2; exit 2 ;;
esac

run_step() {
  local name="$1"
  local log_file="$2"
  shift
  shift
  echo "== $name =="
  mkdir -p "$(dirname "$log_file")"
  if "$@" >"$log_file" 2>&1; then
    cat "$log_file"
  else
    cat "$log_file"
    if [ "$CONTINUE_ON_FAIL" -eq 1 ]; then
      echo "WARNING: step failed but continuing: $name" >&2
    else
      exit 1
    fi
  fi
}

clean_outputs() {
  rm -rf "$ROOT/results/ns3_final" "$ROOT/results/paper_figures" "$ROOT/results/paper_tables" "$ROOT/results/appendix_figures"
  mkdir -p "$ROOT/results/ns3_final" "$ROOT/results/paper_figures" "$ROOT/results/paper_tables" "$ROOT/results/appendix_figures"
  echo "Cleaned final output folders (ns3_final, paper_figures, paper_tables, appendix_figures)."
}

if [ "$MODE" = "--clean" ]; then
  clean_outputs
  exit 0
fi

cd "$ROOT"
mkdir -p "$ROOT/results/ns3_final"
FINAL_RUN_LOG="$ROOT/results/ns3_final/final_run.log"
{
  echo ""
  echo "## $(date -Iseconds)"
  echo "mode=$MODE"
  echo "command=$0 $*"
} >> "$FINAL_RUN_LOG"
exec > >(tee -a "$FINAL_RUN_LOG") 2>&1

if [ "$MODE" != "--figures-only" ]; then
  run_step "tests" "$ROOT/results/ns3_final/test.log" "$ROOT/tests/run_all_tests.sh"
fi

if [ "$MODE" = "--core" ] || [ "$MODE" = "--full" ]; then
  run_step "run real ns-3 scenarios" "$ROOT/results/ns3_final/ns3_scenarios.log" python3 "$ROOT/experiments/run_ns3_scenario.py"
  run_step "aggregate real ns-3 data" "$ROOT/results/ns3_final/aggregation.log" python3 "$ROOT/experiments/analysis/aggregate_ns3_final_results.py"
fi

run_step "generate final figures" "$ROOT/results/ns3_final/plot.log" python3 "$ROOT/experiments/analysis/plot_final_eval_figures.py"
