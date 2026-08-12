#!/usr/bin/env bash
set -euo pipefail
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODE="${1:---core-only}"
"$DIR/run_exp1.sh"
"$DIR/run_exp2.sh"
"$DIR/run_exp3.sh"
"$DIR/run_exp4.sh"
if [ "$MODE" = "--paper" ]; then
  "$DIR/run_exp5.sh"
  "$DIR/run_exp6.sh"
elif [ "$MODE" != "--core-only" ]; then
  echo "Usage: $0 [--core-only|--paper]" >&2
  exit 2
fi
