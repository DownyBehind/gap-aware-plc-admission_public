#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
source "$ROOT/ns3/module_path.txt"
CONFIG="$ROOT/experiments/configs/exp2_fixed_vs_adaptive.json"
OUT="$ROOT/results/e4_fixed_tradeoff"
mkdir -p "$OUT"
cp "$CONFIG" "$OUT/config.json"
if [ ! -x "$NS3_ROOT/ns3" ] || [ ! -d "$NS3_MODULE" ]; then echo "ERROR: ns-3 unavailable; fallback forbidden" >&2; exit 2; fi
export GIT_COMMIT="$(cd "$ROOT/.." && git rev-parse --short HEAD 2>/dev/null || echo unknown)"
(cd "$NS3_ROOT" && ./ns3 run "ev-plc-transition-admission --config=$CONFIG --output=$OUT --experiment=exp2_fixed_vs_adaptive")
python3 -c 'import json, pathlib, sys; s=json.loads(pathlib.Path("'$OUT'/summary.json").read_text()); s["experiment_name"]="e4_fixed_tradeoff"; pathlib.Path("'$OUT'/summary.json").write_text(json.dumps(s,indent=2)+"\n"); sys.exit(0 if s.get("simulator")=="ns-3" and s.get("fallback_used") is False else 1)'
