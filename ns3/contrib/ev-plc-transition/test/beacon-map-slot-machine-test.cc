#include "ns3/beacon-map-slot-machine.h"
#include "ns3/grant-map-scheduler.h"
#include "ns3/test.h"

namespace ns3::tests
{

class BeaconMapSlotMachineCase : public TestCase
{
  public:
    BeaconMapSlotMachineCase() : TestCase("beacon-map slot machine physics-off parity") {}

  private:
    void DoRun() override
    {
        EvPlcParams params;
        GrantMapScheduler scheduler(params);
        BeaconMapSlotMachine machine(params);
        // Representative states: empty, setup-only, hidden, paid, boundary,
        // counterexample, saturated.
        const uint32_t states[][2] = {{0, 0}, {0, 3}, {5, 0}, {10, 4}, {20, 1},
                                      {25, 4}, {37, 1}, {38, 0}, {40, 20}};
        for (const auto& s : states)
        {
            const auto map = scheduler.BuildGrantMap(s[0], s[1], 0);
            const auto played = machine.PlayCycle(map);
            NS_TEST_ASSERT_MSG_EQ(played.finishSlot, scheduler.ComputeFinishTime(s[0], s[1]),
                                  "measured finish must equal formula with physics off");
            NS_TEST_ASSERT_MSG_EQ(played.responseFinishSlots.size(), s[0],
                                  "one response completion per EV");
            if (s[0] > 0)
            {
                NS_TEST_ASSERT_MSG_EQ(played.responseFinishSlots.back() + params.m_bBlkSlots,
                                      played.finishSlot,
                                      "finish must be last response end plus B_blk envelope");
            }
        }
        // Non-overlap and ordering of played frames.
        const auto played = machine.PlayCycle(scheduler.BuildGrantMap(25, 4, 0));
        for (std::size_t i = 1; i < played.frames.size(); ++i)
        {
            NS_TEST_ASSERT_MSG_GT_OR_EQ(played.frames[i].startSlot, played.frames[i - 1].endSlot,
                                        "played frames must not overlap");
        }
    }
};

class BeaconMapSlotMachineTestSuite : public TestSuite
{
  public:
    BeaconMapSlotMachineTestSuite() : TestSuite("ev-plc-beacon-map-slot-machine", Type::UNIT)
    {
        AddTestCase(new BeaconMapSlotMachineCase, TestCase::Duration::QUICK);
    }
};
static BeaconMapSlotMachineTestSuite g_beaconMapSlotMachineTestSuite;

} // namespace ns3::tests
