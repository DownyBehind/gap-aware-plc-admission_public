// NOTE: this tier retains the superseded 20-message / 241-slot SLAC
// sequence. See docs/model/slac_sequence_model.md.
// Stage 5c-ii loss-physics simulator (standalone; promotion into the module
// happens with track B). Replays multi-cycle SLAC message sequences under a
// credit window with debt carry (postponement rule) and optional frame errors.
//
// Message table: the full 20-message SLAC sequence (releases 0..775 ms),
// summing to 241 slots; the analysis uses the conservative per-session
// envelope C_slac = 247 >= 241 (see docs/model/slac_sequence_model.md).
//
// Usage: loss_sim <mode> <per_slac_ppm> <per_dc_ppm> <seeds> <cycles>
//   mode 0 (ii-0):  deterministic message-replay parity columns
//   mode 1 (ii-a):  SLAC-only PER, delta median/p95 + retx accounting
//   mode 2 (ii-b):  full PER, + DC miss rate and worst per-EV response
// IFS/beacon/PRS are 0 here: loss is isolated from overhead (documented).

#include "ns3/ev-plc-params.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

namespace
{

struct Msg
{
    uint32_t slots;
    uint32_t releaseMs;
};

// The 20-message SLAC sequence, kept as printed in an early
// draft's table (not Table I of the paper).
std::vector<Msg>
PaperSequence()
{
    std::vector<Msg> seq;
    seq.push_back({11, 0});    // SLAC_PARM.REQ
    seq.push_back({11, 15});   // SLAC_PARM.CNF
    for (uint32_t i = 0; i < 3; ++i)
    {
        seq.push_back({11, 35 + 20 * i}); // START_ATTEN_CHAR.IND x3 (35..75)
    }
    seq.push_back({11, 95});   // START_ATTEN_CHAR.RSP
    for (uint32_t i = 0; i < 10; ++i)
    {
        seq.push_back({12, 105 + 40 * i}); // MNBC_SOUND.IND x10 (105..465)
    }
    seq.push_back({18, 535});  // ATTEN_CHAR.IND (largest, 18)
    seq.push_back({12, 635});  // ATTEN_CHAR.RSP
    seq.push_back({12, 755});  // SLAC_MATCH.REQ
    seq.push_back({13, 775});  // SLAC_MATCH.CNF
    return seq;
}

struct Session
{
    uint32_t next{0};
    uint32_t startCycle{0};
};

struct CellStats
{
    std::vector<uint32_t> finishes;
    uint32_t maxOverrun{0};
    uint64_t retxSlacSlots{0};
    uint64_t retxDcSlots{0};
    uint64_t servedSlacSlots{0};
    std::vector<uint32_t> occPerCycle; // per-cycle SLAC occupancy (occ_med/occ_max columns)
    uint32_t missCycles{0};
    uint32_t worstResponse{0};
    uint32_t cycles{0};
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

CellStats
RunCell(const ns3::EvPlcParams& p, uint32_t n, uint32_t k, double perSlac, double perDc,
        uint32_t seed, uint32_t warmup, uint32_t measured)
{
    const auto seq = PaperSequence();
    const double tCtrlMs = p.m_tCtrlMs;
    std::mt19937 rng(seed * 7919 + n * 131 + k * 17);
    std::uniform_real_distribution<double> uni(0.0, 1.0);

    std::vector<Session> sessions(k);
    uint32_t debt = 0; // aggregate credit-window debt (carry)
    CellStats stats;

    for (uint32_t cycle = 0; cycle < warmup + measured; ++cycle)
    {
        uint32_t cursor = 0;
        // DC request frames (lossless in modes 0/1; retried in mode 2).
        std::vector<uint32_t> ready(n, 0);
        for (uint32_t ev = 0; ev < n; ++ev)
        {
            uint32_t end = cursor;
            while (true)
            {
                end += p.m_cReqEffSlots;
                if (perDc > 0.0 && uni(rng) < perDc)
                {
                    stats.retxDcSlots += p.m_cReqEffSlots;
                    continue;
                }
                break;
            }
            ready[ev] = end + p.m_cProcSlots;
            cursor = end;
        }

        // SLAC window: credit budget W = q*K with aggregate debt carry.
        // A message may start while consumed < allowance; the last started
        // frame may overrun (bounded by the largest message, 18 <= B_pkt).
        const uint32_t window = p.m_bAuthSlots * k;
        const uint32_t allowance = window > debt ? window - debt : 0;
        uint32_t consumed = 0;
        bool progress = true;
        while (consumed < allowance && progress)
        {
            progress = false;
            for (auto& s : sessions)
            {
                if (consumed >= allowance)
                {
                    break;
                }
                if (s.next >= seq.size())
                {
                    // Steady-state hold: completed sessions restart so the
                    // cell keeps K active sessions (documented).
                    s.next = 0;
                    s.startCycle = cycle;
                }
                const auto& msg = seq[s.next];
                const double elapsedMs = (cycle - s.startCycle) * tCtrlMs;
                if (elapsedMs < msg.releaseMs)
                {
                    continue; // not yet released
                }
                consumed += msg.slots;
                stats.servedSlacSlots += msg.slots;
                if (perSlac > 0.0 && uni(rng) < perSlac)
                {
                    stats.retxSlacSlots += msg.slots; // failed attempt burned slots
                }
                else
                {
                    s.next += 1;
                }
                progress = true;
            }
        }
        debt = consumed > allowance ? debt + consumed - window
                                    : (debt + consumed > window ? debt + consumed - window : 0);
        const uint32_t overrun = consumed > allowance ? consumed - allowance : 0;
        if (cycle >= warmup)
        {
            stats.occPerCycle.push_back(consumed);
        }
        cursor += consumed;

        // DC response frames.
        uint32_t lastEnd = cursor;
        uint32_t worstResponse = 0;
        for (uint32_t ev = 0; ev < n; ++ev)
        {
            uint32_t start = std::max(cursor, ready[ev]);
            uint32_t end = start;
            while (true)
            {
                end += p.m_cResEffSlots;
                if (perDc > 0.0 && uni(rng) < perDc)
                {
                    stats.retxDcSlots += p.m_cResEffSlots;
                    continue;
                }
                break;
            }
            worstResponse = std::max(worstResponse, end);
            cursor = end;
            lastEnd = end;
        }
        if (n == 0)
        {
            lastEnd = cursor;
        }
        const uint32_t finish = (n == 0 && consumed == 0) ? 0 : lastEnd + p.m_bBlkSlots;

        if (cycle >= warmup)
        {
            stats.finishes.push_back(finish);
            stats.maxOverrun = std::max(stats.maxOverrun, overrun);
            stats.worstResponse = std::max(stats.worstResponse, worstResponse);
            if (finish > p.GetScheduledSlots())
            {
                ++stats.missCycles;
            }
            ++stats.cycles;
        }
    }
    return stats;
}

} // namespace

int
main(int argc, char** argv)
{
    const int mode = argc > 1 ? std::atoi(argv[1]) : 0;
    const double perSlac = (argc > 2 ? std::atof(argv[2]) : 0.0) / 1e6;
    const double perDc = (argc > 3 ? std::atof(argv[3]) : 0.0) / 1e6;
    const uint32_t seeds = argc > 4 ? static_cast<uint32_t>(std::atoi(argv[4])) : 1;
    const uint32_t cycles = argc > 5 ? static_cast<uint32_t>(std::atoi(argv[5])) : 1000;
    const uint32_t warmup = 40;

    ns3::EvPlcParams params;
    const uint32_t kValues[] = {0, 1, 4, 8, 16};

    std::cout << "N,K,per_slac,per_dc,finish_med,finish_p95,finish_max,overrun_max,"
                 "retx_slac_slots,retx_dc_slots,served_slac_slots,miss_cycles,total_cycles,"
                 "worst_response,occ_med,occ_max\n";
    for (uint32_t n = 1; n <= 40; ++n)
    {
        for (const auto k : kValues)
        {
            std::vector<uint32_t> pooled;
            std::vector<uint32_t> occPooled;
            uint32_t overrunMax = 0, missCycles = 0, totalCycles = 0, worstResponse = 0;
            uint64_t retxSlac = 0, retxDc = 0, served = 0;
            const uint32_t seedCount = (perSlac > 0.0 || perDc > 0.0) ? seeds : 1;
            for (uint32_t seed = 1; seed <= seedCount; ++seed)
            {
                auto stats = RunCell(params, n, k, perSlac, perDc, seed, warmup,
                                     mode == 0 ? 200 : cycles);
                pooled.insert(pooled.end(), stats.finishes.begin(), stats.finishes.end());
                overrunMax = std::max(overrunMax, stats.maxOverrun);
                retxSlac += stats.retxSlacSlots;
                retxDc += stats.retxDcSlots;
                served += stats.servedSlacSlots;
                missCycles += stats.missCycles;
                totalCycles += stats.cycles;
                worstResponse = std::max(worstResponse, stats.worstResponse);
                occPooled.insert(occPooled.end(), stats.occPerCycle.begin(),
                                 stats.occPerCycle.end());
            }
            const uint32_t med = Percentile(pooled, 0.5);
            const uint32_t p95 = Percentile(pooled, 0.95);
            const uint32_t mx = pooled.empty() ? 0 : *std::max_element(pooled.begin(), pooled.end());
            std::cout << n << ',' << k << ',' << perSlac << ',' << perDc << ',' << med << ','
                      << p95 << ',' << mx << ',' << overrunMax << ',' << retxSlac << ','
                      << retxDc << ',' << served << ',' << missCycles << ',' << totalCycles << ','
                      << worstResponse << ',' << Percentile(occPooled, 0.5) << ','
                      << (occPooled.empty() ? 0 : *std::max_element(occPooled.begin(), occPooled.end()))
                      << "\n";
        }
    }
    return 0;
}
