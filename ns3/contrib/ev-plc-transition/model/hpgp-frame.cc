#include "hpgp-frame.h"

namespace ns3
{

std::string
ToString(HpgpTrafficType type)
{
    switch (type)
    {
    case HpgpTrafficType::DC_REQ:
        return "DC_REQ";
    case HpgpTrafficType::DC_RES:
        return "DC_RES";
    case HpgpTrafficType::SLAC:
        return "SLAC";
    case HpgpTrafficType::OTHER:
        return "OTHER";
    }
    return "OTHER";
}

} // namespace ns3
