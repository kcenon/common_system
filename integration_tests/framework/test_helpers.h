// BSD 3-Clause License
//
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <thread>

namespace integration_tests {
namespace helpers {

/**
 * @brief Read all lines from a file
 * @param filepath Path to the file
 * @return Vector of lines
 */
inline std::vector<std::string> read_file_lines(const std::string& filepath) {
    std::vector<std::string> lines;
    std::ifstream file(filepath);
    std::string line;

    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    return lines;
}

/**
 * @brief Count occurrences of a substring in a file
 * @param filepath Path to the file
 * @param pattern Pattern to search for
 * @return Number of occurrences
 */
inline size_t count_pattern_in_file(const std::string& filepath, const std::string& pattern) {
    auto lines = read_file_lines(filepath);
    size_t count = 0;

    for (const auto& line : lines) {
        size_t pos = 0;
        while ((pos = line.find(pattern, pos)) != std::string::npos) {
            ++count;
            pos += pattern.length();
        }
    }

    return count;
}

/**
 * @brief Create a temporary test directory
 * @param prefix Directory name prefix
 * @return Path to the created directory
 */
inline std::filesystem::path create_temp_directory(const std::string& prefix = "test_") {
    auto temp_dir = std::filesystem::temp_directory_path();
    auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    auto test_dir = temp_dir / (prefix + std::to_string(timestamp));

    std::filesystem::create_directories(test_dir);
    return test_dir;
}

/**
 * @brief Clean up a directory and all its contents
 * @param path Path to the directory
 */
inline void cleanup_directory(const std::filesystem::path& path) {
    if (std::filesystem::exists(path)) {
        std::filesystem::remove_all(path);
    }
}

/**
 * @brief Wait for a condition to become true with timeout
 * @tparam Predicate Callable that returns bool
 * @param condition Condition to wait for
 * @param timeout Maximum time to wait
 * @param check_interval How often to check the condition
 * @return true if condition was met, false if timeout
 */
template<typename Predicate>
bool wait_for_condition(Predicate&& condition,
                       std::chrono::milliseconds timeout = std::chrono::seconds(5),
                       std::chrono::milliseconds check_interval = std::chrono::milliseconds(10)) {
    auto start = std::chrono::steady_clock::now();

    while (!condition()) {
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= timeout) {
            return false;
        }
        std::this_thread::sleep_for(check_interval);
    }

    return true;
}

/**
 * @brief RAII helper for automatic cleanup of resources
 * @tparam CleanupFunc Callable type for cleanup
 */
template<typename CleanupFunc>
class ScopedCleanup {
public:
    explicit ScopedCleanup(CleanupFunc&& cleanup)
        : cleanup_(std::forward<CleanupFunc>(cleanup)) {}

    ~ScopedCleanup() {
        if (!dismissed_) {
            cleanup_();
        }
    }

    void dismiss() { dismissed_ = true; }

    ScopedCleanup(const ScopedCleanup&) = delete;
    ScopedCleanup& operator=(const ScopedCleanup&) = delete;

private:
    CleanupFunc cleanup_;
    bool dismissed_ = false;
};

/**
 * @brief Create a scoped cleanup helper
 * @tparam CleanupFunc Callable type
 * @param cleanup Cleanup function
 * @return ScopedCleanup instance
 */
template<typename CleanupFunc>
auto make_scoped_cleanup(CleanupFunc&& cleanup) {
    return ScopedCleanup<CleanupFunc>(std::forward<CleanupFunc>(cleanup));
}

} // namespace helpers
} // namespace integration_tests
