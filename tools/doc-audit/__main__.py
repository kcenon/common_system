"""CLI entry point for the documentation audit tool.

Usage:
    python -m doc-audit /path/to/project
    python -m doc-audit --ecosystem /path/to/sources
    python -m doc-audit --quick /path/to/project
    python -m doc-audit --format json /path/to/project
"""

import argparse
import sys
from pathlib import Path

from .config import ECOSYSTEM_PROJECTS
from .models import AuditReport
from .utils import find_docs_dir, detect_project_name
from .checker_metadata import check_metadata
from .checker_links import check_links
from .checker_ssot import check_ssot
from .checker_structure import check_structure
from .report import generate_markdown, generate_json


def run_audit(project_path: str, quick: bool = False) -> AuditReport:
    """Run all audit checks on a single project."""
    project_name = detect_project_name(project_path)
    report = AuditReport(project_name=project_name, project_path=project_path)

    docs_dir = find_docs_dir(project_path)
    if docs_dir is None:
        print(f"  [SKIP] {project_name}: no docs/ directory found", file=sys.stderr)
        return report

    # Always run metadata and structure checks
    report.results.append(check_metadata(docs_dir))
    report.results.append(check_structure(docs_dir))

    if not quick:
        report.results.append(check_links(docs_dir))
        report.results.append(check_ssot(docs_dir))

    return report


def print_report(report: AuditReport, fmt: str) -> None:
    """Print a single project report in the requested format."""
    if fmt == "json":
        print(generate_json(report))
    elif fmt == "markdown":
        print(generate_markdown(report))
    else:  # both
        print(generate_markdown(report))
        print("\n--- JSON ---\n")
        print(generate_json(report))


def main() -> int:
    parser = argparse.ArgumentParser(
        prog="doc-audit",
        description="Documentation audit tool for the kcenon ecosystem.",
    )
    parser.add_argument(
        "path",
        help="Project path (single project) or sources root (with --ecosystem)",
    )
    parser.add_argument(
        "--quick",
        action="store_true",
        help="Run only metadata and structure checks (skip links and SSOT)",
    )
    parser.add_argument(
        "--format",
        choices=["markdown", "json", "both"],
        default="markdown",
        help="Output format (default: markdown)",
    )
    parser.add_argument(
        "--ecosystem",
        action="store_true",
        help="Audit all ecosystem projects under the given root path",
    )

    args = parser.parse_args()
    root = Path(args.path).resolve()

    if not root.is_dir():
        print(f"Error: '{root}' is not a directory", file=sys.stderr)
        return 2

    reports: list[AuditReport] = []

    if args.ecosystem:
        for project_name in ECOSYSTEM_PROJECTS:
            project_dir = root / project_name
            if project_dir.is_dir():
                print(f"Auditing {project_name}...", file=sys.stderr)
                reports.append(run_audit(str(project_dir), quick=args.quick))
            else:
                print(f"  [SKIP] {project_name}: directory not found", file=sys.stderr)
    else:
        reports.append(run_audit(str(root), quick=args.quick))

    # Print reports
    for report in reports:
        print_report(report, args.format)

    # Summary for ecosystem mode
    if args.ecosystem and len(reports) > 1:
        total_c = sum(r.total_critical for r in reports)
        total_w = sum(r.total_warnings for r in reports)
        total_i = sum(r.total_info for r in reports)
        total = sum(r.total_findings for r in reports)
        print(
            f"\n=== Ecosystem Summary: {total} findings "
            f"({total_c} critical, {total_w} warnings, {total_i} info) ===",
            file=sys.stderr,
        )

    # Exit code: 1 if any critical findings
    has_critical = any(r.has_critical for r in reports)
    return 1 if has_critical else 0


if __name__ == "__main__":
    sys.exit(main())
