// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file config_watcher.h
 * @brief Configuration hot-reload support with file system watching.
 *
 * This header provides the config_watcher class for monitoring configuration
 * file changes at runtime and automatically reloading configuration.
 *
 * Features:
 * - Cross-platform file system watching (inotify/kqueue/ReadDirectoryChanges)
 * - Change callback system with old/new configuration comparison
 * - Configuration version tracking
 * - Automatic rollback on validation failure
 * - Hot-reloadable vs. non-reloadable field distinction
 *
 * Platform Support:
 * - Linux: inotify
 * - macOS/BSD: kqueue
 * - Windows: ReadDirectoryChangesW
 *
 * @see TICKET-203 for implementation requirements.
 * @see unified_config.h for hot_reloadable field metadata.
 * @see config_loader.h for loading implementation.
 */

#pragma once

#include "config_loader.h"
#include "unified_config.h"

#include <kcenon/common/patterns/result.h>

#include <atomic>
#include <chrono>
#include <deque>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

// Platform-specific includes
#if defined(__linux__)
#include <sys/inotify.h>
#include <unistd.h>
#include <poll.h>
#elif defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/event.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#elif defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace kcenon::common::config {

/**
 * @brief Error codes specific to config_watcher.
 */
namespace watcher_error_codes {
    constexpr int watch_failed = 2001;
    constexpr int reload_failed = 2002;
    constexpr int validation_failed = 2003;
    constexpr int rollback_failed = 2004;
    constexpr int not_started = 2005;
    constexpr int already_running = 2006;
    constexpr int platform_not_supported = 2007;
}

/**
 * @brief Information about a configuration change event.
 */
struct config_change_event {
    /// Timestamp of the change
    std::chrono::system_clock::time_point timestamp;

    /// Configuration version (incrementing counter)
    uint64_t version;

    /// List of changed field paths
    std::vector<std::string> changed_fields;

    /// Whether the change was successful
    bool success;

    /// Error message if change failed
    std::string error_message;
};

/**
 * @brief Represents a configuration snapshot for version history.
 */
struct config_snapshot {
    /// Configuration version number
    uint64_t version;

    /// Timestamp when this configuration was active
    std::chrono::system_clock::time_point timestamp;

    /// The configuration data
    unified_config config;
};

/**
 * @class config_watcher
 * @brief Monitors configuration files for changes and supports hot-reload.
 *
 * The config_watcher provides automatic configuration reloading when the
 * configuration file is modified. It supports:
 * - Platform-native file watching
 * - Callback notifications for configuration changes
 * - Version tracking and history
 * - Automatic rollback on validation failures
 *
 * Usage Example:
 * @code
 * config_watcher watcher("config.yaml");
 *
 * watcher.on_change([](const unified_config& old_cfg,
 *                      const unified_config& new_cfg) {
 *     std::cout << "Configuration updated\n";
 *     // Apply changes...
 * });
 *
 * watcher.start();
 *
 * // ... application runs ...
 *
 * watcher.stop();
 * @endcode
 */
class config_watcher {
public:
    /// Callback type for configuration changes
    using change_callback = std::function<void(const unified_config& old_config,
                                               const unified_config& new_config)>;

    /// Callback type for reload errors
    using error_callback = std::function<void(const std::string& error_message)>;

    /**
     * @brief Construct a config_watcher for the specified file.
     * @param config_path Path to the YAML configuration file
     * @param max_history Maximum number of configuration snapshots to keep (default: 10)
     */
    explicit config_watcher(const std::string& config_path, size_t max_history = 10)
        : config_path_(config_path)
        , version_(0)
        , max_history_(max_history)
        , running_(false)
#if defined(__linux__)
        , inotify_fd_(-1)
        , watch_fd_(-1)
#elif defined(__APPLE__) || defined(__FreeBSD__)
        , kqueue_fd_(-1)
        , file_fd_(-1)
#elif defined(_WIN32)
        , dir_handle_(INVALID_HANDLE_VALUE)
#endif
    {
        // Load initial configuration
        auto result = config_loader::load(config_path_);
        if (result.is_ok()) {
            current_config_ = result.value();
            add_to_history(current_config_);
        } else {
            // Start with defaults if file doesn't exist or can't be loaded
            current_config_ = config_loader::defaults();
            add_to_history(current_config_);
        }
    }

    /**
     * @brief Destructor. Automatically stops watching if running.
     */
    ~config_watcher() {
        stop();
    }

