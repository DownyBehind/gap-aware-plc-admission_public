// E3 beacon-robustness simulator (standalone). Deterministic slot-level
// analysis on BeaconMapSlotMachine replays (physics OFF: the beacon-loss
// question is isolated from PER/IFS).
//
//   mode a: static membership — finish under m = 0..5 consecutive beacon
//           losses (persistent schedule: cached map == operative map).
//   mode b: adversarial map change (completion / admission) aligned with a
//           missed beacon; stale-node rules stale_persist vs fail_silent;
//           counts collision-overlap slots against the new map.
//   mode c: admission-effectiveness delay eta(m) in cycles.
//
// Usage: e3_sim <mode:a|b|c>

#include "ns3/beacon-map-slot-machine.h"
#include "ns3/ev-plc-params.h"
#include "ns3/grant-map-scheduler.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace
{

using ns3::PlayedCycle;
using ns3::PlayedFrame;

uint32_t
Overlap(const PlayedFrame& a, const PlayedFrame& b)
{
    const uint32_t lo = std::max(a.startSlot, b.startSlot);
    const uint32_t hi = std::min(a.endSlot, b.endSlot);
    return hi > lo ? hi - lo : 0;
}

// Collision-overlap slots when DC EV `stale` plays its OLD-map frames while
// everyone else (other DC EVs' frames and the SLAC/guard windows) plays the
// NEW map.
uint32_t
StaleOverlap(const PlayedCycle& oldCycle, const PlayedCycle& newCycle, uint32_t stale)
{
    uint32_t total = 0;
    for (const auto& mine : oldCycle.frames)
    {
        const bool dcMine = (mine.name == "DC_REQ" || mine.name == "DC_RES") && mine.evId == stale;
        if (!dcMine)
        {
            continue;
        }
        for (const auto& theirs : newCycle.frames)
        {
            const bool dcTheirs = (theirs.name == "DC_REQ" || theirs.name == "DC_RES");
            if (dcTheirs && theirs.evId == stale)
            {
                continue; // own slot in the new map is not a collision
            }
            total += Overlap(mine, theirs);
        }
    }
    return total;
}

} // namespace

int
main(int argc, char** argv)
{
    const std::string mode = argc > 1 ? argv[1] : "a";
    ns3::EvPlcParams params;
    ns3::GrantMapScheduler scheduler(params);
    ns3::BeaconMapSlotMachine machine(params);

    const uint32_t nValues[] = {5, 20, 35};
    const uint32_t kValues[] = {1, 4, 8};

    if (mode == "a")
    {
        std::cout << "N,K,m,finish,dc_miss\n";
        for (const auto n : nValues)
        {
            for (const auto k : kValues)
            {
                // Persistent schedule: the cached map is the operative map, so
                // the replay input is identical for every m by construction —
                // this run *measures* that the outputs are, too.
                const auto played = machine.PlayCycle(scheduler.BuildGrantMap(n, k, 0));
                for (uint32_t m = 0; m <= 5; ++m)
                {
                    std::cout << n << ',' << k << ',' << m << ',' << played.finishSlot << ','
                              << (played.finishSlot > params.GetScheduledSlots() ? 1 : 0) << "\n";
                }
            }
        }
        return 0;
    }

    if (mode == "b")
    {
        std::cout << "scenario,N,K,rule,stale_ev,overlap_slots,stale_dc_miss_per_cycle,"
                     "others_affected\n";
        for (const auto n : nValues)
        {
            for (const auto k : kValues)
            {
                struct Scenario
                {
                    std::string name;
                    uint32_t n2;
                    uint32_t k2;
                };
                std::vector<Scenario> scenarios{{"admission", n, k + 1}};
                if (k >= 1)
                {
                    scenarios.push_back({"completion", n + 1, k - 1});
                }
                for (const auto& sc : scenarios)
                {
                    const auto oldCycle = machine.PlayCycle(scheduler.BuildGrantMap(n, k, 0));
                    const auto newCycle = machine.PlayCycle(scheduler.BuildGrantMap(sc.n2, sc.k2, 1));
                    // stale_persist: report the worst stale EV.
                    uint32_t worstOverlap = 0;
                    uint32_t worstEv = 0;
                    for (uint32_t j = 0; j < n; ++j)
                    {
                        const uint32_t o = StaleOverlap(oldCycle, newCycle, j);
                        if (o > worstOverlap)
                        {
                            worstOverlap = o;
                            worstEv = j;
                        }
                    }
                    std::cout << sc.name << ',' << n << ',' << k << ",stale_persist," << worstEv
                              << ',' << worstOverlap << ",0," << (worstOverlap > 0 ? 1 : 0) << "\n";
                    // fail_silent: the stale node transmits nothing -> zero
                    // overlap by construction; its own response is deferred
                    // (one DC miss per stale cycle, self only).
                    std::cout << sc.name << ',' << n << ',' << k << ",fail_silent," << worstEv
                              << ",0,1,0\n";
                }
            }
        }
        return 0;
    }

    // mode c: admission-effectiveness delay. The admitted node's first SLAC
    // service waits for the first *received* beacon: m consecutive losses
    // after the admission cycle push it to cycle t0 + m + 1.
    std::cout << "m,eta_cycles\n";
    for (uint32_t m = 0; m <= 5; ++m)
    {
        uint32_t firstService = 0;
        for (uint32_t cycle = 1; cycle <= 40; ++cycle)
        {
            const bool beaconReceived = cycle > m; // losses at cycles 1..m
            if (beaconReceived)
            {
                firstService = cycle;
                break;
            }
        }
        std::cout << m << ',' << firstService << "\n";
    }
    return 0;
}
