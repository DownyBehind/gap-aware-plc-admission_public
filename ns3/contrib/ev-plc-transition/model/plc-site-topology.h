#ifndef PLC_SITE_TOPOLOGY_H
#define PLC_SITE_TOPOLOGY_H
#include <cstdint>
#include <string>
#include <vector>
namespace ns3 {
struct ChargingPort { uint32_t portId{0}; double cableLengthM{0.0}; uint32_t branchDepth{0}; double x{0.0}; double y{0.0}; };
struct EvseController { uint32_t controllerId{0}; };
struct EvSiteTopology { EvseController controller; std::vector<ChargingPort> ports; };
class PlcSiteTopologyFactory { public: static EvSiteTopology CreateSmallSite(); static EvSiteTopology CreateMediumSite(); static EvSiteTopology CreateDepotSite(); static EvSiteTopology CreateNamed(const std::string& name); };
}
#endif
