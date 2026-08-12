#ifndef EV_PLC_MAC_H
#define EV_PLC_MAC_H

// Track C layer-1 MAC: drives cycles as Simulator events. The scheduled mode
// calls GrantMapScheduler::BuildGrantMap at each cycle boundary (single SoT,
// no logic duplication) and plays the map as a chain of frame TX events over
// PlcSharedChannel. Internal arithmetic stays in integer slots; Time is
// produced only at the scheduling boundary as exact nanoseconds
// (35.84 us = 35840 ns per slot).
// Event-only class: excluded from the standalone build.

#include "ev-plc-params.h"
#include "grant-map-scheduler.h"
#include "plc-shared-channel.h"

#include "ns3/ptr.h"
#include "ns3/simple-ref-count.h"

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace ns3
{

struct MacCycleRecord
{
    uint32_t n{0};
    uint32_t k{0};
    std::vector<uint64_t> responseEndSlots; // per-EV, absolute
    uint64_t lastFrameEndSlot{0};
    uint32_t finishSlot{0}; // cycle-relative, incl. B_blk envelope (0 = empty)
    uint32_t retxCount{0};
};

class EvPlcMac : public SimpleRefCount<EvPlcMac>
{
  public:
    EvPlcMac(const EvPlcParams& params, Ptr<PlcSharedChannel> channel);

    // Demand registration (called by the apps in StartApplication).
    void AddDcEv();
    void AddSlacSession();

    // E5 adversarial hook: realized carry-in/head block at the cycle start.
    void SetHeadBlockSlots(uint32_t slots);

    // Run `cycles` cycles starting at t = 0 (schedules the first cycle event).
    void Start(uint32_t cycles);

    const std::vector<MacCycleRecord>& GetRecords() const;

  private:
    struct PendingFrame
    {
        std::string name; // HEAD / DC_REQ / SLAC_SERVICE / PKT_GUARD / DC_RES
        uint32_t evId{0};
        uint32_t durationSlots{0};
        bool hasGate{false};
        uint64_t gateSlot{0}; // earliest start (absolute), e.g. proc readiness
        bool perApplies{false};
    };

    void Bootstrap();
    void StartCycle();
    void TryNext();
    void OnTxEnd(uint32_t durationSlots);
    static Time SlotsToExactTime(uint64_t slots);

    EvPlcParams m_params;
    GrantMapScheduler m_scheduler;
    Ptr<PlcSharedChannel> m_channel;
    uint32_t m_nDc{0};
    uint32_t m_kSlac{0};
    uint32_t m_headBlockSlots{0};
    uint32_t m_cyclesRemaining{0};
    uint32_t m_cycleIndex{0};
    uint64_t m_cycleStartSlot{0};
    std::deque<PendingFrame> m_queue;
    std::vector<uint64_t> m_readySlot; // per-EV response readiness (absolute)
    MacCycleRecord m_current;
    std::vector<MacCycleRecord> m_records;
};

} // namespace ns3

#endif // EV_PLC_MAC_H
