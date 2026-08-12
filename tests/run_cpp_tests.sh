#!/usr/bin/env bash
# Shared-core rule (Track C C0): a change to the shared core must be green in
# BOTH builds -- the standalone loop always runs; the ns-3 test-runner runs
# whenever the track-B checkout is present.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

"$ROOT/ns3/standalone/build.sh"
"$ROOT/ns3/standalone/build/run_tests"

source "$ROOT/ns3/module_path.txt"
if [ -x "$NS3_ROOT/ns3" ]; then
  (cd "$NS3_ROOT" && ./ns3 build -j"$(nproc)")
  for suite in ev-plc-transition ev-plc-grant-map ev-plc-beacon-map-slot-machine \
               ev-plc-slac-completion ev-plc-processing-gap \
               ev-plc-grant-map-controller-integration hpgp-csma-ca-backoff \
               hpgp-csma-ca-collision hpgp-csma-ca-retry hpgp-csma-ca-python-reference \
               fixed-reservation map-loss-safety link-aware-admission \
               ev-specific-demand slac-sequence-model plc-error-model \
               plc-link-profile plc-site-topology; do
    "$NS3_ROOT"/build/utils/ns3-dev-test-runner-* --test-name="$suite" | tail -1
  done
else
  echo "ns-3 checkout not found at $NS3_ROOT -- standalone-only run" >&2
fi
