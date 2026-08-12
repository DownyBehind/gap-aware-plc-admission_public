#include "slac-session.h"
#include <algorithm>

namespace ns3
{

SlacSession::SlacSession(const EvPlcParams& params, Time start)
    : m_params(params), m_start(start)
{
}

void
SlacSession::Admit()
{
    m_state = SlacSessionState::ACTIVE_AUTH;
}

void
SlacSession::AddService(uint32_t slots)
{
    if (m_state != SlacSessionState::ACTIVE_AUTH)
    {
        return;
    }
    // Completion target is C_slac only: packetization debt is bounded by one
    // frame envelope and absorbed by the +T completion margin.
    m_servedSlots = std::min(m_params.m_cSlacSlots, m_servedSlots + slots);
    if (IsCompleted())
    {
        m_state = SlacSessionState::COMPLETED;
    }
}

bool
SlacSession::IsCompleted() const
{
    return m_servedSlots >= m_params.m_cSlacSlots;
}

bool
SlacSession::IsTimedOut(Time now) const
{
    return (now - m_start) > MilliSeconds(m_params.m_dSlacMs);
}

uint32_t
SlacSession::GetRemainingSlots() const
{
    const uint32_t total = m_params.m_cSlacSlots;
    return m_servedSlots >= total ? 0 : total - m_servedSlots;
}

SlacSessionState
SlacSession::GetState() const
{
    return m_state;
}

} // namespace ns3
