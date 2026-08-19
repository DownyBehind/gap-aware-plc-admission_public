#include "ev-plc-policy-mac.h"

#include "ns3/abort.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"

#include <algorithm>
#include <cmath>

namespace ns3
{

namespace
{

struct Msg
{
    uint32_t slots;
    uint32_t releaseMs;
};

const std::vector<Msg>&
PaperSequence()
{
    static const std::vector<Msg> seq = [] {
        std::vector<Msg> s;
        s.push_back({11, 0});
        s.push_back({11, 15});
        for (uint32_t i = 0; i < 3; ++i)
        {
            s.push_back({11, 35 + 20 * i});
        }
        for (uint32_t i = 0; i < 10; ++i)
        {
            s.push_back({12, 105 + 40 * i});
        }
        s.push_back({18, 535});
        s.push_back({12, 635});
        s.push_back({12, 755});
        s.push_back({13, 775});
        return s;
    }();
    return seq;
}

// Retransmission demand inflation for the link-aware variant: effective
// airtime = base / (1 - p_frame(link)) rounded up.
uint32_t
Inflate(uint32_t baseSlots, double pFrame)
{
    return static_cast<uint32_t>(std::ceil(baseSlots / std::max(1e-9, 1.0 - pFrame)));
}

} // namespace

EvPlcPolicyMac::EvPlcPolicyMac(const EvPlcParams& params, Ptr<PlcSharedChannel> channel)
    : m_params(params), m_channel(channel), m_countController(params),
      m_linkController(params.m_tCtrlSlots, params.m_oMapSlots, params.m_bAuthSlots)
{
}

void
EvPlcPolicyMac::ConfigureAcbs(uint32_t q, uint32_t retryCap)
{
    m_acbs = true;
    m_q = q;
    m_retryCap = retryCap;
    EvPlcParams p = m_params;
    p.m_bAuthSlots = q;
    m_countController = TransitionAdmissionController(p);
    m_linkController = LinkAwareAdmissionController(p.m_tCtrlSlots, p.m_oMapSlots, q);
}

void
EvPlcPolicyMac::ConfigureFixed(double bFix)
{
    m_acbs = false;
    m_bFix = bFix;
}

void
EvPlcPolicyMac::SetAdmissionVariant(PolicyAdmission variant)
{
    m_admission = variant;
}

void
EvPlcPolicyMac::SetCondBEnabled(bool enabled)
{
    m_condBEnabled = enabled;
}

void
EvPlcPolicyMac::SetAggregateCap(bool enabled)
{
    m_aggCap = enabled;
}

void
EvPlcPolicyMac::EnableCycleTrace(bool enabled)
{
    m_traceEnabled = enabled;
}

void
EvPlcPolicyMac::CloseTraceRow()
{
    if (!m_traceEnabled || !m_traceValid)
    {
        return;
    }
    m_stats.cycleTrace.push_back(m_traceRow);
    m_traceValid = false;
}

void
EvPlcPolicyMac::SetErrorSource(PolicyErrorSource source, double iidPer, uint32_t iidSeed)
{
    m_errorSource = source;
    m_iidPer = iidPer;
    m_iidRng.seed(iidSeed);
}

void
EvPlcPolicyMac::ConfigureScenario(uint32_t n0, uint32_t kBurst,
                                  const std::vector<PlcProfileClass>& dcClasses,
                                  const std::vector<PlcProfileClass>& sessionClasses)
{
    m_nDc = n0;
    m_dcClasses = dcClasses;
    m_dcClasses.resize(n0, PlcProfileClass::NOMINAL);
    m_sessions.assign(kBurst, Session{});
    for (uint32_t j = 0; j < kBurst && j < sessionClasses.size(); ++j)
    {
        m_sessions[j].linkClass = sessionClasses[j];
    }
}

Time
EvPlcPolicyMac::SlotsToExactTime(uint64_t slots)
{
    return NanoSeconds(slots * 35840ULL);
}

uint64_t
EvPlcPolicyMac::NowSlot() const
{
    return static_cast<uint64_t>(Simulator::Now().GetNanoSeconds() / 35840ULL);
}

void
EvPlcPolicyMac::SetSessionArrivalCycles(const std::vector<uint32_t>& arrivals)
{
    for (uint32_t j = 0; j < m_sessions.size() && j < arrivals.size(); ++j)
    {
        m_sessions[j].arrivalCycle = arrivals[j];
    }
}

void
EvPlcPolicyMac::Start(uint32_t horizonCycles)
{
    m_horizonCycles = horizonCycles;
    m_cycleIndex = 0;
    Simulator::Schedule(Time(0), &EvPlcPolicyMac::CycleBoundary, this);
}

double
EvPlcPolicyMac::SessionCredit(const Session& session) const
{
    if (!m_acbs)
    {
        uint32_t active = 0;
        for (const auto& s : m_sessions)
        {
            if (s.admittedCycle >= 0 && s.completedCycle < 0 && !s.failed)
            {
                ++active;
            }
        }
        return active > 0 ? m_bFix / active : 0.0;
    }
    // LINK_AWARE credit is handled inline at the cycle boundary (link-
    // inflated q_i); this helper covers the fixed and count-based paths.
    (void)session;
    return static_cast<double>(m_q);
}

bool
EvPlcPolicyMac::AdmitCandidate(const Session& candidate) const
{
    uint32_t kActive = 0;
    for (const auto& s : m_sessions)
    {
        if (s.admittedCycle >= 0 && s.completedCycle < 0 && !s.failed)
        {
            ++kActive;
        }
    }
    if (m_admission == PolicyAdmission::COUNT)
    {
        if (!m_condBEnabled)
        {
            // Cond-A-only ablation: identical Cond A and completion check,
            // post-transition Cond B skipped.
            return m_countController.CheckCondA(m_nDc, kActive) &&
                   m_countController.CheckSlacCompletion();
        }
        return m_countController.Admit(m_nDc, kActive);
    }
    // LINK_AWARE: per-EV demands with retx-inflated airtimes (SoT class).
    std::vector<EvCommunicationDemand> dc;
    for (uint32_t ev = 0; ev < m_nDc; ++ev)
    {
        auto demand = MakeDemand(ev, ev, PlcLinkProfileTable::Get(m_dcClasses[ev]));
        const double pReq = m_channel->LinkFramePer(ev, m_params.m_cReqEffSlots);
        const double pRes = m_channel->LinkFramePer(ev, m_params.m_cResEffSlots);
        demand.cReqEffSlots = Inflate(m_params.m_cReqEffSlots, pReq);
        demand.cResEffSlots = Inflate(m_params.m_cResEffSlots, pRes);
        dc.push_back(demand);
    }
    // Per-session credits q_i = ceil(q / (1 - p_frame(link, 12))) — the same
    // inflation the cycle-boundary accrual uses, so Cond A charges the
    // aggregate window it will actually play: Q_{k'} = sum q_i + q_cand.
    std::vector<EvCommunicationDemand> slac;
    for (const auto& s : m_sessions)
    {
        if (s.admittedCycle >= 0 && s.completedCycle < 0 && !s.failed)
        {
            EvCommunicationDemand d;
            d.creditSlots = static_cast<uint32_t>(std::ceil(
                m_q / std::max(1e-9, 1.0 - m_channel->ClassFramePer(s.linkClass, 12))));
            slac.push_back(d);
        }
    }
    auto cand = MakeDemand(9999, 9999, PlcLinkProfileTable::Get(candidate.linkClass));
    cand.creditSlots = static_cast<uint32_t>(std::ceil(
        m_q / std::max(1e-9, 1.0 - m_channel->ClassFramePer(candidate.linkClass, 12))));
    const auto active = m_linkController.EvaluateActive(dc, slac, cand,
                                                        AdmissionMode::LINK_AWARE_DEMAND_BASED);
    // Projected post-transition state: sessions become DC EVs on their links.
    std::vector<EvCommunicationDemand> projected = dc;
    uint32_t idx = m_nDc;
    for (const auto& s : m_sessions)
    {
        if (s.admittedCycle >= 0 && s.completedCycle < 0 && !s.failed)
        {
            auto d = MakeDemand(idx, idx, PlcLinkProfileTable::Get(s.linkClass));
            d.cReqEffSlots = Inflate(m_params.m_cReqEffSlots,
                                     m_channel->ClassFramePer(s.linkClass, m_params.m_cReqEffSlots));
            d.cResEffSlots = Inflate(m_params.m_cResEffSlots,
                                     m_channel->ClassFramePer(s.linkClass, m_params.m_cResEffSlots));
            projected.push_back(d);
            ++idx;
        }
    }
    auto candDc = MakeDemand(idx, idx, PlcLinkProfileTable::Get(candidate.linkClass));
    candDc.cReqEffSlots = Inflate(m_params.m_cReqEffSlots,
                                  m_channel->ClassFramePer(candidate.linkClass, m_params.m_cReqEffSlots));
    candDc.cResEffSlots = Inflate(m_params.m_cResEffSlots,
                                  m_channel->ClassFramePer(candidate.linkClass, m_params.m_cResEffSlots));
    projected.push_back(candDc);
    const auto terminal = m_linkController.EvaluateTerminal(projected,
                                                            AdmissionMode::LINK_AWARE_DEMAND_BASED);
    return active.admit && terminal.admit;
}

bool
EvPlcPolicyMac::RollFailure(uint32_t evId, uint64_t startSlot, uint32_t durationSlots,
                            PlcProfileClass /*linkClass*/, bool /*isSlac*/)
{
    if (m_errorSource == PolicyErrorSource::INTERNAL_IID)
    {
        return m_iidPer > 0.0 && m_uni(m_iidRng) < m_iidPer;
    }
    return m_channel->FrameFailsAt(evId, startSlot, durationSlots);
}

void
EvPlcPolicyMac::CycleBoundary()
{
    if (m_phase != Phase::DONE)
    {
        // The previous cycle's plan overran its boundary (retx carry): defer —
        // Advance() re-enters here when the plan completes. Keeps the in-
        // flight cycle's start slot valid for miss accounting and prevents a
        // double-driven phase machine.
        m_boundaryPending = true;
        return;
    }
    m_boundaryPending = false;
    CloseTraceRow();
    if (m_cycleIndex >= m_horizonCycles)
    {
        FinalizeStats();
        return;
    }
    m_currentCycle = m_cycleIndex;
    m_cycleStartSlot = static_cast<uint64_t>(m_currentCycle) * m_params.m_tCtrlSlots;

    // Transitions from the previous cycle take effect now.
    for (const auto cls : m_pendingTransitionClasses)
    {
        m_dcClasses.push_back(cls);
        ++m_nDc;
    }
    m_pendingTransitionClasses.clear();
    m_stats.maxNDc = std::max(m_stats.maxNDc, m_nDc);

    // Admission: gap-aware runs Cond A+B with reject-and-retry; fixed reservation
    // has no admission control — everyone is admitted at cycle 0 (e4_sim rule).
    for (auto& session : m_sessions)
    {
        if (session.admittedCycle < 0 && m_currentCycle >= session.arrivalCycle)
        {
            if (!m_acbs)
            {
                session.admittedCycle = static_cast<int>(session.arrivalCycle);
            }
            else if (AdmitCandidate(session))
            {
                session.admittedCycle = static_cast<int>(m_currentCycle);
            }
        }
    }
    m_cycleQuota = 0.0;
    m_cycleConsumed = 0;
    m_blockedFirst = -1;
    m_svcVisited = 0;
    for (auto& session : m_sessions)
    {
        if (session.admittedCycle >= 0 && session.completedCycle < 0 && !session.failed)
        {
            double accrual;
            if (m_acbs && m_admission == PolicyAdmission::LINK_AWARE)
            {
                const double pFrame = m_channel->ClassFramePer(session.linkClass, 12);
                accrual = std::ceil(m_q / std::max(1e-9, 1.0 - pFrame));
            }
            else
            {
                accrual = SessionCredit(session);
            }
            session.credit += accrual;
            if (m_acbs && m_admission == PolicyAdmission::LINK_AWARE)
            {
                m_cycleQuota += accrual; // quota = sum q_i (Sec. V-D)
            }
        }
    }
    {
        uint32_t active = 0;
        for (const auto& session : m_sessions)
        {
            if (session.admittedCycle >= 0 && session.completedCycle < 0 && !session.failed)
            {
                ++active;
            }
        }
        if (!m_acbs)
        {
            m_cycleQuota = active > 0 ? m_bFix : 0.0; // exact, engine-identical
        }
        else if (m_admission != PolicyAdmission::LINK_AWARE)
        {
            m_cycleQuota = static_cast<double>(m_q) * active;
        }
    }

    m_readySlot.assign(m_nDc, 0);
    m_phase = m_nDc > 0 ? Phase::REQUESTS : Phase::SLAC;
    m_evCursor = 0;
    m_sessionCursor = 0;

    if (m_traceEnabled)
    {
        uint32_t kActive = 0;
        for (const auto& sess : m_sessions)
        {
            if (sess.admittedCycle >= 0 && sess.completedCycle < 0 && !sess.failed)
            {
                ++kActive;
            }
        }
        m_traceRow = PolicyCycleTraceRow{m_currentCycle, m_nDc, kActive, 0, 0, 0};
        m_traceValid = true;
    }

    // Slack-occupancy instrumentation: classify the in-flight
    // cycle from its deterministic plan — the SLAC slots the credit/release
    // state would play at PER=0, mirroring the SLAC-phase loop in Advance().
    {
        const auto& seq = PaperSequence();
        uint64_t sPlan = 0;
        for (const auto& session : m_sessions)
        {
            if (session.admittedCycle < 0 || session.completedCycle >= 0 || session.failed)
            {
                continue;
            }
            double credit = session.credit;
            uint32_t next = session.next;
            while (credit > 0.0 && next < seq.size() &&
                   (m_currentCycle - session.admittedCycle) * m_params.m_tCtrlMs >=
                       seq[next].releaseMs)
            {
                credit -= seq[next].slots;
                sPlan += seq[next].slots;
                ++next;
            }
        }
        const uint64_t f0 =
            std::max(static_cast<uint64_t>(m_nDc) * m_params.m_cReqEffSlots + sPlan,
                     static_cast<uint64_t>(m_params.m_cReqEffSlots) + m_params.m_cProcSlots) +
            static_cast<uint64_t>(m_nDc) * m_params.m_cResEffSlots;
        const int64_t slack =
            static_cast<int64_t>(m_params.GetScheduledSlots()) - static_cast<int64_t>(f0);
        m_cycleLowSlack = slack < static_cast<int64_t>(m_params.m_cResEffSlots);
        m_cycleLowSlack2 = slack < 2 * static_cast<int64_t>(m_params.m_cResEffSlots);
        ++m_stats.totalCycles;
        m_stats.minPlanSlack = std::min(m_stats.minPlanSlack, slack);
        if (m_cycleLowSlack)
        {
            ++m_stats.lowSlackCycles;
            m_stats.lowSlackEvCycles += m_nDc;
        }
        if (m_cycleLowSlack2)
        {
            ++m_stats.lowSlack2Cycles;
            m_stats.lowSlack2EvCycles += m_nDc;
        }
    }

    ++m_cycleIndex;
    Simulator::Schedule(SlotsToExactTime(static_cast<uint64_t>(m_cycleIndex) *
                                         m_params.m_tCtrlSlots) -
                            Simulator::Now(),
                        &EvPlcPolicyMac::CycleBoundary, this);
    Advance();
}

void
EvPlcPolicyMac::Advance()
{
    const auto& seq = PaperSequence();
    if (m_phase == Phase::REQUESTS)
    {
        if (m_evCursor < m_nDc)
        {
            const uint64_t start = std::max(m_channel->GetBusyUntilSlot(), m_cycleStartSlot);
            m_channel->Occupy(start, m_params.m_cReqEffSlots);
            Simulator::Schedule(SlotsToExactTime(start + m_params.m_cReqEffSlots) -
                                    Simulator::Now(),
                                &EvPlcPolicyMac::OnTxEnd, this, m_params.m_cReqEffSlots, true,
                                m_evCursor, false);
            return;
        }
        m_phase = Phase::SLAC;
        m_sessionCursor = 0;
    }
    if (m_phase == Phase::SLAC)
    {
        if (!m_aggCap)
        {
            while (m_sessionCursor < m_sessions.size())
            {
                auto& session = m_sessions[m_sessionCursor];
                const bool active =
                    session.admittedCycle >= 0 && session.completedCycle < 0 && !session.failed;
                if (active && session.credit > 0.0 && session.next < seq.size() &&
                    (m_currentCycle - session.admittedCycle) * m_params.m_tCtrlMs >=
                        seq[session.next].releaseMs)
                {
                    const uint32_t slots = seq[session.next].slots;
                    session.credit -= slots;
                    const uint64_t start = std::max(m_channel->GetBusyUntilSlot(), m_cycleStartSlot);
                    m_channel->Occupy(start, slots);
                    if (m_traceEnabled)
                    {
                        m_traceRow.slacPlayed += slots;
                    }
                    Simulator::Schedule(SlotsToExactTime(start + slots) - Simulator::Now(),
                                        &EvPlcPolicyMac::OnTxEnd, this, slots, false, m_sessionCursor,
                                        false);
                    return;
                }
                ++m_sessionCursor;
            }
            m_phase = Phase::RESPONSES;
            m_evCursor = 0;
        }
        else
        {
            // Aggregate window: a session may START a message only while
            // consumed < allowance = quota - gDebt (started messages are
            // non-preemptive and may straddle). Persistent service order:
            // resume from the first session blocked in the previous cycle.
            const double allowance = m_cycleQuota - m_gDebt;
            const size_t sz = m_sessions.size();
            while (m_svcVisited < sz)
            {
                const uint32_t idx =
                    static_cast<uint32_t>((m_svcStart + m_svcVisited) % sz);
                auto& session = m_sessions[idx];
                const bool active =
                    session.admittedCycle >= 0 && session.completedCycle < 0 && !session.failed;
                if (active && session.credit > 0.0 && session.next < seq.size() &&
                    (m_currentCycle - session.admittedCycle) * m_params.m_tCtrlMs >=
                        seq[session.next].releaseMs)
                {
                    if (static_cast<double>(m_cycleConsumed) >= allowance)
                    {
                        if (m_blockedFirst < 0)
                        {
                            m_blockedFirst = static_cast<int>(idx);
                        }
                        ++m_svcVisited;
                        continue;
                    }
                    const uint32_t slots = seq[session.next].slots;
                    session.credit -= slots;
                    m_cycleConsumed += slots;
                    const uint64_t start = std::max(m_channel->GetBusyUntilSlot(), m_cycleStartSlot);
                    m_channel->Occupy(start, slots);
                    if (m_traceEnabled)
                    {
                        m_traceRow.slacPlayed += slots;
                    }
                    Simulator::Schedule(SlotsToExactTime(start + slots) - Simulator::Now(),
                                        &EvPlcPolicyMac::OnTxEnd, this, slots, false, idx,
                                        false);
                    return;
                }
                ++m_svcVisited;
            }
            // SLAC phase complete: A1 assertion + aggregate debt carry.
            NS_ABORT_MSG_IF(static_cast<double>(m_cycleConsumed) > std::ceil(m_cycleQuota) + 17.0,
                            "A1 VIOLATION: consumed " << m_cycleConsumed << " quota "
                                                      << m_cycleQuota);
            m_gDebt = std::max(0.0, m_gDebt + m_cycleConsumed - m_cycleQuota);
            m_svcStart = m_blockedFirst >= 0 ? static_cast<uint32_t>(m_blockedFirst) : 0;
            m_phase = Phase::RESPONSES;
            m_evCursor = 0;
        }
    }
    if (m_phase == Phase::RESPONSES)
    {
        if (m_evCursor < m_nDc)
        {
            const uint64_t channelFree = std::max(m_channel->GetBusyUntilSlot(), m_cycleStartSlot);
            const uint64_t start = std::max(channelFree, m_readySlot[m_evCursor]);
            if (m_traceEnabled && m_evCursor == 0)
            {
                m_traceRow.respStart = static_cast<uint32_t>(start - m_cycleStartSlot);
            }
            m_channel->Occupy(start, m_params.m_cResEffSlots);
            Simulator::Schedule(SlotsToExactTime(start + m_params.m_cResEffSlots) -
                                    Simulator::Now(),
                                &EvPlcPolicyMac::OnTxEnd, this, m_params.m_cResEffSlots, true,
                                m_evCursor, true);
            return;
        }
        m_phase = Phase::DONE;
        if (m_boundaryPending)
        {
            CycleBoundary();
        }
    }
}

void
EvPlcPolicyMac::OnTxEnd(uint32_t durationSlots, bool isDc, uint32_t evId, bool isResponse)
{
    const uint64_t end = NowSlot();
    const auto& seq = PaperSequence();

    if (isDc)
    {
        const bool failed = RollFailure(evId, end - durationSlots, durationSlots,
                                        m_dcClasses[evId], false);
        if (failed)
        {
            // Immediate retransmission.
            m_channel->Occupy(end, durationSlots);
            Simulator::Schedule(SlotsToExactTime(end + durationSlots) - Simulator::Now(),
                                &EvPlcPolicyMac::OnTxEnd, this, durationSlots, true, evId,
                                isResponse);
            return;
        }
        if (!isResponse)
        {
            m_readySlot[evId] = end + m_params.m_cProcSlots;
            ++m_evCursor;
        }
        else
        {
            const uint32_t relative = static_cast<uint32_t>(end - m_cycleStartSlot);
            if (m_traceEnabled && relative > m_traceRow.chanFinish)
            {
                m_traceRow.chanFinish = relative;
            }
            if (relative > m_params.GetScheduledSlots())
            {
                ++m_stats.dcMisses;
                if (m_cycleLowSlack)
                {
                    ++m_stats.dcMissesLowSlack;
                }
                if (m_cycleLowSlack2)
                {
                    ++m_stats.dcMissesLowSlack2;
                }
            }
            if (relative > m_stats.maxRelativeFinish)
            {
                m_stats.maxRelativeFinish = relative;
                m_stats.maxFinishN = m_nDc;
                uint32_t kActive = 0;
                for (const auto& sess : m_sessions)
                {
                    if (sess.admittedCycle >= 0 && sess.completedCycle < 0 && !sess.failed)
                    {
                        ++kActive;
                    }
                }
                m_stats.maxFinishK = kActive;
            }
            ++m_stats.dcEvCycles;
            ++m_evCursor;
        }
        Advance();
        return;
    }

    // SLAC frame of session m_sessionCursor (evId reused as session index).
    auto& session = m_sessions[evId];
    const uint32_t linkEvId = 100000 + evId; // session link ids in channel space
    const bool failed = RollFailure(linkEvId, end - durationSlots, durationSlots,
                                    session.linkClass, true);
    if (failed)
    {
        session.attempts += 1;
        if (m_retryCap > 0 && session.attempts > m_retryCap)
        {
            session.failed = true;
        }
    }
    else
    {
        session.next += 1;
        session.attempts = 0;
        if (session.next == seq.size())
        {
            session.completedCycle = static_cast<int>(m_currentCycle);
            m_pendingTransitionClasses.push_back(session.linkClass);
        }
    }
    Advance();
}

void
EvPlcPolicyMac::FinalizeStats()
{
    for (const auto& session : m_sessions)
    {
        const bool severe = session.linkClass == PlcProfileClass::SEVERE;
        if (session.admittedCycle < 0)
        {
            ++m_stats.neverAdmitted;
            m_stats.waitSumCycles += m_horizonCycles;
            continue;
        }
        ++m_stats.admitted;
        (severe ? m_stats.admittedSevere : m_stats.admittedGood) += 1;
        m_stats.waitSumCycles += session.admittedCycle;
        if (session.failed || session.completedCycle < 0 ||
            session.completedCycle - session.admittedCycle >= 40)
        {
            ++m_stats.dgViolations;
            (severe ? m_stats.dgViolationsSevere : m_stats.dgViolationsGood) += 1;
        }
        else
        {
            ++m_stats.completed;
        }
    }
}

PolicyRunStats
EvPlcPolicyMac::GetStats() const
{
    return m_stats;
}

} // namespace ns3
