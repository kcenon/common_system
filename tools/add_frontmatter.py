#!/usr/bin/env python3
"""Add standardized YAML frontmatter to docs/ markdown files.

Scans a project's docs/ directory for .md files without frontmatter,
infers category from filename patterns, generates a doc_id, and
prepends YAML frontmatter. Idempotent — skips files that already
have the standardized frontmatter block.

Usage:
    python3 tools/add_frontmatter.py <project_root> [--dry-run]
    python3 tools/add_frontmatter.py <project_root> --project <name> [--dry-run]

Examples:
    python3 tools/add_frontmatter.py /path/to/common_system
    python3 tools/add_frontmatter.py /path/to/thread_system --dry-run
"""

import argparse
import os
import re
import sys
from pathlib import Path

# ── Project prefix table ──────────────────────────────────────────────

PREFIX_MAP = {
    "common_system": "COM",
    "thread_system": "THR",
    "logger_system": "LOG",
    "container_system": "CNT",
    "monitoring_system": "MON",
    "database_system": "DBS",
    "network_system": "NET",
    "pacs_system": "PAC",
}

# ── Category inference rules ──────────────────────────────────────────
# Order matters: first match wins. Patterns are matched against the
# uppercase stem (filename without .md / .kr.md extension).

CATEGORY_RULES = [
    # ADR — decision records
    (r"^ADR[-_]", "ADR"),
    # ARCH — architecture documents
    (r"ARCHITECTURE|ARCH(?:ITECTURE)?_DIAGRAM|ARCH(?:ITECTURE)?_GUIDE|"
     r"ARCH(?:ITECTURE)?_ISSUES|RUNTIME_BINDING|DEPENDENCY_ARCHITECTURE|"
     r"PIPELINE_DESIGN|HAZARD_POINTER_DESIGN|VALUE_STORE_DESIGN|"
     r"DOMAIN_SEPARATION|INTERFACE_SEGREGATION|INTERFACE_SEPARATION|"
     r"LOGGER_SYSTEM_ARCHITECTURE|THREAD_SYSTEM_ARCHITECTURE|"
     r"PLUGIN_ARCHITECTURE|COMPOSITION[-_]DESIGN|CRTP[-_]ANALYSIS|"
     r"SESSION[-_]MANAGER[-_]TEMPLATE|LOCK[-_]FREE|"
     r"THREAD_LOCAL_COLLECTOR_DESIGN|QUEUE_POLICY_DESIGN|"
     r"CONDITIONAL_COMPILATION|(?<!PROJECT_)STRUCTURE$", "ARCH"),
    # API — API reference documents
    (r"API_REFERENCE|CLI_REFERENCE|PLUGIN_API_REFERENCE|ERROR_CODE_REGISTRY|"
     r"ERROR_CODES$|TYPE_SYSTEM$|CPP20_CONCEPTS|CONCEPTS$", "API"),
    # PERF — performance and benchmarking
    (r"BENCHMARK|BASELINE|PERFORMANCE|PROFILING|MEMORY_POOL_PERFORMANCE|"
     r"MEMORY_PROFILING|MEMORY_MANAGEMENT|E2E_BENCHMARK|ECOSYSTEM_BENCHMARK|"
     r"SPRINT_\d+_PERFORMANCE|DECORATOR_PERFORMANCE|LOAD_TEST|"
     r"CI_CD_PERFORMANCE|TUNING_GUIDE|NUMA_GUIDE|PERFORMANCE_COOKBOOK", "PERF"),
    # MIGR — migration guides
    (r"MIGRATION|MIGRAT", "MIGR"),
    # SECU — security
    (r"SECURITY|CVE[-_]SCANNING|TLS_SETUP", "SECU"),
    # QUAL — quality assurance
    (r"PRODUCTION_QUALITY|QUALITY$|SANITIZER|STATIC_ANALYSIS|COVERAGE|"
     r"VERIFICATION_REPORT|VALIDATION_REPORT|TESTING_GUIDE|TESTING$|"
     r"EXCEPTION_SAFETY|RELIABILITY_PATTERNS|THREAD_SYSTEM_STABILITY", "QUAL"),
    # INTR — integration
    (r"INTEGRATION|INTEROP|WITH[-_]COMMON|WITH[-_]LOGGER|WITH[-_]MONITORING|"
     r"NETWORK_SYSTEM_BRIDGE|GRPC_INTEGRATION|OPENTELEMETRY|"
     r"DISTRIBUTED_TRACING|OTEL_COLLECTOR|DICOM_CONFORMANCE|"
     r"IHE_INTEGRATION|ECOSYSTEM_BENCHMARK", "INTR"),
    # GUID — user guides, getting started, how-to
    (r"QUICK_START|BUILD_GUIDE|BUILD$|GETTING_STARTED|GUIDE(?!LINES)|"
     r"TUTORIAL|USER_GUIDE|EXAMPLES$|SAMPLES_GUIDE|CONCEPTS_GUIDE|"
     r"OPERATIONS$|CONFIGURATION$|CONFIG_GUIDE|CONFIGURATION_STRATEGIES|"
     r"ADAPTER_GUIDE|ADAPTER_PATTERNS|ASYNC_GUIDE|ASYNC_OPERATIONS|"
     r"ASYNC_WRITERS|CUSTOM_WRITERS|WRITER_GUIDE|WRITER_SELECTION|"
     r"WRITER_HIERARCHY|ARRAY_VALUE|SERIALIZATION|STREAM_PROCESSING|"
     r"LOGGING_BEST_PRACTICES|BEST_PRACTICES|RAII_GUIDELINES|"
     r"SMART_POINTER_GUIDELINES|SINGLETON_GUIDELINES|ERROR_HANDLING|"
     r"ERROR_CODE_GUIDELINES|FAQ$|TROUBLESHOOT|KNOWN_ISSUES|"
     r"GETTING_STARTED|DI_AND_CONCEPTS|FACADE_GUIDE|VCPKG_OVERLAY|"
     r"DTLS_RESILIENT|HTTP_ADVANCED|HTTP2_GUIDE|UDP_SUPPORT|"
     r"UDP_RELIABILITY|PORT_MANAGEMENT|CONNECTION_POOLING|"
     r"GRPC_GUIDE|ORM_GUIDE|AUTOSCALER_GUIDE|COLLECTOR_DEVELOPMENT|"
     r"EXPORTER_DEVELOPMENT|ADVANCED_ALERTS|ALERT_PIPELINE|"
     r"DIAGNOSTICS_METRICS|STORAGE_BACKENDS|QUEUE_SELECTION|"
     r"POLICY_QUEUE|VISITOR_PATTERN|CURRENT_STATE|"
     r"CRITICAL_LOG(?:GING)?|LOG_LEVEL|LOG_SERVER|"
     r"PLUGIN_DEVELOPMENT|PATTERNS$|DESIGN_DECISIONS|"
     r"UNIFIED_API|UNIFIED_SYSTEM|JOB_CANCELLATION|"
     r"THREAD_POOL_API|THREAD_SYSTEM$|SRS$|SDS|PRD$|"
     r"PREFETCH_QUEUE|PACS_IMPLEMENTATION|BACKEND_USAGE|"
     r"LEGACY_API$|DATABASE_BASE$|TRACING$|METRICS_AUDIT$|"
     r"INDEX$|README$", "GUID"),
    # FEAT — feature descriptions
    (r"FEATURES?$|FEATURE_FLAGS|DECORATOR_MIGRATION$", "FEAT"),
    # PROJ — project info
    (r"PROJECT_STRUCTURE|CONTRIBUTING|CODE_OF_CONDUCT|CHANGELOG|"
     r"RELEASING|DEPRECATION|IMPROVEMENT_PROPOSAL|DOCUMENTATION_GUIDELINES|"
     r"DOCUMENTATION_STATISTICS|TRANSLATION_SUMMARY|CHANGELOG_TEMPLATE|"
     r"CI_CD_GUIDE|CI_CD_DASHBOARD|LICENSE_COMPATIBILITY|HEADER_AUDIT|"
     r"COMPATIBILITY$|SOUP$|SOUP[-_]LIST|RUST_PARITY|"
     r"ARCHITECTURE_TEMPLATE|FEATURE_TEMPLATE|GUIDE_TEMPLATE|"
     r"DEPENDENCY_MATRIX|DEPENDENCY_COMPATIBILITY|DEPENDENCY_CONFLICT|"
     r"VALUE_CONTAINER_DECOMPOSITION|VARIANT_VALUE|"
     r"QUEUE_BACKWARD_COMPAT|THREAD_ADAPTER_EVAL|"
     r"PRODUCTION_GUIDE$", "PROJ"),
]

