#ifndef SLAC_SESSION_H
#define SLAC_SESSION_H

#include "ev-plc-params.h"
#include "ns3/nstime.h"
#include <cstdint>

namespace ns3
{

enum class SlacSessionState
{
    WAITING_FIRST_ACCESS,
    ADMITTED,
    ACTIVE_AUTH,
    COMPLETED,
    REJECTED,
    TIMEOUT
};

class SlacSession
{
  public:
    explicit SlacSession(const EvPlcParams& params = EvPlcParams(), Time start = Seconds(0));
    void Admit();
    void AddService(uint32_t slots);
    bool IsCompleted() const;
    bool IsTimedOut(Time now) const;
    uint32_t GetRemainingSlots() const;
    SlacSessionState GetState() const;

  private:
    EvPlcParams m_params;
    Time m_start;
    uint32_t m_servedSlots{0};
    SlacSessionState m_state{SlacSessionState::WAITING_FIRST_ACCESS};
};

} // namespace ns3

#endif // SLAC_SESSION_H
