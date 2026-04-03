"""Checker: SSOT registry consistency."""

import re
from pathlib import Path

from .config import CRITICAL, WARNING, INFO
from .models import Finding, CheckResult
from .utils import discover_md_files, extract_frontmatter, read_file_lines


def _parse_registry_table(readme_path: Path) -> list[dict[str, str]]:
    """Parse the SSOT registry table from docs/README.md.

    Returns a list of dicts with keys: doc_id, topic, file_path, status.
    """
    entries = []
    lines = read_file_lines(readme_path)
    in_table = False
    header_seen = False

    for line in lines:
        line = line.strip()

        # Detect table header row
        if "doc_id" in line and "Authority Document" in line and "|" in line:
            in_table = True
            header_seen = False
            continue

        if in_table and line.startswith("|---"):
            header_seen = True
            continue

        if in_table and header_seen and line.startswith("|"):
            cols = [c.strip() for c in line.split("|")]
            # Filter empty strings from split
            cols = [c for c in cols if c]

            if len(cols) >= 4:
                doc_id = cols[1].strip()
                topic = cols[2].strip()

                # Extract file path from markdown link [name](./path)
                link_match = re.search(r"\]\(\./([^)]+)\)", cols[3])
                file_path = link_match.group(1) if link_match else ""

                status_val = cols[4].strip() if len(cols) >= 5 else ""

                entries.append({
                    "doc_id": doc_id,
                    "topic": topic,
                    "file_path": file_path,
                    "status": status_val,
                })
        elif in_table and header_seen and not line.startswith("|"):
            # End of table
            in_table = False
            header_seen = False

    return entries


def check_ssot(docs_dir: Path) -> CheckResult:
    """Validate SSOT registry consistency."""
    result = CheckResult(checker_name="SSOT")

    readme_path = docs_dir / "README.md"
    if not readme_path.exists():
        result.findings.append(Finding(
            checker="SSOT",
            severity=CRITICAL,
            message="Missing docs/README.md (SSOT registry)",
            file="README.md",
        ))
        return result

    # Parse registry table
    entries = _parse_registry_table(readme_path)

    if not entries:
        result.findings.append(Finding(
            checker="SSOT",
            severity=CRITICAL,
            message="No registry entries found in docs/README.md",
            file="README.md",
        ))
        return result

    # Collect all doc_ids from actual files
    actual_files: dict[str, str] = {}  # rel_path -> doc_id
    all_md_files = discover_md_files(docs_dir)

    for md_file in all_md_files:
        rel = str(md_file.relative_to(docs_dir))
        if rel == "README.md":
            continue
        fm = extract_frontmatter(md_file)
        if fm and "doc_id" in fm:
            actual_files[rel] = fm["doc_id"]

    # Check each registry entry
    registry_files: set[str] = set()
    registry_ids: set[str] = set()

    for entry in entries:
        file_path = entry["file_path"]
        doc_id = entry["doc_id"]
        registry_files.add(file_path)
        registry_ids.add(doc_id)

        # Check file exists
        target = docs_dir / file_path
        if not target.exists():
            result.findings.append(Finding(
                checker="SSOT",
                severity=CRITICAL,
                message=f"Registry entry '{doc_id}' points to "
                        f"non-existent file: {file_path}",
                file="README.md",
            ))
            continue

        # Check doc_id matches
        fm = extract_frontmatter(target)
        if fm:
            actual_id = fm.get("doc_id", "")
            if actual_id and actual_id != doc_id:
                result.findings.append(Finding(
                    checker="SSOT",
                    severity=WARNING,
                    message=f"doc_id mismatch: registry has '{doc_id}', "
                            f"file has '{actual_id}'",
                    file=file_path,
                ))

    # Check for files not listed in registry
    for rel_path, doc_id in actual_files.items():
        if rel_path not in registry_files:
            result.findings.append(Finding(
                checker="SSOT",
                severity=INFO,
                message=f"File '{rel_path}' (doc_id: {doc_id}) "
                        f"not listed in SSOT registry",
                file=rel_path,
            ))

    return result
