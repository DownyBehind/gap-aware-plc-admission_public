#include "ns3/fixed-reservation-controller.h"
#include "ns3/test.h"

namespace ns3::tests
{

class FixedReservationCase : public TestCase
{
  public:
    FixedReservationCase() : TestCase("fixed reservation formulas and tradeoff") {}
  private:
    void DoRun() override
    {
        EvPlcParams p;
        FixedReservationController c(p);
        const uint32_t expected = std::max(10u * 15u + 32u + 21u, 15u + 280u) + 10u * 21u + 21u;
        NS_TEST_ASSERT_MSG_EQ(c.ComputeFixedReservationFinish(10, 32), expected, "fixed finish formula mismatch");
        NS_TEST_ASSERT_MSG_EQ(c.ComputeFixedIdleWaste(0, 32, 0), 32, "K=0 should waste all fixed reservation");
        NS_TEST_ASSERT_MSG_EQ(c.ComputeFixedIdleWaste(10, 32, 32), 0, "no idle waste when demand exceeds reservation");
        const auto timeout = c.RunFixedReservationPeriod(0, 0, 20, 8);
        NS_TEST_ASSERT_MSG_EQ(timeout.slacTimeoutCount > 0, true, "too-small fixed budget should time out many sessions");

        uint32_t fixedWaste = 0;
        uint32_t adaptiveWaste = 0;
        for (auto k : {0u, 0u, 1u, 5u, 0u})
        {
            fixedWaste += c.RunFixedReservationPeriod(0, 0, k, 32).idleWaste;
            adaptiveWaste += c.ComputeAdaptivePeriod(0, 0, k).idleWaste;
        }
        NS_TEST_ASSERT_MSG_EQ(fixedWaste > adaptiveWaste, true, "adaptive should waste less than fixed when K varies");
    }
};

class FixedReservationTestSuite : public TestSuite
{
  public:
    FixedReservationTestSuite() : TestSuite("fixed-reservation", Type::UNIT)
    {
        AddTestCase(new FixedReservationCase, TestCase::Duration::QUICK);
    }
};
static FixedReservationTestSuite g_fixedReservationTestSuite;

} // namespace ns3::tests
