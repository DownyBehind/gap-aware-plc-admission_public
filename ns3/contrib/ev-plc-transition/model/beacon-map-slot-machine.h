#ifndef BEACON_MAP_SLOT_MACHINE_H
#define BEACON_MAP_SLOT_MACHINE_H

#include "ev-plc-params.h"
#include "ev-plc-types.h"
#include "plc-error-model.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ns3
{

// Physics hooks (Stage 5c). All default to OFF so the machine plays the pure
// scheduled-access map; 5c turns them on one effect at a time.
struct SlotMachineConfig
{
    uint32_t beaconSlots{0};      // C_bcn constant term c_0 broadcast at cycle head
    uint32_t prsSlots{0};         // priority-resolution window, once at cycle head
    uint32_t ifsSlots{0};         // inter-frame spacing (CIFS/RIFS abstraction)
    double per{0.0};              // frame error rate (needs errorModel)
    PlcErrorModel* errorModel{nullptr};
    uint32_t beaconLossStreak{0}; // E3 hook: consecutive beacon losses to inject
};

struct PlayedFrame
{
    std::string name; // BEACON / DC_REQ / SLAC_SERVICE / PKT_GUARD / DC_RES
    uint32_t evId{0}; // 0-based EV index for DC frames; 0 otherwise
    uint32_t startSlot{0};
    uint32_t endSlot{0};
};

struct PlayedCycle
{
    uint32_t n{0};
    uint32_t k{0};
    std::vector<PlayedFrame> frames;
    std::vector<uint32_t> responseFinishSlots; // per-EV response completion (E5)
    // Phase boundaries observed from the replay (for parity checks).
    uint32_t beaconEnd{0};
    uint32_t reqEnd{0};
    uint32_t slacEnd{0};
    uint32_t guardEnd{0};
    uint32_t responseStart{0};
    uint32_t lastFrameEnd{0};
    // Measured finish = last played frame end + the carry-in envelope B_blk.
    // B_blk is envelope accounting, not a played frame: with physics OFF no
    // actual carry-in frame exists, but the bound the map guarantees includes it.
    uint32_t finishSlot{0};
};

// Replays a grant map frame by frame and reports the measured finish time.
// This class must never call the finish/gap formulas: its purpose is to be an
// implementation-independent measurement of what the map actually does.
class BeaconMapSlotMachine
{
  public:
    explicit BeaconMapSlotMachine(const EvPlcParams& params,
                                  const SlotMachineConfig& config = SlotMachineConfig());

    PlayedCycle PlayCycle(const GrantMap& map) const;

  private:
    EvPlcParams m_params;
    SlotMachineConfig m_config;
};

} // namespace ns3

#endif // BEACON_MAP_SLOT_MACHINE_H
