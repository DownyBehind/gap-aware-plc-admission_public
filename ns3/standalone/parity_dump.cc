// Physics-OFF parity dump (Stage 5b): plays the grant map for the E1 grid
// through BeaconMapSlotMachine and prints measured finish, formula finish,
// and observed phase boundaries. Consumed by experiments/ns3_e1/run_parity.py.

#include "ns3/beacon-map-slot-machine.h"
#include "ns3/ev-plc-params.h"
#include "ns3/grant-map-scheduler.h"

#include <iostream>

int
main()
{
    ns3::EvPlcParams params;
    ns3::GrantMapScheduler scheduler(params);
    ns3::BeaconMapSlotMachine machine(params); // physics OFF defaults
    const uint32_t kValues[] = {0, 1, 4, 8, 16};
    std::cout << "N,K,finish_measured,finish_formula,beacon_end,req_end,slac_end,guard_end,"
                 "response_start,last_frame_end,first_response_finish\n";
    for (uint32_t n = 1; n <= 40; ++n)
    {
        for (const auto k : kValues)
        {
            const auto played = machine.PlayCycle(scheduler.BuildGrantMap(n, k, 0));
            std::cout << n << ',' << k << ',' << played.finishSlot << ','
                      << scheduler.ComputeFinishTime(n, k) << ',' << played.beaconEnd << ','
                      << played.reqEnd << ',' << played.slacEnd << ',' << played.guardEnd << ','
                      << played.responseStart << ',' << played.lastFrameEnd << ','
                      << (played.responseFinishSlots.empty() ? 0 : played.responseFinishSlots.front())
                      << "\n";
        }
    }
    return 0;
}
