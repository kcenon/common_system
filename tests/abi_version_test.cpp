// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <kcenon/common/config/abi_version.h>
#include <gtest/gtest.h>
#include <string_view>

using namespace kcenon::common::abi;

/**
 * @brief Test suite for ABI version management
 *
 * These tests verify that:
 * 1. ABI version information is correctly embedded
 * 2. Compile-time checks work as expected
 * 3. Runtime version checking functions correctly
 * 4. Link-time symbols are unique per version
 */
class ABIVersionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Verify basic version sanity
        ASSERT_GT(version_major, 0);
        ASSERT_GE(version_minor, 0);
        ASSERT_GE(version_patch, 0);
    }
};

// ============================================================================
// Basic Version Information Tests
// ============================================================================

TEST_F(ABIVersionTest, VersionComponents) {
    // Version components should be reasonable
    EXPECT_LE(version_major, 100);
    EXPECT_LE(version_minor, 100);
    EXPECT_LE(version_patch, 1000);
}

TEST_F(ABIVersionTest, CombinedVersion) {
    // Verify version encoding: 0xMMMMNNPP
    uint32_t expected = (version_major << 16) | (version_minor << 8) | version_patch;
    EXPECT_EQ(version, expected);
}

TEST_F(ABIVersionTest, VersionString) {
    // Version string should not be empty
    EXPECT_FALSE(version_string.empty());

    // Should contain dots
    EXPECT_NE(version_string.find('.'), std::string_view::npos);
}

TEST_F(ABIVersionTest, EventBusVersion) {
    // Event bus ABI version should be in valid range
    EXPECT_GE(event_bus_version, 1);
    EXPECT_LE(event_bus_version, 10);
}

TEST_F(ABIVersionTest, BuildInformation) {
    // Build timestamp should not be empty
    EXPECT_FALSE(build_timestamp.empty());

    // Build type should not be empty
    EXPECT_FALSE(build_type.empty());
}

// ============================================================================
// Compile-Time ABI Checker Tests
// ============================================================================

TEST_F(ABIVersionTest, CompileTimeCheck_SameVersion) {
    // This should compile successfully (exact match)
    EXPECT_NO_THROW({
        [[maybe_unused]] abi_checker<version> checker;
    });
}

// Note: The following test would fail to compile if uncommented,
// which is the intended behavior
/*
TEST_F(ABIVersionTest, CompileTimeCheck_DifferentVersion) {
    // This should NOT compile (version mismatch)
    [[maybe_unused]] abi_checker<0x00FF0000> checker;
}
*/

// ============================================================================
// Runtime ABI Checker Tests
// ============================================================================

TEST_F(ABIVersionTest, RuntimeCheck_ExactMatch) {
    EXPECT_TRUE(check_abi_version(version));
}

TEST_F(ABIVersionTest, RuntimeCheck_Mismatch) {
    // Different version should fail
    uint32_t different_version = version ^ 0x00010000; // Flip major version bit
    EXPECT_FALSE(check_abi_version(different_version));
}

TEST_F(ABIVersionTest, RuntimeCheck_Zero) {
    EXPECT_FALSE(check_abi_version(0));
}

// ============================================================================
// Compatibility Checking Tests
// ============================================================================

TEST_F(ABIVersionTest, Compatibility_SameVersion) {
    EXPECT_TRUE(is_compatible(version));
}

TEST_F(ABIVersionTest, Compatibility_SameMajor_OlderMinor) {
    // Same major, older minor should be compatible
    if (version_minor > 0) {
        uint32_t older_version = version - 0x00000100; // Decrease minor by 1
        EXPECT_TRUE(is_compatible(older_version));
    }
}

TEST_F(ABIVersionTest, Compatibility_SameMajor_NewerMinor) {
    // Same major, newer minor should NOT be compatible
    uint32_t newer_version = version + 0x00000100; // Increase minor by 1
    EXPECT_FALSE(is_compatible(newer_version));
}

TEST_F(ABIVersionTest, Compatibility_DifferentMajor) {
    // Different major version should NOT be compatible
    uint32_t different_major = version ^ 0x00010000; // Flip major version bit
    EXPECT_FALSE(is_compatible(different_major));
}

