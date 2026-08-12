#ifndef PLC_LINK_PROFILE_H
#define PLC_LINK_PROFILE_H
#include "plc-site-topology.h"
#include <cstdint>
#include <string>
namespace ns3 {
enum class PlcProfileClass { GOOD, NOMINAL, DEGRADED, SEVERE };
struct PlcLinkProfile { PlcProfileClass profileClass{PlcProfileClass::NOMINAL}; uint32_t cReqEffSlots{15}; uint32_t cResEffSlots{21}; uint32_t cSlacFrameEffSlots{14}; uint32_t bFrameSlots{21}; uint32_t bBlkSlots{21}; uint32_t bPktSlots{21}; double perReq{0.005}; double perRes{0.005}; double perSlac{0.010}; double mapLossProb{0.002}; };
std::string ToString(PlcProfileClass profileClass);
uint32_t ProfileRank(PlcProfileClass profileClass);
class PlcLinkProfileTable { public: static PlcLinkProfile Get(PlcProfileClass profileClass); static PlcLinkProfile FromPort(const ChargingPort& port); static PlcProfileClass ClassFromPort(const ChargingPort& port); };
}
#endif
