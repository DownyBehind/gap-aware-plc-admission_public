#include "ns3/hpgp-csma-ca-baseline.h"
#include "ns3/test.h"

namespace ns3::tests
{

class HpgpRetryCase : public TestCase
{
  public:
    HpgpRetryCase() : TestCase("HPGP retry CW update and retransmission limit") {}
  private:
    void DoRun() override
    {
        HpgpCsmaCaBaseline b;
        b.AddNode(1); b.AddNode(2);
        b.EnqueueFrame(1, {1, HpgpTrafficType::SLAC, 18});
        b.EnqueueFrame(2, {2, HpgpTrafficType::SLAC, 18});
        b.SetBackoffForTest(1, 0);
        b.SetBackoffForTest(2, 0);
        b.Step();
        NS_TEST_ASSERT_MSG_EQ(b.GetNode(1).GetAttempt(), 1, "attempt should increment after collision");
        NS_TEST_ASSERT_MSG_EQ(b.GetNode(1).GetCw(), 15, "CW should update to BPC1 range");

        HpgpCsmaCaParams p;
        p.m_maxRetries = 0;
        HpgpCsmaCaBaseline d(p);
        d.AddNode(1); d.AddNode(2);
        d.EnqueueFrame(1, {1, HpgpTrafficType::SLAC, 18});
        d.EnqueueFrame(2, {2, HpgpTrafficType::SLAC, 18});
        d.SetBackoffForTest(1, 0);
        d.SetBackoffForTest(2, 0);
        d.Step();
        NS_TEST_ASSERT_MSG_EQ(d.GetMetrics().drops, 2, "both frames should drop when retry limit is zero");
    }
};

class HpgpRetryTestSuite : public TestSuite
{
  public:
    HpgpRetryTestSuite() : TestSuite("hpgp-csma-ca-retry", Type::UNIT)
    {
        AddTestCase(new HpgpRetryCase, TestCase::Duration::QUICK);
    }
};
static HpgpRetryTestSuite g_hpgpRetryTestSuite;

} // namespace ns3::tests
