// BSD 3-Clause License
// Copyright (c) 2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

//
// libFuzzer harness for the foundation error / Result<T> decoding path.
//
// This harness exercises the error-code decoding surface that every system in
// the kcenon ecosystem relies on for exception-free error handling:
//   - kcenon::common::error::get_error_message(int)   (switch-based lookup)
//   - kcenon::common::error::get_category_name(int)    (range bucketing)
//   - kcenon::common::common_error_category::message(int)
//   - kcenon::common::error_info construction
//   - kcenon::common::Result<T> error branch
//     (make_error<T> / is_err / error() / value_or)
//
// The fuzzer treats the first 4 bytes of the input as a (potentially
// adversarial) little-endian signed integer error code and the remaining bytes
// as an arbitrary message/module string, then drives the decoding helpers and
// the Result<T> error path. The goal is to surface undefined behaviour
// (signed-overflow-driven mis-bucketing, out-of-range lookups, allocation
// faults) under -fsanitize=fuzzer,address.
//
// Built only when BUILD_FUZZERS is enabled (Clang + libFuzzer). It is excluded
// from the default build and from CI test/coverage jobs.

#include <kcenon/common/error/error_category.h>
#include <kcenon/common/error/error_codes.h>
#include <kcenon/common/patterns/result.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

namespace {

// Side-effect sink that prevents the optimizer from discarding the decode
// calls; kept at namespace scope so the accumulation across calls is real.
volatile std::size_t g_sink = 0;

// Decode a 4-byte little-endian signed integer from the front of the buffer.
// Falls back to 0-extension when fewer than 4 bytes are available so that tiny
// inputs are still valid fuzz cases.
int decode_code(const uint8_t* data, std::size_t size) {
    std::int32_t code = 0;
    const std::size_t n = size < sizeof(code) ? size : sizeof(code);
    std::memcpy(&code, data, n);
    return static_cast<int>(code);
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, std::size_t size) {
    if (data == nullptr) {
        return 0;
    }

    using namespace kcenon::common;

    const int code = decode_code(data, size);

    // Remaining bytes become the human-supplied message and module strings.
    const std::size_t offset = size < sizeof(std::int32_t) ? size : sizeof(std::int32_t);
    const uint8_t* tail = data + offset;
    const std::size_t tail_size = size - offset;
    const std::size_t split = tail_size / 2;

    std::string message(reinterpret_cast<const char*>(tail), split);
    std::string module(reinterpret_cast<const char*>(tail + split), tail_size - split);

    // 1) switch-based message lookup + range-based category bucketing.
    //    Both must stay total over the entire int domain.
    const std::string_view msg = error::get_error_message(code);
    const std::string_view catname = error::get_category_name(code);
    g_sink ^= msg.size();
    g_sink ^= catname.size();

    // 2) Decentralized category message lookup (heap-allocating default branch).
    const std::string catmsg = common_error_category::instance().message(code);
    g_sink ^= catmsg.size();

    // 3) error_info construction with attacker-controlled strings.
    const error_info info(code, message, module);
    g_sink ^= info.message.size() ^ info.module.size();

    // 4) Result<T> error branch round-trip.
    const Result<int> failed = make_error<int>(code, message, module);
    if (failed.is_err()) {
        const error_info& err = failed.error();
        // Re-bucket the propagated code to exercise the full path.
        const std::string_view recat = error::get_category_name(err.code);
        g_sink ^= recat.size();
    }
    g_sink ^= static_cast<std::size_t>(failed.value_or(-1));

    // 5) Success branch with a value derived from the code, to keep both
    //    alternatives covered.
    const Result<int> succeeded = ok<int>(code);
    if (succeeded.is_ok()) {
        g_sink ^= static_cast<std::size_t>(succeeded.value());
    }

    return 0;
}
