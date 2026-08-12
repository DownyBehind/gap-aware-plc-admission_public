#!/usr/bin/env bash
# Python environment setup. matplotlib is built from source against the
# SYSTEM FreeType: the wheel bundles FreeType 2.6.1, whose text metrics
# differ from the system library and break the figure bounding-box gate
# (tools/check_figures.sh). Requires libfreetype-dev and pkg-config.
#
# Usage: tools/setup_python_env.sh [venv-dir]   (default: <repo>/.venv)
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VENV="${1:-$ROOT/.venv}"

python3 -m venv "$VENV"
# shellcheck disable=SC1091
source "$VENV/bin/activate"

CFG="$(mktemp)"
printf '[libs]\nsystem_freetype = true\n' > "$CFG"
MPLSETUPCFG="$CFG" pip install --no-cache-dir -r "$ROOT/requirements.txt"
rm -f "$CFG"

python - <<'PY'
import matplotlib, matplotlib.ft2font as f, numpy
v = f.__freetype_version__
assert not v.startswith("2.6."), (
    f"matplotlib linked bundled FreeType {v}; rebuild with system_freetype "
    "(this breaks the figure bounding-box gate)")
print(f"python env ready: matplotlib {matplotlib.__version__}, "
      f"numpy {numpy.__version__}, freetype {v}")
PY
