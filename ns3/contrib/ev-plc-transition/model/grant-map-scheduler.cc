#include "grant-map-scheduler.h"
#include <algorithm>

namespace ns3
{

GrantMapScheduler::GrantMapScheduler(const EvPlcParams& params)
    : m_params(params), m_controller(params)
{
}

uint32_t
GrantMapScheduler::ComputeDcReqPhaseLength(uint32_t n) const
{
    return n * m_params.m_cReqEffSlots;
}

uint32_t
GrantMapScheduler::ComputeSlacAuthPhaseLength(uint32_t k) const
{
    return k * m_params.m_bAuthSlots;
}

uint32_t
GrantMapScheduler::ComputeDcResPhaseLength(uint32_t n) const
{
    return n * m_params.m_cResEffSlots;
}

uint32_t
GrantMapScheduler::ComputeFinishTime(uint32_t n, uint32_t k) const
{
    if (n == 0 && k == 0)
    {
        // Empty cycle: no frame for the carry-in envelope (B_blk) to delay.
        return 0;
    }
    const uint32_t authEnd = ComputeDcReqPhaseLength(n) + ComputeSlacAuthPhaseLength(k) + (k > 0 ? m_params.m_bPktSlots : 0);
    // The process floor is the earliest start of the first DC response, so it
    // applies only when a DC stream exists.
    const uint32_t procFloor = n > 0 ? m_params.m_cReqEffSlots + m_params.m_cProcSlots : 0;
    const uint32_t responseStart = std::max(authEnd, procFloor);
    return responseStart + ComputeDcResPhaseLength(n) + m_params.m_bBlkSlots;
}

EvPlcRegime
GrantMapScheduler::ClassifyRegime(uint32_t n, uint32_t k) const
{
    if (n == 0)
    {
        // Matches Python classify_regime: no DC stream, no hidden/paid split.
        return EvPlcRegime::SETUP_ONLY;
    }
    const uint32_t finish = ComputeFinishTime(n, k);
    const uint32_t terminalFinish = m_controller.ComputeDcOnlyFinish(n + k);
    if (finish > m_params.GetScheduledSlots() || terminalFinish > m_params.GetScheduledSlots())
    {
        return EvPlcRegime::REJECTED;
    }
    const uint32_t authWithPkt = ComputeSlacAuthPhaseLength(k) + (k > 0 ? m_params.m_bPktSlots : 0);
    return authWithPkt <= m_controller.ComputeProcessingGap(n) ? EvPlcRegime::HIDDEN : EvPlcRegime::PAID;
}

GrantMap
GrantMapScheduler::BuildGrantMap(uint32_t n, uint32_t k, uint64_t periodIndex) const
{
    GrantMap map;
    map.periodIndex = periodIndex;
    map.nDc = n;
    map.kSlac = k;
    map.regime = ClassifyRegime(n, k);

    uint32_t start = 0;
    map.phases.push_back({"DC_REQ", start, ComputeDcReqPhaseLength(n)});
    start += ComputeDcReqPhaseLength(n);
    map.phases.push_back({"SLAC_SERVICE", start, ComputeSlacAuthPhaseLength(k)});
    start += ComputeSlacAuthPhaseLength(k);
    const uint32_t responseStart = std::max(start + (k > 0 ? m_params.m_bPktSlots : 0), n > 0 ? m_params.m_cReqEffSlots + m_params.m_cProcSlots : 0);
    map.phases.push_back({"DC_RES", responseStart, ComputeDcResPhaseLength(n)});

    map.finishSlot = ComputeFinishTime(n, k);
    map.slackSlots = static_cast<int64_t>(m_params.GetScheduledSlots()) - static_cast<int64_t>(map.finishSlot);
    return map;
}

} // namespace ns3
