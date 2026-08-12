#include "ns3/hpgp-csma-ca-baseline.h"
#include "ns3/test.h"

namespace ns3::tests
{

class HpgpCollisionCase : public TestCase
{
  public:
    HpgpCollisionCase() : TestCase("HPGP collision handling") {}
  private:
    void DoRun() override
    {
        HpgpCsmaCaBaseline b;
        b.AddNode(1); b.AddNode(2);
        b.EnqueueFrame(1, {1, HpgpTrafficType::DC_REQ, 15});
        b.EnqueueFrame(2, {2, HpgpTrafficType::DC_REQ, 15});
        b.SetBackoffForTest(1, 0);
        b.SetBackoffForTest(2, 0);
        b.Step();
        auto m = b.GetMetrics();
        NS_TEST_ASSERT_MSG_EQ(m.collisions, 1, "one collision expected");
        NS_TEST_ASSERT_MSG_EQ(m.retries, 2, "both nodes should retry");
    }
};

class HpgpCollisionTestSuite : public TestSuite
{
  public:
    HpgpCollisionTestSuite() : TestSuite("hpgp-csma-ca-collision", Type::UNIT)
    {
        AddTestCase(new HpgpCollisionCase, TestCase::Duration::QUICK);
    }
};
static HpgpCollisionTestSuite g_hpgpCollisionTestSuite;

} // namespace ns3::tests
