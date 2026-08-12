#ifndef LINK_AWARE_ADMISSION_H
#define LINK_AWARE_ADMISSION_H
#include "ev-plc-node-profile.h"
#include <cstdint>
#include <string>
#include <vector>
namespace ns3 {
enum class AdmissionMode { COUNT_BASED_NOMINAL, COUNT_BASED_WORST_CASE, LINK_AWARE_DEMAND_BASED };
enum class OrderingMode { INPUT_ORDER, WORST_LINK_FIRST };
struct AdmissionDecision { bool admit{false}; uint32_t finishSlots{0}; int64_t slackSlots{0}; std::string reason; };
class LinkAwareAdmissionController { public: LinkAwareAdmissionController(uint32_t tCtrlSlots=1395,uint32_t oMapSlots=0,uint32_t bAuthSlots=7); void SetOrderingMode(OrderingMode mode); AdmissionDecision EvaluateActive(const std::vector<EvCommunicationDemand>& dc, const std::vector<EvCommunicationDemand>& slac, const EvCommunicationDemand& candidate, AdmissionMode mode) const; AdmissionDecision EvaluateTerminal(const std::vector<EvCommunicationDemand>& projectedDc, AdmissionMode mode) const; bool GroundTruthFeasible(const std::vector<EvCommunicationDemand>& dc) const; private: std::vector<EvCommunicationDemand> Ordered(std::vector<EvCommunicationDemand> demands) const; uint32_t ComputeFinish(const std::vector<EvCommunicationDemand>& dc, uint32_t authSlots, uint32_t bPkt, uint32_t bBlk) const; uint32_t m_tCtrlSlots; uint32_t m_oMapSlots; uint32_t m_bAuthSlots; OrderingMode m_ordering{OrderingMode::INPUT_ORDER}; };
}
#endif
