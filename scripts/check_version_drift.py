#!/usr/bin/env python3
"""
SOUP Version Drift Detection Script.

Detects mismatches between dependency versions specified in vcpkg.json overrides
and FetchContent GIT_TAG declarations in CMake files. Ensures reproducibility
across different build paths as required by IEC 62304 SOUP traceability.
"""

import argparse
import json
import os
import re
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple


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


def normalize_version(tag: str) -> str:
    """
    Normalize a version string or GIT_TAG to a comparable form.

    Handles common tag formats:
    - 'v1.14.0'       -> '1.14.0'
    - 'release-1.12.1' -> '1.12.1'
    - '1.8.3'          -> '1.8.3'
    - 'v1.30.2'        -> '1.30.2'
    """
    # Strip leading 'v' prefix
    tag = tag.strip()
    if tag.startswith('v'):
        tag = tag[1:]

    # Strip 'release-' prefix (e.g. googletest uses 'release-1.12.1')
    if tag.startswith('release-'):
        tag = tag[len('release-'):]

    return tag


def parse_vcpkg_overrides(vcpkg_path: Path) -> Dict[str, str]:
    """
    Parse vcpkg.json and extract dependency versions from overrides.

    Returns a dict of {name: version}.
    """
    if not vcpkg_path.exists():
        return {}

    try:
        with open(vcpkg_path, 'r', encoding='utf-8') as f:
            data = json.load(f)
    except (json.JSONDecodeError, IOError) as e:
        print(f"{Colors.RED}Error reading {vcpkg_path}: {e}{Colors.NC}")
        return {}

    overrides = {}
    for entry in data.get('overrides', []):
        name = entry.get('name', '').lower()
        version = entry.get('version', '')
        if name and version:
            overrides[name] = version

    return overrides


def parse_fetchcontent_tags(cmake_files: List[Path]) -> Dict[str, List[Tuple[str, str]]]:
    """
    Parse CMake files and extract FetchContent GIT_TAG declarations.

    Returns a dict of {dep_name: [(git_tag, file_path), ...]}.
    The dep_name is derived from the FetchContent_Declare first argument.
    """
    # Pattern: FetchContent_Declare(<name> ... GIT_TAG <tag> ...)
    # The GIT_TAG may appear on the same line or on subsequent lines.
    declare_pattern = re.compile(
        r'FetchContent_Declare\s*\(\s*(\w+)',
        re.IGNORECASE
    )
    git_tag_pattern = re.compile(
        r'GIT_TAG\s+([^\s)]+)',
        re.IGNORECASE
    )

    results: Dict[str, List[Tuple[str, str]]] = {}

    for cmake_file in cmake_files:
        try:
            content = cmake_file.read_text(encoding='utf-8', errors='ignore')
        except IOError:
            continue

        # Find all FetchContent_Declare blocks
        for declare_match in declare_pattern.finditer(content):
            dep_name = declare_match.group(1).lower()
            start = declare_match.start()

            # Find the closing paren of this declaration
            depth = 0
            end = start
            for i, ch in enumerate(content[start:], start):
                if ch == '(':
                    depth += 1
                elif ch == ')':
                    depth -= 1
                    if depth == 0:
                        end = i + 1
                        break

            block = content[start:end]

            # Extract GIT_TAG from this block
            tag_match = git_tag_pattern.search(block)
            if tag_match:
                git_tag = tag_match.group(1)
                # Skip CMake variables (they start with ${)
                if git_tag.startswith('${'):
                    continue
                if dep_name not in results:
                    results[dep_name] = []
                results[dep_name].append((git_tag, str(cmake_file)))

    return results


def find_cmake_files(project_root: Path) -> List[Path]:
    """Find all CMakeLists.txt and *.cmake files in the project."""
    cmake_files = []
    cmake_files.extend(project_root.rglob('CMakeLists.txt'))
    cmake_files.extend(project_root.rglob('*.cmake'))
    # Exclude build directories
    return [
        f for f in cmake_files
        if not any(part in ('build', '_build', 'build_check', '.cmake') for part in f.parts)
    ]


def check_version_drift(
    vcpkg_versions: Dict[str, str],
    fetchcontent_tags: Dict[str, List[Tuple[str, str]]],
) -> List[Dict]:
    """
    Compare vcpkg overrides against FetchContent GIT_TAGs.

    Returns a list of drift findings, each containing:
      - dep: dependency name
      - vcpkg_version: version in vcpkg.json
      - git_tag: GIT_TAG in CMake
      - git_tag_normalized: normalized GIT_TAG for comparison
      - file: CMake file path
    """
    findings = []

    for dep_name, git_tag_entries in fetchcontent_tags.items():
        vcpkg_version = vcpkg_versions.get(dep_name)
        if vcpkg_version is None:
            # Dependency not pinned in vcpkg.json — not a drift error, skip
            continue

        vcpkg_normalized = normalize_version(vcpkg_version)

        for git_tag, cmake_file in git_tag_entries:
            tag_normalized = normalize_version(git_tag)
            if tag_normalized != vcpkg_normalized:
                findings.append({
                    'dep': dep_name,
                    'vcpkg_version': vcpkg_version,
                    'vcpkg_normalized': vcpkg_normalized,
                    'git_tag': git_tag,
                    'git_tag_normalized': tag_normalized,
                    'file': cmake_file,
                })

    return findings


