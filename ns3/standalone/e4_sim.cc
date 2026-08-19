// E4 five-policy comparison (standalone). Burst of K SLAC sessions arrives at
// cycle 0 over N0 DC-active EVs; completed sessions transition to DC-active
// (N grows). PER applies to every frame. Policies:
//   csma    — real HpgpCsmaCaBaseline contention machine, one instance per
//             cycle (backoff resync at the beacon period; documented),
//             no admission, no budget isolation.
//   fixed   — fixed SLAC reservation alpha*T_sched shared by active sessions
//             (per-session credit = B_fix / K_active), no admission.
//   acbs    — Cond A+B admission (reject-and-retry each cycle) with
//             per-session budget q and per-frame retry cap.
//
// Usage: e4_sim <policy> <param> <cap> <per_ppm> <seeds> <horizon>
//   param: alpha percent for fixed (10/25/50), q for acbs, ignored for csma.

#include "ns3/ev-plc-params.h"
#include "ns3/hpgp-csma-ca-baseline.h"
#include "ns3/transition-admission-controller.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <random>
#include <string>
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

struct Sess
{
    int admittedCycle{-1};
    int completedCycle{-1};
    uint32_t next{0};
    uint32_t attempts{0};
    double credit{0.0};
    bool failed{false};
};

struct Out
{
    uint32_t admitted{0};
    uint32_t neverAdmitted{0};
    double waitSum{0.0};
    uint32_t dgViolations{0};
    uint64_t dcEvCycles{0};
    uint64_t dcMisses{0};
    uint32_t completed{0};
};

void
Finalize(Out& out, std::vector<Sess>& sessions, uint32_t horizon)
{
    for (auto& s : sessions)
    {
        if (s.admittedCycle < 0)
        {
            ++out.neverAdmitted;
            out.waitSum += horizon; // censored at horizon (reject-and-retry wait)
            continue;
        }
        ++out.admitted;
        out.waitSum += s.admittedCycle;
        if (s.failed || s.completedCycle < 0 ||
            s.completedCycle - s.admittedCycle >= 40)
        {
            ++out.dgViolations;
        }
        else
        {
            ++out.completed;
        }
    }
}

Out
RunScheduled(const ns3::EvPlcParams& p, bool acbs, uint32_t q, double bFix, uint32_t cap,
             double per, uint32_t n0, uint32_t k, uint32_t seed, uint32_t horizon,
             uint32_t capMode)
{
    const auto seq = PaperSequence();
    std::mt19937 rng(seed * 9391 + n0 * 449 + k * 37);
    std::uniform_real_distribution<double> uni(0.0, 1.0);
    ns3::EvPlcParams acbsParams = p;
    acbsParams.m_bAuthSlots = q;
    ns3::TransitionAdmissionController controller(acbsParams);

    std::vector<Sess> sessions(k);
    uint32_t nDc = n0;
    uint32_t pendingTransitions = 0;
    Out out;
    double gDebt = 0.0;       // aggregate debt carried across cycles (cap modes)
    uint32_t startCursor = 0; // persistent service pointer (cap modes)

    for (uint32_t cycle = 0; cycle < horizon; ++cycle)
    {
        nDc += pendingTransitions; // completions take effect at the next boundary
        pendingTransitions = 0;

        uint32_t kActive = 0;
        for (const auto& s : sessions)
        {
            if (s.admittedCycle >= 0 && s.completedCycle < 0 && !s.failed)
            {
                ++kActive;
            }
        }
        if (acbs)
        {
            for (auto& s : sessions)
            {
                if (s.admittedCycle < 0 && controller.Admit(nDc, kActive))
                {
                    s.admittedCycle = static_cast<int>(cycle);
                    ++kActive;
                }
            }
        }
        else if (cycle == 0)
        {
            for (auto& s : sessions)
            {
                s.admittedCycle = 0;
            }
            kActive = k;
        }

        // DC request phase with immediate retransmission.
        uint32_t cursor = 0;
        std::vector<uint32_t> ready(nDc, 0);
        for (uint32_t ev = 0; ev < nDc; ++ev)
        {
            uint32_t end = cursor;
            do
            {
                end += p.m_cReqEffSlots;
            } while (per > 0.0 && uni(rng) < per);
            ready[ev] = end + p.m_cProcSlots;
            cursor = end;
        }

        // SLAC window: per-session credit (E2 discipline); cap modes add an
        // aggregate window of quota = perSessionCredit * K_active with
        // carried debt and a persistent service pointer. Per-session credit
        // accounting is unchanged and non-transferable.
        const double perSessionCredit = acbs ? static_cast<double>(q)
                                             : (kActive > 0 ? bFix / kActive : 0.0);
        const double quota = acbs ? static_cast<double>(q) * kActive
                                  : (kActive > 0 ? bFix : 0.0);
        const double allowance = capMode > 0 ? quota - gDebt : 0.0;
        int firstBlocked = -1;
        uint32_t consumed = 0;
        for (uint32_t i = 0; i < k; ++i)
        {
            const uint32_t idx =
                (capMode == 2 ? (startCursor + i) % k : i);
            auto& s = sessions[idx];
            if (s.admittedCycle < 0 || s.completedCycle >= 0 || s.failed)
            {
                continue;
            }
            s.credit += perSessionCredit;
            while (s.credit > 0.0 && s.next < seq.size())
            {
                const auto& msg = seq[s.next];
                if ((cycle - s.admittedCycle) * p.m_tCtrlMs < msg.releaseMs)
                {
                    break;
                }
                if (capMode > 0 && static_cast<double>(consumed) >= allowance)
                {
                    if (firstBlocked < 0)
                    {
                        firstBlocked = static_cast<int>(idx);
                    }
                    break;
                }
                s.credit -= msg.slots;
                consumed += msg.slots;
                if (per > 0.0 && uni(rng) < per)
                {
                    s.attempts += 1;
                    if (cap > 0 && s.attempts > cap)
                    {
                        s.failed = true;
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
                        ++pendingTransitions;
                        break;
                    }
                }
            }
        }
        if (capMode > 0)
        {
            if (static_cast<double>(consumed) > std::ceil(quota) + 17.0)
            {
                std::cerr << "A1 VIOLATION: consumed=" << consumed << " quota=" << quota
                          << " cycle=" << cycle << "\n";
                std::exit(2);
            }
            gDebt = std::max(0.0, gDebt + consumed - quota);
            if (capMode == 2)
            {
                startCursor = firstBlocked >= 0 ? static_cast<uint32_t>(firstBlocked) : 0;
            }
        }
        cursor += consumed;

        // DC response phase.
        for (uint32_t ev = 0; ev < nDc; ++ev)
        {
            uint32_t start = std::max(cursor, ready[ev]);
            uint32_t end = start;
            do
            {
                end += p.m_cResEffSlots;
            } while (per > 0.0 && uni(rng) < per);
            cursor = end;
            if (end > p.GetScheduledSlots())
            {
                ++out.dcMisses;
            }
        }
        out.dcEvCycles += nDc;
    }
    Finalize(out, sessions, horizon);
    return out;
}

