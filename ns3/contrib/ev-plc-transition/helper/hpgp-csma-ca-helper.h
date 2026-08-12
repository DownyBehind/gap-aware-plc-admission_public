#ifndef HPGP_CSMA_CA_HELPER_H
#define HPGP_CSMA_CA_HELPER_H

#include "ns3/hpgp-csma-ca-baseline.h"
#include <string>

namespace ns3
{

class HpgpCsmaCaHelper
{
  public:
    static HpgpBaselineMetrics RunExp1Baseline(const std::string& outputDir, uint32_t seed = 1);
};

} // namespace ns3

#endif // HPGP_CSMA_CA_HELPER_H
