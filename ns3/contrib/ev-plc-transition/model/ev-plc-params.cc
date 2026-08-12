#include "ev-plc-params.h"

namespace ns3
{

EvPlcParams::EvPlcParams()
    : m_tCtrlMs(50.0),
      m_slotDurationUs(35.84),
      m_tCtrlSlots(1395),
      // O_map retained as the constant term c_0 of C_bcn(N,k); the §7 baseline
      // runs with 0 so admission is checked against T = 1395.
      m_oMapSlots(0),
      m_cReqEffSlots(15),
      m_cResEffSlots(21),
      m_cProcSlots(280),
      m_bAuthSlots(7),
      m_bPktSlots(21),
      m_bBlkSlots(21),
      m_cSlacSlots(247),
      m_dSlacMs(2000.0)
{
}

uint32_t
EvPlcParams::GetScheduledSlots() const
{
    return m_tCtrlSlots - m_oMapSlots;
}

double
EvPlcParams::GetSlotDurationUs() const
{
    return m_slotDurationUs;
}

Time
EvPlcParams::SlotsToTime(uint32_t slots) const
{
    return MicroSeconds(slots * m_slotDurationUs);
}

} // namespace ns3
