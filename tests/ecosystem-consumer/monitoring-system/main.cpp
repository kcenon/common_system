// monitoring_system exports static libraries but no public user-facing headers.
// This consumer validates that find_package succeeds and the target links.

#include <iostream>

int main()
{
    std::cout << "monitoring_system: find_package OK" << std::endl;
    return 0;
}