def check_matrix_consistency(
    vcpkg_versions: Dict[str, str],
    matrix_path: Path,
) -> List[Dict]:
    """
    Cross-check vcpkg overrides against DEPENDENCY_MATRIX.md ecosystem standards.

    Parses the Version Override Matrix table and identifies deviations.
    Returns a list of inconsistency findings.
    """
    if not matrix_path.exists():
        return []

    findings = []
    content = matrix_path.read_text(encoding='utf-8')

    # Parse the Version Override Matrix table
    # Header line: | Dependency | License | common | ... | Ecosystem Standard |
    # We need to find the 'Ecosystem Standard' column and the 'common' column.
    table_pattern = re.compile(
        r'^\|([^|]+)\|([^|]+)\|([^|]+)\|([^|]+)\|([^|]+)\|([^|]+)\|([^|]+)\|([^|]+)\|([^|]+)\|([^|]+)\|$',
        re.MULTILINE
    )

    lines = content.splitlines()
    in_matrix = False
    headers: List[str] = []
    common_col = -1
    standard_col = -1

    for line in lines:
        line = line.strip()
        if not line.startswith('|'):
            if in_matrix:
                break
            continue

        cells = [c.strip() for c in line.strip('|').split('|')]

        # Detect header row
        if 'Dependency' in cells[0] and 'Ecosystem Standard' in cells[-1]:
            in_matrix = True
            headers = [c.strip('* ').lower() for c in cells]
            # Find 'common' column (this project)
            for i, h in enumerate(headers):
                if h == 'common':
                    common_col = i
                if 'ecosystem standard' in h:
                    standard_col = i
            continue

        if in_matrix:
            # Skip separator rows
            if set(line.replace('|', '').replace('-', '').replace(':', '').strip()) == set():
                continue

            if len(cells) < max(common_col, standard_col) + 1:
                continue

            dep_raw = cells[0].strip('* ').strip()
            standard_raw = cells[standard_col].strip('* ').strip() if standard_col >= 0 else ''

            # Extract version from standard cell (e.g., '**1.14.0**' or '**1.14.0** (note)')
            std_version_match = re.search(r'[\d]+\.[\d]+(?:\.[\d]+)?', standard_raw)
            if not std_version_match:
                continue
            std_version = std_version_match.group(0)

            # Map dependency name to vcpkg package name
            dep_name = dep_raw.lower()
            dep_name_map = {
                'gtest': 'gtest',
                'benchmark': 'benchmark',
                'fmt': 'fmt',
                'asio': 'asio',
                'openssl': 'openssl',
                'zlib': 'zlib',
                'lz4': 'lz4',
                'spdlog': 'spdlog',
                'grpc': 'grpc',
                'protobuf': 'protobuf',
                'otel c++': 'opentelemetry-cpp',
                'libiconv': 'libiconv',
                'libpqxx': 'libpqxx',
                'libpq': 'libpq',
                'sqlite3': 'sqlite3',
                'mongo-cxx-driver': 'mongo-cxx-driver',
                'hiredis': 'hiredis',
            }
            vcpkg_name = dep_name_map.get(dep_name, dep_name)
            vcpkg_version = vcpkg_versions.get(vcpkg_name)

            if vcpkg_version is None:
                continue

            if normalize_version(vcpkg_version) != normalize_version(std_version):
                findings.append({
                    'dep': dep_raw,
                    'vcpkg_name': vcpkg_name,
                    'vcpkg_version': vcpkg_version,
                    'ecosystem_standard': std_version,
                    'matrix_file': str(matrix_path),
                })

    return findings