TEST_F(ABIVersionTest, Compatibility_SameMajorMinor_DifferentPatch) {
    // Same major/minor, different patch should be compatible
    uint32_t different_patch = version + 1; // Increase patch by 1
    EXPECT_TRUE(is_compatible(different_patch));
}

// ============================================================================
// Link-Time Symbol Tests
// ============================================================================

TEST_F(ABIVersionTest, ABISignature_NotNull) {
    const char* sig = get_abi_signature();
    ASSERT_NE(sig, nullptr);
}

TEST_F(ABIVersionTest, ABISignature_NotEmpty) {
    const char* sig = get_abi_signature();
    EXPECT_GT(std::strlen(sig), 0);
}

TEST_F(ABIVersionTest, ABISignature_ContainsVersionInfo) {
    const char* sig = get_abi_signature();
    std::string sig_str(sig);

    // Signature should contain "kcenon_common_abi"
    EXPECT_NE(sig_str.find("kcenon_common_abi"), std::string::npos);
}

TEST_F(ABIVersionTest, ABISignature_IsStable) {
    // Multiple calls should return the same pointer (static storage)
    const char* sig1 = get_abi_signature();
    const char* sig2 = get_abi_signature();
    EXPECT_EQ(sig1, sig2);
}

TEST_F(ABIVersionTest, ABIInfo_NotNull) {
    const char* info = get_abi_info();
    ASSERT_NE(info, nullptr);
}

TEST_F(ABIVersionTest, ABIInfo_ContainsVersionString) {
    const char* info = get_abi_info();
    std::string info_str(info);

    // Should contain version string
    EXPECT_NE(info_str.find(version_string), std::string::npos);
}

TEST_F(ABIVersionTest, ABIInfo_ContainsSignature) {
    const char* info = get_abi_info();
    const char* sig = get_abi_signature();
    std::string info_str(info);
    std::string sig_str(sig);

    // Info should contain the signature
    EXPECT_NE(info_str.find(sig_str), std::string::npos);
}

// ============================================================================
// Link-Time Enforcement Tests
// ============================================================================

TEST_F(ABIVersionTest, LinkTimeEnforcer_CanCall) {
    // Should be able to call the link-time enforcer
    EXPECT_NO_THROW({
        require_abi_check();
    });
}

TEST_F(ABIVersionTest, LinkTimeEnforcer_MultipleCalls) {
    // Multiple calls should not cause issues
    EXPECT_NO_THROW({
        require_abi_check();
        require_abi_check();
        require_abi_check();
    });
}

// ============================================================================
// Integration Test: Full ABI Check Workflow
// ============================================================================

TEST_F(ABIVersionTest, FullWorkflow) {
    // 1. Get version information
    const char* info = get_abi_info();
    ASSERT_NE(info, nullptr);

    // 2. Verify runtime check passes for current version
    EXPECT_TRUE(check_abi_version(version));

    // 3. Verify compatibility check passes for current version
    EXPECT_TRUE(is_compatible(version));

    // 4. Get signature
    const char* sig = get_abi_signature();
    ASSERT_NE(sig, nullptr);

    // 5. Force link-time check
    EXPECT_NO_THROW({
        require_abi_check();
    });
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(ABIVersionTest, MaxVersionValue) {
    // Test with maximum possible version value
    uint32_t max_version = 0xFFFFFFFF;
    EXPECT_FALSE(is_compatible(max_version));
}

TEST_F(ABIVersionTest, MinVersionValue) {
    // Test with minimum version value
    uint32_t min_version = 0x00000000;
    EXPECT_FALSE(is_compatible(min_version));
}

TEST_F(ABIVersionTest, VersionOverflow) {
    // Ensure version components don't cause overflow
    uint32_t reconstructed = (version_major << 16) | (version_minor << 8) | version_patch;
    EXPECT_EQ(version, reconstructed);

    // No bits should be lost
    EXPECT_EQ((version >> 16) & 0xFFFF, version_major);
    EXPECT_EQ((version >> 8) & 0xFF, version_minor);
    EXPECT_EQ(version & 0xFF, version_patch);
}
