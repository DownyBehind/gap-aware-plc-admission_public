#!/usr/bin/env bash
# One-shot ns-3 environment setup (track B).
#
# Clones the upstream ns-3-dev tree as a SIBLING of this repository, pins the
# commit this work was built and verified against, symlinks the tracked
# contrib module into it, and configures the build. Re-running is safe: an
# existing checkout is reused (fetch + checkout of the pinned commit).
#
# Usage: tools/setup_ns3.sh [--build]
#   --build   also compile ns-3 after configuring (takes a while)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
NS3_DIR="$ROOT/../ns-3-dev"
NS3_REMOTE="https://gitlab.com/nsnam/ns-3-dev.git"
# Upstream commit the experiments in this repository were verified against.
NS3_COMMIT="033dc84a"

if [ ! -d "$NS3_DIR/.git" ]; then
  git clone "$NS3_REMOTE" "$NS3_DIR"
fi
git -C "$NS3_DIR" fetch origin
git -C "$NS3_DIR" checkout "$NS3_COMMIT"

# Expose the tracked contrib module inside the checkout (shared source,
# no copy): ns-3 discovers contrib modules by directory.
mkdir -p "$NS3_DIR/contrib"
ln -sfn "$ROOT/ns3/contrib/ev-plc-transition" "$NS3_DIR/contrib/ev-plc-transition"

# Same configuration the published numbers were reproduced with (core +
# ev-plc-transition + tests only, optimized profile).
(cd "$NS3_DIR" && ./ns3 configure --enable-tests --enable-examples \
    --enable-modules=ev-plc-transition -d optimized)

if [ "${1:-}" = "--build" ]; then
  (cd "$NS3_DIR" && ./ns3 build -j"$(nproc)")
fi

echo "ns-3 ready at $NS3_DIR (commit $NS3_COMMIT)"
echo "verify: cd $NS3_DIR && ./ns3 build && bash $ROOT/tests/run_cpp_tests.sh"