    // Non-copyable, non-movable
    config_watcher(const config_watcher&) = delete;
    config_watcher& operator=(const config_watcher&) = delete;
    config_watcher(config_watcher&&) = delete;
    config_watcher& operator=(config_watcher&&) = delete;

    /**
     * @brief Start watching the configuration file for changes.
     * @return VoidResult indicating success or failure
     */
    VoidResult start() {
        if (running_.load()) {
            return make_error<std::monostate>(
                watcher_error_codes::already_running,
                "Config watcher is already running",
                "config_watcher"
            );
        }

        auto init_result = init_platform_watcher();
        if (init_result.is_err()) {
            return init_result;
        }

        running_.store(true);
        watch_thread_ = std::thread([this]() { watch_loop(); });

        return VoidResult::ok({});
    }

    /**
     * @brief Stop watching the configuration file.
     */
    void stop() {
        if (!running_.load()) {
            return;
        }

        running_.store(false);

        // Signal the watch thread to wake up
        cleanup_platform_watcher();

        if (watch_thread_.joinable()) {
            watch_thread_.join();
        }
    }

    /**
     * @brief Check if the watcher is currently running.
     * @return true if watching, false otherwise
     */
    bool is_running() const {
        return running_.load();
    }

    /**
     * @brief Register a callback for configuration changes.
     *
     * The callback will be invoked whenever the configuration is
     * successfully reloaded. Multiple callbacks can be registered.
     *
     * @param callback Function to call on configuration change
     */
    void on_change(change_callback callback) {
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        change_callbacks_.push_back(std::move(callback));
    }

    /**
     * @brief Register a callback for reload errors.
     *
     * The callback will be invoked when a reload attempt fails.
     *
     * @param callback Function to call on reload error
     */
    void on_error(error_callback callback) {
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        error_callbacks_.push_back(std::move(callback));
    }

    /**
     * @brief Manually trigger a configuration reload.
     * @return VoidResult indicating success or failure
     */
    VoidResult reload() {
        return do_reload();
    }

    /**
     * @brief Get the current configuration.
     * @return Reference to the current unified_config
     */
    const unified_config& current() const {
        std::shared_lock<std::shared_mutex> lock(config_mutex_);
        return current_config_;
    }

    /**
     * @brief Get the current configuration version.
     * @return Current version number
     */
    uint64_t version() const {
        return version_.load();
    }

    /**
     * @brief Get configuration history snapshots.
     * @param count Maximum number of snapshots to return (default: all)
     * @return Vector of configuration snapshots, newest first
     */
    std::vector<config_snapshot> history(size_t count = 0) const {
        std::lock_guard<std::mutex> lock(history_mutex_);

        if (count == 0 || count > history_.size()) {
            return {history_.rbegin(), history_.rend()};
        }

        std::vector<config_snapshot> result;
        result.reserve(count);
        auto it = history_.rbegin();
        for (size_t i = 0; i < count && it != history_.rend(); ++i, ++it) {
            result.push_back(*it);
        }
        return result;
    }

    /**
     * @brief Rollback to a previous configuration version.
     * @param target_version Version number to rollback to
     * @return VoidResult indicating success or failure
     */
    VoidResult rollback(uint64_t target_version) {
        std::lock_guard<std::mutex> lock(history_mutex_);

        for (const auto& snapshot : history_) {
            if (snapshot.version == target_version) {
                std::unique_lock<std::shared_mutex> config_lock(config_mutex_);
                unified_config old_config = current_config_;
                current_config_ = snapshot.config;
                version_.fetch_add(1);

                // Notify callbacks
                config_lock.unlock();
                notify_change(old_config, current_config_);

                return VoidResult::ok({});
            }
        }

        return make_error<std::monostate>(
            watcher_error_codes::rollback_failed,
            "Target version not found in history: " + std::to_string(target_version),
            "config_watcher"
        );
    }

    /**
     * @brief Get the path to the configuration file being watched.
     * @return Configuration file path
     */
    const std::string& config_path() const {
        return config_path_;
    }

