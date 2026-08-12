#ifndef EV_PLC_POLICY_MAC_H
#define EV_PLC_POLICY_MAC_H

// Track C C4: event-driven scheduled-policy MAC (fixed reservation / gap-aware)
// with the E4 scenario semantics — burst sessions, per-session credit
// (docs/model/physics_rules.md), Cond A+B admission with reject-and-retry,
// completion -> DC transition at the next cycle boundary. Frames are played
// as TX-end events; retransmissions re-occupy immediately.
//
// Error sources:
//   INTERNAL_IID — one mt19937 with e4_sim's seed formula and roll order
//                  (bit parity with the slot-machine E4 scheduled columns),
//   CHANNEL      — PlcSharedChannel::FrameFailsAt (attenuation matrix / G-E;
//                  used by G3-(b) and G3-(a)).
// Admission variants: COUNT (TransitionAdmissionController, SoT) or
// LINK_AWARE (LinkAwareAdmissionController with retx-inflated per-EV
// airtimes; sessions additionally get link-inflated credit q_i).
//
// Event-only class: excluded from the standalone build.

#include "ev-plc-params.h"
#include "link-aware-admission.h"
#include "plc-shared-channel.h"
#include "transition-admission-controller.h"

#include "ns3/ptr.h"
#include "ns3/simple-ref-count.h"

#include <cstdint>
#include <random>
#include <vector>

namespace ns3
{

enum class PolicyErrorSource
{
    INTERNAL_IID,
    CHANNEL
};

enum class PolicyAdmission
{
    COUNT,
    LINK_AWARE
};

// Per-cycle trajectory trace (v4.5-M instrumentation): one row per completed
// cycle, channel-basis finish (B_blk excluded), actually played SLAC slots,
// and the first-response start. Additive only; no RNG or scheduling impact.
struct PolicyCycleTraceRow
{
    uint32_t cycle{0};
    uint32_t n{0};
    uint32_t kActive{0};
    uint32_t chanFinish{0};
    uint32_t slacPlayed{0};
    uint32_t respStart{0};
};

struct PolicyRunStats
{
    uint32_t admitted{0};
    uint32_t neverAdmitted{0};
    double waitSumCycles{0.0};
    uint32_t dgViolations{0};
    uint32_t completed{0};
    uint64_t dcMisses{0};
    uint64_t dcEvCycles{0};
    // Theorem-1 instrumentation: worst cycle-relative response finish and the
    // (N, K_active) state in which it occurred.
    uint32_t maxRelativeFinish{0};
    uint32_t maxFinishN{0};
    uint32_t maxFinishK{0};
    // Per-link-group decomposition (G3-(b)): index by PlcProfileClass rank.
    uint32_t dgViolationsSevere{0};
    uint32_t dgViolationsGood{0};
    uint32_t admittedSevere{0};
    uint32_t admittedGood{0};
    // Slack-occupancy instrumentation: per in-flight cycle the
    // deterministic (PER=0) plan finish is F0 = max(C_req*N + S_plan,
    // C_req + C_proc) + C_res*N with S_plan the SLAC slots the credit/release
    // state would play this cycle; the cycle is low-slack when
    // T_sched - F0 < C_res (one worst-case retransmission). Counters only —
    // no behavior or RNG change.
    uint64_t totalCycles{0};
    uint64_t lowSlackCycles{0};
    uint64_t lowSlackEvCycles{0};
    uint64_t dcMissesLowSlack{0};
    // Same, one band up: slack < 2*C_res (two worst-case retransmissions).
    uint64_t lowSlack2Cycles{0};
    uint64_t lowSlack2EvCycles{0};
    uint64_t dcMissesLowSlack2{0};
    // Tightest deterministic plan slack seen (signed; may be negative).
    int64_t minPlanSlack{INT64_MAX};
    // Ablation instrumentation: largest DC population reached.
    uint32_t maxNDc{0};
    // Filled only when cycle tracing is enabled (v4.5-M).
    std::vector<PolicyCycleTraceRow> cycleTrace;
};

class EvPlcPolicyMac : public SimpleRefCount<EvPlcPolicyMac>
{
  public:
    EvPlcPolicyMac(const EvPlcParams& params, Ptr<PlcSharedChannel> channel);

