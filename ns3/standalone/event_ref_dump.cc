// G1 reference for the Track C event-driven MAC: the same serialized replay
// (head -> DC_REQ x N -> SLAC budget block -> PKT_GUARD -> gated DC_RES x N),
// the same RNG seed formula and roll order (one roll per DC frame attempt at
// frame end), the same carry-across-cycles cursor. If the event MAC and this
// reference disagree bit-wise, the difference is an event-ordering effect —
// judged by the pre-registered criteria in docs/model/physics_rules.md.
//
// Usage: event_ref_dump <mode:g1|e5> <per_ppm> <seeds> <cycles>

#include "ns3/ev-plc-params.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace
{

struct CycleOut
{
    uint32_t finish{0};
    uint32_t retx{0};
    uint64_t maxResponseEnd{0};
};

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

std::vector<CycleOut>
RunCell(const ns3::EvPlcParams& p, uint32_t n, uint32_t k, uint32_t head, double per,
        uint32_t seed, uint32_t cycles)
{
    std::mt19937 rng(seed * 4409 + n * 173 + k * 19);
    std::uniform_real_distribution<double> uni(0.0, 1.0);
    auto fails = [&]() { return per > 0.0 && uni(rng) < per; };

    std::vector<CycleOut> outs;
    uint64_t cursor = 0;
    for (uint32_t cycle = 0; cycle < cycles; ++cycle)
    {
        const uint64_t cycleStart = static_cast<uint64_t>(cycle) * p.m_tCtrlSlots;
        cursor = std::max(cursor, cycleStart);
        CycleOut out;
        if (head > 0)
        {
            cursor += head; // envelope, no PER roll
        }
        std::vector<uint64_t> ready(n, 0);
        uint64_t lastEnd = 0;
        for (uint32_t ev = 0; ev < n; ++ev)
        {
            uint64_t end = cursor + p.m_cReqEffSlots;
            while (fails())
            {
                ++out.retx;
                end += p.m_cReqEffSlots;
            }
            ready[ev] = end + p.m_cProcSlots;
            cursor = end;
            lastEnd = end;
        }
        if (k > 0)
        {
            cursor += p.m_bAuthSlots * k; // abstract budget block, no roll
            lastEnd = cursor;
            cursor += p.m_bPktSlots; // guard envelope, no roll
            lastEnd = cursor;
        }
        for (uint32_t ev = 0; ev < n; ++ev)
        {
            uint64_t start = std::max(cursor, ready[ev]);
            uint64_t end = start + p.m_cResEffSlots;
            while (fails())
            {
                ++out.retx;
                end += p.m_cResEffSlots;
            }
            out.maxResponseEnd = std::max(out.maxResponseEnd, end);
            cursor = end;
            lastEnd = end;
        }
        if (lastEnd > 0)
        {
            out.finish = static_cast<uint32_t>(lastEnd - cycleStart + p.m_bBlkSlots);
        }
        outs.push_back(out);
    }
    return outs;
}

} // namespace

int
main(int argc, char** argv)
{
    const std::string mode = argc > 1 ? argv[1] : "g1";
    const double per = (argc > 2 ? std::atof(argv[2]) : 0.0) / 1e6;
    const uint32_t seeds = argc > 3 ? static_cast<uint32_t>(std::atoi(argv[3])) : 1;
    const uint32_t cycles = argc > 4 ? static_cast<uint32_t>(std::atoi(argv[4])) : 1;

    ns3::EvPlcParams params;

    if (mode == "e5")
    {
        std::cout << "N,K,max_response_end\n";
        for (uint32_t n = 1; n <= 40; ++n)
        {
            for (uint32_t k = 0; k <= 20; ++k)
            {
                const auto outs = RunCell(params, n, k, params.m_bBlkSlots, 0.0, 1, 1);
                std::cout << n << ',' << k << ',' << outs.front().maxResponseEnd << "\n";
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
                const auto outs = RunCell(params, n, k, 0, per, seed, cycles);
                std::vector<uint32_t> finishes;
                uint32_t retx = 0;
                for (const auto& o : outs)
                {
                    finishes.push_back(o.finish);
                    retx += o.retx;
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
