#include "ns3/slac-session.h"
#include "ns3/transition-admission-controller.h"
#include "ns3/test.h"

namespace ns3::tests
{

class SlacCompletionCase : public TestCase
{
  public:
    SlacCompletionCase() : TestCase("SLAC completion bound") {}
  private:
    void DoRun() override
    {
        TransitionAdmissionController c;
        NS_TEST_ASSERT_MSG_EQ(c.ComputeSlacCompletionCycles(), 36, "q_slac mismatch");
        NS_TEST_ASSERT_MSG_EQ(c.CheckSlacCompletion(), true, "SLAC bound should pass");
        SlacSession s;
        s.Admit();
        for (uint32_t i = 0; i < 36; ++i)
        {
            s.AddService(7);
        }
        NS_TEST_ASSERT_MSG_EQ(s.IsCompleted(), true, "session should complete after 36 grants");
    }
};

class SlacCompletionTestSuite : public TestSuite
{
  public:
    SlacCompletionTestSuite() : TestSuite("ev-plc-slac-completion", Type::UNIT)
    {
        AddTestCase(new SlacCompletionCase, TestCase::Duration::QUICK);
    }
};
static SlacCompletionTestSuite g_slacCompletionTestSuite;

} // namespace ns3::tests
