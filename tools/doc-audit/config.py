"""Constants and configuration for the documentation audit tool."""

import re

# Required YAML frontmatter fields
REQUIRED_FIELDS = [
    "doc_id",
    "doc_title",
    "doc_version",
    "doc_date",
    "doc_status",
    "project",
    "category",
]

# Valid doc_status values
VALID_STATUSES = {"Draft", "Review", "Released", "Deprecated", "Accepted"}

# Valid category values
VALID_CATEGORIES = {
    "ARCH", "API", "GUID", "PERF", "PROJ", "MIGR",
    "FEAT", "QUAL", "SECU", "INTR", "ADR",
}

# doc_id format: PREFIX-CATEGORY-NNN
DOC_ID_PATTERN = re.compile(r"^[A-Z]{2,4}-[A-Z]{2,4}-\d{3}$")

# Semver pattern (loose)
SEMVER_PATTERN = re.compile(r"^\d+\.\d+\.\d+$")

# ISO 8601 date pattern (YYYY-MM-DD)
DATE_PATTERN = re.compile(r"^\d{4}-\d{2}-\d{2}$")

# Markdown link pattern: [text](url)
LINK_PATTERN = re.compile(r"\[([^\]]*)\]\(([^)]+)\)")

# Standard README sections (from issue #561)
STANDARD_SECTIONS = [
    "Overview",
    "Architecture",
    "Getting Started",
    "API Reference",
    "Features",
    "Configuration",
    "Integration",
    "Performance",
    "Testing",
    "Troubleshooting",
    "Contributing",
    "Changelog",
    "License",
]

# Project prefixes
PROJECT_PREFIXES = {
    "common_system": "COM",
    "thread_system": "THR",
    "logger_system": "LOG",
    "container_system": "CNT",
    "monitoring_system": "MON",
    "database_system": "DBS",
    "network_system": "NET",
    "pacs_system": "PAC",
}

# Ecosystem projects
ECOSYSTEM_PROJECTS = list(PROJECT_PREFIXES.keys())

# Severity levels
CRITICAL = "critical"
WARNING = "warning"
INFO = "info"
