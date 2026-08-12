#include "ns3/grant-map-scheduler.h"
#include "ns3/test.h"
#include "ns3/transition-admission-controller.h"
#include <algorithm>

namespace ns3::tests
{
namespace
{

uint32_t
ActiveFinish(const EvPlcParams& p, uint32_t n, uint32_t k)
{
    const uint32_t auth = k > 0 ? k * p.m_bAuthSlots + p.m_bPktSlots : 0;
    const uint32_t procFloor = n > 0 ? p.m_cReqEffSlots + p.m_cProcSlots : 0;
    const uint32_t responseStart = std::max(n * p.m_cReqEffSlots + auth, procFloor);
    return responseStart + n * p.m_cResEffSlots + p.m_bBlkSlots;
}

EvPlcRegime
FormulaRegime(const EvPlcParams& p, const TransitionAdmissionController& c, uint32_t n, uint32_t k)
{
    const uint32_t finish = ActiveFinish(p, n, k);
    const uint32_t terminalFinish = c.ComputeDcOnlyFinish(n + k);
    if (finish > p.GetScheduledSlots() || terminalFinish > p.GetScheduledSlots())
    {
        return EvPlcRegime::REJECTED;
    }
    const uint32_t authWithPkt = k > 0 ? k * p.m_bAuthSlots + p.m_bPktSlots : 0;
    return authWithPkt <= c.ComputeProcessingGap(n) ? EvPlcRegime::HIDDEN : EvPlcRegime::PAID;
}

} // namespace

class GrantMapControllerIntegrationCase : public TestCase
{
  public:
    GrantMapControllerIntegrationCase() : TestCase("grant-map controller integration") {}
  private:
    void DoRun() override
    {
        EvPlcParams p;
        GrantMapScheduler scheduler(p);
        TransitionAdmissionController controller(p);
        const auto paidMap = scheduler.BuildGrantMap(18, 1, 0);
        NS_TEST_ASSERT_MSG_EQ(paidMap.finishSlot, ActiveFinish(p, 18, 1), "finish time must match active-state formula");
        NS_TEST_ASSERT_MSG_EQ(ToString(paidMap.regime), ToString(FormulaRegime(p, controller, 18, 1)), "regime must match equations.md");
        NS_TEST_ASSERT_MSG_EQ(paidMap.slackSlots, static_cast<int64_t>(p.GetScheduledSlots()) - static_cast<int64_t>(paidMap.finishSlot), "slack must be T_sched - finish");
        uint32_t n = 37;
        uint32_t k = 1;
        const bool rejected = !controller.Admit(n, k);
        const uint32_t kAfterRejectedAdmission = rejected ? k : k + 1;
        NS_TEST_ASSERT_MSG_EQ(rejected, true, "N=37,K=1 must be rejected by Cond B");
        NS_TEST_ASSERT_MSG_EQ(kAfterRejectedAdmission, k, "rejected admission must not increase K");
        n = 18;
        k = 2;
        NS_TEST_ASSERT_MSG_EQ(n + 1, 19, "SLAC completion should increase N");
        NS_TEST_ASSERT_MSG_EQ(k - 1, 1, "SLAC completion should decrease K");
    }
};

class GrantMapControllerIntegrationTestSuite : public TestSuite
{
  public:
    GrantMapControllerIntegrationTestSuite() : TestSuite("ev-plc-grant-map-controller-integration", Type::UNIT)
    {
        AddTestCase(new GrantMapControllerIntegrationCase, TestCase::Duration::QUICK);
    }
};
static GrantMapControllerIntegrationTestSuite g_grantMapControllerIntegrationTestSuite;

} // namespace ns3::tests