    // acbs: perSessionBudget = q, retryCap as in E2/E4. fixed: bFix slots.
    void ConfigureAcbs(uint32_t q, uint32_t retryCap);
    void ConfigureFixed(double bFix);
    void SetAdmissionVariant(PolicyAdmission variant);
    // Ablation: disable Cond B (post-transition check) only; Cond A
    // and every other code path (including RNG draws) are untouched.
    void SetCondBEnabled(bool enabled);
    // Aggregate SLAC window cap: per-cycle quota = sum of per-session
    // accruals (q, or link-inflated q_i), carried debt, persistent service
    // pointer. Default off; per-session credit accounting unchanged.
    void SetAggregateCap(bool enabled);
    void SetErrorSource(PolicyErrorSource source, double iidPer, uint32_t iidSeed);
    // Per-cycle trajectory trace (v4.5-M); default off, dump-only.
    void EnableCycleTrace(bool enabled);
    // Link classes for the first N0 DC EVs and the K sessions (G3-(b));
    // transitioned sessions keep their class. Empty = uniform NOMINAL.
    void ConfigureScenario(uint32_t n0, uint32_t kBurst,
                           const std::vector<PlcProfileClass>& dcClasses = {},
                           const std::vector<PlcProfileClass>& sessionClasses = {});
    // Staggered arrivals (Theorem-1 adversarial interleaving); index-aligned
    // with the burst sessions, default all-zero (burst at cycle 0).
    void SetSessionArrivalCycles(const std::vector<uint32_t>& arrivals);
    void Start(uint32_t horizonCycles);
    PolicyRunStats GetStats() const;

  private:
    struct Session
    {
        uint32_t arrivalCycle{0}; // not a candidate before this boundary
        int admittedCycle{-1};
        int completedCycle{-1};
        uint32_t next{0};
        uint32_t attempts{0};
        double credit{0.0};
        bool failed{false};
        PlcProfileClass linkClass{PlcProfileClass::NOMINAL};
    };

    enum class Phase
    {
        REQUESTS,
        SLAC,
        RESPONSES,
        DONE
    };

    void CycleBoundary();
    void Advance();      // pick the next frame in the serialized cycle plan
    void OnTxEnd(uint32_t durationSlots, bool isDc, uint32_t evId, bool isResponse);
    bool RollFailure(uint32_t evId, uint64_t startSlot, uint32_t durationSlots,
                     PlcProfileClass linkClass, bool isSlac);
    bool AdmitCandidate(const Session& candidate) const;
    double SessionCredit(const Session& session) const;
    void FinalizeStats();
    static Time SlotsToExactTime(uint64_t slots);
    uint64_t NowSlot() const;

    EvPlcParams m_params;
    Ptr<PlcSharedChannel> m_channel;
    TransitionAdmissionController m_countController;
    LinkAwareAdmissionController m_linkController;

    bool m_acbs{true};
    uint32_t m_q{25};
    uint32_t m_retryCap{0};
    double m_bFix{0.0};
    PolicyAdmission m_admission{PolicyAdmission::COUNT};
    bool m_aggCap{false};
    double m_gDebt{0.0};
    double m_cycleQuota{0.0};
    uint32_t m_cycleConsumed{0};
    uint32_t m_svcStart{0};
    uint32_t m_svcVisited{0};
    int m_blockedFirst{-1};
    PolicyErrorSource m_errorSource{PolicyErrorSource::INTERNAL_IID};
    double m_iidPer{0.0};
    std::mt19937 m_iidRng{1};
    std::uniform_real_distribution<double> m_uni{0.0, 1.0};

    uint32_t m_nDc{0};
    std::vector<PlcProfileClass> m_dcClasses;
    std::vector<Session> m_sessions;
    uint32_t m_pendingTransitions{0};
    std::vector<PlcProfileClass> m_pendingTransitionClasses;
    uint32_t m_horizonCycles{0};
    uint32_t m_cycleIndex{0};   // next boundary index
    uint32_t m_currentCycle{0}; // cycle in flight
    uint64_t m_cycleStartSlot{0};

    Phase m_phase{Phase::DONE};
    bool m_boundaryPending{false};
    bool m_condBEnabled{true};
    bool m_traceEnabled{false};
    bool m_traceValid{false};
    PolicyCycleTraceRow m_traceRow;
    void CloseTraceRow();
    bool m_cycleLowSlack{false};  // slack classification of the in-flight cycle
    bool m_cycleLowSlack2{false}; // same, slack < 2 retransmissions
    uint32_t m_evCursor{0};
    uint32_t m_sessionCursor{0};
    std::vector<uint64_t> m_readySlot;
    PolicyRunStats m_stats;
};

} // namespace ns3

#endif // EV_PLC_POLICY_MAC_H
