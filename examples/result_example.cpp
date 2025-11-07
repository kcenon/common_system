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

    // Example 1: Basic usage
    std::cout << "1. Basic division:\n";
    auto result1 = divide(10, 2);
    if (is_ok(result1)) {
        std::cout << "   10 / 2 = " << get_value(result1) << "\n";
    }

    auto result2 = divide(10, 0);
    if (is_error(result2)) {
        auto& err = get_error(result2);
        std::cout << "   Error: " << err.message
                  << " (code: " << err.code << ")\n";
    }

    // Example 2: Using value_or
    std::cout << "\n2. Using value_or:\n";
    auto value = value_or(divide(10, 0), -1);
    std::cout << "   10 / 0 with default -1 = " << value << "\n";

    // Example 3: Pattern matching with if-else
    std::cout << "\n3. Pattern matching:\n";
    auto result3 = parse_and_compute("20/4");
    if (result3.is_ok()) {
        std::cout << "   Success: " << result3.unwrap() << "\n";
    } else {
        std::cout << "   Failed: " << result3.error().message << "\n";
    }

    // Example 4: Monadic operations
    std::cout << "\n4. Monadic operations:\n";
    auto result4 = divide(100, 5);
    auto doubled = map(result4, [](int x) { return x * 2; });
    if (is_ok(doubled)) {
        std::cout << "   (100 / 5) * 2 = " << get_value(doubled) << "\n";
    }

    // Example 5: Chaining with and_then
    std::cout << "\n5. Chaining operations:\n";
    auto chain_result = and_then(
        divide(50, 5),
        [](int x) { return divide(x, 2); }
    );
    if (is_ok(chain_result)) {
        std::cout << "   (50 / 5) / 2 = " << get_value(chain_result) << "\n";
    }

    // Example 6: Error recovery with or_else
    std::cout << "\n6. Error recovery:\n";
    auto recovered = or_else(
        divide(10, 0),
        [](const error_info&) { return ok(0); }
    );
    std::cout << "   10 / 0 with recovery = " << get_value(recovered) << "\n";

    // Example 7: try_catch wrapper
    std::cout << "\n7. Exception wrapping:\n";
    auto wrapped = try_catch<int>([]{
        throw std::runtime_error("Something went wrong");
        return 42;
    }, "example_module");

    if (is_error(wrapped)) {
        auto& err = get_error(wrapped);
        std::cout << "   Caught exception: " << err.message << "\n";
    }

    std::cout << "\n=== Examples completed ===\n";
    return 0;
}