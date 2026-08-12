#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$ROOT/ns3/module_path.txt"
SRC="$ROOT/ns3/contrib/ev-plc-transition"
DST="$NS3_MODULE"
if [ ! -d "$SRC" ]; then
  echo "missing tracked module source: $SRC" >&2
  exit 1
fi
if [ -z "${NS3_MODULE:-}" ]; then
  echo "NS3_MODULE is not set in ns3/module_path.txt" >&2
  exit 1
fi
mkdir -p "$(dirname "$DST")"
rm -rf "$DST"
python3 - "$SRC" "$DST" <<'PY_SYNC'
import shutil, sys
from pathlib import Path
src = Path(sys.argv[1])
dst = Path(sys.argv[2])
shutil.copytree(src, dst, ignore=shutil.ignore_patterns('__pycache__', '*.pyc'))
PY_SYNC
echo "Synced $SRC -> $DST"
