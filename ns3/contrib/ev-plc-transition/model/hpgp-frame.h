#ifndef HPGP_FRAME_H
#define HPGP_FRAME_H

#include <cstdint>
#include <string>

namespace ns3
{

enum class HpgpTrafficType
{
    DC_REQ,
    DC_RES,
    SLAC,
    OTHER
};

struct HpgpFrame
{
    uint32_t nodeId{0};
    HpgpTrafficType type{HpgpTrafficType::OTHER};
    uint32_t durationSlots{0};
    uint32_t attempt{0};
    uint64_t enqueueSlot{0};
    uint64_t startSlot{0};
    uint64_t endSlot{0};
};

std::string ToString(HpgpTrafficType type);

} // namespace ns3

#endif // HPGP_FRAME_H
