"""Checker: YAML frontmatter validation."""

from pathlib import Path

from .config import (
    CRITICAL, WARNING, INFO,
    REQUIRED_FIELDS, VALID_STATUSES, VALID_CATEGORIES,
    DOC_ID_PATTERN, SEMVER_PATTERN, DATE_PATTERN,
)
from .models import Finding, CheckResult
from .utils import discover_md_files, extract_frontmatter


def check_metadata(docs_dir: Path) -> CheckResult:
    """Validate YAML frontmatter for all markdown files in docs/."""
    result = CheckResult(checker_name="Metadata")
    md_files = discover_md_files(docs_dir)
    seen_ids: dict[str, Path] = {}

    for md_file in md_files:
        rel = md_file.relative_to(docs_dir)
        fm = extract_frontmatter(md_file)

        if fm is None:
            result.findings.append(Finding(
                checker="Metadata",
                severity=WARNING,
                message="Missing YAML frontmatter",
                file=str(rel),
            ))
            continue

        # Check required fields
        for field_name in REQUIRED_FIELDS:
            if field_name not in fm or not fm[field_name]:
                result.findings.append(Finding(
                    checker="Metadata",
                    severity=CRITICAL if field_name == "doc_id" else WARNING,
                    message=f"Missing required field: {field_name}",
                    file=str(rel),
                ))

        # Validate doc_id format
        doc_id = fm.get("doc_id", "")
        if doc_id:
            if not DOC_ID_PATTERN.match(doc_id):
                result.findings.append(Finding(
                    checker="Metadata",
                    severity=WARNING,
                    message=f"Invalid doc_id format: '{doc_id}' "
                            f"(expected PREFIX-CATEGORY-NNN)",
                    file=str(rel),
                ))

            # Check for duplicate doc_id
            if doc_id in seen_ids:
                result.findings.append(Finding(
                    checker="Metadata",
                    severity=CRITICAL,
                    message=f"Duplicate doc_id '{doc_id}' "
                            f"(also in {seen_ids[doc_id]})",
                    file=str(rel),
                ))
            else:
                seen_ids[doc_id] = rel

        # Validate doc_version (semver)
        version = fm.get("doc_version", "")
        if version and not SEMVER_PATTERN.match(version):
            result.findings.append(Finding(
                checker="Metadata",
                severity=INFO,
                message=f"Invalid version format: '{version}' (expected X.Y.Z)",
                file=str(rel),
            ))

        # Validate doc_date (ISO 8601)
        date = fm.get("doc_date", "")
        if date and not DATE_PATTERN.match(date):
            result.findings.append(Finding(
                checker="Metadata",
                severity=INFO,
                message=f"Invalid date format: '{date}' (expected YYYY-MM-DD)",
                file=str(rel),
            ))

        # Validate doc_status
        status = fm.get("doc_status", "")
        if status and status not in VALID_STATUSES:
            result.findings.append(Finding(
                checker="Metadata",
                severity=WARNING,
                message=f"Invalid status: '{status}' "
                        f"(expected one of: {', '.join(sorted(VALID_STATUSES))})",
                file=str(rel),
            ))

        # Validate category
        category = fm.get("category", "")
        if category and category not in VALID_CATEGORIES:
            result.findings.append(Finding(
                checker="Metadata",
                severity=INFO,
                message=f"Non-standard category: '{category}' "
                        f"(known: {', '.join(sorted(VALID_CATEGORIES))})",
                file=str(rel),
            ))

    return result
