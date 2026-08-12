#include "plc-site-topology.h"
#include <algorithm>
namespace ns3 {
EvSiteTopology PlcSiteTopologyFactory::CreateSmallSite(){ EvSiteTopology t; t.controller.controllerId=0; const double l[]={5,8,10,12,15,18,22,25}; for(uint32_t i=0;i<8;++i){t.ports.push_back({i,l[i],i<3?0u:1u,static_cast<double>(i),0.0});} return t; }
EvSiteTopology PlcSiteTopologyFactory::CreateMediumSite(){ EvSiteTopology t; t.controller.controllerId=0; for(uint32_t i=0;i<20;++i){double len=5.0+(35.0*i)/19.0; t.ports.push_back({i,len,i%3,static_cast<double>(i%10),static_cast<double>(i/10)});} return t; }
EvSiteTopology PlcSiteTopologyFactory::CreateDepotSite(){ EvSiteTopology t; t.controller.controllerId=0; for(uint32_t i=0;i<40;++i){double len=5.0+(75.0*i)/39.0; t.ports.push_back({i,len,i%4,static_cast<double>(i%10),static_cast<double>(i/10)});} return t; }
EvSiteTopology PlcSiteTopologyFactory::CreateNamed(const std::string& name){ if(name=="small_site"||name=="near_only") return CreateSmallSite(); if(name=="depot_site"||name=="far_heavy"||name=="severe_heavy") return CreateDepotSite(); return CreateMediumSite(); }
}
