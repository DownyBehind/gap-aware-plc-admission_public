#include "ns3/hpgp-csma-ca-baseline.h"
#include "ns3/test.h"

namespace ns3::tests
{

class HpgpBackoffCase : public TestCase
{
  public:
    HpgpBackoffCase() : TestCase("HPGP backoff countdown and freeze") {}
  private:
    void DoRun() override
    {
        HpgpCsmaCaBaseline b;
        b.AddNode(1);
        b.EnqueueFrame(1, {1, HpgpTrafficType::DC_REQ, 15});
        b.SetBackoffForTest(1, 3);
        b.Step(); b.Step(); b.Step();
        NS_TEST_ASSERT_MSG_EQ(static_cast<int>(b.GetNode(1).GetState()), static_cast<int>(HpgpNodeState::TRANSMITTING), "node should transmit after 3 idle slots");

        HpgpCsmaCaBaseline f;
        f.AddNode(1);
        f.EnqueueFrame(1, {1, HpgpTrafficType::DC_REQ, 15});
        f.SetBackoffForTest(1, 3);
        f.ForceMediumBusy(5);
        f.Step(); f.Step(); f.Step();
        NS_TEST_ASSERT_MSG_EQ(f.GetNode(1).GetBackoff(), 3, "backoff must freeze while medium busy");
        f.RunUntil(8);
        NS_TEST_ASSERT_MSG_EQ(static_cast<int>(f.GetNode(1).GetState()), static_cast<int>(HpgpNodeState::TRANSMITTING), "backoff should resume after idle");
    }
};

class HpgpBackoffTestSuite : public TestSuite
{
  public:
    HpgpBackoffTestSuite() : TestSuite("hpgp-csma-ca-backoff", Type::UNIT)
    {
        AddTestCase(new HpgpBackoffCase, TestCase::Duration::QUICK);
    }
};
static HpgpBackoffTestSuite g_hpgpBackoffTestSuite;

} // namespace ns3::tests