    /**
     * @brief Get recent change events.
     * @param count Maximum number of events to return
     * @return Vector of recent change events
     */
    std::vector<config_change_event> recent_events(size_t count = 10) const {
        std::lock_guard<std::mutex> lock(events_mutex_);

        if (count == 0 || count > events_.size()) {
            return {events_.rbegin(), events_.rend()};
        }

        std::vector<config_change_event> result;
        result.reserve(count);
        auto it = events_.rbegin();
        for (size_t i = 0; i < count && it != events_.rend(); ++i, ++it) {
            result.push_back(*it);
        }
        return result;
    }

private:
    /**
     * @brief Initialize platform-specific file watching.
     */
    VoidResult init_platform_watcher() {
#if defined(__linux__)
        return init_inotify();
#elif defined(__APPLE__) || defined(__FreeBSD__)
        return init_kqueue();
#elif defined(_WIN32)
        return init_win32_watcher();
#else
        return make_error<std::monostate>(
            watcher_error_codes::platform_not_supported,
            "File watching not supported on this platform",
            "config_watcher"
        );
#endif
    }

    /**
     * @brief Cleanup platform-specific resources.
     */
    void cleanup_platform_watcher() {
#if defined(__linux__)
        cleanup_inotify();
#elif defined(__APPLE__) || defined(__FreeBSD__)
        cleanup_kqueue();
#elif defined(_WIN32)
        cleanup_win32_watcher();
#endif
    }

#if defined(__linux__)
    /**
     * @brief Initialize inotify for Linux.
     */
    VoidResult init_inotify() {
        inotify_fd_ = inotify_init1(IN_NONBLOCK);
        if (inotify_fd_ < 0) {
            return make_error<std::monostate>(
                watcher_error_codes::watch_failed,
                "Failed to initialize inotify: " + std::string(strerror(errno)),
                "config_watcher"
            );
        }

        // Watch the parent directory (to handle file recreation)
        std::filesystem::path path(config_path_);
        std::string dir_path = path.parent_path().string();
        if (dir_path.empty()) {
            dir_path = ".";
        }

        watch_fd_ = inotify_add_watch(inotify_fd_, dir_path.c_str(),
                                      IN_MODIFY | IN_CREATE | IN_MOVED_TO | IN_CLOSE_WRITE);
        if (watch_fd_ < 0) {
            close(inotify_fd_);
            inotify_fd_ = -1;
            return make_error<std::monostate>(
                watcher_error_codes::watch_failed,
                "Failed to add inotify watch: " + std::string(strerror(errno)),
                "config_watcher"
            );
        }

        return VoidResult::ok({});
    }

    void cleanup_inotify() {
        if (watch_fd_ >= 0) {
            inotify_rm_watch(inotify_fd_, watch_fd_);
            watch_fd_ = -1;
        }
        if (inotify_fd_ >= 0) {
            close(inotify_fd_);
            inotify_fd_ = -1;
        }
    }

    void watch_loop_linux() {
        std::filesystem::path path(config_path_);
        std::string filename = path.filename().string();

        constexpr size_t EVENT_BUF_LEN = 4096;
        alignas(struct inotify_event) char buffer[EVENT_BUF_LEN];

        while (running_.load()) {
            struct pollfd pfd = {inotify_fd_, POLLIN, 0};
            int ret = poll(&pfd, 1, 500);  // 500ms timeout

            if (ret < 0) {
                if (errno == EINTR) continue;
                break;
            }

            if (ret == 0) continue;  // Timeout

            ssize_t len = read(inotify_fd_, buffer, EVENT_BUF_LEN);
            if (len < 0) {
                if (errno == EAGAIN || errno == EINTR) continue;
                break;
            }

            bool should_reload = false;
            for (char* ptr = buffer; ptr < buffer + len; ) {
                auto* event = reinterpret_cast<struct inotify_event*>(ptr);

                if (event->len > 0) {
                    std::string event_name(event->name);
                    if (event_name == filename) {
                        if (event->mask & (IN_MODIFY | IN_CREATE | IN_MOVED_TO | IN_CLOSE_WRITE)) {
                            should_reload = true;
                        }
                    }
                }

                ptr += sizeof(struct inotify_event) + event->len;
            }

            if (should_reload) {
                // Small delay to ensure file write is complete
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                do_reload();
            }
        }
    }
#endif

#if defined(__APPLE__) || defined(__FreeBSD__)
    /**
     * @brief Initialize kqueue for macOS/BSD.
     */
    VoidResult init_kqueue() {
        kqueue_fd_ = kqueue();
        if (kqueue_fd_ < 0) {
            return make_error<std::monostate>(
                watcher_error_codes::watch_failed,
                "Failed to create kqueue: " + std::string(strerror(errno)),
                "config_watcher"
            );
        }

        file_fd_ = open(config_path_.c_str(), O_RDONLY);
        if (file_fd_ < 0) {
            // If file doesn't exist, watch the directory instead
            std::filesystem::path path(config_path_);
            std::string dir_path = path.parent_path().string();
            if (dir_path.empty()) {
                dir_path = ".";
            }

            file_fd_ = open(dir_path.c_str(), O_RDONLY);
            if (file_fd_ < 0) {
                close(kqueue_fd_);
                kqueue_fd_ = -1;
                return make_error<std::monostate>(
                    watcher_error_codes::watch_failed,
                    "Failed to open file/directory for watching: " + std::string(strerror(errno)),
                    "config_watcher"
                );
            }
            watching_directory_ = true;
        }

        struct kevent change;
        EV_SET(&change, file_fd_, EVFILT_VNODE,
               EV_ADD | EV_ENABLE | EV_CLEAR,
               NOTE_WRITE | NOTE_EXTEND | NOTE_RENAME | NOTE_DELETE | NOTE_ATTRIB,
               0, nullptr);

        if (kevent(kqueue_fd_, &change, 1, nullptr, 0, nullptr) < 0) {
            close(file_fd_);
            close(kqueue_fd_);
            file_fd_ = -1;
            kqueue_fd_ = -1;
            return make_error<std::monostate>(
                watcher_error_codes::watch_failed,
                "Failed to register kevent: " + std::string(strerror(errno)),
                "config_watcher"
            );
        }

        return VoidResult::ok({});
    }

