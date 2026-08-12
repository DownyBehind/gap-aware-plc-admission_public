#include "ev-plc-csma-mac.h"

#include "ns3/nstime.h"
#include "ns3/simulator.h"

#include <algorithm>

namespace ns3
{

namespace
{

struct Msg
{
    uint32_t slots;
    uint32_t releaseMs;
};

// Paper Table-I sequence (identical to e4_sim's copy: harness data, not SoT).
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
        s.push_back({11, 95});
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

constexpr uint32_t kEvseNode = 900;
constexpr uint32_t kSessionBase = 2000;

} // namespace

EvPlcCsmaMac::EvPlcCsmaMac(const EvPlcParams& params, Ptr<PlcSharedChannel> channel)
    : m_params(params), m_channel(channel)
{
}

void
EvPlcCsmaMac::SetBackoffRngSeed(uint32_t seed)
{
    m_csmaRng.seed(seed);
}

void
EvPlcCsmaMac::ConfigureScenario(uint32_t n0, uint32_t kBurst)
{
    m_nDc = n0;
    m_kBurst = kBurst;
    m_sessions.assign(kBurst, Session{});
}

Time
EvPlcCsmaMac::SlotsToExactTime(uint64_t slots)
{
    return NanoSeconds(slots * 35840ULL);
}

uint64_t
EvPlcCsmaMac::NowSlot() const
{
    return static_cast<uint64_t>(Simulator::Now().GetNanoSeconds() / 35840ULL);
}

HpgpContentionNode&
EvPlcCsmaMac::NodeOf(uint32_t nodeId)
{
    auto it = m_nodes.find(nodeId);
    if (it == m_nodes.end())
    {
        it = m_nodes.emplace(nodeId, HpgpContentionNode(nodeId)).first;
    }
    return it->second;
}

void
EvPlcCsmaMac::Start(uint32_t horizonCycles)
{
    m_horizonCycles = horizonCycles;
    m_cycleIndex = 0;
    Simulator::Schedule(Time(0), &EvPlcCsmaMac::CycleBoundary, this);
}

void
EvPlcCsmaMac::EnqueueDcRequest(uint32_t evIndex)
{
    HpgpFrame frame;
    frame.nodeId = 1 + evIndex;
    frame.type = HpgpTrafficType::DC_REQ;
    frame.durationSlots = m_params.m_cReqEffSlots;
    NodeOf(1 + evIndex).EnqueueFrame(frame);
}

void
EvPlcCsmaMac::EnqueueResponse(uint32_t evIndex)
{
    HpgpFrame frame;
    frame.nodeId = kEvseNode;
    frame.type = HpgpTrafficType::DC_RES;
    frame.durationSlots = m_params.m_cResEffSlots;
    m_responseFifo.push_back(evIndex);
    NodeOf(kEvseNode).EnqueueFrame(frame);
    Reschedule();
}

void
EvPlcCsmaMac::EnqueueSlacIfReleased(uint32_t sessionIndex)
{
    auto& session = m_sessions[sessionIndex];
    const auto& seq = PaperSequence();
    if (session.completedCycle >= 0 || session.framePending || session.next >= seq.size())
    {
        return;
    }
    if (m_currentCycle * m_params.m_tCtrlMs < seq[session.next].releaseMs)
    {
        return;
    }
    HpgpFrame frame;
    frame.nodeId = kSessionBase + sessionIndex;
    frame.type = HpgpTrafficType::SLAC;
    frame.durationSlots = seq[session.next].slots;
    session.framePending = true;
    NodeOf(kSessionBase + sessionIndex).EnqueueFrame(frame);
}

void
EvPlcCsmaMac::CycleBoundary()
{
    // Close the previous cycle's DC-miss accounting.
    if (m_cycleIndex > 0)
    {
        m_stats.dcMisses += m_nDcAtCycleStart -
                            std::min(m_nDcAtCycleStart, m_responsesThisCycle);
        m_stats.dcEvCycles += m_nDcAtCycleStart;
    }
    if (m_cycleIndex >= m_horizonCycles)
    {
        for (const auto& session : m_sessions)
        {
            if (session.completedCycle < 0 || session.completedCycle >= 40)
            {
                ++m_stats.dgViolations;
            }
            else
            {
                ++m_stats.completedSessions;
            }
        }
        return;
    }

    m_nDc += m_pendingTransitions;
    m_pendingTransitions = 0;
    m_responsesThisCycle = 0;
    m_nDcAtCycleStart = m_nDc;
    m_currentCycle = m_cycleIndex;

    for (uint32_t ev = 0; ev < m_nDc; ++ev)
    {
        EnqueueDcRequest(ev);
    }
    for (uint32_t j = 0; j < m_sessions.size(); ++j)
    {
        EnqueueSlacIfReleased(j);
    }
    ++m_cycleIndex;
    Simulator::Schedule(SlotsToExactTime(static_cast<uint64_t>(m_cycleIndex) *
                                         m_params.m_tCtrlSlots) -
                            Simulator::Now(),
                        &EvPlcCsmaMac::CycleBoundary, this);
    Reschedule();
}

void
EvPlcCsmaMac::Reschedule()
{
    ++m_serial;
    const uint64_t now = NowSlot();
    if (m_channel->GetBusyUntilSlot() > now || m_collisionActive)
    {
        // Medium busy: freeze (no decrement — TickBusyFreeze semantics); wake
        // exactly when the medium frees.
        const uint64_t wake = std::max(m_channel->GetBusyUntilSlot(), now);
        Simulator::Schedule(SlotsToExactTime(wake) - Simulator::Now(), &EvPlcCsmaMac::OnWake,
                            this, m_serial);
        return;
    }
    m_idleStartSlot = std::max(m_idleStartSlot, now);

    // Closed-form countdown: contender i becomes ready after remaining_i idle
    // slots from m_idleStartSlot.
    uint64_t bestReady = UINT64_MAX;
    for (auto& [id, node] : m_nodes)
    {
        if (node.GetState() != HpgpNodeState::CONTENDING || !node.PeekFrame())
        {
            continue;
        }
        if (m_remaining.find(id) == m_remaining.end())
        {
            m_remaining[id] = node.GetBackoff();
        }
        uint64_t ready = m_idleStartSlot + m_remaining[id];
        const auto gate = m_gateSlot.find(id);
        if (gate != m_gateSlot.end())
        {
            ready = std::max(ready, gate->second);
        }
        bestReady = std::min(bestReady, ready);
    }
    if (bestReady == UINT64_MAX)
    {
        return; // nothing pending
    }
    Simulator::Schedule(SlotsToExactTime(std::max(bestReady, now)) - Simulator::Now(),
                        &EvPlcCsmaMac::OnWake, this, m_serial);
}

void
EvPlcCsmaMac::OnWake(uint64_t serial)
{
    if (serial != m_serial)
    {
        return; // stale event
    }
    const uint64_t now = NowSlot();
    if (m_channel->GetBusyUntilSlot() > now)
    {
        Reschedule();
        return;
    }
    m_collisionActive = false;

    // Winners: contenders whose countdown elapsed (and gate passed).
    const uint64_t idleElapsed = now - m_idleStartSlot;
    std::vector<uint32_t> winners;
    for (auto& [id, node] : m_nodes)
    {
        if (node.GetState() != HpgpNodeState::CONTENDING || !node.PeekFrame())
        {
            continue;
        }
        const auto gate = m_gateSlot.find(id);
        if (gate != m_gateSlot.end() && now < gate->second)
        {
            continue;
        }
        if (m_remaining[id] <= idleElapsed)
        {
            winners.push_back(id);
        }
    }
    // Countdown bookkeeping for the losers.
    for (auto& [id, remaining] : m_remaining)
    {
        remaining = remaining > idleElapsed ? remaining - static_cast<uint32_t>(idleElapsed) : 0;
    }
    m_idleStartSlot = now;

    if (winners.empty())
    {
        Reschedule();
        return;
    }
    if (winners.size() == 1)
    {
        auto& node = NodeOf(winners.front());
        node.StartTransmission(now);
        const uint32_t duration = node.PeekFrame()->durationSlots;
        m_channel->Occupy(now, duration);
        Simulator::Schedule(SlotsToExactTime(now + duration) - Simulator::Now(),
                            &EvPlcCsmaMac::OnTxEnd, this, winners.front());
        return;
    }

    // Collision: medium busy for the longest involved frame (baseline
    // semantics); every collider updates via HandleCollision (SoT).
    ++m_stats.collisions;
    uint32_t duration = 1;
    for (const auto id : winners)
    {
        duration = std::max(duration, NodeOf(id).PeekFrame()->durationSlots);
    }
    m_channel->Occupy(now, duration);
    m_collisionActive = true;
    for (const auto id : winners)
    {
        auto& node = NodeOf(id);
        const HpgpFrame before = *node.PeekFrame();
        const bool retry = node.HandleCollision(m_csmaParams, m_csmaRng, now);
        m_remaining[id] = node.GetBackoff();
        if (!retry)
        {
            ++m_stats.drops;
            // Persistent retry (e4_sim semantics): dropped frames re-enter
            // fresh; session/DC bookkeeping is untouched by the drop.
            HpgpFrame fresh = before;
            fresh.attempt = 0;
            node.EnqueueFrame(fresh);
            if (node.GetState() == HpgpNodeState::CONTENDING)
            {
                m_remaining[id] = node.GetBackoff();
            }
        }
    }
    Simulator::Schedule(SlotsToExactTime(now + duration) - Simulator::Now(),
                        &EvPlcCsmaMac::OnCollisionEnd, this);
}

void
EvPlcCsmaMac::OnCollisionEnd()
{
    m_collisionActive = false;
    Reschedule();
}

void
EvPlcCsmaMac::OnTxEnd(uint32_t nodeId)
{
    auto& node = NodeOf(nodeId);
    const uint64_t end = NowSlot();
    const HpgpFrame frame = *node.PeekFrame();

    const bool failed =
        m_channel->FrameFailsAt(nodeId, end - frame.durationSlots, frame.durationSlots);
    const HpgpFrame done = node.HandleSuccess(end);
    (void)done;

    if (failed)
    {
        HpgpFrame fresh = frame;
        fresh.attempt = 0;
        node.EnqueueFrame(fresh);
    }
    else if (frame.type == HpgpTrafficType::DC_REQ)
    {
        const uint32_t evIndex = nodeId - 1;
        Simulator::Schedule(SlotsToExactTime(end + m_params.m_cProcSlots) - Simulator::Now(),
                            &EvPlcCsmaMac::EnqueueResponse, this, evIndex);
    }
    else if (frame.type == HpgpTrafficType::DC_RES)
    {
        if (!m_responseFifo.empty())
        {
            m_responseFifo.pop_front();
        }
        ++m_responsesThisCycle;
    }
    else if (frame.type == HpgpTrafficType::SLAC)
    {
        const uint32_t sessionIndex = nodeId - kSessionBase;
        auto& session = m_sessions[sessionIndex];
        session.framePending = false;
        session.next += 1;
        if (session.next == PaperSequence().size())
        {
            session.completedCycle = static_cast<int>(m_currentCycle);
            ++m_pendingTransitions;
        }
        else
        {
            EnqueueSlacIfReleased(sessionIndex);
        }
    }

    if (node.GetState() == HpgpNodeState::CONTENDING)
    {
        node.StartBackoff(m_csmaParams, m_csmaRng);
        m_remaining[nodeId] = node.GetBackoff();
    }
    Reschedule();
}

CsmaRunStats
EvPlcCsmaMac::GetStats() const
{
    return m_stats;
}

} // namespace ns3
