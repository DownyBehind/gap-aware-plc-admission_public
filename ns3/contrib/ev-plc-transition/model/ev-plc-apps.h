#ifndef EV_PLC_APPS_H
#define EV_PLC_APPS_H

// Track C layer-1 applications. Thin by design at C1: they register demand
// with the coordinating MAC (structure: one app per Node); the session state
// machine (credits, retries) moves here in C3/C4.
// Event-only classes: excluded from the standalone build.

#include "ev-plc-mac.h"

#include "ns3/application.h"
#include "ns3/ptr.h"

namespace ns3
{

class DcControlApp : public Application
{
  public:
    static TypeId GetTypeId();
    void SetMac(Ptr<EvPlcMac> mac);

  private:
    void StartApplication() override;
    void StopApplication() override
    {
    }

    Ptr<EvPlcMac> m_mac;
};

class SlacSessionApp : public Application
{
  public:
    static TypeId GetTypeId();
    void SetMac(Ptr<EvPlcMac> mac);

  private:
    void StartApplication() override;
    void StopApplication() override
    {
    }

    Ptr<EvPlcMac> m_mac;
};

} // namespace ns3

#endif // EV_PLC_APPS_H
