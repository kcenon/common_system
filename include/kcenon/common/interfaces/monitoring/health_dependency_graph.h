// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file health_dependency_graph.h
 * @brief DAG-based health check dependency management.
 *
 * This header defines the health_dependency_graph class which manages
 * dependencies between health checks using a directed acyclic graph.
 */

#pragma once

#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "health_check.h"

namespace kcenon::common::interfaces {

/**
 * @class health_dependency_graph
 * @brief Manages dependencies between health checks as a DAG
 *
 * This class allows defining dependencies between health checks and
 * executing them in the correct order. It supports cycle detection
 * and topological sorting for proper execution order.
 *
 * Example usage:
 * @code
 * health_dependency_graph graph;
 * graph.add_node("database", db_check);
 * graph.add_node("cache", cache_check);
 * graph.add_node("api", api_check);
 *
 * // api depends on database and cache
 * graph.add_dependency("api", "database");
 * graph.add_dependency("api", "cache");
 *
 * // Execute in topological order
 * auto order = graph.topological_sort();
 * for (const auto& name : order.value()) {
 *     auto result = graph.check_with_dependencies(name);
 * }
 * @endcode
 */
class health_dependency_graph {
public:
    health_dependency_graph() = default;
    ~health_dependency_graph() = default;

    health_dependency_graph(const health_dependency_graph&) = delete;
    health_dependency_graph& operator=(const health_dependency_graph&) = delete;
    health_dependency_graph(health_dependency_graph&&) = delete;
    health_dependency_graph& operator=(health_dependency_graph&&) = delete;

    /**
     * @brief Add a health check node to the graph
     * @param name Unique name for this node
     * @param check Health check implementation
     * @return Result indicating success or failure
     */
    Result<bool> add_node(const std::string& name, std::shared_ptr<health_check> check) {
        if (name.empty()) {
            return {error_info{1, "Node name cannot be empty", "health_dependency_graph"}};
        }
        if (!check) {
            return {error_info{2, "Health check cannot be null", "health_dependency_graph"}};
        }

        std::lock_guard<std::mutex> lock(mutex_);

        if (nodes_.find(name) != nodes_.end()) {
            return {error_info{3, "Node already exists: " + name, "health_dependency_graph"}};
        }

        nodes_[name] = std::move(check);
        dependencies_[name] = {};
        dependents_[name] = {};

        return ok(true);
    }

    /**
     * @brief Remove a health check node from the graph
     * @param name Name of the node to remove
     * @return Result indicating success or failure
     */
    Result<bool> remove_node(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (nodes_.find(name) == nodes_.end()) {
            return {error_info{1, "Node not found: " + name, "health_dependency_graph"}};
        }

        // Remove all dependencies to/from this node
        for (const auto& dep : dependencies_[name]) {
            dependents_[dep].erase(name);
        }
        for (const auto& dep : dependents_[name]) {
            dependencies_[dep].erase(name);
        }

        nodes_.erase(name);
        dependencies_.erase(name);
        dependents_.erase(name);

        return ok(true);
    }

    /**
     * @brief Add a dependency between two nodes
     * @param dependent The node that depends on another
     * @param dependency The node being depended upon
     * @return Result indicating success or failure
     */
    Result<bool> add_dependency(const std::string& dependent, const std::string& dependency) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (nodes_.find(dependent) == nodes_.end()) {
            return {error_info{1, "Dependent node not found: " + dependent,
                               "health_dependency_graph"}};
        }
        if (nodes_.find(dependency) == nodes_.end()) {
            return {error_info{2, "Dependency node not found: " + dependency,
                               "health_dependency_graph"}};
        }

        if (would_create_cycle_internal(dependent, dependency)) {
            return {error_info{3,
                               "Adding dependency would create a cycle: " + dependent + " -> " +
                                   dependency,
                               "health_dependency_graph"}};
        }

        dependencies_[dependent].insert(dependency);
        dependents_[dependency].insert(dependent);

        return ok(true);
    }

    /**
     * @brief Remove a dependency between two nodes
     * @param dependent The dependent node
     * @param dependency The dependency node
     * @return Result indicating success or failure
     */
    Result<bool> remove_dependency(const std::string& dependent, const std::string& dependency) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (nodes_.find(dependent) == nodes_.end()) {
            return {error_info{1, "Dependent node not found: " + dependent,
                               "health_dependency_graph"}};
        }

        dependencies_[dependent].erase(dependency);
        if (nodes_.find(dependency) != nodes_.end()) {
            dependents_[dependency].erase(dependent);
        }

