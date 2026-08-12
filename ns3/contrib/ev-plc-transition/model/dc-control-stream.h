#ifndef DC_CONTROL_STREAM_H
#define DC_CONTROL_STREAM_H

#include "ev-plc-params.h"
#include "ns3/nstime.h"
#include <cstdint>
#include <vector>

namespace ns3
{

enum class DcControlState
{
    ACTIVE,
    COMPLETED
};

struct DcControlJob
{
    uint64_t periodIndex{0};
    Time release;
    Time requestFinish;
    Time responseReady;
    Time responseFinish;
    Time deadline;
};

class DcControlStream
{
  public:
    explicit DcControlStream(const EvPlcParams& params = EvPlcParams());
    DcControlJob GeneratePeriodJob(uint64_t periodIndex);
    Time GetDeadline(uint64_t periodIndex) const;
    const std::vector<DcControlJob>& GetJobs() const;

  private:
    EvPlcParams m_params;
    std::vector<DcControlJob> m_jobs;
};

} // namespace ns3

#endif // DC_CONTROL_STREAM_H
