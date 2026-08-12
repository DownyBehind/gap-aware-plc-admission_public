#include "ns3/test.h"
#include "ns3/plc-site-topology.h"
#include "ns3/plc-link-profile.h"
#include "ns3/plc-error-model.h"
#include "ns3/slac-sequence-model.h"
#include "ns3/ev-plc-node-profile.h"
#include "ns3/link-aware-admission.h"
using namespace ns3;
namespace ns3::tests {
class MapLossSafetyCase : public TestCase { public: MapLossSafetyCase():TestCase("map-loss-safety"){} private: void DoRun() override { PlcErrorModel e; auto p=PlcLinkProfileTable::Get(PlcProfileClass::SEVERE); e.SetPerOverride(1.0); NS_TEST_ASSERT_MSG_EQ(e.FrameSucceeds(1,HpgpTrafficType::OTHER,p), false, "map loss occurs"); LinkAwareAdmissionController c; std::vector<EvCommunicationDemand> existing; for(uint32_t i=0;i<10;++i) existing.push_back(MakeDemand(i,i,PlcLinkProfileTable::Get(PlcProfileClass::NOMINAL))); NS_TEST_ASSERT_MSG_EQ(c.GroundTruthFeasible(existing), true, "existing map remains safe"); } };
class MapLossSafetySuite : public TestSuite { public: MapLossSafetySuite():TestSuite("map-loss-safety", Type::UNIT){ AddTestCase(new MapLossSafetyCase, TestCase::Duration::QUICK); } };
static MapLossSafetySuite g_MapLossSafetySuite;
}
