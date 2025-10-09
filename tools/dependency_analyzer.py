#!/usr/bin/env python3
"""
Dependency Analyzer for C++ Multi-System Project

Analyzes CMakeLists.txt files and #include directives to:
1. Build dependency graph between systems
2. Detect circular dependencies
3. Identify layer violations
4. Generate visualization-ready output

Author: kcenon
Date: 2025-10-10
Phase: 4 - Dependency Refactoring
"""

import re
import os
import sys
from pathlib import Path
from collections import defaultdict, deque
from typing import Dict, List, Set, Tuple, Optional


class DependencyAnalyzer:
    """Analyzes dependencies across multiple C++ systems."""

    def __init__(self, base_path: str):
        self.base_path = Path(base_path)
        self.systems = [
            'common_system',
            'thread_system',
            'logger_system',
            'monitoring_system',
            'container_system',
            'database_system',
            'network_system'
        ]

        # Dependency graph: system -> list of systems it depends on
        self.cmake_deps: Dict[str, Set[str]] = defaultdict(set)
        # Include dependencies: system -> list of systems it includes
        self.include_deps: Dict[str, Set[str]] = defaultdict(set)
        # Combined dependencies
        self.all_deps: Dict[str, Set[str]] = defaultdict(set)

    def parse_cmake_dependencies(self, system: str) -> Set[str]:
        """Extract target_link_libraries dependencies from CMakeLists.txt."""
        cmake_file = self.base_path / system / 'CMakeLists.txt'
        dependencies = set()

        if not cmake_file.exists():
            print(f"Warning: {cmake_file} not found")
            return dependencies

        try:
            content = cmake_file.read_text(encoding='utf-8')

            # Pattern: target_link_libraries(target ... system_name ...)
            # Look for other systems in target_link_libraries
            link_pattern = r'target_link_libraries\s*\([^)]+\)'

            for match in re.finditer(link_pattern, content, re.MULTILINE | re.DOTALL):
                link_stmt = match.group(0)

                # Check if any other system is mentioned
                for other_system in self.systems:
                    if other_system != system and other_system in link_stmt:
                        dependencies.add(other_system)

            # Also check find_package and add_subdirectory
            for other_system in self.systems:
                if other_system != system:
                    if f'find_package({other_system}' in content:
                        dependencies.add(other_system)
                    if f'add_subdirectory({other_system}' in content:
                        dependencies.add(other_system)

        except Exception as e:
            print(f"Error parsing {cmake_file}: {e}")

        return dependencies

    def parse_include_dependencies(self, system: str) -> Set[str]:
        """Extract #include dependencies from source files."""
        dependencies = set()

        # Check include directory
        include_dir = self.base_path / system / 'include'
        src_dir = self.base_path / system / 'src'

        for directory in [include_dir, src_dir]:
            if not directory.exists():
                continue

            # Find all .h, .hpp, .cpp files
            for ext in ['*.h', '*.hpp', '*.cpp', '*.cc']:
                for source_file in directory.rglob(ext):
                    try:
                        content = source_file.read_text(encoding='utf-8')

                        # Pattern: #include <kcenon/system_name/...>
                        # or #include "kcenon/system_name/..."
                        include_pattern = r'#include\s+[<"]kcenon/(\w+)/'

                        for match in re.finditer(include_pattern, content):
                            included_system = match.group(1)

                            # Map directory name to system name
                            # e.g., 'logger' -> 'logger_system'
                            full_system_name = f"{included_system}_system"

                            if full_system_name in self.systems and full_system_name != system:
                                dependencies.add(full_system_name)

                            # Also handle 'common' -> 'common_system'
                            if included_system == 'common' and system != 'common_system':
                                dependencies.add('common_system')

                    except Exception as e:
                        print(f"Error parsing {source_file}: {e}")

        return dependencies

    def analyze_all_systems(self):
        """Analyze all systems and build dependency graphs."""
        print("=" * 80)
        print("Dependency Analysis - Phase 4: Dependency Refactoring")
        print("=" * 80)
        print()

        for system in self.systems:
            print(f"Analyzing {system}...")

            # Parse CMake dependencies
            cmake_deps = self.parse_cmake_dependencies(system)
            self.cmake_deps[system] = cmake_deps

            # Parse include dependencies
            include_deps = self.parse_include_dependencies(system)
            self.include_deps[system] = include_deps

            # Combine all dependencies
            self.all_deps[system] = cmake_deps | include_deps

            if self.all_deps[system]:
                print(f"  → Depends on: {', '.join(sorted(self.all_deps[system]))}")
            else:
                print(f"  → No dependencies (foundation)")
            print()

    def detect_cycles(self) -> List[List[str]]:
        """Find circular dependencies using DFS."""
        cycles = []
        visited = set()
        rec_stack = []

        def dfs(node: str, path: List[str]) -> bool:
            """DFS with cycle detection."""
            visited.add(node)
            rec_stack.append(node)
            path.append(node)

            for neighbor in self.all_deps.get(node, set()):
                if neighbor not in visited:
                    if dfs(neighbor, path):
                        return True
                elif neighbor in rec_stack:
                    # Found a cycle
                    cycle_start = rec_stack.index(neighbor)
                    cycle = rec_stack[cycle_start:] + [neighbor]
                    if cycle not in cycles:
                        cycles.append(cycle)
                    return True

            rec_stack.pop()
            return False

        for system in self.systems:
            if system not in visited:
                dfs(system, [])

        return cycles

    def calculate_levels(self) -> Dict[str, int]:
        """Calculate dependency levels (0 = no deps, higher = more deps)."""
        levels = {}

        # Use topological sort approach
        in_degree = {s: 0 for s in self.systems}

        for system in self.systems:
            for dep in self.all_deps[system]:
                in_degree[dep] += 1

        # Start with systems that have no dependencies
        queue = deque([s for s in self.systems if in_degree[s] == 0])
        level = 0

        while queue:
            level_size = len(queue)
            for _ in range(level_size):
                system = queue.popleft()
                levels[system] = level

                # Reduce in-degree for dependents
                for other in self.systems:
                    if system in self.all_deps[other]:
                        in_degree[other] -= 1
                        if in_degree[other] == 0:
                            queue.append(other)
            level += 1

        # Handle cyclic dependencies
        for system in self.systems:
            if system not in levels:
                levels[system] = -1  # Cycle indicator

        return levels

    def generate_report(self) -> str:
        """Generate comprehensive dependency report."""
        report = []

        report.append("# Dependency Analysis Report")
        report.append("")
        report.append("**Date**: 2025-10-10")
        report.append("**Phase**: Phase 4 - Dependency Refactoring")
        report.append("**Analyzer**: dependency_analyzer.py")
        report.append("")
        report.append("---")
        report.append("")

        # Summary
        report.append("## Executive Summary")
        report.append("")

        total_deps = sum(len(deps) for deps in self.all_deps.values())
        report.append(f"- **Total Systems**: {len(self.systems)}")
        report.append(f"- **Total Dependencies**: {total_deps}")
        report.append(f"- **Average Dependencies per System**: {total_deps / len(self.systems):.1f}")
        report.append("")

        # Detect cycles
        cycles = self.detect_cycles()
        report.append(f"- **Circular Dependencies Found**: {len(cycles)}")
        report.append("")

        if cycles:
            report.append("### ⚠️ Circular Dependencies Detected")
            report.append("")
            for i, cycle in enumerate(cycles, 1):
                cycle_str = ' → '.join(cycle)
                report.append(f"{i}. {cycle_str}")
            report.append("")
        else:
            report.append("### ✅ No Circular Dependencies")
            report.append("")

        report.append("---")
        report.append("")

        # Dependency levels
        report.append("## Dependency Levels")
        report.append("")
        levels = self.calculate_levels()

        report.append("**Level 0** (Foundation - No Dependencies):")
        level_0 = [s for s, l in levels.items() if l == 0]
        if level_0:
            for system in sorted(level_0):
                report.append(f"- {system}")
        else:
            report.append("- None (All systems have dependencies)")
        report.append("")

        for level in range(1, max(levels.values()) + 1):
            level_systems = [s for s, l in levels.items() if l == level]
            if level_systems:
                report.append(f"**Level {level}**:")
                for system in sorted(level_systems):
                    deps = ', '.join(sorted(self.all_deps[system]))
                    report.append(f"- {system} → [{deps}]")
                report.append("")

        # Systems in cycles
        cycle_systems = [s for s, l in levels.items() if l == -1]
        if cycle_systems:
            report.append("**Cyclic** (Part of circular dependency):")
            for system in sorted(cycle_systems):
                deps = ', '.join(sorted(self.all_deps[system]))
                report.append(f"- {system} → [{deps}]")
            report.append("")

        report.append("---")
        report.append("")

        # Detailed dependency breakdown
        report.append("## Detailed Dependency Breakdown")
        report.append("")

        for system in sorted(self.systems):
            report.append(f"### {system}")
            report.append("")

            cmake_deps = self.cmake_deps[system]
            include_deps = self.include_deps[system]
            all_deps = self.all_deps[system]

            if not all_deps:
                report.append("**Status**: Foundation system (no dependencies)")
            else:
                report.append(f"**CMake Dependencies**: {', '.join(sorted(cmake_deps)) if cmake_deps else 'None'}")
                report.append(f"**Include Dependencies**: {', '.join(sorted(include_deps)) if include_deps else 'None'}")
                report.append(f"**Total Dependencies**: {len(all_deps)}")

                # Check for mismatches
                cmake_only = cmake_deps - include_deps
                include_only = include_deps - cmake_deps

                if cmake_only:
                    report.append(f"**⚠️ CMake-only** (not in includes): {', '.join(sorted(cmake_only))}")
                if include_only:
                    report.append(f"**⚠️ Include-only** (not in CMake): {', '.join(sorted(include_only))}")

            report.append("")

        report.append("---")
        report.append("")

        # Recommendations
        report.append("## Refactoring Recommendations")
        report.append("")

        if cycles:
            report.append("### Priority 1: Break Circular Dependencies")
            report.append("")
            report.append("**Strategy**: Dependency Injection via Interfaces")
            report.append("")

            for i, cycle in enumerate(cycles, 1):
                report.append(f"#### Cycle {i}: {' → '.join(cycle)}")
                report.append("")
                report.append("**Recommended Approach**:")
                report.append("")
                report.append("1. Create interface in `common_system`:")
                report.append("```cpp")
                report.append(f"namespace common::interfaces {{")
                report.append(f"    class I{cycle[0].replace('_system', '').capitalize()} {{")
                report.append("        virtual ~I() = default;")
                report.append("        // Abstract interface methods")
                report.append("    };")
                report.append("}")
                report.append("```")
                report.append("")
                report.append("2. Inject dependency instead of direct linking")
                report.append("3. Update CMakeLists.txt to remove circular link")
                report.append("")

        # Check for layer violations
        report.append("### Priority 2: Check Layer Violations")
        report.append("")

        # Define expected layers
        expected_layers = {
            'common_system': 0,
            'thread_system': 1,
            'logger_system': 1,
            'monitoring_system': 2,
            'container_system': 1,
            'database_system': 2,
            'network_system': 2
        }

        violations = []
        for system, expected_level in expected_layers.items():
            for dep in self.all_deps[system]:
                dep_level = expected_layers.get(dep, 0)
                if dep_level >= expected_level and dep != 'common_system':
                    violations.append((system, dep, expected_level, dep_level))

        if violations:
            report.append("**⚠️ Potential Layer Violations Detected:**")
            report.append("")
            for system, dep, sys_level, dep_level in violations:
                report.append(f"- {system} (L{sys_level}) → {dep} (L{dep_level})")
            report.append("")
        else:
            report.append("**✅ No obvious layer violations detected**")
            report.append("")

        report.append("---")
        report.append("")

        # Graphviz output
        report.append("## Dependency Graph (Graphviz DOT)")
        report.append("")
        report.append("```dot")
        report.append("digraph DependencyGraph {")
        report.append("    rankdir=BT;  // Bottom to Top")
        report.append("    node [shape=box, style=filled];")
        report.append("")

        # Color nodes by level
        for system, level in levels.items():
            if level == -1:
                color = "lightcoral"  # Cyclic
            elif level == 0:
                color = "lightgreen"  # Foundation
            elif level == 1:
                color = "lightyellow"  # Level 1
            else:
                color = "lightblue"  # Higher levels

            report.append(f'    "{system}" [fillcolor={color}];')

        report.append("")

        # Add edges
        for system in sorted(self.systems):
            for dep in sorted(self.all_deps[system]):
                report.append(f'    "{system}" -> "{dep}";')

        report.append("}")
        report.append("```")
        report.append("")

        report.append("**Legend**:")
        report.append("- Green: Foundation (no dependencies)")
        report.append("- Yellow: Level 1 (depends only on foundation)")
        report.append("- Blue: Level 2+ (higher-level systems)")
        report.append("- Red: Part of circular dependency")
        report.append("")

        report.append("---")
        report.append("")
        report.append("**Analysis Complete**: 2025-10-10")
        report.append("**Next Steps**: Review cycles and plan refactoring")
        report.append("")

        return '\n'.join(report)

    def generate_graphviz_file(self, output_file: str):
        """Generate standalone Graphviz DOT file."""
        levels = self.calculate_levels()

        lines = []
        lines.append("digraph DependencyGraph {")
        lines.append("    rankdir=BT;  // Bottom to Top")
        lines.append("    node [shape=box, style=filled];")
        lines.append("    ")

        # Color nodes by level
        for system, level in levels.items():
            if level == -1:
                color = "lightcoral"
            elif level == 0:
                color = "lightgreen"
            elif level == 1:
                color = "lightyellow"
            else:
                color = "lightblue"

            lines.append(f'    "{system}" [fillcolor={color}];')

        lines.append("    ")

        # Add edges
        for system in sorted(self.systems):
            for dep in sorted(self.all_deps[system]):
                lines.append(f'    "{system}" -> "{dep}";')

        lines.append("}")

        with open(output_file, 'w') as f:
            f.write('\n'.join(lines))

        print(f"Graphviz DOT file written to: {output_file}")


