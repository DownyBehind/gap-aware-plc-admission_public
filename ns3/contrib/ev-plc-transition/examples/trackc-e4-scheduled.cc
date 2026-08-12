// Track C C4: event-driven scheduled policies (fixed / gap-aware) over the E4
// grid, INTERNAL_IID error source with e4_sim's seed formula and roll order.
// Bit parity with e4_sim is expected on every cell where the approximation
// never overran T (admitted regime); overload cells may differ because the
// slot-machine runner reset its cycle cursor (discarding overrun carry) —
// classified by the harness, direction-checked.
//
// Usage flags: --policy=fixed|acbs --param=<alpha%|q> --cap=<n> --perPpm --seeds --horizon

#include "ns3/core-module.h"
#include "ns3/ev-plc-params.h"
#include "ns3/ev-plc-policy-mac.h"
#include "ns3/plc-shared-channel.h"

#include <iostream>
#include <string>

using namespace ns3;

int
main(int argc, char* argv[])
{
    std::string policy = "acbs";
    uint32_t param = 25;
    uint32_t cap = 3;
    uint32_t perPpm = 1000;
    uint32_t seeds = 20;
    uint32_t horizon = 120;
    bool slackCsv = false;
    bool condB = true;
    bool ablationCsv = false;
    bool aggCap = false;
    CommandLine cmd(__FILE__);
    cmd.AddValue("condB", "enable Cond B (ablation: 0 = Cond A only)", condB);
    cmd.AddValue("ablationCsv",
                 "emit ablation columns (adds max_n) instead of the default schema",
                 ablationCsv);
    cmd.AddValue("policy", "fixed or acbs", policy);
    cmd.AddValue("param", "alpha percent (fixed) or q (acbs)", param);
    cmd.AddValue("cap", "retry cap (acbs)", cap);
    cmd.AddValue("perPpm", "frame error rate in ppm", perPpm);
    cmd.AddValue("seeds", "seed count", seeds);
    cmd.AddValue("horizon", "cycles", horizon);
    cmd.AddValue("slackCsv",
                 "emit slack-occupancy columns instead of the default schema",
                 slackCsv);
    cmd.AddValue("aggCap", "enable the aggregate SLAC window cap", aggCap);
    cmd.Parse(argc, argv);

    EvPlcParams params;
    const uint32_t n0Values[] = {0, 15, 30};
    const uint32_t kValues[] = {1, 2, 5, 10, 20, 35};

    if (ablationCsv)
    {
        std::cout << "policy,param,N0,K,seed,admitted,never_admitted,"
                     "wait_sum_cycles,dg_violations,completed,dc_misses,"
                     "dc_ev_cycles,max_n\n";
    }
    else if (slackCsv)
    {
        std::cout << "policy,param,N0,K,seed,total_cycles,low_slack_cycles,"
                     "low_slack_ev_cycles,low_slack2_cycles,low_slack2_ev_cycles,"
                     "dc_ev_cycles,dc_misses,dc_misses_low_slack,dc_misses_low_slack2,"
                     "min_plan_slack\n";
    }
    else
    {
        std::cout << "policy,param,N0,K,seed,admitted,never_admitted,wait_sum_cycles,"
                     "dg_violations,completed,dc_misses,dc_ev_cycles\n";
    }
    for (const auto n0 : n0Values)
    {
        for (const auto k : kValues)
        {
            for (uint32_t seed = 1; seed <= seeds; ++seed)
            {
                Ptr<PlcSharedChannel> channel = Create<PlcSharedChannel>();
                Ptr<EvPlcPolicyMac> mac = Create<EvPlcPolicyMac>(params, channel);
                if (policy == "fixed")
                {
                    mac->SetAggregateCap(aggCap);
                    mac->ConfigureFixed(params.GetScheduledSlots() * param / 100.0);
                }
                else
                {
                    mac->SetAggregateCap(aggCap), mac->ConfigureAcbs(param, cap);
                }
                mac->SetCondBEnabled(condB);
                mac->SetErrorSource(PolicyErrorSource::INTERNAL_IID, perPpm / 1e6,
                                    seed * 9391 + n0 * 449 + k * 37);
                mac->ConfigureScenario(n0, k);
                mac->Start(horizon);
                Simulator::Run();
                const auto stats = mac->GetStats();
                Simulator::Destroy();
                if (ablationCsv)
                {
                    std::cout << policy << ',' << param << ',' << n0 << ',' << k
                              << ',' << seed << ',' << stats.admitted << ','
                              << stats.neverAdmitted << ',' << stats.waitSumCycles
                              << ',' << stats.dgViolations << ',' << stats.completed
                              << ',' << stats.dcMisses << ',' << stats.dcEvCycles
                              << ',' << stats.maxNDc << "\n";
                }
                else if (slackCsv)
                {
                    std::cout << policy << ',' << param << ',' << n0 << ',' << k << ',' << seed
                              << ',' << stats.totalCycles << ',' << stats.lowSlackCycles << ','
                              << stats.lowSlackEvCycles << ',' << stats.lowSlack2Cycles << ','
                              << stats.lowSlack2EvCycles << ',' << stats.dcEvCycles << ','
                              << stats.dcMisses << ',' << stats.dcMissesLowSlack << ','
                              << stats.dcMissesLowSlack2 << ',' << stats.minPlanSlack << "\n";
                }
                else
                {
                    std::cout << policy << ',' << param << ',' << n0 << ',' << k << ',' << seed
                              << ',' << stats.admitted << ',' << stats.neverAdmitted << ','
                              << stats.waitSumCycles << ',' << stats.dgViolations << ','
                              << stats.completed << ',' << stats.dcMisses << ','
                              << stats.dcEvCycles << "\n";
                }
            }
        }
    }
    return 0;
}
