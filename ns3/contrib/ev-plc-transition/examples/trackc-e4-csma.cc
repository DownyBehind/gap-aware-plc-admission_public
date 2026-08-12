// Track C C3: event-driven CSMA over the E4 grid — same scenario semantics
// and output schema as e4_sim's csma policy, minus the per-cycle resync
// approximation (contention state and backlog persist across cycles).
// Compared cell-by-cell by experiments/ns3_e1/run_trackc_c3.py under the
// pre-registered criteria (docs/model/physics_rules.md (CSMA rules)).

#include "ns3/core-module.h"
#include "ns3/ev-plc-csma-mac.h"
#include "ns3/ev-plc-params.h"
#include "ns3/plc-shared-channel.h"

#include <iostream>

using namespace ns3;

int
main(int argc, char* argv[])
{
    uint32_t perPpm = 1000;
    uint32_t seeds = 20;
    uint32_t horizon = 120;
    int singleN0 = -1; // single-cell mode (e.g. the (30,0) adjudication)
    int singleK = -1;
    CommandLine cmd(__FILE__);
    cmd.AddValue("perPpm", "frame error rate in ppm", perPpm);
    cmd.AddValue("seeds", "seed count", seeds);
    cmd.AddValue("horizon", "cycles per run", horizon);
    cmd.AddValue("singleN0", "run a single cell: N0", singleN0);
    cmd.AddValue("singleK", "run a single cell: K", singleK);
    cmd.Parse(argc, argv);

    EvPlcParams params;
    std::vector<uint32_t> n0Values{0, 15, 30};
    std::vector<uint32_t> kValues{1, 2, 5, 10, 20, 35};
    if (singleN0 >= 0 && singleK >= 0)
    {
        n0Values = {static_cast<uint32_t>(singleN0)};
        kValues = {static_cast<uint32_t>(singleK)};
    }

    std::cout << "N0,K,seed,dg_violations,completed,dc_misses,dc_ev_cycles,collisions,drops\n";
    for (const auto n0 : n0Values)
    {
        for (const auto k : kValues)
        {
            for (uint32_t seed = 1; seed <= seeds; ++seed)
            {
                Ptr<PlcSharedChannel> channel = Create<PlcSharedChannel>();
                channel->SetPer(perPpm / 1e6);
                channel->SetRngSeed(seed * 7727 + n0 * 349 + k * 41);
                Ptr<EvPlcCsmaMac> mac = Create<EvPlcCsmaMac>(params, channel);
                mac->SetBackoffRngSeed(seed * 100003 + n0 * 17 + k * 5);
                mac->ConfigureScenario(n0, k);
                mac->Start(horizon);
                Simulator::Run();
                const auto stats = mac->GetStats();
                Simulator::Destroy();
                std::cout << n0 << ',' << k << ',' << seed << ',' << stats.dgViolations << ','
                          << stats.completedSessions << ',' << stats.dcMisses << ','
                          << stats.dcEvCycles << ',' << stats.collisions << ',' << stats.drops
                          << "\n";
            }
        }
    }
    return 0;
}
