// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file unwrap_demo.cpp
 * @brief Demonstration of improved unwrap() with source location
 */

#include "kcenon/common/patterns/result.h"
#include <iostream>
#include <string>

using namespace common;

// Helper function that returns an error
Result<int> divide(int numerator, int denominator) {
    if (denominator == 0) {
        return make_error<int>(
            -1,
            "Division by zero",
            "math_module",
            "Cannot divide by zero"
        );
    }
    return ok(numerator / denominator);
}

// Function that returns None
Optional<std::string> find_user(int id) {
    if (id == 42) {
        return Optional<std::string>("Alice");
    }
    return Optional<std::string>(std::nullopt);
}

void demonstrate_improved_error_messages() {
    std::cout << "=== Improved unwrap() Error Messages Demo ===\n\n";

    // Demo 1: Result<T> unwrap with detailed error info
    std::cout << "1. Result<T>::unwrap() with error:\n";
    std::cout << "-----------------------------------\n";
    try {
        auto result = divide(10, 0);
        // This will throw with detailed source location
        int value = result.unwrap();  // Line will be captured automatically
        std::cout << "Value: " << value << "\n";
    } catch (const std::exception& e) {
        std::cout << "Exception caught:\n" << e.what() << "\n\n";
    }

    // Demo 2: Optional<T> unwrap with source location
    std::cout << "2. Optional<T>::unwrap() with None:\n";
    std::cout << "------------------------------------\n";
    try {
        auto user = find_user(999);
        // This will throw with source location
        std::string name = user.unwrap();  // Line will be captured automatically
        std::cout << "User: " << name << "\n";
    } catch (const std::exception& e) {
        std::cout << "Exception caught:\n" << e.what() << "\n\n";
    }

    // Demo 3: Safe alternatives (recommended)
    std::cout << "3. Safe alternatives to unwrap():\n";
    std::cout << "----------------------------------\n";

    auto result = divide(10, 0);

    // Option A: unwrap_or with default value
    int value1 = result.unwrap_or(0);
    std::cout << "unwrap_or(0): " << value1 << "\n";

    // Option B: Check before accessing
    if (result.is_ok()) {
        std::cout << "Value: " << result.value() << "\n";
    } else {
        auto error = result.error();
        std::cout << "Error: " << error.message << "\n";
        std::cout << "  Code: " << error.code << "\n";
        std::cout << "  Module: " << error.module << "\n";
    }

    // Option C: get_if_ok pattern
    if (const int* value_ptr = get_if_ok(result)) {
        std::cout << "Value via get_if_ok: " << *value_ptr << "\n";
    } else {
        std::cout << "get_if_ok returned nullptr (error state)\n";
    }

    // Option D: Monadic operations
    auto doubled = divide(10, 2)
        .map([](int x) { return x * 2; })
        .unwrap_or(-1);
    std::cout << "Monadic map result: " << doubled << "\n";

    std::cout << "\n=== Demo Complete ===\n";
}

int main() {
    demonstrate_improved_error_messages();
    return 0;
}
