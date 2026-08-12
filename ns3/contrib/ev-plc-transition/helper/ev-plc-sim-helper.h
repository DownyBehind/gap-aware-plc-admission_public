#ifndef EV_PLC_SIM_HELPER_H
#define EV_PLC_SIM_HELPER_H

#include "ns3/ev-plc-params.h"
#include "ns3/grant-map-scheduler.h"
#include <string>

namespace ns3
{

class EvPlcSimHelper
{
  public:
    void Configure(const EvPlcParams& params);
    void AddInitialDcEvs(uint32_t n);
    void ScheduleSlacArrivals(uint32_t k);
    void Run(uint32_t periods);
    void ExportMetrics(const std::string& path,
                       const std::string& experiment,
                       const std::string& configFile = "",
                       const std::string& algorithm = "",
                       uint32_t seed = 1,
                       const std::string& ns3Command = "") const;

  private:
    EvPlcParams m_params;
    uint32_t m_initialDc{0};
    uint32_t m_activeSlac{0};
    uint32_t m_periods{0};
};

} // namespace ns3

#endif // EV_PLC_SIM_HELPER_H
