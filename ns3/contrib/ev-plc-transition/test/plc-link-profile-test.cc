#include "ns3/test.h"
#include "ns3/plc-site-topology.h"
#include "ns3/plc-link-profile.h"
#include "ns3/plc-error-model.h"
#include "ns3/slac-sequence-model.h"
#include "ns3/ev-plc-node-profile.h"
#include "ns3/link-aware-admission.h"
using namespace ns3;
namespace ns3::tests {
class PlcLinkProfileCase : public TestCase { public: PlcLinkProfileCase():TestCase("plc-link-profile"){} private: void DoRun() override { NS_TEST_ASSERT_MSG_EQ(static_cast<int>(PlcLinkProfileTable::ClassFromPort({0,5,0,0,0})), static_cast<int>(PlcProfileClass::GOOD), "5m good"); NS_TEST_ASSERT_MSG_EQ(static_cast<int>(PlcLinkProfileTable::ClassFromPort({0,20,1,0,0})), static_cast<int>(PlcProfileClass::NOMINAL), "20m nominal"); NS_TEST_ASSERT_MSG_EQ(static_cast<int>(PlcLinkProfileTable::ClassFromPort({0,35,1,0,0})), static_cast<int>(PlcProfileClass::DEGRADED), "35m degraded"); NS_TEST_ASSERT_MSG_EQ(static_cast<int>(PlcLinkProfileTable::ClassFromPort({0,60,1,0,0})), static_cast<int>(PlcProfileClass::SEVERE), "60m severe"); NS_TEST_ASSERT_MSG_EQ(static_cast<int>(PlcLinkProfileTable::ClassFromPort({0,20,3,0,0})), static_cast<int>(PlcProfileClass::SEVERE), "branch depth severe"); NS_TEST_ASSERT_MSG_EQ(PlcLinkProfileTable::Get(PlcProfileClass::GOOD).cReqEffSlots < PlcLinkProfileTable::Get(PlcProfileClass::SEVERE).cReqEffSlots, true, "good less than severe"); } };
class PlcLinkProfileSuite : public TestSuite { public: PlcLinkProfileSuite():TestSuite("plc-link-profile", Type::UNIT){ AddTestCase(new PlcLinkProfileCase, TestCase::Duration::QUICK); } };
static PlcLinkProfileSuite g_PlcLinkProfileSuite;
}
