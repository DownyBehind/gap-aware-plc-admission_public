#include "plc-error-model.h"
namespace ns3 {
void PlcErrorModel::SetSeed(uint32_t seed){ m_rng.seed(seed); }
void PlcErrorModel::SetPerOverride(double per){ m_hasOverride=true; m_overridePer=per; }
void PlcErrorModel::ClearPerOverride(){ m_hasOverride=false; }
double PlcErrorModel::SelectPer(HpgpTrafficType type,const PlcLinkProfile& p) const{ if(m_hasOverride) return m_overridePer; switch(type){case HpgpTrafficType::DC_REQ:return p.perReq;case HpgpTrafficType::DC_RES:return p.perRes;case HpgpTrafficType::SLAC:return p.perSlac;case HpgpTrafficType::OTHER:return p.mapLossProb;} return p.perReq; }
bool PlcErrorModel::FrameSucceeds(uint32_t evId,HpgpTrafficType type,const PlcLinkProfile& profile){ (void)evId; double per=SelectPer(type,profile); if(per<=0.0) return true; if(per>=1.0) return false; std::uniform_real_distribution<double> dist(0.0,1.0); return dist(m_rng)>=per; }
}
