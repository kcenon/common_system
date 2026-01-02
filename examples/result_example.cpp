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

/**
 * @file result_example.cpp
 * @brief Example demonstrating the Result pattern usage
 */

#include <kcenon/common/patterns/result.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace kcenon::common;

// Example function that can fail
Result<int> divide(int a, int b) {
    if (b == 0) {
        return make_error<int>(
            error_codes::INVALID_ARGUMENT,
            "Division by zero",
            "math_module"
        );
    }
    return ok(a / b);
}

// Example function that reads a file
Result<std::string> read_file(const std::string& path) {
    if (path.empty()) {
        return make_error<std::string>(
            error_codes::INVALID_ARGUMENT,
            "Path cannot be empty",
            "file_module"
        );
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        return make_error<std::string>(
            error_codes::NOT_FOUND,
            "File not found: " + path,
            "file_module"
        );
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return ok(content);
}

// Example of chaining operations
Result<int> parse_and_compute(const std::string& expr) {
    // Simple parser for "a/b" format
    auto pos = expr.find('/');
    if (pos == std::string::npos) {
        return make_error<int>(
            error_codes::INVALID_ARGUMENT,
            "Invalid expression format",
            "parser"
        );
    }

    try {
        int a = std::stoi(expr.substr(0, pos));
        int b = std::stoi(expr.substr(pos + 1));
        return divide(a, b);
    } catch (const std::exception& e) {
        return make_error<int>(
            error_codes::INVALID_ARGUMENT,
            std::string("Parse error: ") + e.what(),
            "parser"
        );
    }
}

int main() {
    std::cout << "=== Result Pattern Examples ===\n\n";

    // Example 1: Basic usage with member methods
    std::cout << "1. Basic division:\n";
    auto result1 = divide(10, 2);
    if (result1.is_ok()) {
        std::cout << "   10 / 2 = " << result1.value() << "\n";
    }

    auto result2 = divide(10, 0);
    if (result2.is_err()) {
        const auto& err = result2.error();
        std::cout << "   Error: " << err.message
                  << " (code: " << err.code << ")\n";
    }

    // Example 2: Using value_or / unwrap_or
    std::cout << "\n2. Using value_or:\n";
    auto value = divide(10, 0).unwrap_or(-1);
    std::cout << "   10 / 0 with default -1 = " << value << "\n";

    // Example 3: Pattern matching with if-else
    std::cout << "\n3. Pattern matching:\n";
    auto result3 = parse_and_compute("20/4");
    if (result3.is_ok()) {
        std::cout << "   Success: " << result3.unwrap() << "\n";
    } else {
        std::cout << "   Failed: " << result3.error().message << "\n";
    }

    // Example 4: Monadic operations using member methods
    std::cout << "\n4. Monadic operations:\n";
    auto result4 = divide(100, 5);
    auto doubled = result4.map([](int x) { return x * 2; });
    if (doubled.is_ok()) {
        std::cout << "   (100 / 5) * 2 = " << doubled.value() << "\n";
    }

    // Example 5: Chaining with and_then using member methods
    std::cout << "\n5. Chaining operations:\n";
    auto chain_result = divide(50, 5)
        .and_then([](int x) { return divide(x, 2); });
    if (chain_result.is_ok()) {
        std::cout << "   (50 / 5) / 2 = " << chain_result.value() << "\n";
    }

    // Example 6: Error recovery with or_else using member methods
    std::cout << "\n6. Error recovery:\n";
    auto recovered = divide(10, 0)
        .or_else([](const error_info&) { return ok(0); });
    std::cout << "   10 / 0 with recovery = " << recovered.value() << "\n";

    // Example 7: try_catch wrapper
    std::cout << "\n7. Exception wrapping:\n";
    auto wrapped = try_catch<int>([]{
        throw std::runtime_error("Something went wrong");
        return 42;
    }, "example_module");

    if (wrapped.is_err()) {
        const auto& err = wrapped.error();
        std::cout << "   Caught exception: " << err.message << "\n";
    }

    std::cout << "\n=== Examples completed ===\n";
    return 0;
}