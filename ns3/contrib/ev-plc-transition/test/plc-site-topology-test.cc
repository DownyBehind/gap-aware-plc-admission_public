#include "ns3/test.h"
#include "ns3/plc-site-topology.h"
#include "ns3/plc-link-profile.h"
#include "ns3/plc-error-model.h"
#include "ns3/slac-sequence-model.h"
#include "ns3/ev-plc-node-profile.h"
#include "ns3/link-aware-admission.h"
using namespace ns3;
namespace ns3::tests {
class PlcSiteTopologyCase : public TestCase { public: PlcSiteTopologyCase():TestCase("plc-site-topology"){} private: void DoRun() override { auto small=PlcSiteTopologyFactory::CreateSmallSite(); NS_TEST_ASSERT_MSG_EQ(small.ports.size(),8,"small site ports"); auto depot=PlcSiteTopologyFactory::CreateDepotSite(); NS_TEST_ASSERT_MSG_EQ(depot.ports.size(),40,"depot ports"); } };
class PlcSiteTopologySuite : public TestSuite { public: PlcSiteTopologySuite():TestSuite("plc-site-topology", Type::UNIT){ AddTestCase(new PlcSiteTopologyCase, TestCase::Duration::QUICK); } };
static PlcSiteTopologySuite g_PlcSiteTopologySuite;
}