def main():
    parser = argparse.ArgumentParser(
        description='Detect SOUP version drift between vcpkg.json and FetchContent GIT_TAGs'
    )
    parser.add_argument(
        '--project-root',
        type=Path,
        default=None,
        help='Project root directory (default: parent of scripts/)',
    )
    parser.add_argument(
        '--vcpkg-json',
        type=Path,
        default=None,
        help='Path to vcpkg.json (default: <project-root>/vcpkg.json)',
    )
    parser.add_argument(
        '--matrix',
        type=Path,
        default=None,
        help='Path to DEPENDENCY_MATRIX.md for cross-repo consistency check',
    )
    parser.add_argument(
        '--no-matrix',
        action='store_true',
        help='Skip DEPENDENCY_MATRIX.md consistency check',
    )
    parser.add_argument(
        '--no-color',
        action='store_true',
        help='Disable colored output',
    )
    parser.add_argument(
        '--verbose',
        action='store_true',
        help='Show all parsed versions, not only drift findings',
    )

    args = parser.parse_args()

    if args.no_color or not sys.stdout.isatty():
        Colors.disable()

    # Resolve paths
    script_dir = Path(__file__).parent
    project_root = args.project_root or script_dir.parent
    vcpkg_path = args.vcpkg_json or (project_root / 'vcpkg.json')
    matrix_path = args.matrix or (project_root / 'DEPENDENCY_MATRIX.md')

    print(f"{Colors.BLUE}========================================{Colors.NC}")
    print(f"{Colors.BLUE}SOUP Version Drift Detection{Colors.NC}")
    print(f"{Colors.BLUE}========================================{Colors.NC}")
    print(f"Project root : {project_root}")
    print(f"vcpkg.json   : {vcpkg_path}")
    print()

    exit_code = 0

    # --- Step 1: Parse vcpkg.json overrides ---
    print(f"{Colors.YELLOW}Parsing vcpkg.json overrides...{Colors.NC}")
    vcpkg_versions = parse_vcpkg_overrides(vcpkg_path)

    if not vcpkg_versions:
        print(f"  No overrides found in vcpkg.json")
    else:
        for dep, ver in sorted(vcpkg_versions.items()):
            print(f"  {dep}: {ver}")
    print()

    # --- Step 2: Parse FetchContent GIT_TAGs ---
    print(f"{Colors.YELLOW}Scanning CMake files for FetchContent GIT_TAGs...{Colors.NC}")
    cmake_files = find_cmake_files(project_root)
    fetchcontent_tags = parse_fetchcontent_tags(cmake_files)

    if not fetchcontent_tags:
        print(f"  No FetchContent GIT_TAG declarations found")
    elif args.verbose:
        for dep, entries in sorted(fetchcontent_tags.items()):
            for tag, cmake_file in entries:
                rel = Path(cmake_file).relative_to(project_root) if project_root in Path(cmake_file).parents else cmake_file
                print(f"  {dep}: {tag}  ({rel})")
    else:
        print(f"  Found FetchContent declarations for: {', '.join(sorted(fetchcontent_tags.keys()))}")
    print()

    # --- Step 3: Detect drift ---
    print(f"{Colors.YELLOW}Checking for version drift...{Colors.NC}")
    drift_findings = check_version_drift(vcpkg_versions, fetchcontent_tags)

    if drift_findings:
        exit_code = 1
        for f in drift_findings:
            rel = Path(f['file']).relative_to(project_root) if project_root in Path(f['file']).parents else f['file']
            print(
                f"{Colors.RED}DRIFT{Colors.NC}  {f['dep']}: "
                f"vcpkg.json={f['vcpkg_version']} "
                f"vs GIT_TAG={f['git_tag']} "
                f"(in {rel})"
            )
        print(f"\n{Colors.RED}Found {len(drift_findings)} version drift issue(s){Colors.NC}")
    else:
        print(f"{Colors.GREEN}No version drift detected{Colors.NC}")
    print()

    # --- Step 4: Cross-repo matrix consistency (optional) ---
    if not args.no_matrix:
        print(f"{Colors.YELLOW}Checking against DEPENDENCY_MATRIX.md...{Colors.NC}")
        if not matrix_path.exists():
            print(f"  {Colors.YELLOW}DEPENDENCY_MATRIX.md not found, skipping cross-repo check{Colors.NC}")
        else:
            matrix_findings = check_matrix_consistency(vcpkg_versions, matrix_path)
            if matrix_findings:
                exit_code = 1
                for f in matrix_findings:
                    print(
                        f"{Colors.RED}MATRIX MISMATCH{Colors.NC}  {f['dep']} ({f['vcpkg_name']}): "
                        f"vcpkg.json={f['vcpkg_version']} "
                        f"vs ecosystem standard={f['ecosystem_standard']}"
                    )
                print(f"\n{Colors.RED}Found {len(matrix_findings)} matrix inconsistency issue(s){Colors.NC}")
            else:
                print(f"{Colors.GREEN}All versions consistent with DEPENDENCY_MATRIX.md{Colors.NC}")
        print()

    # --- Summary ---
    print(f"{Colors.BLUE}========================================{Colors.NC}")
    print(f"{Colors.BLUE}Summary{Colors.NC}")
    print(f"{Colors.BLUE}========================================{Colors.NC}")
    print(f"vcpkg.json overrides      : {len(vcpkg_versions)}")
    print(f"FetchContent declarations  : {sum(len(v) for v in fetchcontent_tags.values())}")
    print(f"Version drift issues       : {len(drift_findings)}")

    if exit_code == 0:
        print(f"{Colors.GREEN}No SOUP version drift detected!{Colors.NC}")
    else:
        print(f"{Colors.RED}SOUP version drift detected — fix version inconsistencies before merging{Colors.NC}")

    sys.exit(exit_code)


if __name__ == '__main__':
    main()
