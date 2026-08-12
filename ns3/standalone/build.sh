#!/usr/bin/env bash
# Standalone build: compiles the ev-plc-transition module and
# its TestSuites without an ns-3 checkout, using the stub headers in
# include/ns3. Outputs land in build/. Numbers produced here are labeled
# "slot-accurate simulator"; the real ns-3 build is track B.
set -euo pipefail
cd "$(dirname "$0")"

MODULE=../contrib/ev-plc-transition
mkdir -p include/ns3 build

# Forward the module's own headers so `#include "ns3/x.h"` resolves.
for h in "$MODULE"/model/*.h; do
    ln -sf "../../$MODULE/model/$(basename "$h")" "include/ns3/$(basename "$h")"
done
for h in "$MODULE"/helper/*.h; do
    ln -sf "../../$MODULE/helper/$(basename "$h")" "include/ns3/$(basename "$h")"
done

CXX=${CXX:-g++}
FLAGS=(-std=c++20 -O2 -Wall -Iinclude)
# Event-only classes need Simulator/Node and are excluded here; the shared
# core (schedulers, admission, formulas) stays identical in both builds
# (build-list separation, no #ifdef -- TRACKC_PLAN 1.2).
EVENT_ONLY="plc-shared-channel ev-plc-mac ev-plc-csma-mac ev-plc-policy-mac ev-plc-apps"
SRCS=()
for f in "$MODULE"/model/*.cc "$MODULE"/helper/*.cc; do
    base=$(basename "$f" .cc)
    skip=0
    for e in $EVENT_ONLY; do [ "$base" = "$e" ] && skip=1; done
    [ $skip -eq 0 ] && SRCS+=("$f")
done

# Test files for event-only classes are likewise excluded from standalone.
TEST_SRCS=()
for f in "$MODULE"/test/*.cc; do
    base=$(basename "$f" .cc)
    case "$base" in plc-shared-channel-test|trackc-dual-mode-test) continue ;; esac
    TEST_SRCS+=("$f")
done
"$CXX" "${FLAGS[@]}" "${SRCS[@]}" "${TEST_SRCS[@]}" runner_main.cc -o build/run_tests
"$CXX" "${FLAGS[@]}" "${SRCS[@]}" "$MODULE"/examples/ev-plc-transition-admission.cc -o build/ev-plc-sim
"$CXX" "${FLAGS[@]}" "${SRCS[@]}" regime_grid.cc -o build/regime_grid
"$CXX" "${FLAGS[@]}" "${SRCS[@]}" parity_dump.cc -o build/parity_dump
"$CXX" "${FLAGS[@]}" "${SRCS[@]}" overhead_dump.cc -o build/overhead_dump
"$CXX" "${FLAGS[@]}" "${SRCS[@]}" loss_sim.cc -o build/loss_sim
"$CXX" "${FLAGS[@]}" "${SRCS[@]}" e2_sim.cc -o build/e2_sim
"$CXX" "${FLAGS[@]}" "${SRCS[@]}" e3_sim.cc -o build/e3_sim
"$CXX" "${FLAGS[@]}" "${SRCS[@]}" e4_sim.cc -o build/e4_sim
"$CXX" "${FLAGS[@]}" "${SRCS[@]}" e5_sim.cc -o build/e5_sim
"$CXX" "${FLAGS[@]}" "${SRCS[@]}" event_ref_dump.cc -o build/event_ref_dump

echo "built: build/run_tests build/ev-plc-sim build/regime_grid build/parity_dump build/overhead_dump build/loss_sim build/e2_sim build/e3_sim build/e4_sim build/e5_sim build/event_ref_dump"
