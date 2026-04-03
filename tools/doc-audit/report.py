"""Report generation for documentation audit results."""

import json
from datetime import date
from pathlib import Path

from .config import CRITICAL, WARNING, INFO
from .models import AuditReport, Finding


def _severity_icon(severity: str) -> str:
    """Return a text marker for severity level."""
    return {"critical": "[CRITICAL]", "warning": "[WARNING]", "info": "[INFO]"}.get(
        severity, "[?]"
    )


def generate_markdown(report: AuditReport) -> str:
    """Generate a Markdown-formatted audit report."""
    lines: list[str] = []
    lines.append(f"# Documentation Audit Report: {report.project_name}")
    lines.append("")
    lines.append(f"**Project**: `{report.project_path}`")
    lines.append(f"**Date**: {date.today().isoformat()}")
    lines.append("")

    # Summary
    lines.append("## Summary")
    lines.append("")
    lines.append(f"| Metric | Count |")
    lines.append(f"|--------|-------|")
    lines.append(f"| Total findings | {report.total_findings} |")
    lines.append(f"| Critical | {report.total_critical} |")
    lines.append(f"| Warnings | {report.total_warnings} |")
    lines.append(f"| Info | {report.total_info} |")
    lines.append("")

    if report.has_critical:
        lines.append(
            "**Result**: FAIL — critical issues found that must be resolved."
        )
    elif report.total_warnings > 0:
        lines.append("**Result**: PASS with warnings.")
    else:
        lines.append("**Result**: PASS — no issues found.")
    lines.append("")

    # Per-checker details
    for result in report.results:
        lines.append(f"## {result.checker_name} ({result.total} findings)")
        lines.append("")

        if not result.findings:
            lines.append("No issues found.")
            lines.append("")
            continue

        for finding in result.findings:
            loc = finding.file
            if finding.line is not None:
                loc = f"{finding.file}:{finding.line}"
            icon = _severity_icon(finding.severity)
            lines.append(f"- {icon} `{loc}` — {finding.message}")

        lines.append("")

    return "\n".join(lines)


def generate_json(report: AuditReport) -> str:
    """Generate a JSON-formatted audit report."""
    data = {
        "project_name": report.project_name,
        "project_path": report.project_path,
        "date": date.today().isoformat(),
        "summary": {
            "total": report.total_findings,
            "critical": report.total_critical,
            "warnings": report.total_warnings,
            "info": report.total_info,
            "pass": not report.has_critical,
        },
        "results": [],
    }

    for result in report.results:
        checker_data = {
            "checker": result.checker_name,
            "total": result.total,
            "critical": result.critical_count,
            "warnings": result.warning_count,
            "info": result.info_count,
            "findings": [],
        }
        for finding in result.findings:
            checker_data["findings"].append(
                {
                    "severity": finding.severity,
                    "message": finding.message,
                    "file": finding.file,
                    "line": finding.line,
                }
            )
        data["results"].append(checker_data)

    return json.dumps(data, indent=2)
