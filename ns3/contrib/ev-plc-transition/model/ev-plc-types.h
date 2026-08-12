#ifndef EV_PLC_TYPES_H
#define EV_PLC_TYPES_H

#include <cstdint>
#include <string>
#include <vector>

namespace ns3
{

enum class EvPlcRegime
{
    SETUP_ONLY,
    HIDDEN,
    PAID,
    REJECTED
};

struct GrantMapPhase
{
    std::string name;
    uint32_t startSlot{0};
    uint32_t durationSlots{0};
};

struct GrantMap
{
    uint64_t periodIndex{0};
    uint32_t nDc{0};
    uint32_t kSlac{0};
    uint32_t finishSlot{0};
    int64_t slackSlots{0};
    EvPlcRegime regime{EvPlcRegime::REJECTED};
    std::vector<GrantMapPhase> phases;
};

std::string ToString(EvPlcRegime regime);

} // namespace ns3

#endif // EV_PLC_TYPES_H
