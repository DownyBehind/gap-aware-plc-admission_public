// Dumps GrantMapScheduler::ClassifyRegime over the full (N, K) grid as CSV.
// Consumed by tests/test_regime_semantics_parity.py to check that the C++
// regime semantics match src/formulas/transition_formulas.classify_regime.

#include "ns3/ev-plc-params.h"
#include "ns3/ev-plc-types.h"
#include "ns3/grant-map-scheduler.h"

#include <iostream>

int
main()
{
    ns3::EvPlcParams params;
    ns3::GrantMapScheduler scheduler(params);
    std::cout << "N,K,regime\n";
    for (uint32_t n = 0; n <= 45; ++n)
    {
        for (uint32_t k = 0; k <= 20; ++k)
        {
            std::cout << n << ',' << k << ',' << ns3::ToString(scheduler.ClassifyRegime(n, k)) << "\n";
        }
    }
    return 0;
}
