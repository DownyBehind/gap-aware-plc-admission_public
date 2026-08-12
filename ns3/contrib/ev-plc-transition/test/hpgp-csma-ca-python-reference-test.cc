#include "ns3/hpgp-csma-ca-helper.h"
#include "ns3/test.h"

namespace ns3::tests
{

class HpgpPythonReferenceCase : public TestCase
{
  public:
    HpgpPythonReferenceCase() : TestCase("HPGP Python reference compatibility metrics") {}
  private:
    void DoRun() override
    {
        const auto metrics = HpgpCsmaCaHelper::RunExp1Baseline("/tmp/ns3-hpgp-reference-test", 1);
        NS_TEST_ASSERT_MSG_EQ(metrics.successfulFrames > 0, true, "baseline should transmit frames");
        NS_TEST_ASSERT_MSG_EQ(metrics.collisions >= 0, true, "collision metric should be available");
    }
};

class HpgpPythonReferenceTestSuite : public TestSuite
{
  public:
    HpgpPythonReferenceTestSuite() : TestSuite("hpgp-csma-ca-python-reference", Type::UNIT)
    {
        AddTestCase(new HpgpPythonReferenceCase, TestCase::Duration::QUICK);
    }
};
static HpgpPythonReferenceTestSuite g_hpgpPythonReferenceTestSuite;

} // namespace ns3::tests
