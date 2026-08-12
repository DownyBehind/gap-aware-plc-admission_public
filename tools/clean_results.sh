#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
find "$ROOT/results" -mindepth 1 -maxdepth 1 ! -name README.md -exec rm -rf {} +
