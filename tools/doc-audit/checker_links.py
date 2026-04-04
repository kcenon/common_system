"""Checker: Internal link validation."""

import re
from pathlib import Path
from urllib.parse import urlparse

from .config import CRITICAL, WARNING, LINK_PATTERN
from .models import Finding, CheckResult
from .utils import discover_md_files, read_file_lines, extract_headings


def check_links(docs_dir: Path) -> CheckResult:
    """Validate all internal markdown links in docs/ files."""
    result = CheckResult(checker_name="Links")
    md_files = discover_md_files(docs_dir)

    for md_file in md_files:
        rel = md_file.relative_to(docs_dir)
        lines = read_file_lines(md_file)

        in_code_block = False
        for line_num, line in enumerate(lines, start=1):
            stripped = line.strip()
            if stripped.startswith("```"):
                in_code_block = not in_code_block
                continue
            if in_code_block:
                continue

            for match in LINK_PATTERN.finditer(line):
                link_text = match.group(1)
                link_url = match.group(2)

                # Skip external URLs
                parsed = urlparse(link_url)
                if parsed.scheme in ("http", "https", "mailto", "ftp"):
                    continue

                # Split URL into path and anchor
                if "#" in link_url:
                    path_part, anchor = link_url.rsplit("#", 1)
                else:
                    path_part = link_url
                    anchor = None

                # Skip empty path (pure anchor reference within same file)
                if not path_part and anchor:
                    headings = extract_headings(md_file)
                    if anchor not in headings:
                        result.findings.append(Finding(
                            checker="Links",
                            severity=WARNING,
                            message=f"Broken anchor '#{anchor}' in same-file link",
                            file=str(rel),
                            line=line_num,
                        ))
                    continue

                if not path_part:
                    continue

                # Resolve relative path
                target = (md_file.parent / path_part).resolve()

                if not target.exists():
                    result.findings.append(Finding(
                        checker="Links",
                        severity=CRITICAL,
                        message=f"Broken link: [{link_text}]({link_url}) "
                                f"— target not found",
                        file=str(rel),
                        line=line_num,
                    ))
                    continue

                # Validate anchor in target file
                if anchor and target.suffix == ".md":
                    headings = extract_headings(target)
                    if anchor not in headings:
                        result.findings.append(Finding(
                            checker="Links",
                            severity=WARNING,
                            message=f"Broken anchor '#{anchor}' in "
                                    f"[{link_text}]({link_url})",
                            file=str(rel),
                            line=line_num,
                        ))

    return result