Out
RunCsma(const ns3::EvPlcParams& p, double per, uint32_t n0, uint32_t k, uint32_t seed,
        uint32_t horizon)
{
    const auto seq = PaperSequence();
    std::mt19937 rng(seed * 7727 + n0 * 349 + k * 41);
    std::uniform_real_distribution<double> uni(0.0, 1.0);

    std::vector<Sess> sessions(k);
    for (auto& s : sessions)
    {
        s.admittedCycle = 0; // contention access: everyone starts immediately
    }
    uint32_t nDc = n0;
    uint32_t pendingTransitions = 0;
    Out out;

    for (uint32_t cycle = 0; cycle < horizon; ++cycle)
    {
        nDc += pendingTransitions;
        pendingTransitions = 0;

        // Fresh contention machine per cycle (backoff resync at the beacon
        // period boundary; approximation documented in the harness).
        ns3::HpgpCsmaCaParams cp;
        ns3::HpgpCsmaCaBaseline machine(cp);
        machine.SetSeed(seed * 100003 + cycle);

        const uint32_t evseNode = 900;
        machine.AddNode(evseNode);
        for (uint32_t ev = 0; ev < nDc; ++ev)
        {
            machine.AddNode(1 + ev);
            ns3::HpgpFrame f;
            f.nodeId = 1 + ev;
            f.type = ns3::HpgpTrafficType::DC_REQ;
            f.durationSlots = p.m_cReqEffSlots;
            machine.EnqueueFrame(1 + ev, f);
        }
        for (uint32_t j = 0; j < k; ++j)
        {
            auto& s = sessions[j];
            machine.AddNode(2000 + j);
            if (s.completedCycle < 0 && s.next < seq.size() &&
                cycle * p.m_tCtrlMs >= seq[s.next].releaseMs)
            {
                ns3::HpgpFrame f;
                f.nodeId = 2000 + j;
                f.type = ns3::HpgpTrafficType::SLAC;
                f.durationSlots = seq[s.next].slots;
                machine.EnqueueFrame(2000 + j, f);
            }
        }

        std::deque<std::pair<uint64_t, uint32_t>> pendingRes; // (ready slot, ev)
        uint32_t responsesDone = 0;
        std::size_t traceIdx = 0;
        for (uint32_t slot = 0; slot < p.m_tCtrlSlots; ++slot)
        {
            while (!pendingRes.empty() && pendingRes.front().first <= slot)
            {
                ns3::HpgpFrame f;
                f.nodeId = evseNode;
                f.type = ns3::HpgpTrafficType::DC_RES;
                f.durationSlots = p.m_cResEffSlots;
                machine.EnqueueFrame(evseNode, f);
                pendingRes.pop_front();
            }
            machine.Step();
            const auto& trace = machine.GetTrace();
            for (; traceIdx < trace.size(); ++traceIdx)
            {
                const auto& e = trace[traceIdx];
                const bool frameOver = e.eventType == "success";
                const bool dropped = e.eventType == "collision_drop";
                if (!frameOver && !dropped)
                {
                    continue;
                }
                const bool perFail = frameOver && per > 0.0 && uni(rng) < per;
                if (dropped || perFail)
                {
                    // Persistent retry: re-enqueue the same frame.
                    ns3::HpgpFrame f;
                    f.nodeId = e.nodeId;
                    f.type = e.trafficType;
                    f.durationSlots = e.trafficType == ns3::HpgpTrafficType::DC_REQ
                                          ? p.m_cReqEffSlots
                                          : (e.trafficType == ns3::HpgpTrafficType::DC_RES
                                                 ? p.m_cResEffSlots
                                                 : (e.nodeId >= 2000 &&
                                                    sessions[e.nodeId - 2000].next < seq.size()
                                                        ? seq[sessions[e.nodeId - 2000].next].slots
                                                        : 12));
                    machine.EnqueueFrame(e.nodeId, f);
                    continue;
                }
                if (e.trafficType == ns3::HpgpTrafficType::DC_REQ)
                {
                    pendingRes.push_back({e.frameEnd + p.m_cProcSlots, e.nodeId - 1});
                }
                else if (e.trafficType == ns3::HpgpTrafficType::DC_RES)
                {
                    ++responsesDone;
                }
                else if (e.trafficType == ns3::HpgpTrafficType::SLAC && e.nodeId >= 2000)
                {
                    auto& s = sessions[e.nodeId - 2000];
                    s.next += 1;
                    if (s.next == seq.size())
                    {
                        s.completedCycle = static_cast<int>(cycle);
                        ++pendingTransitions;
                    }
                    else if (cycle * p.m_tCtrlMs >= seq[s.next].releaseMs)
                    {
                        ns3::HpgpFrame f;
                        f.nodeId = e.nodeId;
                        f.type = ns3::HpgpTrafficType::SLAC;
                        f.durationSlots = seq[s.next].slots;
                        machine.EnqueueFrame(e.nodeId, f);
                    }
                }
            }
        }
        out.dcMisses += nDc - std::min(nDc, responsesDone);
        out.dcEvCycles += nDc;
    }
    Finalize(out, sessions, horizon);
    return out;
}

} // namespace

