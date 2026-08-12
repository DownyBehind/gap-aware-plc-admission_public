#include "ns3/test.h"
#include "ns3/plc-site-topology.h"
#include "ns3/plc-link-profile.h"
#include "ns3/plc-error-model.h"
#include "ns3/slac-sequence-model.h"
#include "ns3/ev-plc-node-profile.h"
#include "ns3/link-aware-admission.h"
using namespace ns3;
namespace ns3::tests {
class PlcErrorModelCase : public TestCase { public: PlcErrorModelCase():TestCase("plc-error-model"){} private: void DoRun() override { PlcErrorModel e; e.SetSeed(7); auto p=PlcLinkProfileTable::Get(PlcProfileClass::NOMINAL); e.SetPerOverride(0.0); NS_TEST_ASSERT_MSG_EQ(e.FrameSucceeds(1,HpgpTrafficType::DC_REQ,p), true, "per0 succeeds"); e.SetPerOverride(1.0); NS_TEST_ASSERT_MSG_EQ(e.FrameSucceeds(1,HpgpTrafficType::DC_REQ,p), false, "per1 fails"); e.ClearPerOverride(); e.SetSeed(3); bool a=e.FrameSucceeds(1,HpgpTrafficType::SLAC,p); e.SetSeed(3); bool b=e.FrameSucceeds(1,HpgpTrafficType::SLAC,p); NS_TEST_ASSERT_MSG_EQ(a,b,"seed reproducible"); } };
class PlcErrorModelSuite : public TestSuite { public: PlcErrorModelSuite():TestSuite("plc-error-model", Type::UNIT){ AddTestCase(new PlcErrorModelCase, TestCase::Duration::QUICK); } };
static PlcErrorModelSuite g_PlcErrorModelSuite;
}
