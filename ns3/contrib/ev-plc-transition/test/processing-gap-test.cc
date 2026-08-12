#include "ns3/transition-admission-controller.h"
#include "ns3/test.h"

namespace ns3::tests
{

class ProcessingGapCase : public TestCase
{
  public:
    ProcessingGapCase() : TestCase("processing gap and DC-only finish") {}
  private:
    void DoRun() override
    {
        TransitionAdmissionController c;
        NS_TEST_ASSERT_MSG_EQ(c.ComputeProcessingGap(1), 280, "G(1) mismatch");
        NS_TEST_ASSERT_MSG_EQ(c.ComputeProcessingGap(20), 0, "N* should be 20");
        NS_TEST_ASSERT_MSG_EQ(c.ComputeProcessingGap(10) >= c.ComputeProcessingGap(11), true, "gap should be monotonic");
        NS_TEST_ASSERT_MSG_EQ(c.ComputeDcOnlyFinish(38), 1389, "F_DC(38) mismatch");
    }
};

class ProcessingGapTestSuite : public TestSuite
{
  public:
    ProcessingGapTestSuite() : TestSuite("ev-plc-processing-gap", Type::UNIT)
    {
        AddTestCase(new ProcessingGapCase, TestCase::Duration::QUICK);
    }
};
static ProcessingGapTestSuite g_processingGapTestSuite;

} // namespace ns3::tests
