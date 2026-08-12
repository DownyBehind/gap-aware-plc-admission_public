#include "ns3/grant-map-scheduler.h"
#include "ns3/test.h"

namespace ns3::tests
{

class GrantMapSchedulerCase : public TestCase
{
  public:
    GrantMapSchedulerCase() : TestCase("grant map scheduler phases") {}
  private:
    void DoRun() override
    {
        EvPlcParams params;
        GrantMapScheduler scheduler(params);
        NS_TEST_ASSERT_MSG_EQ(scheduler.ComputeDcReqPhaseLength(3), 45, "request phase mismatch");
        NS_TEST_ASSERT_MSG_EQ(scheduler.ComputeSlacAuthPhaseLength(2), 14, "auth phase mismatch");
        NS_TEST_ASSERT_MSG_EQ(scheduler.ComputeDcResPhaseLength(3), 63, "response phase mismatch");
        auto map = scheduler.BuildGrantMap(3, 2, 7);
        NS_TEST_ASSERT_MSG_EQ(map.periodIndex, 7, "period index mismatch");
        NS_TEST_ASSERT_MSG_EQ(map.phases.size(), 3, "scheduled region should not trace O_map as a phase");
        for (std::size_t i = 1; i < map.phases.size(); ++i)
        {
            const auto prevEnd = map.phases[i - 1].startSlot + map.phases[i - 1].durationSlots;
            NS_TEST_ASSERT_MSG_GT_OR_EQ(map.phases[i].startSlot, prevEnd, "grant-map phases must not overlap");
        }
        NS_TEST_ASSERT_MSG_EQ(map.finishSlot, scheduler.ComputeFinishTime(3, 2), "finish should use scheduled-region slots");
        NS_TEST_ASSERT_MSG_EQ(map.slackSlots, static_cast<int64_t>(params.GetScheduledSlots()) - static_cast<int64_t>(map.finishSlot), "slack mismatch");
    }
};

class GrantMapSchedulerTestSuite : public TestSuite
{
  public:
    GrantMapSchedulerTestSuite() : TestSuite("ev-plc-grant-map", Type::UNIT)
    {
        AddTestCase(new GrantMapSchedulerCase, TestCase::Duration::QUICK);
    }
};
static GrantMapSchedulerTestSuite g_grantMapSchedulerTestSuite;

} // namespace ns3::tests