# Fallback: if no rule matches
DEFAULT_CATEGORY = "GUID"


def detect_project_name(project_root: Path) -> str:
    """Detect project name from directory name."""
    return project_root.name


def get_prefix(project_name: str) -> str:
    """Get the doc_id prefix for a project."""
    prefix = PREFIX_MAP.get(project_name)
    if not prefix:
        raise ValueError(
            f"Unknown project '{project_name}'. "
            f"Known projects: {', '.join(sorted(PREFIX_MAP))}"
        )
    return prefix


def normalize_stem(filepath: Path) -> str:
    """Extract the uppercase stem for category matching.

    Handles .kr.md double extensions by stripping both.
    """
    name = filepath.name
    if name.endswith(".kr.md"):
        stem = name[: -len(".kr.md")]
    else:
        stem = filepath.stem
    return stem.upper().replace("-", "_")


def infer_category(filepath: Path) -> str:
    """Infer document category from filename patterns."""
    stem = normalize_stem(filepath)

    # Also consider parent directory name for subdirectory files
    parent = filepath.parent.name.upper()

    for pattern, category in CATEGORY_RULES:
        if re.search(pattern, stem):
            return category

    # Subdirectory-based fallback
    subdir_map = {
        "ADVANCED": "ARCH",
        "ARCHITECTURE": "ARCH",
        "PERFORMANCE": "PERF",
        "GUIDES": "GUID",
        "CONTRIBUTING": "PROJ",
        "TEMPLATES": "PROJ",
        "SECURITY": "SECU",
        "INTEGRATION": "INTR",
        "MIGRATION": "MIGR",
        "DESIGN": "ARCH",
    }
    if parent in subdir_map:
        return subdir_map[parent]

    return DEFAULT_CATEGORY


