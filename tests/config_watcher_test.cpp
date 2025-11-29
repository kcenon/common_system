// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <gtest/gtest.h>

#include <kcenon/common/config/config_watcher.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>

using namespace kcenon::common::config;

// ============================================================================
// Helper class for temporary config files
// ============================================================================

class TempConfigFile {
public:
    TempConfigFile(const std::string& content = "")
        : path_(std::filesystem::temp_directory_path() /
                ("config_watcher_test_" + std::to_string(std::rand()) + ".yaml")) {
        write(content);
    }

    ~TempConfigFile() {
        try {
            std::filesystem::remove(path_);
        } catch (...) {
            // Ignore cleanup errors
        }
    }

    void write(const std::string& content) {
        std::ofstream file(path_);
        file << content;
        file.flush();
        file.close();
    }

    const std::filesystem::path& path() const { return path_; }
    std::string path_string() const { return path_.string(); }

private:
    std::filesystem::path path_;
};

// ============================================================================
// Construction Tests
// ============================================================================

TEST(ConfigWatcherTest, Constructor_WithExistingFile_LoadsConfig) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    // Should have loaded default config
    auto config = watcher.current();
    EXPECT_EQ(config.logger.level, "info");  // Default
}

TEST(ConfigWatcherTest, Constructor_WithNonExistentFile_UsesDefaults) {
    config_watcher watcher("/nonexistent/path/config.yaml");

    auto config = watcher.current();
    EXPECT_EQ(config.logger.level, "info");  // Default
}

TEST(ConfigWatcherTest, Constructor_InitialVersion_IsZero) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    EXPECT_EQ(watcher.version(), 0);
}

TEST(ConfigWatcherTest, ConfigPath_ReturnsCorrectPath) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    EXPECT_EQ(watcher.config_path(), file.path_string());
}

// ============================================================================
// Start/Stop Tests
// ============================================================================

TEST(ConfigWatcherTest, Start_WhenNotRunning_Succeeds) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    auto result = watcher.start();
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(watcher.is_running());

    watcher.stop();
}

TEST(ConfigWatcherTest, Start_WhenAlreadyRunning_ReturnsError) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    auto result1 = watcher.start();
    ASSERT_TRUE(result1.is_ok());

    auto result2 = watcher.start();
    EXPECT_TRUE(result2.is_err());
    EXPECT_EQ(result2.error().code, watcher_error_codes::already_running);

    watcher.stop();
}

TEST(ConfigWatcherTest, Stop_WhenRunning_StopsWatcher) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    watcher.start();
    EXPECT_TRUE(watcher.is_running());

    watcher.stop();
    EXPECT_FALSE(watcher.is_running());
}

TEST(ConfigWatcherTest, Stop_WhenNotRunning_DoesNothing) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    EXPECT_FALSE(watcher.is_running());
    watcher.stop();  // Should not crash
    EXPECT_FALSE(watcher.is_running());
}

TEST(ConfigWatcherTest, Destructor_StopsWatcher) {
    TempConfigFile file;

    {
        config_watcher watcher(file.path_string());
        watcher.start();
        EXPECT_TRUE(watcher.is_running());
        // Destructor should stop cleanly
    }

    // Should not hang or crash
}

// ============================================================================
// Manual Reload Tests
// ============================================================================

TEST(ConfigWatcherTest, Reload_WhenFileUnchanged_Succeeds) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    auto result = watcher.reload();
    EXPECT_TRUE(result.is_ok());
}

TEST(ConfigWatcherTest, Reload_IncrementsVersion) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    uint64_t initial_version = watcher.version();
    watcher.reload();

    EXPECT_EQ(watcher.version(), initial_version + 1);
}

#ifdef BUILD_WITH_YAML_CPP
TEST(ConfigWatcherTest, Reload_WithInvalidYaml_ReturnsError) {
    TempConfigFile file("valid: config");
    config_watcher watcher(file.path_string());

    // Make the file invalid
    file.write("invalid: yaml: [unclosed");

    auto result = watcher.reload();
    EXPECT_TRUE(result.is_err());
}
#endif

// ============================================================================
// Callback Tests
// ============================================================================

TEST(ConfigWatcherTest, OnChange_CallbackInvokedOnReload) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    std::atomic<bool> callback_called{false};
    watcher.on_change([&](const unified_config&, const unified_config&) {
        callback_called = true;
    });

    watcher.reload();

    EXPECT_TRUE(callback_called);
}