int
main(int argc, char** argv)
{
    const std::string policy = argc > 1 ? argv[1] : "acbs";
    const uint32_t param = argc > 2 ? static_cast<uint32_t>(std::atoi(argv[2])) : 25;
    const uint32_t cap = argc > 3 ? static_cast<uint32_t>(std::atoi(argv[3])) : 0;
    const double per = (argc > 4 ? std::atof(argv[4]) : 1000.0) / 1e6;
    const uint32_t seeds = argc > 5 ? static_cast<uint32_t>(std::atoi(argv[5])) : 20;
    const uint32_t horizon = argc > 6 ? static_cast<uint32_t>(std::atoi(argv[6])) : 120;
    const uint32_t capMode = argc > 7 ? static_cast<uint32_t>(std::atoi(argv[7])) : 0;

    ns3::EvPlcParams params;
    const uint32_t n0Values[] = {0, 15, 30};
    const uint32_t kValues[] = {1, 2, 5, 10, 20, 35};

    std::cout << "policy,param,N0,K,seed,admitted,never_admitted,wait_sum_cycles,"
                 "dg_violations,completed,dc_misses,dc_ev_cycles\n";
    for (const auto n0 : n0Values)
    {
        for (const auto k : kValues)
        {
            for (uint32_t seed = 1; seed <= seeds; ++seed)
            {
                Out out;
                if (policy == "csma")
                {
                    out = RunCsma(params, per, n0, k, seed, horizon);
                }
                else if (policy == "fixed")
                {
                    const double bFix = params.GetScheduledSlots() * param / 100.0;
                    out = RunScheduled(params, false, 0, bFix, 0, per, n0, k, seed, horizon, capMode);
                }
                else
                {
                    out = RunScheduled(params, true, param, 0.0, cap, per, n0, k, seed, horizon, capMode);
                }
                std::cout << policy << ',' << param << ',' << n0 << ',' << k << ',' << seed
                          << ',' << out.admitted << ',' << out.neverAdmitted << ','
                          << out.waitSum << ',' << out.dgViolations << ',' << out.completed
                          << ',' << out.dcMisses << ',' << out.dcEvCycles << "\n";
            }
        }
    }
    return 0;
}
