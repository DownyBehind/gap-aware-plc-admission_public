// Stage 5c-i overhead-physics dump: plays the E1 grid with IFS/beacon/PRS
// enabled and prints measured finish per cell. Usage:
//   overhead_dump <ifs_slots> <beacon_slots> <prs_slots>
// Consumed by experiments/ns3_e1/run_overhead.py.

#include "ns3/beacon-map-slot-machine.h"
#include "ns3/ev-plc-params.h"
#include "ns3/grant-map-scheduler.h"

#include <cstdlib>
#include <iostream>

int
main(int argc, char** argv)
{
    ns3::SlotMachineConfig config;
    if (argc > 1)
    {
        config.ifsSlots = static_cast<uint32_t>(std::atoi(argv[1]));
    }
    if (argc > 2)
    {
        config.beaconSlots = static_cast<uint32_t>(std::atoi(argv[2]));
    }
    if (argc > 3)
    {
        config.prsSlots = static_cast<uint32_t>(std::atoi(argv[3]));
    }

    ns3::EvPlcParams params;
    ns3::GrantMapScheduler scheduler(params);
    ns3::BeaconMapSlotMachine machine(params, config);
    const uint32_t kValues[] = {0, 1, 4, 8, 16};
    std::cout << "N,K,ifs,beacon,prs,finish_measured\n";
    for (uint32_t n = 1; n <= 40; ++n)
    {
        for (const auto k : kValues)
        {
            const auto played = machine.PlayCycle(scheduler.BuildGrantMap(n, k, 0));
            std::cout << n << ',' << k << ',' << config.ifsSlots << ',' << config.beaconSlots
                      << ',' << config.prsSlots << ',' << played.finishSlot << "\n";
        }
    }
    return 0;
}
