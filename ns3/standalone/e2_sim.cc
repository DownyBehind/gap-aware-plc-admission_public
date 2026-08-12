// E2 loss-aware admission simulator (standalone). Runs a K-session burst to
// completion under full-frame PER with a per-session cycle budget q and an
// optional per-frame retry cap (worst-case provisioning). The window shrinks
// adaptively as sessions complete (gap-aware: W(t) = q * K_active(t)).
//
// Usage: e2_sim <q> <retry_cap(0=unlimited)> <per_ppm> <seeds> <horizon_cycles>
// Output: one row per (N, K, seed) over the E2 grid.

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

std::vector<Msg>
PaperSequence()
{
    std::vector<Msg> seq;
    seq.push_back({11, 0});
    seq.push_back({11, 15});
    for (uint32_t i = 0; i < 3; ++i)
    {
        seq.push_back({11, 35 + 20 * i});
    }
    seq.push_back({11, 95});
    for (uint32_t i = 0; i < 10; ++i)
    {
        seq.push_back({12, 105 + 40 * i});
    }
    seq.push_back({18, 535});
    seq.push_back({12, 635});
    seq.push_back({12, 755});
    seq.push_back({13, 775});
    return seq;
}

struct Session
{
    uint32_t next{0};
    uint32_t attempts{0};   // attempts on the current frame
    int32_t credit{0};      // per-session budget account (q accrued per cycle)
    int completedCycle{-1}; // -1 = not completed
    bool failed{false};     // retry cap exhausted
};

struct Row
{
    uint32_t violations{0};   // completion cycle >= 40 or never completed
    uint32_t failures{0};     // retry-cap exhaustion (must stay <= eps level)
    uint32_t completed{0};
    int maxCompletionCycle{-1};
    uint32_t dcMissCycles{0};
    uint32_t worstResponse{0};
    // aggregate-cap probe instrumentation (stderr only; stdout unchanged)
    uint32_t maxConsumed{0};
    uint32_t cyclesOverQk{0};
    uint32_t activeCycles{0};
    uint32_t gDebtMax{0};
    uint64_t gDebtSum{0};
    uint32_t gDebtFinal{0};
    uint32_t a1Violations{0};
    uint32_t maxServiceLag{0}; // max consecutive cycles a session stays
                               // eligible (credit>0, released message)
                               // yet unserved under the aggregate cap
    std::vector<int> completionCycles;
    std::vector<uint8_t> failedFlags;
};

