#ifndef TRANSITION_ADMISSION_CONTROLLER_H
#define TRANSITION_ADMISSION_CONTROLLER_H

#include "ev-plc-params.h"
#include <cstdint>

namespace ns3
{

class TransitionAdmissionController
{
  public:
    explicit TransitionAdmissionController(const EvPlcParams& params = EvPlcParams());

    bool CheckCondA(uint32_t n, uint32_t k) const;
    bool CheckCondB(uint32_t n, uint32_t k) const;
    bool Admit(uint32_t n, uint32_t k) const;
    double ComputeTransitionAmplification() const;
    uint32_t ComputeProcessingGap(uint32_t n) const;
    uint32_t ComputeDcOnlyFinish(uint32_t n) const;
    uint32_t ComputeSlacCompletionCycles() const;
    bool CheckSlacCompletion() const;
    uint32_t ComputeSlackDegradationPerCompletion() const;
    const EvPlcParams& GetParams() const;

  private:
    EvPlcParams m_params;
};

} // namespace ns3

#endif // TRANSITION_ADMISSION_CONTROLLER_H
