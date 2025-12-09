// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file concepts.h
 * @brief Unified header for all C++20 concepts in common_system.
 *
 * This header provides a single include point for all concepts defined
 * in the common_system library. Including this header gives access to
 * all concept definitions for compile-time type validation.
 *
 * Available concept categories:
 * - Core concepts: Result/Optional type constraints (core.h)
 * - Callable concepts: Function and executor constraints (callable.h)
 * - Event concepts: Event bus type constraints (event.h)
 * - Service concepts: DI container constraints (service.h)
 * - Container concepts: Collection type constraints (container.h)
 *
 * Requirements:
 * - C++20 compiler with concepts support
 * - GCC 10+, Clang 10+, MSVC 2022+
 *
 * Example usage:
 * @code
 * #include <kcenon/common/concepts/concepts.h>
 *
 * using namespace kcenon::common::concepts;
 *
 * template<Resultable R>
 * void process(const R& result) {
 *     if (result.is_ok()) {
 *         // Handle success
 *     }
 * }
 *
 * template<EventHandler<MyEvent> H>
 * uint64_t subscribe(H&& handler) {
 *     return bus.subscribe<MyEvent>(std::forward<H>(handler));
 * }
 * @endcode
 *
 * @see core.h for Result/Optional concepts
 * @see callable.h for callable and executor concepts
 * @see event.h for event bus concepts
 * @see service.h for dependency injection concepts
 * @see container.h for container type concepts
 *
 * Benefits of using concepts:
 * - **Clearer error messages**: Template errors are displayed as concept
 *   violations instead of hundreds of lines of SFINAE failures
 * - **Self-documenting code**: Concepts express type requirements explicitly
 * - **Better IDE support**: More accurate auto-completion and type hints
 * - **Code simplification**: Eliminates std::enable_if boilerplate
 *
 * Migration from SFINAE:
 * @code
 * // Before (SFINAE)
 * template<typename F,
 *          typename = std::enable_if_t<
 *              std::is_invocable_v<F> &&
 *              std::is_void_v<std::invoke_result_t<F>>>>
 * void execute_async(F&& func);
 *
 * // After (Concepts)
 * template<VoidCallable F>
 * void execute_async(F&& func);
 * @endcode
 */

#pragma once

// Core concepts for Result/Optional types
#include "core.h"

// Callable and executor concepts
#include "callable.h"

// Event bus concepts
#include "event.h"

// Dependency injection concepts
#include "service.h"

// Container and collection concepts
#include "container.h"

/**
 * @namespace kcenon::common::concepts
 * @brief C++20 concepts for compile-time type validation.
 *
 * This namespace contains all concept definitions used throughout
 * the common_system library. Concepts provide:
 *
 * - Compile-time type checking with clear error messages
 * - Self-documenting interface requirements
 * - Simplified template constraints (replacing SFINAE)
 * - Better IDE support for auto-completion
 *
 * Concept Categories:
 *
 * **Result/Optional (core.h)**:
 * - Resultable: Types with is_ok()/is_err() methods
 * - Unwrappable: Types supporting value extraction
 * - Mappable: Types supporting map() transformation
 * - Chainable: Types supporting and_then() chaining
 * - MonadicResult: Complete monadic Result type
 * - OptionalLike: Optional value containers
 *
 * **Callable (callable.h)**:
 * - Invocable: Basic callable types
 * - VoidCallable: Callables returning void
 * - Predicate: Boolean-returning callables
 * - JobLike: Types satisfying IJob interface
 * - ExecutorLike: Types satisfying IExecutor interface
 *
 * **Event (event.h)**:
 * - EventType: Valid event types
 * - EventHandler: Event handler callables
 * - EventFilter: Event filter predicates
 * - TimestampedEvent: Events with timestamps
 * - ErrorEvent: Error event types
 * - MetricEvent: Metric event types
 *
 * **Service (service.h)**:
 * - ServiceInterface: Valid service interfaces
 * - ServiceImplementation: Valid implementations
 * - ServiceFactory: Service factory callables
 * - ServiceContainerLike: Container interfaces
 * - Validatable: Self-validating types
 *
 * **Container (container.h)**:
 * - Container: Basic container requirements
 * - SequenceContainer: Sequential containers
 * - AssociativeContainer: Key-based containers
 * - ResizableContainer: Resizable containers
 * - CircularBuffer: Circular buffer types
 */

namespace kcenon::common::concepts {
// All concepts are defined in their respective headers
// This namespace block serves as documentation
} // namespace kcenon::common::concepts
