#include "ns3/test.h"
#include "ns3/plc-site-topology.h"
#include "ns3/plc-link-profile.h"
#include "ns3/plc-error-model.h"
#include "ns3/slac-sequence-model.h"
#include "ns3/ev-plc-node-profile.h"
#include "ns3/link-aware-admission.h"
using namespace ns3;
namespace ns3::tests {
class EvSpecificDemandCase : public TestCase { public: EvSpecificDemandCase():TestCase("ev-specific-demand"){} private: void DoRun() override { auto good=MakeDemand(1,1,PlcLinkProfileTable::Get(PlcProfileClass::GOOD)); auto severe=MakeDemand(2,2,PlcLinkProfileTable::Get(PlcProfileClass::SEVERE)); NS_TEST_ASSERT_MSG_EQ(good.cReqEffSlots < severe.cReqEffSlots, true, "heterogeneous req"); NS_TEST_ASSERT_MSG_EQ(good.cResEffSlots < severe.cResEffSlots, true, "heterogeneous res"); } };
class EvSpecificDemandSuite : public TestSuite { public: EvSpecificDemandSuite():TestSuite("ev-specific-demand", Type::UNIT){ AddTestCase(new EvSpecificDemandCase, TestCase::Duration::QUICK); } };
static EvSpecificDemandSuite g_EvSpecificDemandSuite;
}
