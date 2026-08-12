#ifndef FIXED_RESERVATION_SCHEDULER_H
#define FIXED_RESERVATION_SCHEDULER_H

#include "ev-plc-params.h"
#include "ev-plc-types.h"
#include <cstdint>
#include <vector>

namespace ns3
{

struct FixedGrantMap
{
    uint64_t period{0};
    uint32_t nDc{0};
    uint32_t kSlac{0};
    uint32_t bFix{0};
    uint32_t finishSlot{0};
    int64_t slackSlots{0};
    uint32_t usedSlacService{0};
    uint32_t idleWaste{0};
    std::vector<GrantMapPhase> phases;
};

class FixedReservationScheduler
{
  public:
    explicit FixedReservationScheduler(const EvPlcParams& params = EvPlcParams());

    uint32_t ComputeFixedReservationFinish(uint32_t n, uint32_t bFix) const;
    bool CheckFixedActiveFeasibility(uint32_t n, uint32_t bFix) const;
    uint32_t ComputeFixedIdleWaste(uint32_t k, uint32_t bFix, uint32_t usedService) const;
    FixedGrantMap BuildFixedReservationGrantMap(uint32_t n, uint32_t k, uint32_t bFix) const;

  private:
    EvPlcParams m_params;
};

} // namespace ns3

#endif // FIXED_RESERVATION_SCHEDULER_H
