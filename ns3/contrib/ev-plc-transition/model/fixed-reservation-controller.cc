#include "fixed-reservation-controller.h"
#include "transition-admission-controller.h"
#include <algorithm>

namespace ns3
{

FixedReservationController::FixedReservationController(const EvPlcParams& params)
    : m_params(params), m_scheduler(params)
{
}

uint32_t FixedReservationController::ComputeFixedReservationFinish(uint32_t n, uint32_t bFix) const { return m_scheduler.ComputeFixedReservationFinish(n, bFix); }
bool FixedReservationController::CheckFixedActiveFeasibility(uint32_t n, uint32_t bFix) const { return m_scheduler.CheckFixedActiveFeasibility(n, bFix); }
uint32_t FixedReservationController::ComputeFixedIdleWaste(uint32_t k, uint32_t bFix, uint32_t usedService) const { return m_scheduler.ComputeFixedIdleWaste(k, bFix, usedService); }
FixedGrantMap FixedReservationController::BuildFixedReservationGrantMap(uint32_t n, uint32_t k, uint32_t bFix) const { return m_scheduler.BuildFixedReservationGrantMap(n, k, bFix); }

void
FixedReservationController::DistributeFixedSlacService(uint32_t bFix, std::vector<SlacSession>& activeSessions) const
{
    if (activeSessions.empty() || bFix == 0)
    {
        return;
    }
    uint32_t remaining = bFix;
    std::size_t index = 0;
    while (remaining > 0 && !activeSessions.empty())
    {
        auto& session = activeSessions[index % activeSessions.size()];
        if (!session.IsCompleted())
        {
            session.AddService(1);
            --remaining;
        }
        ++index;
        if (index > activeSessions.size() * (bFix + 1))
        {
            break;
        }
    }
}

FixedReservationMetrics
FixedReservationController::RunFixedReservationPeriod(uint32_t periodIndex, uint32_t n, uint32_t k, uint32_t bFix) const
{
    const auto map = m_scheduler.BuildFixedReservationGrantMap(n, k, bFix);
    FixedReservationMetrics metrics;
    metrics.period = periodIndex;
    metrics.n = n;
    metrics.k = k;
    metrics.bFix = bFix;
    metrics.usedSlacService = map.usedSlacService;
    metrics.idleWaste = map.idleWaste;
    metrics.dcFinishSlot = map.finishSlot;
    metrics.dcSlack = map.slackSlots;
    metrics.dcDeadlineMiss = map.finishSlot > m_params.GetScheduledSlots();

    const uint32_t workPerSession = m_params.m_cSlacSlots;
    if (k == 0)
    {
        metrics.activeSlacRemaining = 0;
        return metrics;
    }
    const uint32_t cyclesNeeded = (workPerSession + std::max<uint32_t>(1, bFix / k) - 1) / std::max<uint32_t>(1, bFix / k);
    const double completionMs = (cyclesNeeded + 1) * m_params.m_tCtrlMs;
    if (completionMs <= m_params.m_dSlacMs)
    {
        metrics.slacCompletedCount = k;
        metrics.activeSlacRemaining = 0;
    }
    else
    {
        metrics.slacTimeoutCount = k;
        metrics.activeSlacRemaining = k;
    }
    return metrics;
}

FixedReservationMetrics
FixedReservationController::ComputeAdaptivePeriod(uint32_t periodIndex, uint32_t n, uint32_t k) const
{
    TransitionAdmissionController controller(m_params);
    const uint32_t adaptiveBudget = k * m_params.m_bAuthSlots;
    FixedReservationMetrics metrics;
    metrics.period = periodIndex;
    metrics.n = n;
    metrics.k = k;
    metrics.bFix = adaptiveBudget;
    metrics.usedSlacService = adaptiveBudget;
    metrics.idleWaste = 0;
    metrics.dcFinishSlot = std::max(n * m_params.m_cReqEffSlots + adaptiveBudget + (k > 0 ? m_params.m_bPktSlots : 0),
                                    m_params.m_cReqEffSlots + m_params.m_cProcSlots) + n * m_params.m_cResEffSlots + m_params.m_bBlkSlots;
    metrics.dcSlack = static_cast<int64_t>(m_params.GetScheduledSlots()) - static_cast<int64_t>(metrics.dcFinishSlot);
    metrics.dcDeadlineMiss = metrics.dcFinishSlot > m_params.GetScheduledSlots();
    if (k > 0 && controller.CheckSlacCompletion())
    {
        metrics.slacCompletedCount = k;
    }
    return metrics;
}

} // namespace ns3
