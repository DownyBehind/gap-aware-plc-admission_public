#ifndef GRANT_MAP_SCHEDULER_H
#define GRANT_MAP_SCHEDULER_H

#include "ev-plc-params.h"
#include "ev-plc-types.h"
#include "transition-admission-controller.h"
#include <cstdint>

namespace ns3
{

class GrantMapScheduler
{
  public:
    explicit GrantMapScheduler(const EvPlcParams& params = EvPlcParams());

    GrantMap BuildGrantMap(uint32_t n, uint32_t k, uint64_t periodIndex = 0) const;
    uint32_t ComputeDcReqPhaseLength(uint32_t n) const;
    uint32_t ComputeSlacAuthPhaseLength(uint32_t k) const;
    uint32_t ComputeDcResPhaseLength(uint32_t n) const;
    uint32_t ComputeFinishTime(uint32_t n, uint32_t k) const;
    EvPlcRegime ClassifyRegime(uint32_t n, uint32_t k) const;

  private:
    EvPlcParams m_params;
    TransitionAdmissionController m_controller;
};

} // namespace ns3

#endif // GRANT_MAP_SCHEDULER_H
