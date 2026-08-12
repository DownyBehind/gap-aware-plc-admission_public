// Theorem-1 transition-trajectory adjudication (Section IV lock hold).
// The envelope argument breaks for q_wc in {19, 25}: C_dc - q < B_pkt, and
// the intermediate state (37,1) reached from admitting at (36,2) has
// envelope value 1399 > T under q=25. This experiment measures whether the
// REALIZED trajectory ever exceeds T: N0 = 36 DC EVs, session 1 arrives at
// cycle 0, session 2 arrives at a swept offset (adversarial interleaving of
// completion vs. the fresh session's heaviest release windows). Event
// engine, PER = 0, per-EV response finishes measured per cycle.
//
// Usage: trackc-thm1 --q=<19|25> --cap=<n> [--n0=36] [--maxOffset=40]

#include "ns3/core-module.h"
#include "ns3/ev-plc-params.h"
#include "ns3/ev-plc-policy-mac.h"
#include "ns3/plc-shared-channel.h"

#include <iostream>
#include <vector>

using namespace ns3;

int
main(int argc, char* argv[])
{
    uint32_t q = 25;
    uint32_t cap = 3;
    uint32_t n0 = 36;
    uint32_t maxOffset = 40;
    bool cellCsv = false;
    bool aggCap = false;
    uint32_t sessionsK = 2;
    CommandLine cmd(__FILE__);
    cmd.AddValue("q", "per-session budget (19 or 25)", q);
    cmd.AddValue("cap", "retry cap", cap);
    cmd.AddValue("n0", "initial DC population", n0);
    cmd.AddValue("maxOffset", "max arrival offset of session 2", maxOffset);
    cmd.AddValue("cellCsv",
                 "emit the per-cycle trajectory trace (v4.5-M) instead of the"
                 " aggregate columns",
                 cellCsv);
    cmd.AddValue("aggCap", "enable the aggregate SLAC window cap", aggCap);
    cmd.AddValue("sessions", "number of SLAC sessions (arrivals i*offset)", sessionsK);
    cmd.Parse(argc, argv);

    EvPlcParams params;
    if (cellCsv)
    {
        std::cout << "q,offset,cycle,N,K,chan_finish,finish_bblk,slac_played,"
                     "overrun_vs_qk,resp_start\n";
    }
    else
    {
        std::cout << "q,offset,max_relative_finish,at_N,at_K,over_T,dc_misses\n";
    }
    for (uint32_t offset = 0; offset <= maxOffset; ++offset)
    {
        Ptr<PlcSharedChannel> channel = Create<PlcSharedChannel>();
        Ptr<EvPlcPolicyMac> mac = Create<EvPlcPolicyMac>(params, channel);
        mac->ConfigureAcbs(q, cap);
        mac->SetAggregateCap(aggCap);
        mac->EnableCycleTrace(cellCsv);
        mac->SetErrorSource(PolicyErrorSource::INTERNAL_IID, 0.0, 1); // PER = 0
        mac->ConfigureScenario(n0, sessionsK);
        std::vector<uint32_t> arrivals;
        for (uint32_t i = 0; i < sessionsK; ++i)
        {
            arrivals.push_back(i * offset);
        }
        mac->SetSessionArrivalCycles(arrivals);
        mac->Start(120);
        Simulator::Run();
        const auto stats = mac->GetStats();
        Simulator::Destroy();
        if (cellCsv)
        {
            for (const auto& row : stats.cycleTrace)
            {
                const uint32_t budget = q * row.kActive;
                const uint32_t overrun =
                    row.slacPlayed > budget ? row.slacPlayed - budget : 0;
                std::cout << q << ',' << offset << ',' << row.cycle << ','
                          << row.n << ',' << row.kActive << ',' << row.chanFinish
                          << ',' << row.chanFinish + params.m_bBlkSlots << ','
                          << row.slacPlayed << ',' << overrun << ','
                          << row.respStart << "\n";
            }
            continue;
        }
        std::cout << q << ',' << offset << ',' << stats.maxRelativeFinish << ','
                  << stats.maxFinishN << ',' << stats.maxFinishK << ','
                  << (stats.maxRelativeFinish > params.GetScheduledSlots() ? 1 : 0) << ','
                  << stats.dcMisses << "\n";
    }
    return 0;
}