Row
RunCell(const ns3::EvPlcParams& p, uint32_t n, uint32_t k, uint32_t q, uint32_t cap, double per,
        uint32_t seed, uint32_t horizon, uint32_t capMode)
{
    const auto seq = PaperSequence();
    std::mt19937 rng(seed * 6011 + n * 211 + k * 31 + q * 3);
    std::uniform_real_distribution<double> uni(0.0, 1.0);

    std::vector<Session> sessions(k);
    Row row;
    uint32_t gDebt = 0;        // aggregate debt carried across cycles (cap modes)
    uint32_t startCursor = 0;  // persistent service pointer (capMode 2)
    std::vector<uint32_t> lagStreak(k, 0); // per-session deferred-cycle streaks

    for (uint32_t cycle = 0; cycle < horizon; ++cycle)
    {
        uint32_t cursor = 0;
        std::vector<uint32_t> ready(n, 0);
        for (uint32_t ev = 0; ev < n; ++ev)
        {
            uint32_t end = cursor;
            while (true)
            {
                end += p.m_cReqEffSlots;
                if (per > 0.0 && uni(rng) < per)
                {
                    continue; // DC retransmission, unlimited
                }
                break;
            }
            ready[ev] = end + p.m_cProcSlots;
            cursor = end;
        }

        // Per-session credit accounting (debt-neutral semantics):
        // q is a *per-active-session* contribution, so each session accrues q
        // slots of credit per cycle and may start its next released message
        // while its own credit is positive; a non-preemptive start may drive
        // the credit negative (packetization debt), repaid by later accrual.
        // An earlier aggregate-window round-robin discipline was measurably
        // wrong: early finishers shrank the adaptive window and starved
        // laggards (deterministic D_g violations at p=0) — kept here as a
        // negative service-discipline finding.
        uint32_t kActive = 0;
        for (const auto& s : sessions)
        {
            if (s.completedCycle < 0 && !s.failed)
            {
                ++kActive;
            }
        }
        const uint32_t qk = q * kActive;
        uint32_t consumed = 0;
        if (capMode == 0)
        {
            for (auto& s : sessions)
            {
                if (s.completedCycle >= 0 || s.failed)
                {
                    continue;
                }
                s.credit += static_cast<int32_t>(q);
                while (s.credit > 0 && s.next < seq.size())
                {
                    const auto& msg = seq[s.next];
                    if (cycle * p.m_tCtrlMs < msg.releaseMs)
                    {
                        break;
                    }
                    s.credit -= static_cast<int32_t>(msg.slots);
                    consumed += msg.slots;
                    if (per > 0.0 && uni(rng) < per)
                    {
                        s.attempts += 1;
                        if (cap > 0 && s.attempts > cap)
                        {
                            s.failed = true; // retry cap exhausted (prob <= eps by design)
                            break;
                        }
                    }
                    else
                    {
                        s.next += 1;
                        s.attempts = 0;
                        if (s.next == seq.size())
                        {
                            s.completedCycle = static_cast<int>(cycle);
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            // Aggregate-cap layer: allowance = q*K_active - g_debt; a session
            // may START a message only while consumed < allowance (started
            // messages are non-preemptive and may straddle). Per-session
            // credit accounting is unchanged and non-transferable (A2).
            for (auto& s : sessions)
            {
                if (s.completedCycle < 0 && !s.failed)
                {
                    s.credit += static_cast<int32_t>(q);
                }
            }
            const int64_t allowance =
                static_cast<int64_t>(qk) - static_cast<int64_t>(gDebt);
            int firstBlocked = -1;
            std::vector<uint8_t> eligible(k, 0), servedFlag(k, 0);
            for (uint32_t j = 0; j < k; ++j)
            {
                const auto& s = sessions[j];
                if (s.completedCycle < 0 && !s.failed && s.credit > 0 &&
                    s.next < seq.size() &&
                    cycle * p.m_tCtrlMs >= seq[s.next].releaseMs)
                {
                    eligible[j] = 1;
                }
            }
            for (uint32_t i = 0; i < k; ++i)
            {
                const uint32_t idx = (capMode == 2 ? (startCursor + i) % k : i);
                auto& s = sessions[idx];
                if (s.completedCycle >= 0 || s.failed)
                {
                    continue;
                }
                while (s.credit > 0 && s.next < seq.size())
                {
                    const auto& msg = seq[s.next];
                    if (cycle * p.m_tCtrlMs < msg.releaseMs)
                    {
                        break;
                    }
                    if (static_cast<int64_t>(consumed) >= allowance)
                    {
                        if (firstBlocked < 0)
                        {
                            firstBlocked = static_cast<int>(idx);
                        }
                        break;
                    }
                    s.credit -= static_cast<int32_t>(msg.slots);
                    consumed += msg.slots;
                    servedFlag[idx] = 1;
                    if (per > 0.0 && uni(rng) < per)
                    {
                        s.attempts += 1;
                        if (cap > 0 && s.attempts > cap)
                        {
                            s.failed = true; // retry cap exhausted (prob <= eps by design)
                            break;
                        }
                    }
                    else
                    {
                        s.next += 1;
                        s.attempts = 0;
                        if (s.next == seq.size())
                        {
                            s.completedCycle = static_cast<int>(cycle);
                            break;
                        }
                    }
                }
            }
            gDebt = gDebt + consumed > qk ? gDebt + consumed - qk : 0; // max(0, gDebt + consumed - qK)
            for (uint32_t j = 0; j < k; ++j)
            {
                if (eligible[j] && !servedFlag[j])
                {
                    ++lagStreak[j];
                    row.maxServiceLag = std::max(row.maxServiceLag, lagStreak[j]);
                }
                else
                {
                    lagStreak[j] = 0;
                }
            }
            if (capMode == 2)
            {
                startCursor = firstBlocked >= 0 ? static_cast<uint32_t>(firstBlocked) : 0;
            }
        }
        if (kActive > 0)
        {
            ++row.activeCycles;
            row.maxConsumed = std::max(row.maxConsumed, consumed);
            if (consumed > qk)
            {
                ++row.cyclesOverQk;
            }
            if (consumed > qk + 17)
            {
                ++row.a1Violations; // Lemma 1 aggregate bound violated
                if (capMode > 0)
                {
                    std::cerr << "A1 VIOLATION: consumed=" << consumed << " qk=" << qk
                              << " cycle=" << cycle << "\n";
                    std::exit(2);
                }
            }
            row.gDebtMax = std::max(row.gDebtMax, gDebt);
            row.gDebtSum += gDebt;
        }
        row.gDebtFinal = gDebt;
        cursor += consumed;

        uint32_t lastEnd = cursor;
        for (uint32_t ev = 0; ev < n; ++ev)
        {
            uint32_t start = std::max(cursor, ready[ev]);
            uint32_t end = start;
            while (true)
            {
                end += p.m_cResEffSlots;
                if (per > 0.0 && uni(rng) < per)
                {
                    continue;
                }
                break;
            }
            cursor = end;
            lastEnd = end;
        }
        const uint32_t finish = n > 0 ? lastEnd + p.m_bBlkSlots
                                      : (consumed > 0 ? lastEnd + p.m_bBlkSlots : 0);
        row.worstResponse = std::max(row.worstResponse, finish);
        if (finish > p.GetScheduledSlots())
        {
            ++row.dcMissCycles;
        }
    }

    for (const auto& s : sessions)
    {
        row.completionCycles.push_back(s.completedCycle);
        row.failedFlags.push_back(s.failed ? 1 : 0);
        if (s.failed)
        {
            ++row.failures;
        }
        else if (s.completedCycle >= 0 && s.completedCycle < 40)
        {
            ++row.completed;
            row.maxCompletionCycle = std::max(row.maxCompletionCycle, s.completedCycle);
        }
        else
        {
            ++row.violations; // completed at/after cycle 40, or never
            if (s.completedCycle >= 0)
            {
                row.maxCompletionCycle = std::max(row.maxCompletionCycle, s.completedCycle);
            }
        }
    }
    return row;
}

} // namespace

int
main(int argc, char** argv)
{
    const uint32_t q = argc > 1 ? static_cast<uint32_t>(std::atoi(argv[1])) : 7;
    const uint32_t cap = argc > 2 ? static_cast<uint32_t>(std::atoi(argv[2])) : 0;
    const double per = (argc > 3 ? std::atof(argv[3]) : 0.0) / 1e6;
    const uint32_t seeds = argc > 4 ? static_cast<uint32_t>(std::atoi(argv[4])) : 20;
    const uint32_t horizon = argc > 5 ? static_cast<uint32_t>(std::atoi(argv[5])) : 90;
    const uint32_t capMode = argc > 6 ? static_cast<uint32_t>(std::atoi(argv[6])) : 0;

    ns3::EvPlcParams params;
    const uint32_t nValues[] = {5, 15, 25, 35};
    const uint32_t kValues[] = {1, 4, 8, 16};

    std::cout << "N,K,q,cap,per,seed,violations,failures,completed,max_completion_cycle,"
                 "dc_miss_cycles,horizon,worst_response\n";
    for (const auto n : nValues)
    {
        for (const auto k : kValues)
        {
            for (uint32_t seed = 1; seed <= seeds; ++seed)
            {
                const auto row = RunCell(params, n, k, q, cap, per, seed, horizon, capMode);
                std::cout << n << ',' << k << ',' << q << ',' << cap << ',' << per << ','
                          << seed << ',' << row.violations << ',' << row.failures << ','
                          << row.completed << ',' << row.maxCompletionCycle << ','
                          << row.dcMissCycles << ',' << horizon << ',' << row.worstResponse
                          << "\n";
                std::cerr << "STATS," << n << ',' << k << ',' << capMode << ',' << seed << ','
                          << row.maxConsumed << ',' << row.cyclesOverQk << ','
                          << row.activeCycles << ',' << row.gDebtMax << ',' << row.gDebtSum
                          << ',' << row.gDebtFinal << ',' << row.a1Violations << ','
                          << row.maxServiceLag << ",comp=";
                for (size_t i = 0; i < row.completionCycles.size(); ++i)
                {
                    std::cerr << (i ? "|" : "")
                              << (row.failedFlags[i] ? -2 : row.completionCycles[i]);
                }
                std::cerr << "\n";
            }
        }
    }
    return 0;
}
