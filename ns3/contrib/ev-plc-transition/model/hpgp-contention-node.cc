#include "hpgp-contention-node.h"
#include <algorithm>

namespace ns3
{

HpgpContentionNode::HpgpContentionNode(uint32_t nodeId)
    : m_nodeId(nodeId)
{
}

void
HpgpContentionNode::EnqueueFrame(const HpgpFrame& frame)
{
    m_queue.push_back(frame);
    if (!m_current && m_state == HpgpNodeState::IDLE)
    {
        m_current = m_queue.front();
        m_queue.pop_front();
        m_state = HpgpNodeState::CONTENDING;
    }
}

uint32_t
HpgpContentionNode::DrawBackoff(uint32_t cw, std::mt19937& rng) const
{
    std::uniform_int_distribution<uint32_t> dist(0, cw);
    return dist(rng);
}

void
HpgpContentionNode::StartBackoff(const HpgpCsmaCaParams& params, std::mt19937& rng)
{
    if (!m_current)
    {
        return;
    }
    const auto [dc, cw] = params.GetDcAndCw(m_bpc);
    m_dc = dc;
    m_cw = cw;
    m_backoff = DrawBackoff(cw, rng);
    m_state = HpgpNodeState::CONTENDING;
}

void
HpgpContentionNode::SetBackoffForTest(uint32_t backoff, uint32_t cw, uint32_t dc)
{
    m_backoff = backoff;
    m_cw = cw;
    m_dc = dc;
    m_state = m_current ? HpgpNodeState::CONTENDING : m_state;
}

void
HpgpContentionNode::TickIdle()
{
    if (m_state == HpgpNodeState::CONTENDING && m_backoff > 0)
    {
        --m_backoff;
    }
}

void
HpgpContentionNode::TickBusyFreeze()
{
    // Verification baseline uses the requested freeze semantics: backoff does not decrement while busy.
}

bool
HpgpContentionNode::IsReadyToTransmit() const
{
    return m_state == HpgpNodeState::CONTENDING && m_current && m_backoff == 0;
}

void
HpgpContentionNode::StartTransmission(uint64_t startSlot)
{
    if (!m_current)
    {
        return;
    }
    m_current->startSlot = startSlot;
    m_current->endSlot = startSlot + m_current->durationSlots;
    m_state = HpgpNodeState::TRANSMITTING;
}

bool
HpgpContentionNode::HandleCollision(const HpgpCsmaCaParams& params, std::mt19937& rng, uint64_t nowSlot)
{
    if (!m_current)
    {
        return false;
    }
    ++m_retryCount;
    ++m_current->attempt;
    m_current->startSlot = nowSlot;
    m_current->endSlot = nowSlot + std::max<uint32_t>(1, params.m_collisionDurationSlots);
    if (m_current->attempt > params.m_maxRetries)
    {
        ++m_dropCount;
        m_current.reset();
        m_state = HpgpNodeState::IDLE;
        if (!m_queue.empty())
        {
            m_current = m_queue.front();
            m_queue.pop_front();
            m_bpc = 0;
            StartBackoff(params, rng);
        }
        return false;
    }
    m_bpc = std::min<uint32_t>(m_current->attempt, params.m_maxBpc);
    StartBackoff(params, rng);
    return true;
}

HpgpFrame
HpgpContentionNode::HandleSuccess(uint64_t endSlot)
{
    HpgpFrame completed = *m_current;
    completed.endSlot = endSlot;
    ++m_successCount;
    m_current.reset();
    m_state = HpgpNodeState::IDLE;
    m_bpc = 0;
    m_dc = 0;
    m_backoff = 0;
    m_cw = 0;
    if (!m_queue.empty())
    {
        m_current = m_queue.front();
        m_queue.pop_front();
        m_state = HpgpNodeState::CONTENDING;
    }
    return completed;
}

uint32_t HpgpContentionNode::GetNodeId() const { return m_nodeId; }
uint32_t HpgpContentionNode::GetBackoff() const { return m_backoff; }
uint32_t HpgpContentionNode::GetCw() const { return m_cw; }
uint32_t HpgpContentionNode::GetAttempt() const { return m_current ? m_current->attempt : 0; }
uint32_t HpgpContentionNode::GetRetryCount() const { return m_retryCount; }
uint32_t HpgpContentionNode::GetDropCount() const { return m_dropCount; }
uint32_t HpgpContentionNode::GetSuccessCount() const { return m_successCount; }
HpgpNodeState HpgpContentionNode::GetState() const { return m_state; }
const HpgpFrame* HpgpContentionNode::PeekFrame() const { return m_current ? &(*m_current) : nullptr; }
bool HpgpContentionNode::HasFrame() const { return static_cast<bool>(m_current) || !m_queue.empty(); }

} // namespace ns3
