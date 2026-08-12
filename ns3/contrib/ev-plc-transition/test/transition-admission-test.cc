#include "ns3/transition-admission-controller.h"
#include "ns3/test.h"

namespace ns3::tests
{

class TransitionAdmissionFormulaCase : public TestCase
{
  public:
    TransitionAdmissionFormulaCase() : TestCase("transition admission formal equations") {}
  private:
    void DoRun() override
    {
        TransitionAdmissionController c;
        NS_TEST_ASSERT_MSG_EQ(c.CheckCondA(37, 1), true, "Cond A should pass for N=37,K=1");
        NS_TEST_ASSERT_MSG_EQ(c.CheckCondB(37, 1), false, "Cond B should fail for N=37,K=1");
        NS_TEST_ASSERT_MSG_EQ(c.Admit(37, 1), false, "Admission should fail when Cond B fails");
        NS_TEST_ASSERT_MSG_EQ_TOL(c.ComputeTransitionAmplification(), 36.0 / 7.0, 1e-9, "alpha_tr mismatch");
        NS_TEST_ASSERT_MSG_EQ(c.ComputeSlackDegradationPerCompletion(), 29, "slack degradation mismatch");
    }
};

class TransitionAdmissionTestSuite : public TestSuite
{
  public:
    TransitionAdmissionTestSuite() : TestSuite("ev-plc-transition", Type::UNIT)
    {
        AddTestCase(new TransitionAdmissionFormulaCase, TestCase::Duration::QUICK);
    }
};
static TransitionAdmissionTestSuite g_transitionAdmissionTestSuite;

} // namespace ns3::tests
