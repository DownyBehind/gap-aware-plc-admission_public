#include "hpgp-csma-ca-params.h"
#include <algorithm>

namespace ns3
{

HpgpCsmaCaParams::HpgpCsmaCaParams()
    : m_slotDurationUs(35.84),
      m_tCtrlSlots(1395),
      m_maxBpc(4),
      m_maxRetries(4),
      m_collisionDurationSlots(1),
      m_ca3Params{{0, 7}, {1, 15}, {3, 15}, {15, 31}}
{
}

std::pair<uint32_t, uint32_t>
HpgpCsmaCaParams::GetDcAndCw(uint32_t bpc) const
{
    const auto index = std::min<uint32_t>(bpc, m_ca3Params.size() - 1);
    return m_ca3Params[index];
}

Time
HpgpCsmaCaParams::SlotsToTime(uint64_t slots) const
{
    return MicroSeconds(static_cast<double>(slots) * m_slotDurationUs);
}

uint64_t
HpgpCsmaCaParams::TimeToSlots(Time time) const
{
    return static_cast<uint64_t>(time.GetMicroSeconds() / m_slotDurationUs);
}

} // namespace ns3
