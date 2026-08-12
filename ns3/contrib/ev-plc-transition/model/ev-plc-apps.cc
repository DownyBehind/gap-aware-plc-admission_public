#include "ev-plc-apps.h"

namespace ns3
{

TypeId
DcControlApp::GetTypeId()
{
    static TypeId tid = TypeId("ns3::DcControlApp").SetParent<Application>().AddConstructor<DcControlApp>();
    return tid;
}

void
DcControlApp::SetMac(Ptr<EvPlcMac> mac)
{
    m_mac = mac;
}

void
DcControlApp::StartApplication()
{
    m_mac->AddDcEv();
}

TypeId
SlacSessionApp::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SlacSessionApp").SetParent<Application>().AddConstructor<SlacSessionApp>();
    return tid;
}

void
SlacSessionApp::SetMac(Ptr<EvPlcMac> mac)
{
    m_mac = mac;
}

void
SlacSessionApp::StartApplication()
{
    m_mac->AddSlacSession();
}

} // namespace ns3
