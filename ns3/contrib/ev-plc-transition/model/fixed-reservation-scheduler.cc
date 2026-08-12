#include "fixed-reservation-scheduler.h"
#include <algorithm>

namespace ns3
{

FixedReservationScheduler::FixedReservationScheduler(const EvPlcParams& params)
    : m_params(params)
{
}

uint32_t
FixedReservationScheduler::ComputeFixedReservationFinish(uint32_t n, uint32_t bFix) const
{
    const uint32_t responseStart = std::max(n * m_params.m_cReqEffSlots + bFix + m_params.m_bPktSlots,
                                            m_params.m_cReqEffSlots + m_params.m_cProcSlots);
    return responseStart + n * m_params.m_cResEffSlots + m_params.m_bBlkSlots;
}

bool
FixedReservationScheduler::CheckFixedActiveFeasibility(uint32_t n, uint32_t bFix) const
{
    return ComputeFixedReservationFinish(n, bFix) <= m_params.GetScheduledSlots();
}

uint32_t
FixedReservationScheduler::ComputeFixedIdleWaste(uint32_t k, uint32_t bFix, uint32_t usedService) const
{
    if (k == 0)
    {
        return bFix;
    }
    return usedService >= bFix ? 0 : bFix - usedService;
}

FixedGrantMap
FixedReservationScheduler::BuildFixedReservationGrantMap(uint32_t n, uint32_t k, uint32_t bFix) const
{
    FixedGrantMap map;
    map.nDc = n;
    map.kSlac = k;
    map.bFix = bFix;
    map.usedSlacService = k == 0 ? 0 : std::min<uint32_t>(bFix, k * m_params.m_bAuthSlots);
    map.idleWaste = ComputeFixedIdleWaste(k, bFix, map.usedSlacService);
    map.finishSlot = ComputeFixedReservationFinish(n, bFix);
    map.slackSlots = static_cast<int64_t>(m_params.GetScheduledSlots()) - static_cast<int64_t>(map.finishSlot);

    uint32_t start = 0;
    map.phases.push_back({"O_map", 0, m_params.m_oMapSlots});
    map.phases.push_back({"DC_REQ", start, n * m_params.m_cReqEffSlots});
    start += n * m_params.m_cReqEffSlots;
    map.phases.push_back({"FIXED_SLAC_RESERVATION", start, bFix});
    start += bFix;
    const uint32_t resStart = std::max(start + m_params.m_bPktSlots, m_params.m_cReqEffSlots + m_params.m_cProcSlots);
    map.phases.push_back({"DC_RES", resStart, n * m_params.m_cResEffSlots});
    return map;
}

} // namespace ns3
