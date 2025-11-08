// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file abi_version_example.cpp
 * @brief Example demonstrating ABI version checking
 */

#include <kcenon/common/config/abi_version.h>
#include <iostream>
#include <iomanip>

int main() {
    using namespace kcenon::common::abi;

    std::cout << "=== common_system ABI Information ===\n\n";

    // Display version components
    std::cout << "Version: " << version_string << "\n";
    std::cout << "Version (hex): 0x"
              << std::hex << std::setw(8) << std::setfill('0') << version << "\n";
    std::cout << std::dec;
    std::cout << "Major: " << version_major << "\n";
    std::cout << "Minor: " << version_minor << "\n";
    std::cout << "Patch: " << version_patch << "\n";
    std::cout << "\n";

    // Display ABI-specific versions
    std::cout << "Event Bus ABI Version: " << event_bus_version << "\n";
    std::cout << "\n";

    // Display build information
    std::cout << "Build Type: " << build_type << "\n";
    std::cout << "Build Timestamp: " << build_timestamp << "\n";
    std::cout << "\n";

    // Compile-time ABI checking example
    std::cout << "=== ABI Compatibility Checks ===\n\n";

    // This will compile successfully (version matches)
    abi_checker<0x00010000> version_check;
    std::cout << "✓ Compile-time check passed for version 0x00010000\n";

    // Runtime version checking
    uint32_t required_version = 0x00010000; // 1.0.0
    if (check_abi_version(required_version)) {
        std::cout << "✓ Runtime version check passed\n";
    } else {
        std::cout << "✗ Runtime version check failed\n";
    }

    // Compatibility checking
    if (is_compatible(0x00010000)) {
        std::cout << "✓ Compatible with version 1.0.0\n";
    }

    if (is_compatible(0x00000100)) {
        std::cout << "✓ Compatible with version 0.1.0\n";
    } else {
        std::cout << "✗ Not compatible with version 0.1.0 (major version mismatch)\n";
    }

    std::cout << "\n";

    // Link-time symbol check
    std::cout << "=== Link-Time ABI Signature ===\n\n";
    std::cout << "ABI Signature: " << get_abi_signature() << "\n";

    return 0;
}
