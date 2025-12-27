#!/usr/bin/env python3
"""
Circular Dependency Detection Script for common_system.

Analyzes header include relationships and detects circular dependency patterns.
"""

import argparse
import os
import re
import sys
from collections import defaultdict
from pathlib import Path
from typing import Dict, List, Set, Tuple, Optional


class Colors:
    """ANSI color codes for terminal output."""
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    BLUE = '\033[0;34m'
    NC = '\033[0m'  # No Color

    @classmethod
    def disable(cls):
        """Disable colors (for non-terminal output)."""
        cls.RED = cls.GREEN = cls.YELLOW = cls.BLUE = cls.NC = ''


# Standard library headers to ignore
SYSTEM_HEADERS = {
    'algorithm', 'any', 'array', 'atomic', 'bitset', 'cassert', 'cctype',
    'cerrno', 'cfloat', 'chrono', 'climits', 'cmath', 'compare', 'complex',
    'concepts', 'condition_variable', 'coroutine', 'csetjmp', 'csignal',
    'cstdarg', 'cstddef', 'cstdint', 'cstdio', 'cstdlib', 'cstring', 'ctime',
    'cwchar', 'cwctype', 'deque', 'exception', 'execution', 'expected',
    'filesystem', 'format', 'forward_list', 'fstream', 'functional', 'future',
    'initializer_list', 'iomanip', 'ios', 'iosfwd', 'iostream', 'istream',
    'iterator', 'latch', 'limits', 'list', 'locale', 'map', 'memory',
    'memory_resource', 'mutex', 'new', 'numbers', 'numeric', 'optional',
    'ostream', 'queue', 'random', 'ranges', 'ratio', 'regex', 'scoped_allocator',
    'semaphore', 'set', 'shared_mutex', 'source_location', 'span', 'spanstream',
    'sstream', 'stack', 'stdexcept', 'stop_token', 'streambuf', 'string',
    'string_view', 'syncstream', 'system_error', 'thread', 'tuple', 'type_traits',
    'typeindex', 'typeinfo', 'unordered_map', 'unordered_set', 'utility',
    'valarray', 'variant', 'vector', 'version',
}


def is_system_header(header: str) -> bool:
    """Check if a header is a system/standard library header."""
    # Direct match
    if header in SYSTEM_HEADERS:
        return True
    # Headers starting with c (C library wrappers)
    if header.startswith('c') and header[1:] in {'assert', 'ctype', 'errno',
                                                   'float', 'limits', 'locale',
                                                   'math', 'setjmp', 'signal',
                                                   'stdarg', 'stddef', 'stdio',
                                                   'stdlib', 'string', 'time',
                                                   'wchar', 'wctype'}:
        return True
    # std:: prefixed
    if header.startswith('std'):
        return True
    return False


def extract_includes(file_path: Path, include_dir: Path) -> List[str]:
    """Extract include directives from a file and resolve paths."""
    includes = []
    include_pattern = re.compile(r'^\s*#include\s*[<"]([^>"]+)[>"]')

    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            for line in f:
                match = include_pattern.match(line)
                if match:
                    include = match.group(1)

                    # Skip system headers
                    if is_system_header(include):
                        continue

                    # Resolve the include path
                    resolved = resolve_include(include, file_path, include_dir)
                    if resolved:
                        includes.append(resolved)
    except IOError:
        pass

    return includes


def resolve_include(include: str, from_file: Path, include_dir: Path) -> Optional[str]:
    """Resolve an include path to a normalized form relative to include_dir."""
    from_dir = from_file.parent

    if include.startswith('../') or include.startswith('./'):
        # Relative path
        resolved = (from_dir / include).resolve()
        try:
            return str(resolved.relative_to(include_dir))
        except ValueError:
            return None
    elif include.startswith('kcenon/'):
        # Absolute project path
        return include
    elif '/' not in include:
        # Same directory include
        same_dir_file = from_dir / include
        if same_dir_file.exists():
            try:
                return str(same_dir_file.relative_to(include_dir))
            except ValueError:
                return None
    return None


def build_dependency_graph(include_dir: Path) -> Dict[str, List[str]]:
    """Build a dependency graph from all header files."""
    graph = defaultdict(list)

    for file_path in include_dir.rglob('*.h'):
        rel_path = str(file_path.relative_to(include_dir))
        includes = extract_includes(file_path, include_dir)
        for inc in includes:
            graph[rel_path].append(inc)

    for file_path in include_dir.rglob('*.hpp'):
        rel_path = str(file_path.relative_to(include_dir))
        includes = extract_includes(file_path, include_dir)
        for inc in includes:
            graph[rel_path].append(inc)

    return graph


def find_direct_cycles(graph: Dict[str, List[str]]) -> List[Tuple[str, str]]:
    """Find direct circular includes (A -> B and B -> A)."""
    cycles = []
    checked = set()

    for src, deps in graph.items():
        for dst in deps:
            pair = tuple(sorted([src, dst]))
            if pair not in checked:
                checked.add(pair)
                if dst in graph and src in graph[dst]:
                    cycles.append((src, dst))

    return cycles


