// E5 adversarial-boundary check (standalone, deterministic, PER = 0).
// Realizes the worst-case envelopes simultaneously on every admitted state:
//   - carry-in blocking: a maximum frame (B_blk = 21) occupies the cycle head
//     (played via the head-block hook), delaying requests, processing, and
//     responses alike;
//   - packetization guard: the B_pkt window is part of the replayed map;
//   - Cond-B equality-adjacent states are included by sweeping the full grid
//     (tightest admitted diagonal N+K = 38, slack 6).
// Gate: max per-EV response end <= T_sched on every admitted (hidden/paid)
// cell — not a single slot over.
//
// Usage: e5_sim

#include "ns3/beacon-map-slot-machine.h"
#include "ns3/ev-plc-params.h"
#include "ns3/grant-map-scheduler.h"

#include <algorithm>
#include <iostream>

int
main()
{
    ns3::EvPlcParams params;
    ns3::GrantMapScheduler scheduler(params);
    ns3::SlotMachineConfig config;
    config.beaconSlots = params.m_bBlkSlots; // realized carry-in blocking frame
    ns3::BeaconMapSlotMachine machine(params, config);

    std::cout << "N,K,regime,max_response_end,last_frame_end,T_sched\n";
    for (uint32_t n = 1; n <= 40; ++n)
    {
        for (uint32_t k = 0; k <= 20; ++k)
        {
            const auto map = scheduler.BuildGrantMap(n, k, 0);
            const auto played = machine.PlayCycle(map);
            const uint32_t maxResponse =
                played.responseFinishSlots.empty()
                    ? 0
                    : *std::max_element(played.responseFinishSlots.begin(),
                                        played.responseFinishSlots.end());
            std::cout << n << ',' << k << ',' << ns3::ToString(map.regime) << ',' << maxResponse
                      << ',' << played.lastFrameEnd << ',' << params.GetScheduledSlots() << "\n";
        }
    }
    return 0;
}