def extract_title(content: str, filepath: Path) -> str:
    """Extract document title from the first markdown heading.

    Falls back to a humanized filename if no heading is found.
    """
    for line in content.split("\n"):
        line = line.strip()
        # Skip language selector lines
        if line.startswith(">") and ("Language" in line or "한국어" in line):
            continue
        match = re.match(r"^#\s+(.+)$", line)
        if match:
            title = match.group(1).strip()
            # Remove markdown links from title
            title = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", title)
            # Remove badge images
            title = re.sub(r"!\[[^\]]*\]\([^)]+\)", "", title).strip()
            if title:
                return title

    # Fallback: humanize filename
    stem = normalize_stem(filepath)
    return stem.replace("_", " ").title()


def has_frontmatter(content: str) -> bool:
    """Check if content already starts with YAML frontmatter."""
    return content.startswith("---\n") or content.startswith("---\r\n")


def has_standard_frontmatter(content: str) -> bool:
    """Check if content has our standardized frontmatter (with doc_id)."""
    if not has_frontmatter(content):
        return False
    # Look for doc_id field in the frontmatter block
    match = re.match(r"^---\n(.*?)\n---", content, re.DOTALL)
    if match:
        return "doc_id:" in match.group(1)
    return False


def build_frontmatter(
    doc_id: str,
    title: str,
    project: str,
    category: str,
    date: str,
) -> str:
    """Build the YAML frontmatter block."""
    # Escape quotes in title
    safe_title = title.replace('"', '\\"')
    return (
        f'---\n'
        f'doc_id: "{doc_id}"\n'
        f'doc_title: "{safe_title}"\n'
        f'doc_version: "1.0.0"\n'
        f'doc_date: "{date}"\n'
        f'doc_status: "Released"\n'
        f'project: "{project}"\n'
        f'category: "{category}"\n'
        f'---\n'
    )


def strip_existing_frontmatter(content: str) -> str:
    """Remove existing YAML frontmatter if present.

    This handles cases like Jekyll frontmatter that should be replaced.
    Only strips if the file starts with --- and has a closing ---.
    """
    if not has_frontmatter(content):
        return content
    # Don't strip if it's already our standard frontmatter
    if has_standard_frontmatter(content):
        return content
    match = re.match(r"^---\r?\n.*?\r?\n---\r?\n?", content, re.DOTALL)
    if match:
        return content[match.end():]
    return content


