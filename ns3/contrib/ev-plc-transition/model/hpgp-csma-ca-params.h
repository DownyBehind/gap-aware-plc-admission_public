#ifndef HPGP_CSMA_CA_PARAMS_H
#define HPGP_CSMA_CA_PARAMS_H

#include "ns3/nstime.h"
#include <cstdint>
#include <utility>
#include <vector>

namespace ns3
{

class HpgpCsmaCaParams
{
  public:
    HpgpCsmaCaParams();

    std::pair<uint32_t, uint32_t> GetDcAndCw(uint32_t bpc) const;
    Time SlotsToTime(uint64_t slots) const;
    uint64_t TimeToSlots(Time time) const;

    double m_slotDurationUs;
    uint32_t m_tCtrlSlots;
    uint32_t m_maxBpc;
    uint32_t m_maxRetries;
    uint32_t m_collisionDurationSlots;
    std::vector<std::pair<uint32_t, uint32_t>> m_ca3Params;
};

} // namespace ns3

#endif // HPGP_CSMA_CA_PARAMS_H
