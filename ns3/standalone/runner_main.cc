#include "ns3/test.h"

int
main()
{
    const int failures = ns3::RunAllRegisteredTests();
    return failures == 0 ? 0 : 1;
}
