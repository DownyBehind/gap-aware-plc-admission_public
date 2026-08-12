// Track C C1 gate G1: event-driven scheduled MAC over the E1/E5 grids.
// Builds a real ns-3 world per cell (EVSE node + N DC-EV nodes with
// DcControlApp + K session nodes with SlacSessionApp, one PlcSharedChannel),
// runs EvPlcMac under Simulator, and prints the same CSV schema as the
// standalone reference (ns3/standalone/event_ref_dump.cc) for bit comparison.
//
//   --mode=g1  : N=1..40 x K in {0,1,4,8,16}; per-cell finish stats
//   --mode=e5  : N=1..40 x K in {0..20}, head block 21 realized, PER=0;
//                per-cell max response end (cycle-relative)
//   --perPpm, --seeds, --cycles as in the reference.

#include "ns3/core-module.h"
#include "ns3/ev-plc-apps.h"
#include "ns3/ev-plc-mac.h"
#include "ns3/ev-plc-params.h"
#include "ns3/node.h"
#include "ns3/plc-shared-channel.h"

#include <algorithm>
#include <iostream>
#include <vector>

using namespace ns3;

namespace
{

uint32_t
Percentile(std::vector<uint32_t>& values, double q)
{
    if (values.empty())
    {
        return 0;
    }
    std::sort(values.begin(), values.end());
    const std::size_t idx = static_cast<std::size_t>(q * (values.size() - 1) + 0.5);
    return values[idx];
}

std::vector<MacCycleRecord>
RunCell(const EvPlcParams& params, uint32_t n, uint32_t k, uint32_t head, double per,
        uint32_t seed, uint32_t cycles)
{
    Ptr<PlcSharedChannel> channel = Create<PlcSharedChannel>();
    channel->SetPer(per);
    channel->SetRngSeed(seed * 4409 + n * 173 + k * 19);
    Ptr<EvPlcMac> mac = Create<EvPlcMac>(params, channel);
    mac->SetHeadBlockSlots(head);

    // Real Node/Application structure: demand is registered by apps at t = 0,
    // the MAC's first cycle event is inserted afterwards (FIFO at equal time).
    std::vector<Ptr<Node>> nodes;
    Ptr<Node> evse = CreateObject<Node>();
    nodes.push_back(evse);
    for (uint32_t ev = 0; ev < n; ++ev)
    {
        Ptr<Node> node = CreateObject<Node>();
        Ptr<DcControlApp> app = CreateObject<DcControlApp>();
        app->SetMac(mac);
        node->AddApplication(app);
        app->SetStartTime(Seconds(0));
        nodes.push_back(node);
    }
    for (uint32_t j = 0; j < k; ++j)
    {
        Ptr<Node> node = CreateObject<Node>();
        Ptr<SlacSessionApp> app = CreateObject<SlacSessionApp>();
        app->SetMac(mac);
        node->AddApplication(app);
        app->SetStartTime(Seconds(0));
        nodes.push_back(node);
    }
    mac->Start(cycles);
    Simulator::Run();
    const auto records = mac->GetRecords();
    Simulator::Destroy();
    return records;
}

} // namespace

int
main(int argc, char* argv[])
{
    std::string mode = "g1";
    uint32_t perPpm = 0;
    uint32_t seeds = 1;
    uint32_t cycles = 1;
    CommandLine cmd(__FILE__);
    cmd.AddValue("mode", "g1 or e5", mode);
    cmd.AddValue("perPpm", "frame error rate in ppm (DC frames)", perPpm);
    cmd.AddValue("seeds", "seed count", seeds);
    cmd.AddValue("cycles", "cycles per seed", cycles);
    cmd.Parse(argc, argv);

    EvPlcParams params;
    const double per = perPpm / 1e6;

    if (mode == "e5")
    {
        std::cout << "N,K,max_response_end\n";
        for (uint32_t n = 1; n <= 40; ++n)
        {
            for (uint32_t k = 0; k <= 20; ++k)
            {
                const auto records = RunCell(params, n, k, params.m_bBlkSlots, 0.0, 1, 1);
                uint64_t maxEnd = 0;
                for (const auto end : records.front().responseEndSlots)
                {
                    maxEnd = std::max(maxEnd, end);
                }
                std::cout << n << ',' << k << ',' << maxEnd << "\n";
            }
        }
        return 0;
    }

    const uint32_t kValues[] = {0, 1, 4, 8, 16};
    std::cout << "N,K,seed,finish_med,finish_p95,finish_max,retx\n";
    for (uint32_t n = 1; n <= 40; ++n)
    {
        for (const auto k : kValues)
        {
            for (uint32_t seed = 1; seed <= seeds; ++seed)
            {
                const auto records = RunCell(params, n, k, 0, per, seed, cycles);
                std::vector<uint32_t> finishes;
                uint32_t retx = 0;
                for (const auto& r : records)
                {
                    finishes.push_back(r.finishSlot);
                    retx += r.retxCount;
                }
                const uint32_t med = Percentile(finishes, 0.5);
                const uint32_t p95 = Percentile(finishes, 0.95);
                const uint32_t mx = *std::max_element(finishes.begin(), finishes.end());
                std::cout << n << ',' << k << ',' << seed << ',' << med << ',' << p95 << ','
                          << mx << ',' << retx << "\n";
            }
        }
    }
    return 0;
}
