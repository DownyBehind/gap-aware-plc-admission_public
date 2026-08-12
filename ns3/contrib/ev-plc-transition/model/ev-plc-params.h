#ifndef EV_PLC_PARAMS_H
#define EV_PLC_PARAMS_H

#include "ns3/nstime.h"
#include <cstdint>

namespace ns3
{

class EvPlcParams
{
  public:
    EvPlcParams();

    uint32_t GetScheduledSlots() const;
    double GetSlotDurationUs() const;
    Time SlotsToTime(uint32_t slots) const;

    double m_tCtrlMs;
    double m_slotDurationUs;
    uint32_t m_tCtrlSlots;
    uint32_t m_oMapSlots;
    uint32_t m_cReqEffSlots;
    uint32_t m_cResEffSlots;
    uint32_t m_cProcSlots;
    uint32_t m_bAuthSlots;
    uint32_t m_bPktSlots;
    uint32_t m_bBlkSlots;
    uint32_t m_cSlacSlots;
    double m_dSlacMs;
};

} // namespace ns3

#endif // EV_PLC_PARAMS_H
