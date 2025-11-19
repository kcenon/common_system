#pragma once

/**
 * @file forward.h
 * @brief Forward declarations for common_system types
 *
 * This header provides forward declarations for commonly used types
 * in the common_system module to reduce compilation dependencies.
 */

namespace kcenon::common {

// Core types
template<typename T> class Result;
template<typename T> class Option;

// Pattern classes
namespace patterns {
    class Observer;
    class Subject;
    template<typename T> class Singleton;
}

// Interface classes
namespace interfaces {
    class Serializable;
    class Comparable;
    class Hashable;
}

// Error handling
class error_info;
enum class error_code : int;

// Utility classes
namespace utils {
    class Logger;
    class Timer;
    class StringUtils;
}

} // namespace kcenon::common