#include "transition-admission-controller.h"
#include <algorithm>
#include <cassert>

namespace ns3
{

TransitionAdmissionController::TransitionAdmissionController(const EvPlcParams& params)
    : m_params(params)
{
}

bool
TransitionAdmissionController::CheckCondA(uint32_t n, uint32_t k) const
{
    const uint32_t channelTerm = n * m_params.m_cReqEffSlots + m_params.m_bAuthSlots * (k + 1) + m_params.m_bPktSlots;
    // The process floor exists only when a DC stream exists.
    const uint32_t processingTerm = n > 0 ? m_params.m_cReqEffSlots + m_params.m_cProcSlots : 0;
    const uint32_t finish = std::max(channelTerm, processingTerm) + n * m_params.m_cResEffSlots + m_params.m_bBlkSlots;
    return finish <= m_params.GetScheduledSlots();
}

bool
TransitionAdmissionController::CheckCondB(uint32_t n, uint32_t k) const
{
    return ComputeDcOnlyFinish(n + k + 1) <= m_params.GetScheduledSlots();
}

bool
TransitionAdmissionController::Admit(uint32_t n, uint32_t k) const
{
    return CheckCondA(n, k) && CheckCondB(n, k) && CheckSlacCompletion();
}

double
TransitionAdmissionController::ComputeTransitionAmplification() const
{
    return static_cast<double>(m_params.m_cReqEffSlots + m_params.m_cResEffSlots) / m_params.m_bAuthSlots;
}

uint32_t
TransitionAdmissionController::ComputeProcessingGap(uint32_t n) const
{
    // Matches Python G_processing_gap: defined only for n >= 1; the n = 0
    // state is SETUP_ONLY and must be classified before consulting the gap.
    assert(n >= 1);
    const int64_t gap = static_cast<int64_t>(m_params.m_cProcSlots) - static_cast<int64_t>(n - 1) * m_params.m_cReqEffSlots;
    return gap > 0 ? static_cast<uint32_t>(gap) : 0;
}

uint32_t
TransitionAdmissionController::ComputeDcOnlyFinish(uint32_t n) const
{
    const uint32_t responseStart = std::max(n * m_params.m_cReqEffSlots, m_params.m_cReqEffSlots + m_params.m_cProcSlots);
    return responseStart + n * m_params.m_cResEffSlots + m_params.m_bBlkSlots;
}

uint32_t
TransitionAdmissionController::ComputeSlacCompletionCycles() const
{
    // Packetization debt (B_pkt) is excluded: bounded by one frame envelope under
    // debt-neutral packetization and absorbed by the +1 cycle rounding margin.
    return (m_params.m_cSlacSlots + m_params.m_bAuthSlots - 1) / m_params.m_bAuthSlots;
}

bool
TransitionAdmissionController::CheckSlacCompletion() const
{
    // Admission-time per-session multiple check, L_init-inclusive: (cycles + 1) * T.
    const double completionMs = (ComputeSlacCompletionCycles() + 1) * m_params.m_tCtrlMs;
    return completionMs <= m_params.m_dSlacMs;
}

uint32_t
TransitionAdmissionController::ComputeSlackDegradationPerCompletion() const
{
    return m_params.m_cReqEffSlots + m_params.m_cResEffSlots - m_params.m_bAuthSlots;
}

const EvPlcParams&
TransitionAdmissionController::GetParams() const
{
    return m_params;
}

} // namespace ns3