    void cleanup_kqueue() {
        // Only close kqueue to signal the watch thread to exit
        // The watch thread will handle file_fd_ cleanup
        int kq = kqueue_fd_.exchange(-1);
        if (kq >= 0) {
            close(kq);
        }
    }

    void watch_loop_kqueue() {
        struct kevent event;
        struct timespec timeout = {0, 500000000};  // 500ms

        while (running_.load()) {
            int kq = kqueue_fd_.load();
            if (kq < 0) break;

            int n = kevent(kq, nullptr, 0, &event, 1, &timeout);

            if (n < 0) {
                if (errno == EINTR) continue;
                break;
            }

            if (n == 0) continue;  // Timeout

            if (event.fflags & (NOTE_WRITE | NOTE_EXTEND | NOTE_RENAME | NOTE_ATTRIB)) {
                // Small delay to ensure file write is complete
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                // If file was deleted/renamed and we need to re-watch
                if (event.fflags & (NOTE_DELETE | NOTE_RENAME)) {
                    // Try to reopen the file
                    std::lock_guard<std::mutex> lock(file_fd_mutex_);
                    if (file_fd_ >= 0) {
                        close(file_fd_);
                    }
                    file_fd_ = open(config_path_.c_str(), O_RDONLY);
                    if (file_fd_ >= 0) {
                        int kq_local = kqueue_fd_.load();
                        if (kq_local >= 0) {
                            struct kevent change;
                            EV_SET(&change, file_fd_, EVFILT_VNODE,
                                   EV_ADD | EV_ENABLE | EV_CLEAR,
                                   NOTE_WRITE | NOTE_EXTEND | NOTE_RENAME | NOTE_DELETE | NOTE_ATTRIB,
                                   0, nullptr);
                            kevent(kq_local, &change, 1, nullptr, 0, nullptr);
                        }
                    }
                }

                do_reload();
            }
        }

        // Cleanup file_fd_ when exiting the loop
        std::lock_guard<std::mutex> lock(file_fd_mutex_);
        if (file_fd_ >= 0) {
            close(file_fd_);
            file_fd_ = -1;
        }
    }

    bool watching_directory_ = false;
#endif

#if defined(_WIN32)
    /**
     * @brief Initialize ReadDirectoryChangesW for Windows.
     */
    VoidResult init_win32_watcher() {
        std::filesystem::path path(config_path_);
        std::wstring dir_path = path.parent_path().wstring();
        if (dir_path.empty()) {
            dir_path = L".";
        }

        dir_handle_ = CreateFileW(
            dir_path.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            nullptr
        );

        if (dir_handle_ == INVALID_HANDLE_VALUE) {
            return make_error<std::monostate>(
                watcher_error_codes::watch_failed,
                "Failed to open directory for watching",
                "config_watcher"
            );
        }

        return VoidResult::ok({});
    }

    void cleanup_win32_watcher() {
        if (dir_handle_ != INVALID_HANDLE_VALUE) {
            CancelIo(dir_handle_);
            CloseHandle(dir_handle_);
            dir_handle_ = INVALID_HANDLE_VALUE;
        }
    }

