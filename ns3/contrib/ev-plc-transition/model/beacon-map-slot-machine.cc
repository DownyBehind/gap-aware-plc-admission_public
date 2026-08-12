#include "beacon-map-slot-machine.h"

#include <algorithm>

namespace ns3
{

BeaconMapSlotMachine::BeaconMapSlotMachine(const EvPlcParams& params,
                                           const SlotMachineConfig& config)
    : m_params(params), m_config(config)
{
}

PlayedCycle
BeaconMapSlotMachine::PlayCycle(const GrantMap& map) const
{
    const uint32_t n = map.nDc;
    const uint32_t k = map.kSlac;
    const uint32_t ifs = m_config.ifsSlots;

    PlayedCycle cycle;
    cycle.n = n;
    cycle.k = k;

    uint32_t cursor = 0;

    // Cycle head: beacon broadcast (c_0) followed contiguously by the PRS
    // window (rule 1); one IFS separates a non-empty head block from the
    // first scheduled frame (rule 2). See docs/model/physics_rules.md — that
    // table is normative for every IFS placement below.
    if (m_config.beaconSlots > 0)
    {
        cycle.frames.push_back({"BEACON", 0, cursor, cursor + m_config.beaconSlots});
        cursor += m_config.beaconSlots;
    }
    if (m_config.prsSlots > 0)
    {
        cycle.frames.push_back({"PRS", 0, cursor, cursor + m_config.prsSlots});
        cursor += m_config.prsSlots;
    }
    if (cursor > 0)
    {
        cursor += ifs;
    }
    cycle.beaconEnd = cursor;

    // DC request frames; each EV's off-channel processing starts when its own
    // request frame completes.
    std::vector<uint32_t> responseReady(n, 0);
    for (uint32_t ev = 0; ev < n; ++ev)
    {
        const uint32_t start = cursor;
        const uint32_t end = start + m_params.m_cReqEffSlots;
        cycle.frames.push_back({"DC_REQ", ev, start, end});
        responseReady[ev] = end + m_params.m_cProcSlots;
        cursor = end + (ev + 1 < n ? ifs : 0);
    }
    if (n > 0)
    {
        cursor += ifs; // spacing between the request block and what follows
    }
    cycle.reqEnd = cursor;

    // SLAC service. Parity mode plays the same abstraction as Layer 1: the
    // per-period budget q*K as one contiguous block followed by the B_pkt
    // guard envelope of the last non-preemptive SLAC frame. Replaying the
    // real 20-message SLAC sequence is deferred to Stage 5c-ii (there the
    // guard becomes a measured overrun bounded by B_pkt).
    if (k > 0)
    {
        const uint32_t slacLen = m_params.m_bAuthSlots * k;
        cycle.frames.push_back({"SLAC_SERVICE", 0, cursor, cursor + slacLen});
        cursor += slacLen;
        cycle.slacEnd = cursor;
        cycle.frames.push_back({"PKT_GUARD", 0, cursor, cursor + m_params.m_bPktSlots});
        cursor += m_params.m_bPktSlots;
        cycle.guardEnd = cursor;
        cursor += ifs;
    }
    else
    {
        cycle.slacEnd = cursor;
        cycle.guardEnd = cursor;
    }

    // DC response frames: each starts when the channel is free AND the EV's
    // own processing is done. No formula: readiness comes from the replayed
    // request completion times above.
    uint32_t lastEnd = cursor;
    for (uint32_t ev = 0; ev < n; ++ev)
    {
        const uint32_t start = std::max(cursor, responseReady[ev]);
        const uint32_t end = start + m_params.m_cResEffSlots;
        cycle.frames.push_back({"DC_RES", ev, start, end});
        cycle.responseFinishSlots.push_back(end);
        if (ev == 0)
        {
            cycle.responseStart = start;
        }
        cursor = end + (ev + 1 < n ? ifs : 0);
        lastEnd = end;
    }
    if (n == 0)
    {
        cycle.responseStart = cursor;
        lastEnd = cycle.guardEnd > cycle.beaconEnd ? cycle.guardEnd : cycle.beaconEnd;
    }
    cycle.lastFrameEnd = lastEnd;

    if (cycle.frames.empty())
    {
        // Empty cycle: nothing played, nothing for the carry-in envelope to
        // delay.
        cycle.finishSlot = 0;
    }
    else
    {
        cycle.finishSlot = lastEnd + m_params.m_bBlkSlots;
    }
    return cycle;
}

} // namespace ns3