def process_project(
    project_root: Path,
    project_name: str | None = None,
    dry_run: bool = False,
    date: str = "2026-04-04",
) -> dict:
    """Process all docs/ markdown files in a project.

    Returns a summary dict with counts and details.
    """
    if project_name is None:
        project_name = detect_project_name(project_root)

    prefix = get_prefix(project_name)
    docs_dir = project_root / "docs"

    if not docs_dir.is_dir():
        return {
            "project": project_name,
            "prefix": prefix,
            "error": f"docs/ directory not found at {docs_dir}",
            "added": 0,
            "skipped": 0,
            "files": [],
        }

    # Collect all .md files
    md_files = sorted(docs_dir.rglob("*.md"))

    # Track category counters for sequential numbering
    category_counters: dict[str, int] = {}
    added = []
    skipped = []

    for filepath in md_files:
        content = filepath.read_text(encoding="utf-8")

        # Skip files that already have our standardized frontmatter
        if has_standard_frontmatter(content):
            skipped.append(str(filepath.relative_to(project_root)))
            continue

        # Strip non-standard frontmatter (e.g., Jekyll)
        clean_content = strip_existing_frontmatter(content)

        category = infer_category(filepath)
        title = extract_title(clean_content, filepath)

        # Generate sequential number within category
        category_counters.setdefault(category, 0)
        category_counters[category] += 1
        seq = category_counters[category]

        doc_id = f"{prefix}-{category}-{seq:03d}"

        frontmatter = build_frontmatter(
            doc_id=doc_id,
            title=title,
            project=project_name,
            category=category,
            date=date,
        )

        new_content = frontmatter + "\n" + clean_content

        rel_path = str(filepath.relative_to(project_root))
        added.append({
            "file": rel_path,
            "doc_id": doc_id,
            "category": category,
            "title": title,
        })

        if not dry_run:
            filepath.write_text(new_content, encoding="utf-8")

    return {
        "project": project_name,
        "prefix": prefix,
        "added": len(added),
        "skipped": len(skipped),
        "total": len(md_files),
        "files": added,
        "skipped_files": skipped,
        "category_counts": dict(sorted(category_counters.items())),
    }


def print_report(result: dict, verbose: bool = False) -> None:
    """Print a summary report for a processed project."""
    proj = result["project"]
    prefix = result["prefix"]

    if "error" in result:
        print(f"\n[ERROR] {proj} ({prefix}): {result['error']}")
        return

    added = result["added"]
    skipped = result["skipped"]
    total = result["total"]

    print(f"\n{'=' * 60}")
    print(f"Project: {proj} ({prefix})")
    print(f"{'=' * 60}")
    print(f"  Total .md files:  {total}")
    print(f"  Frontmatter added: {added}")
    print(f"  Already had FM:    {skipped}")

    if result.get("category_counts"):
        print(f"\n  Category breakdown:")
        for cat, count in result["category_counts"].items():
            print(f"    {cat:6s}: {count}")

    if verbose and result["files"]:
        print(f"\n  Files processed:")
        for f in result["files"]:
            print(f"    {f['doc_id']:16s}  {f['category']:6s}  {f['file']}")

    print()


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Add YAML frontmatter to docs/ markdown files."
    )
    parser.add_argument(
        "project_root",
        type=Path,
        help="Path to the project root directory",
    )
    parser.add_argument(
        "--project",
        type=str,
        default=None,
        help="Override project name (default: infer from directory name)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Preview changes without writing files",
    )
    parser.add_argument(
        "--date",
        type=str,
        default="2026-04-04",
        help="Date to use in frontmatter (default: 2026-04-04)",
    )
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Show detailed per-file output",
    )

    args = parser.parse_args()

    project_root = args.project_root.resolve()
    if not project_root.is_dir():
        print(f"Error: {project_root} is not a directory", file=sys.stderr)
        sys.exit(1)

    if args.dry_run:
        print("[DRY RUN] No files will be modified.\n")

    result = process_project(
        project_root=project_root,
        project_name=args.project,
        dry_run=args.dry_run,
        date=args.date,
    )

    print_report(result, verbose=args.verbose)

    if result.get("error"):
        sys.exit(1)

    # Verify no duplicates
    doc_ids = [f["doc_id"] for f in result["files"]]
    if len(doc_ids) != len(set(doc_ids)):
        dupes = [d for d in doc_ids if doc_ids.count(d) > 1]
        print(f"WARNING: Duplicate doc_ids found: {set(dupes)}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