    void watch_loop_win32() {
        std::filesystem::path path(config_path_);
        std::wstring filename = path.filename().wstring();

        constexpr DWORD BUFFER_SIZE = 4096;
        alignas(DWORD) char buffer[BUFFER_SIZE];

        OVERLAPPED overlapped = {};
        overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

        while (running_.load()) {
            DWORD bytes_returned = 0;
            BOOL success = ReadDirectoryChangesW(
                dir_handle_,
                buffer,
                BUFFER_SIZE,
                FALSE,
                FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE,
                nullptr,
                &overlapped,
                nullptr
            );

            if (!success) {
                break;
            }

            DWORD wait_result = WaitForSingleObject(overlapped.hEvent, 500);

            if (wait_result == WAIT_TIMEOUT) {
                CancelIo(dir_handle_);
                continue;
            }

            if (wait_result != WAIT_OBJECT_0) {
                break;
            }

            if (!GetOverlappedResult(dir_handle_, &overlapped, &bytes_returned, FALSE)) {
                break;
            }

            ResetEvent(overlapped.hEvent);

            bool should_reload = false;
            auto* notification = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);

            while (true) {
                std::wstring changed_name(notification->FileName,
                                         notification->FileNameLength / sizeof(WCHAR));

                if (changed_name == filename) {
                    should_reload = true;
                }

                if (notification->NextEntryOffset == 0) break;
                notification = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                    reinterpret_cast<char*>(notification) + notification->NextEntryOffset);
            }

            if (should_reload) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                do_reload();
            }
        }

        CloseHandle(overlapped.hEvent);
    }
