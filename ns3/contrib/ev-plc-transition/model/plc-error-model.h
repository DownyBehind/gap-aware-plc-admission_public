#ifndef PLC_ERROR_MODEL_H
#define PLC_ERROR_MODEL_H
#include "hpgp-frame.h"
#include "plc-link-profile.h"
#include <cstdint>
#include <random>
namespace ns3 {
class PlcErrorModel { public: void SetSeed(uint32_t seed); bool FrameSucceeds(uint32_t evId, HpgpTrafficType type, const PlcLinkProfile& profile); void SetPerOverride(double per); void ClearPerOverride(); private: double SelectPer(HpgpTrafficType type, const PlcLinkProfile& profile) const; bool m_hasOverride{false}; double m_overridePer{0.0}; std::mt19937 m_rng{1}; };
}
#endif
