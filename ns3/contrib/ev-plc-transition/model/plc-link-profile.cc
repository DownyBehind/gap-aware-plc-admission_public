#include "plc-link-profile.h"
namespace ns3 {
std::string ToString(PlcProfileClass c){ switch(c){case PlcProfileClass::GOOD:return "GOOD";case PlcProfileClass::NOMINAL:return "NOMINAL";case PlcProfileClass::DEGRADED:return "DEGRADED";case PlcProfileClass::SEVERE:return "SEVERE";} return "NOMINAL"; }
uint32_t ProfileRank(PlcProfileClass c){ switch(c){case PlcProfileClass::GOOD:return 0;case PlcProfileClass::NOMINAL:return 1;case PlcProfileClass::DEGRADED:return 2;case PlcProfileClass::SEVERE:return 3;} return 1; }
PlcLinkProfile PlcLinkProfileTable::Get(PlcProfileClass c){ PlcLinkProfile p; p.profileClass=c; switch(c){case PlcProfileClass::GOOD:p={c,12,18,11,21,21,21,0.001,0.001,0.002,0.001};break;case PlcProfileClass::NOMINAL:p={c,15,21,14,21,21,21,0.005,0.005,0.010,0.002};break;case PlcProfileClass::DEGRADED:p={c,22,32,18,32,42,32,0.020,0.020,0.030,0.005};break;case PlcProfileClass::SEVERE:p={c,30,45,24,45,60,45,0.050,0.050,0.080,0.010};break;} return p; }
PlcProfileClass PlcLinkProfileTable::ClassFromPort(const ChargingPort& port){ if(port.branchDepth>=3||port.cableLengthM>45.0) return PlcProfileClass::SEVERE; if(port.cableLengthM<=10.0||port.branchDepth==0) return PlcProfileClass::GOOD; if(port.cableLengthM<=25.0) return PlcProfileClass::NOMINAL; return PlcProfileClass::DEGRADED; }
PlcLinkProfile PlcLinkProfileTable::FromPort(const ChargingPort& port){ return Get(ClassFromPort(port)); }
}
