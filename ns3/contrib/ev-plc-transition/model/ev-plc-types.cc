#include "ev-plc-types.h"

namespace ns3
{

std::string
ToString(EvPlcRegime regime)
{
    switch (regime)
    {
    case EvPlcRegime::SETUP_ONLY:
        return "setup_only";
    case EvPlcRegime::HIDDEN:
        return "hidden";
    case EvPlcRegime::PAID:
        return "paid";
    case EvPlcRegime::REJECTED:
        return "rejected";
    }
    return "rejected";
}

} // namespace ns3
