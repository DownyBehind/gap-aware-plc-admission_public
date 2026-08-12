// C3 item 4: scheduled and CSMA MACs must coexist on one PlcSharedChannel —
// the occupancy arbitration serializes both modes (E4 rerun precondition).
// ns-3-only test (Simulator + event-only classes).

#include "ns3/ev-plc-csma-mac.h"
#include "ns3/ev-plc-mac.h"
#include "ns3/plc-shared-channel.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

namespace ns3::tests
{

class TrackCDualModeCase : public TestCase
{
  public:
    TrackCDualModeCase() : TestCase("scheduled and csma MACs share one channel") {}

  private:
    void DoRun() override
    {
        EvPlcParams params;
        Ptr<PlcSharedChannel> channel = Create<PlcSharedChannel>();
        channel->SetRngSeed(3);

        // Scheduled MAC: N=5, K=1 -> occupies the head of the cycle.
        Ptr<EvPlcMac> scheduled = Create<EvPlcMac>(params, channel);
        for (uint32_t i = 0; i < 5; ++i)
        {
            scheduled->AddDcEv();
        }
        scheduled->AddSlacSession();
        scheduled->Start(1);

        // CSMA MAC: a single contender wanting the medium from t = 0.
        Ptr<EvPlcCsmaMac> csma = Create<EvPlcCsmaMac>(params, channel);
        csma->SetBackoffRngSeed(11);
        csma->ConfigureScenario(1, 0); // one DC EV, no sessions
        csma->Start(1);

        Simulator::Run();
        const auto records = scheduled->GetRecords();
        const auto stats = csma->GetStats();
        Simulator::Destroy();

        // Scheduled result must be exactly the physics-off finish (the CSMA
        // node deferred; arbitration is half-duplex for both modes)...
        NS_TEST_ASSERT_MSG_EQ(records.size(), 1, "one scheduled cycle");
        GrantMapScheduler reference(params);
        NS_TEST_ASSERT_MSG_EQ(records.front().finishSlot, reference.ComputeFinishTime(5, 1),
                              "scheduled cycle must be undisturbed while csma defers");
        // ...and the CSMA node still got its request/response through after
        // the scheduled frames (no starvation, no overlap-induced loss).
        NS_TEST_ASSERT_MSG_EQ(stats.dcMisses, 0, "csma node must complete after deferring");
        NS_TEST_ASSERT_MSG_EQ(stats.collisions, 0, "single csma contender cannot collide");
    }
};

class TrackCDualModeTestSuite : public TestSuite
{
  public:
    TrackCDualModeTestSuite() : TestSuite("trackc-dual-mode", Type::UNIT)
    {
        AddTestCase(new TrackCDualModeCase, TestCase::Duration::QUICK);
    }
};
static TrackCDualModeTestSuite g_trackCDualModeTestSuite;

} // namespace ns3::tests
