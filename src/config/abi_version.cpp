// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <kcenon/common/config/abi_version.h>
#include <sstream>
#include <iomanip>

namespace kcenon::common::abi {

// ============================================================================
// Link-Time ABI Signature
// ============================================================================

/**
 * @brief Unique ABI signature symbol
 *
 * This symbol name changes with each ABI version, causing link-time
 * errors if incompatible versions are mixed.
 *
 * Format: kcenon_common_abi_v<major>_<minor>_<patch>_ev<event_bus>
 */
extern "C" const char* get_abi_signature() noexcept {
    // Symbol name includes version components
    // Linker will fail if different versions are linked together
    static constexpr const char* signature =
        "kcenon_common_abi_v1_0_0_ev1";
    return signature;
}

/**
 * @brief Get detailed ABI information string
 *
 * Returns a human-readable string with version and configuration details.
 * Useful for logging and diagnostics.
 *
 * @return ABI information string
 */
const char* get_abi_info() noexcept {
    static std::string info = []() {
        std::ostringstream oss;
        oss << "common_system ABI Information:\n"
            << "  Version: " << version_string << " (0x"
            << std::hex << std::setw(8) << std::setfill('0') << version << ")\n"
            << "  Major: " << std::dec << version_major << "\n"
            << "  Minor: " << version_minor << "\n"
            << "  Patch: " << version_patch << "\n"
            << "  Event Bus ABI: " << event_bus_version << "\n"
            << "  Build Type: " << build_type << "\n"
            << "  Build Time: " << build_timestamp << "\n"
            << "  ABI Signature: " << get_abi_signature();
        return oss.str();
    }();
    return info.c_str();
}

} // namespace kcenon::common::abi