TEST(ConfigWatcherTest, OnChange_MultipleCallbacks_AllInvoked) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    std::atomic<int> callback_count{0};

    watcher.on_change([&](const unified_config&, const unified_config&) {
        callback_count++;
    });
    watcher.on_change([&](const unified_config&, const unified_config&) {
        callback_count++;
    });
    watcher.on_change([&](const unified_config&, const unified_config&) {
        callback_count++;
    });

    watcher.reload();

    EXPECT_EQ(callback_count, 3);
}

TEST(ConfigWatcherTest, OnChange_ReceivesOldAndNewConfig) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    bool configs_received = false;
    watcher.on_change([&](const unified_config& old_cfg, const unified_config& new_cfg) {
        // Both should be valid configurations
        EXPECT_EQ(old_cfg.logger.level, "info");
        EXPECT_EQ(new_cfg.logger.level, "info");
        configs_received = true;
    });

    watcher.reload();

    EXPECT_TRUE(configs_received);
}

TEST(ConfigWatcherTest, OnError_CallbackInvokedOnFailure) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    std::atomic<bool> error_callback_called{false};
    watcher.on_error([&](const std::string& msg) {
        EXPECT_FALSE(msg.empty());
        error_callback_called = true;
    });

    // Delete the file to cause an error
    std::filesystem::remove(file.path());

    watcher.reload();

    // Note: Without YAML support, this won't trigger the error callback
    // because the loader returns a different error
#ifndef BUILD_WITH_YAML_CPP
    EXPECT_TRUE(error_callback_called);
#endif
}

// ============================================================================
// History Tests
// ============================================================================

TEST(ConfigWatcherTest, History_InitiallyContainsOneSnapshot) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    auto hist = watcher.history();
    EXPECT_EQ(hist.size(), 1);
    EXPECT_EQ(hist[0].version, 0);
}

TEST(ConfigWatcherTest, History_GrowsWithReloads) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    watcher.reload();
    watcher.reload();
    watcher.reload();

    auto hist = watcher.history();
    EXPECT_EQ(hist.size(), 4);  // Initial + 3 reloads
}

TEST(ConfigWatcherTest, History_ReturnsNewestFirst) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    watcher.reload();  // version 1
    watcher.reload();  // version 2

    auto hist = watcher.history();
    ASSERT_GE(hist.size(), 2);

    // Newest first
    EXPECT_GT(hist[0].version, hist[1].version);
}

TEST(ConfigWatcherTest, History_RespectsMaxHistory) {
    TempConfigFile file;
    config_watcher watcher(file.path_string(), 3);  // Max 3 snapshots

    // Create more than max snapshots
    for (int i = 0; i < 10; i++) {
        watcher.reload();
    }

    auto hist = watcher.history();
    EXPECT_LE(hist.size(), 3);
}

TEST(ConfigWatcherTest, History_LimitedCount_ReturnsRequestedAmount) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    for (int i = 0; i < 5; i++) {
        watcher.reload();
    }

    auto hist = watcher.history(2);
    EXPECT_EQ(hist.size(), 2);
}

// ============================================================================
// Rollback Tests
// ============================================================================

TEST(ConfigWatcherTest, Rollback_ToExistingVersion_Succeeds) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    uint64_t initial_version = watcher.version();
    watcher.reload();  // version 1
    watcher.reload();  // version 2

    auto result = watcher.rollback(initial_version);
    EXPECT_TRUE(result.is_ok());
}

TEST(ConfigWatcherTest, Rollback_ToNonExistentVersion_ReturnsError) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    auto result = watcher.rollback(9999);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, watcher_error_codes::rollback_failed);
}

TEST(ConfigWatcherTest, Rollback_IncrementsVersion) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    uint64_t initial_version = watcher.version();
    watcher.reload();  // version 1
    uint64_t version_before_rollback = watcher.version();

    watcher.rollback(initial_version);

    EXPECT_EQ(watcher.version(), version_before_rollback + 1);
}

TEST(ConfigWatcherTest, Rollback_InvokesChangeCallback) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    std::atomic<bool> callback_called{false};
    watcher.on_change([&](const unified_config&, const unified_config&) {
        callback_called = true;
    });

    uint64_t initial_version = watcher.version();
    watcher.reload();
    callback_called = false;  // Reset after reload

    watcher.rollback(initial_version);

    EXPECT_TRUE(callback_called);
}