        return ok(true);
    }

    /**
     * @brief Get all dependencies of a node
     * @param name Node name
     * @return Set of dependency names
     */
    [[nodiscard]] std::set<std::string> get_dependencies(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = dependencies_.find(name);
        if (it != dependencies_.end()) {
            return it->second;
        }
        return {};
    }

    /**
     * @brief Get all nodes that depend on a given node
     * @param name Node name
     * @return Set of dependent names
     */
    [[nodiscard]] std::set<std::string> get_dependents(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = dependents_.find(name);
        if (it != dependents_.end()) {
            return it->second;
        }
        return {};
    }

    /**
     * @brief Check if adding a dependency would create a cycle
     * @param from Source node
     * @param to Target node
     * @return true if a cycle would be created
     */
    [[nodiscard]] bool would_create_cycle(const std::string& from, const std::string& to) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return would_create_cycle_internal(from, to);
    }

    /**
     * @brief Get topological sort of all nodes
     * @return Result containing sorted node names or error if cycle exists
     */
    [[nodiscard]] Result<std::vector<std::string>> topological_sort() const {
        std::lock_guard<std::mutex> lock(mutex_);

        std::unordered_map<std::string, int> in_degree;
        for (const auto& [name, _] : nodes_) {
            in_degree[name] = 0;
        }

        for (const auto& [name, deps] : dependencies_) {
            for (const auto& dep : deps) {
                (void)dep;
            }
            in_degree[name] = static_cast<int>(deps.size());
        }

        std::queue<std::string> zero_in_degree;
        for (const auto& [name, degree] : in_degree) {
            if (degree == 0) {
                zero_in_degree.push(name);
            }
        }

        std::vector<std::string> result;
        result.reserve(nodes_.size());

        while (!zero_in_degree.empty()) {
            std::string current = zero_in_degree.front();
            zero_in_degree.pop();
            result.push_back(current);

            for (const auto& dependent : dependents_.at(current)) {
                --in_degree[dependent];
                if (in_degree[dependent] == 0) {
                    zero_in_degree.push(dependent);
                }
            }
        }

        if (result.size() != nodes_.size()) {
            return {error_info{1, "Cycle detected in dependency graph",
                               "health_dependency_graph"}};
        }

        return ok(result);
    }

    /**
     * @brief Execute health check with its dependencies
     * @param name Name of the node to check
     * @return Result containing the health check result or error
     *
     * This method first checks all dependencies recursively.
     * If any dependency fails with unhealthy status, the dependent
     * check is marked as unhealthy without executing it.
     */
    Result<health_check_result> check_with_dependencies(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto node_it = nodes_.find(name);
        if (node_it == nodes_.end()) {
            return {error_info{1, "Node not found: " + name, "health_dependency_graph"}};
        }

        std::unordered_map<std::string, health_check_result> results;
        return check_with_dependencies_internal(name, results);
    }

    /**
     * @brief Get the impact of a node failure
     * @param name Name of the failed node
     * @return Set of all nodes that would be affected by this failure
     */
    [[nodiscard]] std::set<std::string> get_failure_impact(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mutex_);

        std::set<std::string> impacted;
        std::queue<std::string> to_visit;
        to_visit.push(name);

        while (!to_visit.empty()) {
            std::string current = to_visit.front();
            to_visit.pop();

            auto it = dependents_.find(current);
            if (it != dependents_.end()) {
                for (const auto& dependent : it->second) {
                    if (impacted.find(dependent) == impacted.end()) {
                        impacted.insert(dependent);
                        to_visit.push(dependent);
                    }
                }
            }
        }

        return impacted;
    }

    /**
     * @brief Check if a node exists
     * @param name Node name
     * @return true if node exists
     */
    [[nodiscard]] bool has_node(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return nodes_.find(name) != nodes_.end();
    }

    /**
     * @brief Get number of nodes
     * @return Number of nodes in the graph
     */
    [[nodiscard]] std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return nodes_.size();
    }

    /**
     * @brief Check if graph is empty
     * @return true if no nodes exist
     */
    [[nodiscard]] bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return nodes_.empty();
    }

    /**
     * @brief Clear all nodes and dependencies
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        nodes_.clear();
        dependencies_.clear();
        dependents_.clear();
    }

    /**
     * @brief Get all node names
     * @return Vector of all node names
     */
    [[nodiscard]] std::vector<std::string> get_all_nodes() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> names;
        names.reserve(nodes_.size());
        for (const auto& [name, _] : nodes_) {
            names.push_back(name);
        }
        return names;
    }

private:
    [[nodiscard]] bool would_create_cycle_internal(const std::string& from,
                                                   const std::string& to) const {
        // Adding "from depends on to" edge
        // This would create a cycle if 'from' is reachable from 'to' via dependencies
        // i.e., to -> ... -> from already exists in the dependency chain
        std::unordered_set<std::string> visited;
        std::queue<std::string> queue;
        queue.push(to);

        while (!queue.empty()) {
            std::string current = queue.front();
            queue.pop();

            if (current == from) {
                return true;
            }

            if (visited.find(current) != visited.end()) {
                continue;
            }
            visited.insert(current);

            // Follow the dependencies of the current node
            auto it = dependencies_.find(current);
            if (it != dependencies_.end()) {
                for (const auto& dep : it->second) {
                    queue.push(dep);
                }
            }
        }

        return false;
    }

    Result<health_check_result> check_with_dependencies_internal(
        const std::string& name,
        std::unordered_map<std::string, health_check_result>& results) {
        // Check if already computed
        auto result_it = results.find(name);
        if (result_it != results.end()) {
            return ok(result_it->second);
        }

        auto node_it = nodes_.find(name);
        if (node_it == nodes_.end()) {
            return {error_info{1, "Node not found: " + name, "health_dependency_graph"}};
        }

        // Check all dependencies first
        bool dependency_failed = false;
        std::string failure_reason;

        for (const auto& dep : dependencies_[name]) {
            auto dep_result = check_with_dependencies_internal(dep, results);
            if (dep_result.is_err()) {
                dependency_failed = true;
                failure_reason = "Dependency check failed: " + dep;
                break;
            }

            if (dep_result.value().status == health_status::unhealthy) {
                dependency_failed = true;
                failure_reason = "Dependency unhealthy: " + dep;
                break;
            }
        }

        health_check_result result;
        if (dependency_failed) {
            result.status = health_status::unhealthy;
            result.message = failure_reason;
            result.metadata["skipped"] = "true";
            result.metadata["reason"] = "dependency_failure";
        } else {
            result = node_it->second->check();
        }

        results[name] = result;
        return ok(result);
    }

    std::unordered_map<std::string, std::shared_ptr<health_check>> nodes_;
    std::unordered_map<std::string, std::set<std::string>> dependencies_;
    std::unordered_map<std::string, std::set<std::string>> dependents_;
    mutable std::mutex mutex_;
};

}  // namespace kcenon::common::interfaces