def main():
    """Main entry point."""
    # Default to parent directory of tools/
    script_dir = Path(__file__).parent
    base_path = script_dir.parent

    print(f"Analyzing systems in: {base_path}")
    print()

    analyzer = DependencyAnalyzer(str(base_path))
    analyzer.analyze_all_systems()

    # Generate report
    report = analyzer.generate_report()

    # Write report to file
    docs_dir = base_path / 'docs'
    docs_dir.mkdir(exist_ok=True)

    report_file = docs_dir / 'DEPENDENCY_GRAPH.md'
    report_file.write_text(report, encoding='utf-8')

    print("=" * 80)
    print(f"Report written to: {report_file}")
    print()

    # Generate Graphviz file
    dot_file = docs_dir / 'dependency_graph.dot'
    analyzer.generate_graphviz_file(str(dot_file))

    print()
    print("To visualize the graph:")
    print(f"  dot -Tpng {dot_file} -o {docs_dir}/dependency_graph.png")
    print(f"  dot -Tsvg {dot_file} -o {docs_dir}/dependency_graph.svg")
    print()

    # Print summary
    cycles = analyzer.detect_cycles()
    if cycles:
        print("⚠️  CIRCULAR DEPENDENCIES FOUND!")
        print()
        for i, cycle in enumerate(cycles, 1):
            print(f"  {i}. {' → '.join(cycle)}")
        print()
        print("See DEPENDENCY_GRAPH.md for refactoring recommendations.")
        return 1
    else:
        print("✅ No circular dependencies found!")
        print()
        print("Dependency analysis complete. Review DEPENDENCY_GRAPH.md for details.")
        return 0


if __name__ == '__main__':
    sys.exit(main())