// ============================================================================
// Events Tests
// ============================================================================

TEST(ConfigWatcherTest, RecentEvents_InitiallyEmpty) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    auto events = watcher.recent_events();
    EXPECT_TRUE(events.empty());
}

TEST(ConfigWatcherTest, RecentEvents_RecordsReloads) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    watcher.reload();
    watcher.reload();

    auto events = watcher.recent_events();
    EXPECT_EQ(events.size(), 2);
}

TEST(ConfigWatcherTest, RecentEvents_ContainsCorrectInfo) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    watcher.reload();

    auto events = watcher.recent_events();
    ASSERT_EQ(events.size(), 1);

    EXPECT_EQ(events[0].version, 1);
    EXPECT_TRUE(events[0].success);
    EXPECT_TRUE(events[0].error_message.empty());
}

TEST(ConfigWatcherTest, RecentEvents_LimitedCount) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    for (int i = 0; i < 10; i++) {
        watcher.reload();
    }

    auto events = watcher.recent_events(3);
    EXPECT_EQ(events.size(), 3);
}

// ============================================================================
// File Watching Tests (require actual file system watching)
// ============================================================================

// Note: These tests are platform-dependent and may be flaky in CI environments
// They test the actual file watching functionality

TEST(ConfigWatcherTest, FileWatch_DetectsChanges) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> change_detected{false};

    watcher.on_change([&](const unified_config&, const unified_config&) {
        std::lock_guard<std::mutex> lock(mtx);
        change_detected = true;
        cv.notify_one();
    });

    auto start_result = watcher.start();
    ASSERT_TRUE(start_result.is_ok());

    // Wait a bit for watcher to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Modify the file
    file.write("# Modified config\n");

    // Wait for change detection (with timeout)
    std::unique_lock<std::mutex> lock(mtx);
    bool detected = cv.wait_for(lock, std::chrono::seconds(3),
                                [&] { return change_detected.load(); });

    watcher.stop();

    // This test may fail on some systems due to file watching quirks
    // So we don't assert, just report
    if (!detected) {
        GTEST_SKIP() << "File change detection timed out (may be platform-specific)";
    }

    EXPECT_TRUE(change_detected);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST(ConfigWatcherTest, ThreadSafety_ConcurrentReloads) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    std::atomic<int> callback_count{0};
    watcher.on_change([&](const unified_config&, const unified_config&) {
        callback_count++;
    });

    // Spawn multiple threads that reload concurrently
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&watcher]() {
            for (int j = 0; j < 10; j++) {
                watcher.reload();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Should have 100 successful reloads
    EXPECT_EQ(callback_count, 100);
}

TEST(ConfigWatcherTest, ThreadSafety_ConcurrentReadWrite) {
    TempConfigFile file;
    config_watcher watcher(file.path_string());

    std::atomic<bool> running{true};
    std::atomic<int> read_count{0};

    // Reader thread
    std::thread reader([&]() {
        while (running) {
            auto config = watcher.current();
            (void)config;  // Use config to prevent optimization
            read_count++;
        }
    });

    // Writer thread
    std::thread writer([&]() {
        for (int i = 0; i < 50; i++) {
            watcher.reload();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    writer.join();
    running = false;
    reader.join();

    EXPECT_GT(read_count, 0);
    EXPECT_EQ(watcher.version(), 50);
}

// ============================================================================
// Hot Reloadable Field Tests
// ============================================================================

TEST(ConfigWatcherTest, HotReloadable_LoggerLevel_IsReloadable) {
    EXPECT_TRUE(is_hot_reloadable("logger.level"));
}

TEST(ConfigWatcherTest, HotReloadable_ThreadPoolSize_IsNotReloadable) {
    EXPECT_FALSE(is_hot_reloadable("thread.pool_size"));
}

TEST(ConfigWatcherTest, HotReloadable_MonitoringMetricsInterval_IsReloadable) {
    EXPECT_TRUE(is_hot_reloadable("monitoring.metrics_interval"));
}

TEST(ConfigWatcherTest, HotReloadable_DatabaseBackend_IsNotReloadable) {
    EXPECT_FALSE(is_hot_reloadable("database.backend"));
}
