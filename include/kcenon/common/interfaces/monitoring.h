// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file monitoring.h
 * @brief Umbrella header for monitoring interfaces.
 *
 * This header provides convenient access to all monitoring interfaces,
 * including the IMetricCollector for real-time metric emission.
 *
 * The monitoring interfaces work together:
 * - monitoring_interface.h: IMonitor for reading metrics and health status
 * - metric_collector_interface.h: IMetricCollector for emitting metrics
 *
 * @see monitoring_interface.h for IMonitor details
 * @see monitoring/metric_collector_interface.h for IMetricCollector details
 */

#pragma once

#include "monitoring/metric_collector_interface.h"
#include "monitoring/health_check.h"
#include "monitoring/composite_health_check.h"
#include "monitoring/health_dependency_graph.h"
#include "monitoring/health_check_builder.h"
#include "monitoring/health_monitor.h"
