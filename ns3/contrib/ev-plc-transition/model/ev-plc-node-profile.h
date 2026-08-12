#ifndef EV_PLC_NODE_PROFILE_H
#define EV_PLC_NODE_PROFILE_H
#include "plc-link-profile.h"
#include <cstdint>
namespace ns3 {
struct EvCommunicationDemand { uint32_t evId{0}; uint32_t portId{0}; PlcProfileClass profileClass{PlcProfileClass::NOMINAL}; uint32_t cReqEffSlots{15}; uint32_t cResEffSlots{21}; uint32_t cProcSlots{280}; uint32_t bFrameSlots{21}; uint32_t bBlkSlots{21}; uint32_t bPktSlots{21}; double perReq{0.005}; double perRes{0.005}; uint32_t creditSlots{0}; };
EvCommunicationDemand MakeDemand(uint32_t evId, uint32_t portId, const PlcLinkProfile& profile, uint32_t cProcSlots = 280);
}
#endif
