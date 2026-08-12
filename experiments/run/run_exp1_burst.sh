#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
source "$ROOT/ns3/module_path.txt"
EXP="exp1_burst_sweep"
CONFIG="$ROOT/experiments/configs/$EXP.json"
OUT="$ROOT/results/$EXP"
mkdir -p "$OUT"
cp "$CONFIG" "$OUT/config.json"
if [ ! -x "$NS3_ROOT/ns3" ] || [ ! -d "$NS3_MODULE" ]; then
  echo "ERROR: ns-3 module is unavailable; fallback results are forbidden" >&2
  exit 2
fi
(cd "$NS3_ROOT" && ./ns3 run "ev-plc-transition-admission --config=$CONFIG --output=$OUT --experiment=$EXP")
python3 -c 'import json, pathlib, sys; s=json.loads(pathlib.Path("'$OUT'/summary.json").read_text()); sys.exit(0 if s.get("simulator")=="ns-3" and s.get("fallback_used") is False else 1)'
