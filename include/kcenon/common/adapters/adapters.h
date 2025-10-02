/*
 * BSD 3-Clause License
 * Copyright (c) 2025, kcenon
 */

#pragma once

/**
 * @file adapters.h
 * @brief Common adapter utilities for type-safe and zero-cost adaptation
 *
 * This header provides:
 * - typed_adapter: Base class with wrapper depth tracking and type safety
 * - smart_adapter: Factory for creating zero-cost adapters when possible
 *
 * Usage:
 * @code
 * // Create a smart adapter (zero-cost if possible)
 * auto executor = make_smart_adapter<IExecutor>(thread_pool);
 *
 * // Unwrap an adapter to get underlying implementation
 * auto pool = unwrap_adapter<thread_pool_type>(executor);
 *
 * // Check wrapper depth
 * if (executor->get_wrapper_depth() > 1) {
 *     // Multiple layers of wrapping detected
 * }
 * @endcode
 */

#include "typed_adapter.h"
#include "smart_adapter.h"
