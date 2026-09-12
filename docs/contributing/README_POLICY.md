---
doc_id: "COM-GUID-029"
doc_title: "README Evidence Policy"
doc_version: "1.0.0"
doc_date: "2026-09-12"
doc_status: "Released"
project: "common_system"
category: "GUID"
---

# README Evidence Policy

The root [English](../../README.md) and [Korean](../../README.kr.md) READMEs are
entry points. Keep setup usable and link to detailed guides. Each file must be
nonempty and at most 300 physical lines, including code, comments, and blank lines.
Do not compress unrelated paragraphs or add filler to meet a numerical target.

## Local Check

Run from the repository root:

```sh
python3 -m unittest discover -s scripts/tests -p 'test_readme_lint.py'
python3 scripts/readme_lint.py
```

The [standard-library linter](../../scripts/readme_lint.py) defaults to both root
READMEs, resolves paths from its own repository, and accepts explicit file paths
and `--root` for other checkouts or fixtures. Findings use `path:line: rule: message`.
Violations and unreadable files return a nonzero status. It never requests network
access. [Documentation Audit](../../.github/workflows/doc-audit.yml) runs the tests
and linter as failing steps, including when workflows or linter files change.

## Status, Release, and Badges

Show a title, then `Status: active`, `Status: maintenance`, or
`Status: experimental` and a linked release version within the first 20 lines.
The maintenance classification describes project activity, not a quality guarantee.
The linter checks the format and matching release URL; a reviewer must separately
verify the current published release against GitHub. Distinguish staged metadata
from published releases.

Use badges only for existing workflows in this repository. Their image and click
URLs must identify the same `.yml` or `.yaml` file in `.github/workflows/`.
Query parameters are allowed. Markdown images, reference-style badges, and HTML
`<a><img></a>` badges are checked. Ordinary images and text links are not badges;
badge URLs/services or an explicit badge label identify badge images.
Keep license and service links as prose instead of adding nonworkflow badges.

## Claims and Density

Remove promotional qualifiers, including production-ready, enterprise-grade,
battle-tested, blazing, world-class, comprehensive, robust, seamless, 100%,
guaranteed, zero-overhead/warning/leak/race variants, unsupported quality grades,
and their Korean equivalents. A citation does not exempt a banned qualifier.

The linter recognizes speedup multipliers, latency and throughput units,
percentages, test pass counts, and allocation counts in visible prose. Measurement
table headers give context to bare numeric cells; such tables belong in benchmark
documentation even when sourced. Keep measured coverage and sanitizer results on
their workflow pages, with a specific run if making a claim about an outcome.

Versions, C++20, error-code ranges, dates, list numbers, link destinations, and
fenced or indented code examples are not measurements. Inline-code figures and
visible link/image labels still count. Backtick and tilde fences inside lists or
blockquotes, and multiline HTML comments, are recognized. A configured percentage
needs an explicit policy/target/threshold label and a link on the same line to an
existing local configuration file; observed results cannot use this exception.

Density is `100 * distinct prose lines containing a qualifier or measurement /
total physical lines`. A line counts once even with several matches; sourced
figures still count. The maximum is 1.0 per 100 lines. Aim for no such lines in
either README. A wrapped figure may occupy several lines; this is a line metric,
not a count of English or Korean sentences.

## Optional Headline Source Marker

Prefer a link to [benchmark evidence](../BENCHMARKS.md) without a headline figure.
A future figure may use this exact marker form:

```markdown
Measured claim. <!-- source: docs/BENCHMARKS.md#recorded-snapshot (2026-09-12, Ubuntu / GCC) -->
```

Use a real measurement in place of the placeholder. Put the marker on the same
line, or on a comment-only line immediately before or after exactly one claim.
Blank lines, intervening prose, ambiguous neighboring claims, and markers inside
code do not provide an exemption. The date must be valid, the environment nonempty,
and the repository-local Markdown document and anchor must exist.

The linked section must identify the environment, measurement date, exact command,
and retained raw result file. Reviewers must verify that these support the named
workload and statistic in the claim. The parser checks marker structure and local
links, not scientific validity or semantic agreement with benchmark results.

The focused Markdown parser and claim patterns are regression checks, not a proof
that arbitrary wording is accurate. Review new syntax and new claim forms when
editing either README, and add behavior tests when the policy needs to recognize them.
