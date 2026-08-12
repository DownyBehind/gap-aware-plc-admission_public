#include "dc-control-stream.h"

namespace ns3
{

DcControlStream::DcControlStream(const EvPlcParams& params)
    : m_params(params)
{
}

DcControlJob
DcControlStream::GeneratePeriodJob(uint64_t periodIndex)
{
    DcControlJob job;
    job.periodIndex = periodIndex;
    job.release = MilliSeconds(periodIndex * m_params.m_tCtrlMs);
    job.requestFinish = job.release + m_params.SlotsToTime(m_params.m_cReqEffSlots);
    job.responseReady = job.requestFinish + m_params.SlotsToTime(m_params.m_cProcSlots);
    job.responseFinish = job.responseReady + m_params.SlotsToTime(m_params.m_cResEffSlots);
    job.deadline = GetDeadline(periodIndex);
    m_jobs.push_back(job);
    return job;
}

Time
DcControlStream::GetDeadline(uint64_t periodIndex) const
{
    return MilliSeconds((periodIndex + 1) * m_params.m_tCtrlMs);
}

const std::vector<DcControlJob>&
DcControlStream::GetJobs() const
{
    return m_jobs;
}

} // namespace ns3