#endif

    /**
     * @brief Main watch loop - dispatches to platform-specific implementation.
     */
    void watch_loop() {
#if defined(__linux__)
        watch_loop_linux();
#elif defined(__APPLE__) || defined(__FreeBSD__)
        watch_loop_kqueue();
#elif defined(_WIN32)
        watch_loop_win32();
#endif
    }

    /**
     * @brief Perform the actual configuration reload.
     */
    VoidResult do_reload() {
        auto result = config_loader::load(config_path_);

        config_change_event event;
        event.timestamp = std::chrono::system_clock::now();
        event.version = version_.load() + 1;

        if (result.is_err()) {
            event.success = false;
            event.error_message = result.error().message;
            add_event(event);
            notify_error(result.error().message);
            return make_error<std::monostate>(result.error());
        }

        unified_config new_config = result.value();

        // Validate the new configuration
        auto validation_result = config_loader::validate(new_config);
        if (validation_result.is_err()) {
            event.success = false;
            event.error_message = validation_result.error().message;
            add_event(event);
            notify_error("Validation failed: " + validation_result.error().message);
            return make_error<std::monostate>(
                watcher_error_codes::validation_failed,
                "Configuration validation failed: " + validation_result.error().message,
                "config_watcher"
            );
        }

        // Get changed fields
        std::unique_lock<std::shared_mutex> lock(config_mutex_);
        event.changed_fields = get_changed_fields(current_config_, new_config);

        // Check if any non-hot-reloadable fields changed
        std::vector<std::string> non_reloadable_changes;
        for (const auto& field : event.changed_fields) {
            if (!is_hot_reloadable(field)) {
                non_reloadable_changes.push_back(field);
            }
        }

        // Update configuration
        unified_config old_config = current_config_;
        current_config_ = new_config;
        version_.fetch_add(1);
        event.version = version_.load();
        event.success = true;

        // Add to history
        add_to_history(new_config);

        lock.unlock();

        // Record event
        add_event(event);

        // Notify callbacks
        notify_change(old_config, new_config);

        return VoidResult::ok({});
    }

    /**
     * @brief Compare two configurations and return changed field paths.
     */
    static std::vector<std::string> get_changed_fields(
        const unified_config& old_cfg,
        const unified_config& new_cfg
    ) {
        std::vector<std::string> changes;

        // Thread config
        if (old_cfg.thread.pool_size != new_cfg.thread.pool_size) {
            changes.push_back("thread.pool_size");
        }
        if (old_cfg.thread.queue_type != new_cfg.thread.queue_type) {
            changes.push_back("thread.queue_type");
        }
        if (old_cfg.thread.max_queue_size != new_cfg.thread.max_queue_size) {
            changes.push_back("thread.max_queue_size");
        }

        // Logger config
        if (old_cfg.logger.level != new_cfg.logger.level) {
            changes.push_back("logger.level");
        }
        if (old_cfg.logger.async != new_cfg.logger.async) {
            changes.push_back("logger.async");
        }
        if (old_cfg.logger.buffer_size != new_cfg.logger.buffer_size) {
            changes.push_back("logger.buffer_size");
        }
        if (old_cfg.logger.file_path != new_cfg.logger.file_path) {
            changes.push_back("logger.file_path");
        }
        if (old_cfg.logger.writers != new_cfg.logger.writers) {
            changes.push_back("logger.writers");
        }

        // Monitoring config
        if (old_cfg.monitoring.enabled != new_cfg.monitoring.enabled) {
            changes.push_back("monitoring.enabled");
        }
        if (old_cfg.monitoring.metrics_interval != new_cfg.monitoring.metrics_interval) {
            changes.push_back("monitoring.metrics_interval");
        }
        if (old_cfg.monitoring.tracing.enabled != new_cfg.monitoring.tracing.enabled) {
            changes.push_back("monitoring.tracing.enabled");
        }
        if (old_cfg.monitoring.tracing.sampling_rate != new_cfg.monitoring.tracing.sampling_rate) {
            changes.push_back("monitoring.tracing.sampling_rate");
        }

        // Database config
        if (old_cfg.database.backend != new_cfg.database.backend) {
            changes.push_back("database.backend");
        }
        if (old_cfg.database.connection_string != new_cfg.database.connection_string) {
            changes.push_back("database.connection_string");
        }

        // Network config
        if (old_cfg.network.tls.enabled != new_cfg.network.tls.enabled) {
            changes.push_back("network.tls.enabled");
        }
        if (old_cfg.network.compression != new_cfg.network.compression) {
            changes.push_back("network.compression");
        }
        if (old_cfg.network.buffer_size != new_cfg.network.buffer_size) {
            changes.push_back("network.buffer_size");
        }

        return changes;
    }

    /**
     * @brief Add a configuration to history.
     */
    void add_to_history(const unified_config& config) {
        std::lock_guard<std::mutex> lock(history_mutex_);

        config_snapshot snapshot;
        snapshot.version = version_.load();
        snapshot.timestamp = std::chrono::system_clock::now();
        snapshot.config = config;

        history_.push_back(snapshot);

        // Trim history if needed
        while (history_.size() > max_history_) {
            history_.pop_front();
        }
    }

    /**
     * @brief Add a change event to the event log.
     */
    void add_event(const config_change_event& event) {
        std::lock_guard<std::mutex> lock(events_mutex_);
        events_.push_back(event);

        // Keep only recent events
        while (events_.size() > 100) {
            events_.pop_front();
        }
    }

    /**
     * @brief Notify all registered change callbacks.
     */
    void notify_change(const unified_config& old_cfg, const unified_config& new_cfg) {
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        for (const auto& callback : change_callbacks_) {
            try {
                callback(old_cfg, new_cfg);
            } catch (...) {
                // Ignore callback exceptions
            }
        }
    }

    /**
     * @brief Notify all registered error callbacks.
     */
    void notify_error(const std::string& message) {
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        for (const auto& callback : error_callbacks_) {
            try {
                callback(message);
            } catch (...) {
                // Ignore callback exceptions
            }
        }
    }

    // Configuration state
    std::string config_path_;
    unified_config current_config_;
    mutable std::shared_mutex config_mutex_;

    // Version tracking
    std::atomic<uint64_t> version_;

    // History
    size_t max_history_;
    mutable std::mutex history_mutex_;
    std::deque<config_snapshot> history_;

    // Events
    mutable std::mutex events_mutex_;
    std::deque<config_change_event> events_;

    // Callbacks
    std::mutex callbacks_mutex_;
    std::vector<change_callback> change_callbacks_;
    std::vector<error_callback> error_callbacks_;

    // Watch thread
    std::atomic<bool> running_;
    std::thread watch_thread_;

    // Platform-specific handles
#if defined(__linux__)
    int inotify_fd_;
    int watch_fd_;
#elif defined(__APPLE__) || defined(__FreeBSD__)
    std::atomic<int> kqueue_fd_;
    int file_fd_;
    std::mutex file_fd_mutex_;
#elif defined(_WIN32)
    HANDLE dir_handle_;
#endif
};

}  // namespace kcenon::common::config
