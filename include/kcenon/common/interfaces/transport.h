// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file transport.h
 * @brief Umbrella header for transport interfaces.
 *
 * This header provides convenient access to all transport interfaces,
 * including HTTP and UDP client abstractions. These interfaces enable
 * dependency injection for network communication across the ecosystem.
 *
 * @see http_client_interface.h for HTTP client details
 * @see udp_client_interface.h for UDP client details
 */

#pragma once

#include "transport/http_client_interface.h"
#include "transport/udp_client_interface.h"
