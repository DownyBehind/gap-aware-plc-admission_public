#ifndef EV_PLC_CSMA_MAC_H
#define EV_PLC_CSMA_MAC_H

// Track C C3: event-driven HPGP CSMA/CA MAC over PlcSharedChannel. The
// backoff countdown is re-expressed as "resume-time events + closed-form
// idle-elapsed arithmetic" (no slot ticking); the BPC/CW table and the
// collision/drop/success state transitions are performed exclusively through
// HpgpCsmaCaParams / HpgpContentionNode (single SoT — see
// docs/model/physics_rules.md (CSMA rules)). Backoff draws come from a DEDICATED RNG
// stream (§6.1); frame-error rolls stay on the channel's frame stream.
//
// The E4 scenario semantics (burst sessions with the paper Table-I sequence,
// proc-gated EVSE responses, completion -> DC transition at the next cycle
// boundary, persistent frame retry on error/drop) mirror
// ns3/standalone/e4_sim.cc's CSMA runner minus its per-cycle resync
// approximation: contention state and backlog persist across cycles.
//
// Event-only class: excluded from the standalone build.

#include "ev-plc-params.h"
#include "hpgp-contention-node.h"
#include "hpgp-csma-ca-params.h"
#include "plc-shared-channel.h"

#include "ns3/ptr.h"
#include "ns3/simple-ref-count.h"

#include <cstdint>
#include <deque>
#include <map>
#include <random>
#include <vector>

namespace ns3
{

struct CsmaRunStats
{
    uint64_t dcMisses{0};
    uint64_t dcEvCycles{0};
    uint32_t dgViolations{0};
    uint32_t completedSessions{0};
    uint32_t collisions{0};
    uint32_t drops{0};
};

class EvPlcCsmaMac : public SimpleRefCount<EvPlcCsmaMac>
{
  public:
    EvPlcCsmaMac(const EvPlcParams& params, Ptr<PlcSharedChannel> channel);

    void SetBackoffRngSeed(uint32_t seed);
    void ConfigureScenario(uint32_t n0, uint32_t kBurst);
    void Start(uint32_t horizonCycles);
    CsmaRunStats GetStats() const;

  private:
    struct Session
    {
        uint32_t next{0};
        int completedCycle{-1};
        bool framePending{false};
    };

    void CycleBoundary();
    void Reschedule();
    void OnWake(uint64_t serial);
    // TX/collision completions are physical events: never serial-guarded
    // (a mid-flight Reschedule must not cancel a frame in the air).
    void OnTxEnd(uint32_t nodeId);
    void OnCollisionEnd();
    void EnqueueResponse(uint32_t evIndex);
    void EnqueueDcRequest(uint32_t evIndex);
    void EnqueueSlacIfReleased(uint32_t sessionIndex);
    HpgpContentionNode& NodeOf(uint32_t nodeId);
    static Time SlotsToExactTime(uint64_t slots);
    uint64_t NowSlot() const;

    EvPlcParams m_params;
    HpgpCsmaCaParams m_csmaParams;
    Ptr<PlcSharedChannel> m_channel;
    std::mt19937 m_csmaRng{1}; // dedicated backoff stream (§6.1)

    std::map<uint32_t, HpgpContentionNode> m_nodes;
    std::map<uint32_t, uint32_t> m_remaining; // idle-slots left per contender
    std::map<uint32_t, uint64_t> m_gateSlot;  // earliest eligible start
    uint64_t m_idleStartSlot{0};
    bool m_collisionActive{false};
    uint64_t m_serial{0}; // stale-event guard

    uint32_t m_nDc{0};
    uint32_t m_kBurst{0};
    uint32_t m_pendingTransitions{0};
    uint32_t m_horizonCycles{0};
    uint32_t m_cycleIndex{0};
    uint32_t m_currentCycle{0}; // cycle whose frames are on the air (mid-cycle checks)
    uint32_t m_responsesThisCycle{0};
    uint32_t m_nDcAtCycleStart{0};
    std::deque<uint32_t> m_responseFifo; // ev order of pending EVSE responses
    std::vector<Session> m_sessions;
    CsmaRunStats m_stats;
};

} // namespace ns3

#endif // EV_PLC_CSMA_MAC_H
