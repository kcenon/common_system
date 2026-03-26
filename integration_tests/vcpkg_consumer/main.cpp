/**
 * @file main.cpp
 * @brief Minimal test consumer for kcenon Tier 0-5 vcpkg build chain validation
 *
 * This program verifies that all 8 kcenon ecosystem libraries can be found,
 * included, and linked in a single consumer project. It does not exercise
 * runtime functionality — it validates build chain integrity only.
 *
 * Ports are conditionally included via HAS_*_SYSTEM compile definitions
 * set by CMakeLists.txt based on which ports were found.
 */

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

// Tier 0: common_system (always required)
#ifdef HAS_COMMON_SYSTEM
#include <kcenon/common/common.h>
#endif

// Tier 1: thread_system (optional)
#ifdef HAS_THREAD_SYSTEM
#include <thread_system/utilities/formatter.h>
#endif

// Tier 1: container_system (optional)
#ifdef HAS_CONTAINER_SYSTEM
#include <container.h>
#endif

// Tier 2: logger_system (optional)
#ifdef HAS_LOGGER_SYSTEM
#include <logger_system/core/logger.h>
#endif

// Tier 3: monitoring_system (optional — header-light, link-only validation)
// monitoring_system exports static libraries but no public user-facing headers

// Tier 3: database_system (optional)
#ifdef HAS_DATABASE_SYSTEM
#include <database_system/database/database_manager.h>
#endif

// Tier 4: network_system (optional — use types/result.h to avoid network_system.h
// which references internal/ headers not installed in vcpkg layout)
#ifdef HAS_NETWORK_SYSTEM
#include <kcenon/network/types/result.h>
#endif

// Tier 5: pacs_system (optional)
#ifdef HAS_PACS_SYSTEM
#include <pacs/core/dicom_dataset.hpp>
#endif

int main()
{
    std::cout << "=== kcenon vcpkg build chain validation ===" << std::endl;

    int total = 0;
    int found = 0;

    auto check = [&](const char* name, bool available) {
        total++;
        if (available)
        {
            found++;
            std::cout << "  [OK]   " << name << std::endl;
        }
        else
        {
            std::cout << "  [SKIP] " << name << std::endl;
        }
    };

#ifdef HAS_COMMON_SYSTEM
    check("kcenon-common-system     (Tier 0)", true);
#else
    check("kcenon-common-system     (Tier 0)", false);
#endif

#ifdef HAS_THREAD_SYSTEM
    check("kcenon-thread-system     (Tier 1)", true);
#else
    check("kcenon-thread-system     (Tier 1)", false);
#endif

#ifdef HAS_CONTAINER_SYSTEM
    check("kcenon-container-system  (Tier 1)", true);
#else
    check("kcenon-container-system  (Tier 1)", false);
#endif

#ifdef HAS_LOGGER_SYSTEM
    check("kcenon-logger-system     (Tier 2)", true);
#else
    check("kcenon-logger-system     (Tier 2)", false);
#endif

#ifdef HAS_MONITORING_SYSTEM
    check("kcenon-monitoring-system (Tier 3)", true);
#else
    check("kcenon-monitoring-system (Tier 3)", false);
#endif

#ifdef HAS_DATABASE_SYSTEM
    check("kcenon-database-system   (Tier 3)", true);
#else
    check("kcenon-database-system   (Tier 3)", false);
#endif

#ifdef HAS_NETWORK_SYSTEM
    check("kcenon-network-system    (Tier 4)", true);
#else
    check("kcenon-network-system    (Tier 4)", false);
#endif

#ifdef HAS_PACS_SYSTEM
    check("kcenon-pacs-system       (Tier 5)", true);
#else
    check("kcenon-pacs-system       (Tier 5)", false);
#endif

    std::cout << std::endl;
    std::cout << found << "/" << total << " ports validated." << std::endl;

    if (found == total)
    {
        std::cout << "Build chain validation PASSED." << std::endl;
        return EXIT_SUCCESS;
    }
    else
    {
        std::cout << "Build chain validation PARTIAL." << std::endl;
        return (found > 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
}
