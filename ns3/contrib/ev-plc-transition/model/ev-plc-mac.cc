#include "ev-plc-mac.h"

#include "ns3/nstime.h"
#include "ns3/simulator.h"

#include <algorithm>

namespace ns3
{

EvPlcMac::EvPlcMac(const EvPlcParams& params, Ptr<PlcSharedChannel> channel)
    : m_params(params), m_scheduler(params), m_channel(channel)
{
}

void
EvPlcMac::AddDcEv()
{
    ++m_nDc;
}

void
EvPlcMac::AddSlacSession()
{
    ++m_kSlac;
}

void
EvPlcMac::SetHeadBlockSlots(uint32_t slots)
{
    m_headBlockSlots = slots;
}

Time
EvPlcMac::SlotsToExactTime(uint64_t slots)
{
    // 35.84 us per slot = exactly 35840 ns: integer conversion only, so the
    // event timeline carries no floating-point drift (TRACKC_PLAN §1.3).
    return NanoSeconds(slots * 35840ULL);
}

void
EvPlcMac::Start(uint32_t cycles)
{
    m_cyclesRemaining = cycles;
    m_cycleIndex = 0;
    // Two-hop bootstrap: Application::Initialize re-schedules StartApplication
    // as a fresh t=0 event, so a directly scheduled StartCycle would run
    // before any demand is registered. Hopping once puts StartCycle behind
    // every StartApplication in the t=0 FIFO.
    Simulator::Schedule(Time(0), &EvPlcMac::Bootstrap, this);
}

void
EvPlcMac::Bootstrap()
{
    Simulator::Schedule(Time(0), &EvPlcMac::StartCycle, this);
}

void
EvPlcMac::StartCycle()
{
    m_cycleStartSlot = static_cast<uint64_t>(m_cycleIndex) * m_params.m_tCtrlSlots;

    // Single SoT: the map (phases, regime, formula finish) comes from the
    // GrantMapScheduler; this MAC only *plays* it as events.
    const auto map = m_scheduler.BuildGrantMap(m_nDc, m_kSlac, m_cycleIndex);
    (void)map; // regime/finish metadata available for tracing; replay follows

    m_queue.clear();
    m_readySlot.assign(m_nDc, 0);
    m_current = MacCycleRecord{};
    m_current.n = m_nDc;
    m_current.k = m_kSlac;

    if (m_headBlockSlots > 0)
    {
        m_queue.push_back({"HEAD", 0, m_headBlockSlots, false, 0, false});
    }
    for (uint32_t ev = 0; ev < m_nDc; ++ev)
    {
        m_queue.push_back({"DC_REQ", ev, m_params.m_cReqEffSlots, false, 0, true});
    }
    if (m_kSlac > 0)
    {
        m_queue.push_back({"SLAC_SERVICE", 0, m_params.m_bAuthSlots * m_kSlac, false, 0, false});
        m_queue.push_back({"PKT_GUARD", 0, m_params.m_bPktSlots, false, 0, false});
    }
    for (uint32_t ev = 0; ev < m_nDc; ++ev)
    {
        // Gate filled when this EV's request completes (proc readiness).
        m_queue.push_back({"DC_RES", ev, m_params.m_cResEffSlots, true, 0, true});
    }
    TryNext();
}

void
EvPlcMac::TryNext()
{
    if (m_queue.empty())
    {
        // Cycle done: record and schedule the next cycle boundary.
        if (m_current.lastFrameEndSlot > 0)
        {
            m_current.finishSlot = static_cast<uint32_t>(
                m_current.lastFrameEndSlot - m_cycleStartSlot + m_params.m_bBlkSlots);
        }
        m_records.push_back(m_current);
        ++m_cycleIndex;
        if (--m_cyclesRemaining > 0)
        {
            const uint64_t next = static_cast<uint64_t>(m_cycleIndex) * m_params.m_tCtrlSlots;
            Simulator::Schedule(SlotsToExactTime(next) - Simulator::Now(), &EvPlcMac::StartCycle,
                                this);
        }
        return;
    }
    auto& frame = m_queue.front();
    const uint64_t free = m_channel->GetBusyUntilSlot();
    const uint64_t nowFloor = std::max(free, m_cycleStartSlot);
    const uint64_t start = frame.hasGate ? std::max(nowFloor, frame.gateSlot) : nowFloor;
    m_channel->Occupy(start, frame.durationSlots);
    Simulator::Schedule(SlotsToExactTime(start + frame.durationSlots) - Simulator::Now(),
                        &EvPlcMac::OnTxEnd, this, frame.durationSlots);
}

void
EvPlcMac::OnTxEnd(uint32_t durationSlots)
{
    auto frame = m_queue.front();
    const uint64_t end = m_channel->GetBusyUntilSlot();

    if (frame.perApplies &&
        m_channel->FrameFailsAt(frame.evId, end - durationSlots, durationSlots))
    {
        // Immediate retransmission: same frame, channel stays with us.
        ++m_current.retxCount;
        m_channel->Occupy(end, durationSlots);
        Simulator::Schedule(SlotsToExactTime(end + durationSlots) - Simulator::Now(),
                            &EvPlcMac::OnTxEnd, this, durationSlots);
        return;
    }

    if (frame.name == "DC_REQ")
    {
        m_readySlot[frame.evId] = end + m_params.m_cProcSlots;
        // Fill the gate of this EV's response (queued later in this cycle).
        for (auto& pending : m_queue)
        {
            if (pending.name == "DC_RES" && pending.evId == frame.evId)
            {
                pending.gateSlot = m_readySlot[frame.evId];
                break;
            }
        }
    }
    else if (frame.name == "DC_RES")
    {
        m_current.responseEndSlots.push_back(end);
    }
    if (frame.name != "HEAD")
    {
        m_current.lastFrameEndSlot = end;
    }
    m_queue.pop_front();
    TryNext();
}

const std::vector<MacCycleRecord>&
EvPlcMac::GetRecords() const
{
    return m_records;
}

} // namespace ns3
