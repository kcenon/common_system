"""Utility functions for file discovery and path resolution."""

import os
import re
from pathlib import Path
from typing import Optional


def find_docs_dir(project_path: str) -> Optional[Path]:
    """Find the docs/ directory in a project."""
    docs_dir = Path(project_path) / "docs"
    if docs_dir.is_dir():
        return docs_dir
    return None


def discover_md_files(docs_dir: Path) -> list[Path]:
    """Discover all .md files in the docs directory recursively."""
    return sorted(docs_dir.rglob("*.md"))


def extract_frontmatter(filepath: Path) -> Optional[dict[str, str]]:
    """Extract YAML frontmatter from a markdown file.

    Returns a dict of key-value pairs, or None if no frontmatter found.
    """
    try:
        with open(filepath, "r", encoding="utf-8") as f:
            content = f.read()
    except (OSError, UnicodeDecodeError):
        return None

    if not content.startswith("---"):
        return None

    end_idx = content.find("---", 3)
    if end_idx == -1:
        return None

    fm_text = content[3:end_idx].strip()
    result = {}
    for line in fm_text.split("\n"):
        if ":" in line:
            key, val = line.split(":", 1)
            key = key.strip()
            val = val.strip().strip('"')
            result[key] = val

    return result


def read_file_lines(filepath: Path) -> list[str]:
    """Read a file and return its lines."""
    try:
        with open(filepath, "r", encoding="utf-8") as f:
            return f.readlines()
    except (OSError, UnicodeDecodeError):
        return []


def detect_project_name(project_path: str) -> str:
    """Detect the project name from the directory name."""
    return Path(project_path).name


def slugify(text: str) -> str:
    """Convert heading text to a markdown anchor slug."""
    text = text.lower().strip()
    text = re.sub(r"[^\w\s-]", "", text)
    text = re.sub(r"[\s]+", "-", text)
    text = re.sub(r"-+", "-", text)
    return text.strip("-")


def extract_headings(filepath: Path) -> list[str]:
    """Extract all markdown heading anchors from a file."""
    headings = []
    lines = read_file_lines(filepath)
    for line in lines:
        match = re.match(r"^#{1,6}\s+(.+)$", line.strip())
        if match:
            headings.append(slugify(match.group(1)))
    return headings