def find_transitive_cycles(graph: Dict[str, List[str]]) -> List[List[str]]:
    """Find transitive circular dependencies using DFS."""
    visited: Set[str] = set()
    rec_stack: Set[str] = set()
    cycles: List[List[str]] = []

    def dfs(node: str, path: List[str]) -> Optional[List[str]]:
        visited.add(node)
        rec_stack.add(node)
        path.append(node)

        for neighbor in graph.get(node, []):
            if neighbor not in visited:
                cycle = dfs(neighbor, path)
                if cycle:
                    return cycle
            elif neighbor in rec_stack:
                # Found cycle
                try:
                    cycle_start = path.index(neighbor)
                    return path[cycle_start:] + [neighbor]
                except ValueError:
                    pass

        path.pop()
        rec_stack.remove(node)
        return None

    # Get all nodes (both sources and targets)
    all_nodes = set(graph.keys())
    for deps in graph.values():
        all_nodes.update(deps)

    for node in sorted(all_nodes):
        if node not in visited:
            cycle = dfs(node, [])
            if cycle:
                cycles.append(cycle)

    return cycles


def main():
    parser = argparse.ArgumentParser(
        description='Detect circular dependencies in header files'
    )
    parser.add_argument(
        '--include-dir',
        type=Path,
        default=None,
        help='Include directory to analyze (default: include/)'
    )
    parser.add_argument(
        '--output',
        type=Path,
        default=None,
        help='Write report to file'
    )
    parser.add_argument(
        '--verbose',
        action='store_true',
        help='Enable verbose output'
    )
    parser.add_argument(
        '--no-color',
        action='store_true',
        help='Disable colored output'
    )

    args = parser.parse_args()

    if args.no_color or not sys.stdout.isatty():
        Colors.disable()

    # Determine include directory
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    include_dir = args.include_dir or (project_root / 'include')

    if not include_dir.exists():
        print(f"{Colors.RED}Error: Include directory not found: {include_dir}{Colors.NC}")
        sys.exit(1)

    print(f"{Colors.BLUE}========================================{Colors.NC}")
    print(f"{Colors.BLUE}Circular Dependency Detection{Colors.NC}")
    print(f"{Colors.BLUE}========================================{Colors.NC}")
    print(f"Analyzing: {include_dir}")
    print()

    # Build dependency graph
    print(f"{Colors.YELLOW}Building dependency graph...{Colors.NC}")
    graph = build_dependency_graph(include_dir)

    total_deps = sum(len(deps) for deps in graph.values())
    print(f"Found {total_deps} include relationships")

    if args.verbose and total_deps > 0:
        print()
        print("Dependencies:")
        count = 0
        for src, deps in sorted(graph.items()):
            for dst in deps:
                print(f"  {src} -> {dst}")
                count += 1
                if count >= 20:
                    break
            if count >= 20:
                print(f"  ... ({total_deps} total)")
                break
    print()

    exit_code = 0

    # Check for direct circular includes
    print(f"{Colors.YELLOW}Checking for direct circular includes...{Colors.NC}")
    direct_cycles = find_direct_cycles(graph)

    if direct_cycles:
        for src, dst in direct_cycles:
            print(f"{Colors.RED}Direct circular include:{Colors.NC} {src} <-> {dst}")
        print(f"{Colors.RED}Found {len(direct_cycles)} direct circular include(s){Colors.NC}")
        exit_code = 1
    else:
        print(f"{Colors.GREEN}No direct circular includes found{Colors.NC}")
    print()

    # Check for transitive cycles
    print(f"{Colors.YELLOW}Checking for transitive circular dependencies...{Colors.NC}")
    transitive_cycles = find_transitive_cycles(graph)

    if transitive_cycles:
        for cycle in transitive_cycles[:5]:
            print(f"{Colors.RED}Cycle:{Colors.NC} {' -> '.join(cycle)}")
        if len(transitive_cycles) > 5:
            print(f"  ... and {len(transitive_cycles) - 5} more")
        exit_code = 1
    else:
        print(f"{Colors.GREEN}No transitive circular dependencies found{Colors.NC}")
    print()

    # Summary
    print(f"{Colors.BLUE}========================================{Colors.NC}")
    print(f"{Colors.BLUE}Summary{Colors.NC}")
    print(f"{Colors.BLUE}========================================{Colors.NC}")
    print(f"Total include relationships: {total_deps}")
    print(f"Direct circular includes: {len(direct_cycles)}")
    print(f"Transitive cycles: {len(transitive_cycles)}")

    if exit_code == 0:
        print(f"{Colors.GREEN}No circular dependency issues detected!{Colors.NC}")
    else:
        print(f"{Colors.RED}Circular dependency issues found!{Colors.NC}")

    # Write output file if specified
    if args.output:
        with open(args.output, 'w') as f:
            f.write("# Circular Dependency Analysis Report\n")
            f.write(f"Date: {os.popen('date +%Y-%m-%d').read().strip()}\n\n")
            f.write("## Summary\n")
            f.write(f"- Total include relationships: {total_deps}\n")
            f.write(f"- Direct circular includes: {len(direct_cycles)}\n")
            f.write(f"- Transitive cycles: {len(transitive_cycles)}\n\n")
            f.write("## Dependency Graph\n")
            for src, deps in sorted(graph.items()):
                for dst in deps:
                    f.write(f"{src} -> {dst}\n")
        print()
        print(f"Report written to: {args.output}")

    sys.exit(exit_code)


if __name__ == '__main__':
    main()
