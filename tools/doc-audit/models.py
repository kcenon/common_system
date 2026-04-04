"""Data models for audit findings and results."""

from dataclasses import dataclass, field
from typing import Optional


@dataclass
class Finding:
    """A single audit finding."""
    checker: str
    severity: str  # "critical", "warning", "info"
    message: str
    file: str
    line: Optional[int] = None

    def __str__(self) -> str:
        loc = self.file
        if self.line is not None:
            loc = f"{self.file}:{self.line}"
        return f"[{self.severity.upper()}] {self.checker}: {loc} — {self.message}"


@dataclass
class CheckResult:
    """Result from a single checker run."""
    checker_name: str
    findings: list[Finding] = field(default_factory=list)

    @property
    def critical_count(self) -> int:
        return sum(1 for f in self.findings if f.severity == "critical")

    @property
    def warning_count(self) -> int:
        return sum(1 for f in self.findings if f.severity == "warning")

    @property
    def info_count(self) -> int:
        return sum(1 for f in self.findings if f.severity == "info")

    @property
    def total(self) -> int:
        return len(self.findings)


@dataclass
class AuditReport:
    """Complete audit report for a project."""
    project_name: str
    project_path: str
    results: list[CheckResult] = field(default_factory=list)

    @property
    def total_findings(self) -> int:
        return sum(r.total for r in self.results)

    @property
    def total_critical(self) -> int:
        return sum(r.critical_count for r in self.results)

    @property
    def total_warnings(self) -> int:
        return sum(r.warning_count for r in self.results)

    @property
    def total_info(self) -> int:
        return sum(r.info_count for r in self.results)

    @property
    def has_critical(self) -> bool:
        return self.total_critical > 0
