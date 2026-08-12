#include "hpgp-csma-ca-baseline.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <numeric>

namespace ns3
{

HpgpCsmaCaBaseline::HpgpCsmaCaBaseline(const HpgpCsmaCaParams& params)
    : m_params(params), m_rng(1)
{
}

void HpgpCsmaCaBaseline::SetSeed(uint32_t seed) { m_rng.seed(seed); }
// Long runs (hours of charging time) must disable tracing: m_trace grows per event.
void HpgpCsmaCaBaseline::SetTraceEnabled(bool enabled) { m_traceEnabled = enabled; }
void HpgpCsmaCaBaseline::AddNode(uint32_t nodeId) { EnsureNode(nodeId); }

HpgpContentionNode&
HpgpCsmaCaBaseline::EnsureNode(uint32_t nodeId)
{
    auto it = m_nodes.find(nodeId);
    if (it == m_nodes.end())
    {
        it = m_nodes.emplace(nodeId, HpgpContentionNode(nodeId)).first;
    }
    return it->second;
}

void
HpgpCsmaCaBaseline::EnqueueFrame(uint32_t nodeId, HpgpFrame frame)
{
    frame.nodeId = nodeId;
    frame.enqueueSlot = m_currentSlot;
    auto& node = EnsureNode(nodeId);
    node.EnqueueFrame(frame);
    node.StartBackoff(m_params, m_rng);
    const auto* current = node.PeekFrame();
    Log({m_currentSlot, "enqueue", nodeId, frame.type, current ? current->attempt : 0, node.GetBackoff(), node.GetCw(), false, false, 0, 0, false, IsMediumBusy() ? "busy" : "idle"});
}

void
HpgpCsmaCaBaseline::SetBackoffForTest(uint32_t nodeId, uint32_t backoff, uint32_t cw, uint32_t dc)
{
    EnsureNode(nodeId).SetBackoffForTest(backoff, cw, dc);
}

bool
HpgpCsmaCaBaseline::IsMediumBusy() const
{
    return m_transmittingNode || m_currentSlot < m_mediumBusyUntil;
}

void
HpgpCsmaCaBaseline::ForceMediumBusy(uint64_t untilSlot)
{
    m_mediumBusyUntil = std::max(m_mediumBusyUntil, untilSlot);
}

void
HpgpCsmaCaBaseline::Step()
{
    if (m_transmittingNode && m_currentSlot >= m_mediumBusyUntil)
    {
        auto& node = m_nodes.at(*m_transmittingNode);
        auto frame = node.HandleSuccess(m_currentSlot);
        Log({m_currentSlot, "success", frame.nodeId, frame.type, frame.attempt, node.GetBackoff(), node.GetCw(), false, false, frame.startSlot, frame.endSlot, true, "idle"});
        m_transmittingNode.reset();
        if (node.HasFrame())
        {
            node.StartBackoff(m_params, m_rng);
        }
    }

    const bool busy = IsMediumBusy();
    for (auto& [id, node] : m_nodes)
    {
        if (busy)
        {
            node.TickBusyFreeze();
        }
        else
        {
            node.TickIdle();
        }
    }
    if (!IsMediumBusy())
    {
        ResolveContention();
    }
    ++m_currentSlot;
}

void
HpgpCsmaCaBaseline::ResolveContention()
{
    std::vector<uint32_t> ready;
    for (const auto& [id, node] : m_nodes)
    {
        if (node.IsReadyToTransmit())
        {
            ready.push_back(id);
        }
    }
    if (ready.empty())
    {
        return;
    }
    if (ready.size() == 1)
    {
        auto& node = m_nodes.at(ready.front());
        node.StartTransmission(m_currentSlot);
        const auto* frame = node.PeekFrame();
        m_transmittingNode = ready.front();
        m_mediumBusyUntil = frame ? frame->endSlot : m_currentSlot;
        Log({m_currentSlot, "tx_start", ready.front(), frame ? frame->type : HpgpTrafficType::OTHER, frame ? frame->attempt : 0, node.GetBackoff(), node.GetCw(), false, false, m_currentSlot, m_mediumBusyUntil, false, "busy"});
        return;
    }

    ++m_collisions;
    uint32_t collisionDuration = 1;
    for (auto id : ready)
    {
        const auto* frame = m_nodes.at(id).PeekFrame();
        if (frame)
        {
            collisionDuration = std::max(collisionDuration, frame->durationSlots);
        }
    }
    m_mediumBusyUntil = m_currentSlot + collisionDuration;
    for (auto id : ready)
    {
        auto& node = m_nodes.at(id);
        const auto* before = node.PeekFrame();
        const auto type = before ? before->type : HpgpTrafficType::OTHER;
        const auto attempt = before ? before->attempt : 0;
        const bool retry = node.HandleCollision(m_params, m_rng, m_currentSlot);
        Log({m_currentSlot, retry ? "collision_retry" : "collision_drop", id, type, attempt + 1, node.GetBackoff(), node.GetCw(), true, retry, m_currentSlot, m_mediumBusyUntil, false, "busy"});
    }
}

void HpgpCsmaCaBaseline::RunUntil(uint64_t endSlot) { while (m_currentSlot < endSlot) { Step(); } }

void
HpgpCsmaCaBaseline::RunUntilIdle(uint64_t maxSlots)
{
    while (m_currentSlot < maxSlots)
    {
        bool any = IsMediumBusy();
        for (const auto& [id, node] : m_nodes)
        {
            any = any || node.HasFrame();
        }
        if (!any)
        {
            break;
        }
        Step();
    }
}

void HpgpCsmaCaBaseline::Log(const HpgpTraceEvent& event) { if (m_traceEnabled) { m_trace.push_back(event); } }
const std::vector<HpgpTraceEvent>& HpgpCsmaCaBaseline::GetTrace() const { return m_trace; }
uint64_t HpgpCsmaCaBaseline::GetCurrentSlot() const { return m_currentSlot; }
const HpgpContentionNode& HpgpCsmaCaBaseline::GetNode(uint32_t nodeId) const { return m_nodes.at(nodeId); }

HpgpBaselineMetrics
HpgpCsmaCaBaseline::GetMetrics() const
{
    HpgpBaselineMetrics m;
    m.collisions = m_collisions;
    std::vector<uint64_t> latencies;
    for (const auto& e : m_trace)
    {
        if (e.eventType == "success")
        {
            ++m.successfulFrames;
            latencies.push_back(e.frameEnd - e.frameStart);
        }
        if (e.collision && e.retry)
        {
            ++m.retries;
        }
    }
    for (const auto& [id, node] : m_nodes)
    {
        m.drops += node.GetDropCount();
    }
    if (!latencies.empty())
    {
        m.maxLatencySlots = *std::max_element(latencies.begin(), latencies.end());
        m.averageLatencySlots = static_cast<double>(std::accumulate(latencies.begin(), latencies.end(), uint64_t{0})) / latencies.size();
    }
    return m;
}

void
HpgpCsmaCaBaseline::ExportTraceCsv(const std::string& path) const
{
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream out(path);
    out << "time_slot,event_type,node_id,traffic_type,attempt,backoff,cw,collision,retry,frame_start,frame_end,success,medium_state\n";
    for (const auto& e : m_trace)
    {
        out << e.timeSlot << ',' << e.eventType << ',' << e.nodeId << ',' << ToString(e.trafficType) << ','
            << e.attempt << ',' << e.backoff << ',' << e.cw << ',' << (e.collision ? 1 : 0) << ','
            << (e.retry ? 1 : 0) << ',' << e.frameStart << ',' << e.frameEnd << ',' << (e.success ? 1 : 0) << ','
            << e.mediumState << "\n";
    }
}

} // namespace ns3
