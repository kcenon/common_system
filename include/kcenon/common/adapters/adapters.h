/*
 * BSD 3-Clause License
 * Copyright (c) 2025, kcenon
 */

#pragma once

/**
 * @file adapters.h
 * @brief Unified adapter utilities for type-safe and zero-cost adaptation
 *
 * This header provides a unified adapter system:
 * - adapter<T>: Generic wrapper for values and smart pointers
 * - adapter_traits<T>: Type traits for adapter configuration
 * - interface_adapter<I, Impl>: Interface adaptation with depth tracking
 * - adapter_factory: Factory for creating zero-cost adapters
 *
 * Usage:
 * @code
 * // Create value adapter
 * auto val = make_adapter<int>(42);
 *
 * // Create shared pointer adapter
 * auto shared = make_shared_adapter<MyClass>(args...);
 *
 * // Create interface adapter (zero-cost if possible)
 * auto executor = make_interface_adapter<IExecutor>(thread_pool);
 *
 * // Unwrap an adapter to get underlying implementation
 * auto pool = unwrap_adapter<thread_pool_type>(executor);
 *
 * // Check wrapper depth
 * if (executor->get_wrapper_depth() > 1) {
 *     // Multiple layers of wrapping detected
 * }
 * @endcode
 *
 * Migration Guide:
 * - typed_adapter<I, T> -> interface_adapter<I, T>
 * - make_smart_adapter<I>(impl) -> make_interface_adapter<I>(impl)
 * - smart_adapter_factory::make_adapter -> adapter_factory::create
 */

#include "adapter.h"
#include "typed_adapter.h"
#include "smart_adapter.h"
