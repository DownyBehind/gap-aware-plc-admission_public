#include "ns3/test.h"
#include "ns3/plc-site-topology.h"
#include "ns3/plc-link-profile.h"
#include "ns3/plc-error-model.h"
#include "ns3/slac-sequence-model.h"
#include "ns3/ev-plc-node-profile.h"
#include "ns3/link-aware-admission.h"
using namespace ns3;
namespace ns3::tests {
class LinkAwareAdmissionCase : public TestCase { public: LinkAwareAdmissionCase():TestCase("link-aware-admission"){} private: void DoRun() override { std::vector<EvCommunicationDemand> nearDc, farDc; for(uint32_t i=0;i<30;++i){ nearDc.push_back(MakeDemand(i,i,PlcLinkProfileTable::Get(PlcProfileClass::GOOD))); farDc.push_back(MakeDemand(i,i,PlcLinkProfileTable::Get(PlcProfileClass::SEVERE))); } LinkAwareAdmissionController c; auto n1=c.EvaluateTerminal(nearDc,AdmissionMode::LINK_AWARE_DEMAND_BASED); auto f1=c.EvaluateTerminal(farDc,AdmissionMode::LINK_AWARE_DEMAND_BASED); NS_TEST_ASSERT_MSG_EQ(n1.admit, true, "near feasible"); NS_TEST_ASSERT_MSG_EQ(f1.admit, false, "far infeasible"); auto countNear=c.EvaluateTerminal(nearDc,AdmissionMode::COUNT_BASED_NOMINAL); auto countFar=c.EvaluateTerminal(farDc,AdmissionMode::COUNT_BASED_NOMINAL); NS_TEST_ASSERT_MSG_EQ(countNear.admit, countFar.admit, "count-based treats same N equal"); auto worst=c.EvaluateTerminal(nearDc,AdmissionMode::COUNT_BASED_WORST_CASE); NS_TEST_ASSERT_MSG_EQ(worst.admit, false, "worst conservative"); } };
class LinkAwareCondACreditCase : public TestCase { public: LinkAwareCondACreditCase():TestCase("link-aware-condA-credit-sum"){} private: void DoRun() override {
  // Two heterogeneous sessions: q_i = 7 (clean link) and q_i = 9 (degraded link).
  // Sum q_i + q_cand = 7 + 9 + 7 = 23, whereas uniform q*(K+1) = 7*3 = 21.
  LinkAwareAdmissionController c(1395, 0, 7);
  std::vector<EvCommunicationDemand> dc; for(uint32_t i=0;i<20;++i){ dc.push_back(MakeDemand(i,i,PlcLinkProfileTable::Get(PlcProfileClass::NOMINAL))); } // 20 NOMINAL EVs: sum req = 300 > process floor 295, so the auth term is visible in the finish.
  std::vector<EvCommunicationDemand> slac(2); slac[0].creditSlots = 7; slac[1].creditSlots = 9;
  auto cand = MakeDemand(1,1,PlcLinkProfileTable::Get(PlcProfileClass::GOOD)); cand.creditSlots = 7;
  const auto withCredits = c.EvaluateActive(dc, slac, cand, AdmissionMode::LINK_AWARE_DEMAND_BASED);
  // Uniform fallback path: no creditSlots annotated -> q*(K+1).
  std::vector<EvCommunicationDemand> slacU(2);
  auto candU = MakeDemand(1,1,PlcLinkProfileTable::Get(PlcProfileClass::GOOD));
  const auto uniform = c.EvaluateActive(dc, slacU, candU, AdmissionMode::LINK_AWARE_DEMAND_BASED);
  // Both admit here; the finish must differ by exactly the credit surplus (23 - 21 = 2).
  NS_TEST_ASSERT_MSG_EQ(withCredits.admit, true, "credit-sum admits");
  NS_TEST_ASSERT_MSG_EQ(withCredits.finishSlots - uniform.finishSlots, 2u, "Cond A charges sum q_i, not q*(K+1)");
} };
class LinkAwareAdmissionSuite : public TestSuite { public: LinkAwareAdmissionSuite():TestSuite("link-aware-admission", Type::UNIT){ AddTestCase(new LinkAwareAdmissionCase, TestCase::Duration::QUICK); AddTestCase(new LinkAwareCondACreditCase, TestCase::Duration::QUICK); } };
static LinkAwareAdmissionSuite g_LinkAwareAdmissionSuite;
}
