#include "ns3/test.h"
#include "ns3/plc-site-topology.h"
#include "ns3/plc-link-profile.h"
#include "ns3/plc-error-model.h"
#include "ns3/slac-sequence-model.h"
#include "ns3/ev-plc-node-profile.h"
#include "ns3/link-aware-admission.h"
using namespace ns3;
namespace ns3::tests {
class SlacSequenceModelCase : public TestCase { public: SlacSequenceModelCase():TestCase("slac-sequence-model"){} private: void DoRun() override { auto s=SlacSequenceModel::LegacySequence(); NS_TEST_ASSERT_MSG_EQ(SlacSequenceModel::CountMessages(s,SlacMessageType::MNBC_SOUND_IND),10,"mnbc count"); NS_TEST_ASSERT_MSG_EQ(SlacSequenceModel::ComputeTotalSlots(s),196,"sequence total"); } };
class SlacSequenceModelSuite : public TestSuite { public: SlacSequenceModelSuite():TestSuite("slac-sequence-model", Type::UNIT){ AddTestCase(new SlacSequenceModelCase, TestCase::Duration::QUICK); } };
static SlacSequenceModelSuite g_SlacSequenceModelSuite;
}
