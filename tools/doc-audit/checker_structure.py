"""Checker: README standard section compliance."""

import re
from pathlib import Path

from .config import CRITICAL, WARNING, INFO, STANDARD_SECTIONS
from .models import Finding, CheckResult
from .utils import read_file_lines


def _extract_h2_sections(filepath: Path) -> list[str]:
    """Extract all H2 section titles from a markdown file."""
    sections = []
    lines = read_file_lines(filepath)
    for line in lines:
        match = re.match(r"^##\s+(.+)$", line.strip())
        if match:
            sections.append(match.group(1).strip())
    return sections


def _normalize_section(name: str) -> str:
    """Normalize section name for comparison."""
    # Remove emojis and special characters, lowercase
    name = re.sub(r"[^\w\s]", "", name).strip().lower()
    return name


def check_structure(docs_dir: Path) -> CheckResult:
    """Validate README.md compliance with the 13-section standard."""
    result = CheckResult(checker_name="Structure")

    # Check the project root README.md (one level up from docs/)
    project_dir = docs_dir.parent
    readme_path = project_dir / "README.md"

    if not readme_path.exists():
        result.findings.append(Finding(
            checker="Structure",
            severity=CRITICAL,
            message="Missing project README.md",
            file="README.md",
        ))
        return result

    actual_sections = _extract_h2_sections(readme_path)
    actual_normalized = [_normalize_section(s) for s in actual_sections]

    # Check for each standard section
    for standard_section in STANDARD_SECTIONS:
        norm = _normalize_section(standard_section)
        found = False
        for actual_norm in actual_normalized:
            if norm in actual_norm or actual_norm in norm:
                found = True
                break

        if not found:
            # Determine severity based on section importance
            if standard_section in ("Overview", "Getting Started", "License"):
                severity = WARNING
            else:
                severity = INFO

            result.findings.append(Finding(
                checker="Structure",
                severity=severity,
                message=f"Missing standard section: '## {standard_section}'",
                file="README.md",
            ))

    return result
