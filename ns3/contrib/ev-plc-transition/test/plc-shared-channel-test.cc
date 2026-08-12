// C2 unit tests: closed-form frame failure vs hand-computed values, sojourn
// overlap arithmetic, and a Gilbert-Elliott clustering smoke test (the burst
// experiments themselves are C4 — this only proves the machine works).
// ns-3-only test (references the event-only channel class).

#include "ns3/plc-shared-channel.h"
#include "ns3/test.h"

namespace ns3::tests
{

class PlcSharedChannelClosedFormCase : public TestCase
{
  public:
    PlcSharedChannelClosedFormCase() : TestCase("frame-failure closed form (hand values)") {}

  private:
    void DoRun() override
    {
        // 1 - 0.99^10
        NS_TEST_ASSERT_MSG_EQ_TOL(PlcSharedChannel::FrameFailureProbability(10, 0, 0.01, 0.3),
                                  0.095617924991196, 1e-12, "all-good frame");
        // 1 - 0.99^4 * 0.5^6
        NS_TEST_ASSERT_MSG_EQ_TOL(PlcSharedChannel::FrameFailureProbability(4, 6, 0.01, 0.5),
                                  0.98499068734375, 1e-12, "straddling frame");
        // 1 - 0.9^21
        NS_TEST_ASSERT_MSG_EQ_TOL(PlcSharedChannel::FrameFailureProbability(0, 21, 0.0, 0.1),
                                  0.8905810108684876, 1e-12, "all-bad frame");
    }
};

class PlcSharedChannelOverlapCase : public TestCase
{
  public:
    PlcSharedChannelOverlapCase() : TestCase("G-E sojourn overlap arithmetic") {}

  private:
    void DoRun() override
    {
        PlcSharedChannel channel;
        channel.ForceGeSegmentsForTest({{false, 100}, {true, 160}, {false, 1000}});
        auto [g1, b1] = channel.GoodBadOverlap(95, 20);
        NS_TEST_ASSERT_MSG_EQ(g1, 5, "overlap [95,115) good");
        NS_TEST_ASSERT_MSG_EQ(b1, 15, "overlap [95,115) bad");
        auto [g2, b2] = channel.GoodBadOverlap(90, 10);
        NS_TEST_ASSERT_MSG_EQ(g2, 10, "overlap [90,100) good");
        NS_TEST_ASSERT_MSG_EQ(b2, 0, "overlap [90,100) bad");
        auto [g3, b3] = channel.GoodBadOverlap(98, 72);
        NS_TEST_ASSERT_MSG_EQ(g3, 12, "overlap [98,170) good");
        NS_TEST_ASSERT_MSG_EQ(b3, 60, "overlap [98,170) bad");
    }
};

class PlcSharedChannelGeSmokeCase : public TestCase
{
  public:
    PlcSharedChannelGeSmokeCase() : TestCase("G-E clustering smoke (extreme bad sojourn)") {}

  private:
    void DoRun() override
    {
        PlcSharedChannel channel;
        channel.SetRngSeed(5);
        channel.SetGeRngSeed(7);
        // Mean good sojourn 200 slots, mean bad 100 slots (~6 frames), heavy
        // bad-state slot PER: failures must arrive in runs, not i.i.d.
        channel.EnableGilbertElliott(1.0 / 200.0, 1.0 / 100.0, 1e-4, 0.3);

        const uint32_t frames = 2000;
        const uint32_t frameSlots = 15;
        uint32_t failures = 0;
        uint32_t pairs = 0;      // consecutive-frame pairs with both failing
        uint32_t maxRun = 0;
        uint32_t run = 0;
        bool prevFail = false;
        for (uint32_t i = 0; i < frames; ++i)
        {
            const bool fail = channel.FrameFailsAt(0, static_cast<uint64_t>(i) * frameSlots,
                                                   frameSlots);
            if (fail)
            {
                ++failures;
                run += 1;
                maxRun = std::max(maxRun, run);
                if (prevFail)
                {
                    ++pairs;
                }
            }
            else
            {
                run = 0;
            }
            prevFail = fail;
        }
        NS_TEST_ASSERT_MSG_GT_OR_EQ(failures, 100, "failures must occur");
        NS_TEST_ASSERT_MSG_GT_OR_EQ(maxRun, 3, "burst run-length must exceed 2");
        // Clustering: P(fail | prev fail) must clearly exceed the marginal.
        const double marginal = static_cast<double>(failures) / frames;
        const double conditional = static_cast<double>(pairs) / failures;
        NS_TEST_ASSERT_MSG_GT_OR_EQ(conditional, 1.5 * marginal,
                                    "failures must cluster (G-E, not i.i.d.)");
    }
};

class PlcSharedChannelTestSuite : public TestSuite
{
  public:
    PlcSharedChannelTestSuite() : TestSuite("plc-shared-channel-ge", Type::UNIT)
    {
        AddTestCase(new PlcSharedChannelClosedFormCase, TestCase::Duration::QUICK);
        AddTestCase(new PlcSharedChannelOverlapCase, TestCase::Duration::QUICK);
        AddTestCase(new PlcSharedChannelGeSmokeCase, TestCase::Duration::QUICK);
    }
};
static PlcSharedChannelTestSuite g_plcSharedChannelTestSuite;

} // namespace ns3::tests
