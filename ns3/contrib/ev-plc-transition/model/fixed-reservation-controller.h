#ifndef FIXED_RESERVATION_CONTROLLER_H
#define FIXED_RESERVATION_CONTROLLER_H

#include "fixed-reservation-scheduler.h"
#include "slac-session.h"
#include <cstdint>
#include <vector>

namespace ns3
{

struct FixedReservationMetrics
{
    uint32_t period{0};
    uint32_t n{0};
    uint32_t k{0};
    uint32_t bFix{0};
    uint32_t usedSlacService{0};
    uint32_t idleWaste{0};
    uint32_t dcFinishSlot{0};
    int64_t dcSlack{0};
    bool dcDeadlineMiss{false};
    uint32_t slacCompletedCount{0};
    uint32_t slacTimeoutCount{0};
    uint32_t activeSlacRemaining{0};
};

class FixedReservationController
{
  public:
    explicit FixedReservationController(const EvPlcParams& params = EvPlcParams());

    uint32_t ComputeFixedReservationFinish(uint32_t n, uint32_t bFix) const;
    bool CheckFixedActiveFeasibility(uint32_t n, uint32_t bFix) const;
    uint32_t ComputeFixedIdleWaste(uint32_t k, uint32_t bFix, uint32_t usedService) const;
    FixedGrantMap BuildFixedReservationGrantMap(uint32_t n, uint32_t k, uint32_t bFix) const;
    void DistributeFixedSlacService(uint32_t bFix, std::vector<SlacSession>& activeSessions) const;
    FixedReservationMetrics RunFixedReservationPeriod(uint32_t periodIndex, uint32_t n, uint32_t k, uint32_t bFix) const;

    FixedReservationMetrics ComputeAdaptivePeriod(uint32_t periodIndex, uint32_t n, uint32_t k) const;

  private:
    EvPlcParams m_params;
    FixedReservationScheduler m_scheduler;
};

} // namespace ns3

#endif // FIXED_RESERVATION_CONTROLLER_H
