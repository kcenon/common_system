#!/usr/bin/env python3
"""Single-source-of-truth (SSOT) drift guard for the error-code registry.

The centralized error-code registry exists in two physical forms:

  1. include/kcenon/common/error/error_codes.h  -- the classic header (SSOT)
  2. src/modules/error.cppm                      -- the C++20 module partition

The module partition re-declares the entire registry inline instead of
``#include``-ing the header (Apple Clang cannot build modules, so the header
cannot simply be imported there yet). That duplication is a silent drift risk:
a code added to or changed in the header but not mirrored in the ``.cppm``
(or vice versa) diverges between module-built and header-built consumers with
no compiler error.

This script parses both registries and fails (exit 1) when they disagree:

  * a ``constexpr int <name> = <expr>;`` present in one file but not the other
    (within the same namespace path), or
  * the same name resolving to a different integer value, or
  * a ``category`` enum entry present in one file but not the other, or with a
    different value.

It exits 0 when the two registries are identical, 1 on any divergence, and 2 on
a usage / IO error. Only the Python 3 standard library is used so the script
runs anywhere ``python3`` is available, with no build toolchain required.

The header is the authoritative SSOT: when reconciling, edit the ``.cppm`` to
match ``error_codes.h``, never the reverse.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

# --------------------------------------------------------------------------- #
# Locations (resolved relative to the repository root, which is the parent of
# the scripts/ directory this file lives in).
# --------------------------------------------------------------------------- #
REPO_ROOT = Path(__file__).resolve().parent.parent
HEADER_PATH = REPO_ROOT / "include" / "kcenon" / "common" / "error" / "error_codes.h"
MODULE_PATH = REPO_ROOT / "src" / "modules" / "error.cppm"

# --------------------------------------------------------------------------- #
# Parsing
# --------------------------------------------------------------------------- #
# A namespace opener. Handles both the classic header form ("namespace error {")
# and the module form that uses a qualified name on one line
# ("export namespace kcenon::common::error {"). The captured group is the
# "::"-joined namespace path opened by this single brace.
_NS_OPEN = re.compile(r"^\s*(?:export\s+)?namespace\s+([A-Za-z_][\w:]*)\s*\{")
# A constexpr int declaration: "constexpr int <name> = <expr>;".
_CONSTEXPR_INT = re.compile(r"^\s*constexpr\s+int\s+([A-Za-z_]\w*)\s*=\s*([^;]+);")
# A category enum entry: "name = value," inside the enum body.
_ENUM_ENTRY = re.compile(r"^\s*([A-Za-z_]\w*)\s*=\s*(-?\d+)\s*,?\s*$")

# Outer namespace segments that wrap the whole registry. Stripped from every
# dotted key so the two files compare on equal footing regardless of whether
# the wrapper is spelled as nested "namespace kcenon::common { namespace error"
# blocks (header) or a single "namespace kcenon::common::error" (module).
_OUTER_NS = ("kcenon", "common", "error")


def _strip_comment(line: str) -> str:
    """Remove a trailing // comment (registry files use no // inside exprs)."""
    idx = line.find("//")
    return line[:idx] if idx != -1 else line


def _evaluate(expr: str, namespace_consts: dict, category_values: dict) -> int:
    """Resolve an integer constexpr expression.

    Handles the forms that appear in the registry:
      * an integer literal,
      * ``static_cast<int>(category::X)`` referencing a category enumerator,
      * ``base - N`` / ``base + N`` where ``base`` is a constant already seen
        in the current namespace and N is an integer literal.
    Raises ValueError if it cannot resolve.
    """
    expr = expr.strip()

    # static_cast<int>(category::thread_system) -> the category enum value.
    m = re.match(r"static_cast<int>\(category::([A-Za-z_]\w*)\)", expr)
    if m:
        name = m.group(1)
        if name not in category_values:
            raise ValueError(f"unknown category enumerator: {name}")
        return category_values[name]

    # Plain integer literal.
    if re.fullmatch(r"-?\d+", expr):
        return int(expr)

    # "<name> - <int>" or "<name> + <int>" referencing a same-namespace const.
    m = re.fullmatch(r"([A-Za-z_]\w*)\s*([+-])\s*(\d+)", expr)
    if m:
        base_name, op, num = m.group(1), m.group(2), int(m.group(3))
        if base_name not in namespace_consts:
            raise ValueError(f"unresolved base reference: {base_name}")
        base_val = namespace_consts[base_name]
        return base_val + num if op == "+" else base_val - num

    raise ValueError(f"unparseable expression: {expr!r}")


def _normalize_key(segments: list, name: str) -> str:
    """Build the comparison key, stripping the outer wrapper namespace.

    ``segments`` is the active namespace path (e.g. ["kcenon", "common",
    "error", "codes", "thread_system"]). The leading ``_OUTER_NS`` wrapper is
    removed so the header and module spellings collapse to the same key
    (e.g. "codes.thread_system.pool_full").
    """
    path = list(segments)
    outer = list(_OUTER_NS)
    if path[: len(outer)] == outer:
        path = path[len(outer):]
    return ".".join(path + [name]) if path else name


def parse_registry(path: Path) -> dict:
    """Parse a registry file into a flat {normalized.name: value} map.

    Both ``constexpr int`` declarations and ``category`` enum entries are
    collected, keyed by their namespace path with the outer wrapper stripped.
    """
    if not path.is_file():
        raise FileNotFoundError(f"registry file not found: {path}")

    lines = path.read_text(encoding="utf-8").splitlines()

    results: dict = {}
    # Namespace frames: each entry is (segments_list, brace_depth_at_open).
    # A single opener line may declare several "::"-joined segments but still
    # opens exactly one brace, so the frame records all its segments together.
    ns_frames: list = []
    # Per-frame constant table for resolving "base - N" expressions, parallel
    # to ns_frames so it is popped together with its namespace.
    consts_frames: list = []
    # category enum values, shared across the file for static_cast resolution.
    category_values: dict = {}

    brace_depth = 0
    in_enum = False
    enum_open_depth = 0

    def active_segments() -> list:
        segs: list = []
        for frame_segs, _ in ns_frames:
            segs.extend(frame_segs)
        return segs

    def active_consts() -> dict:
        return consts_frames[-1] if consts_frames else {}

    for raw in lines:
        line = _strip_comment(raw)
        opens = line.count("{")
        closes = line.count("}")

        # --- category enum body -------------------------------------------- #
        if not in_enum and re.search(r"\benum\s+class\s+category\b", line):
            in_enum = True
            enum_open_depth = brace_depth
            brace_depth += opens - closes
            continue

        if in_enum:
            entry = _ENUM_ENTRY.match(line)
            if entry:
                name, val = entry.group(1), int(entry.group(2))
                category_values[name] = val
                results[f"category.{name}"] = val
            brace_depth += opens - closes
            if brace_depth <= enum_open_depth:
                in_enum = False
            continue

        # --- namespace open ------------------------------------------------ #
        ns = _NS_OPEN.match(line)
        if ns:
            segments = ns.group(1).split("::")
            ns_frames.append((segments, brace_depth))
            consts_frames.append({})
            brace_depth += 1
            continue

        # --- constexpr int declaration ------------------------------------- #
        decl = _CONSTEXPR_INT.match(line)
        if decl:
            name, expr = decl.group(1), decl.group(2).strip()
            value = _evaluate(expr, active_consts(), category_values)
            if consts_frames:
                consts_frames[-1][name] = value
            results[_normalize_key(active_segments(), name)] = value
            brace_depth += opens - closes
            continue

        # --- generic brace tracking / namespace close --------------------- #
        brace_depth += opens - closes
        # Pop any namespace frames whose opening depth is now above the
        # current brace depth (their closing brace was just consumed).
        while ns_frames and brace_depth <= ns_frames[-1][1]:
            ns_frames.pop()
            consts_frames.pop()

    return results


# --------------------------------------------------------------------------- #
# Comparison
# --------------------------------------------------------------------------- #
def diff_registries(header: dict, module: dict) -> list:
    """Return a list of human-readable divergence messages (empty if in sync)."""
    problems: list = []

    header_keys = set(header)
    module_keys = set(module)

    for name in sorted(header_keys - module_keys):
        problems.append(
            f"MISSING IN .cppm : {name} = {header[name]} "
            f"(present in error_codes.h, absent from error.cppm)"
        )
    for name in sorted(module_keys - header_keys):
        problems.append(
            f"EXTRA IN .cppm   : {name} = {module[name]} "
            f"(present in error.cppm, absent from error_codes.h)"
        )
    for name in sorted(header_keys & module_keys):
        if header[name] != module[name]:
            problems.append(
                f"VALUE MISMATCH   : {name} -> error_codes.h={header[name]} "
                f"vs error.cppm={module[name]}"
            )

    return problems


def main(argv: list) -> int:
    try:
        header = parse_registry(HEADER_PATH)
        module = parse_registry(MODULE_PATH)
    except (FileNotFoundError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

    # Defensive sanity check: a parser that silently extracts nothing must not
    # masquerade as "in sync". The registry always has dozens of entries.
    if not header or not module:
        print(
            "error: parsed an empty registry "
            f"(header={len(header)}, module={len(module)}); "
            "the parser or a source file is broken.",
            file=sys.stderr,
        )
        return 2

    problems = diff_registries(header, module)

    if problems:
        print("Error-code registry SSOT drift detected between:")
        print(f"  header : {HEADER_PATH.relative_to(REPO_ROOT)}  (SSOT)")
        print(f"  module : {MODULE_PATH.relative_to(REPO_ROOT)}")
        print()
        for p in problems:
            print(f"  {p}")
        print()
        print(
            "Reconcile by editing src/modules/error.cppm to match "
            "error_codes.h (the header is the SSOT)."
        )
        return 1

    print(
        f"Error-code registry SSOT OK: {len(header)} entries match between "
        "error_codes.h and error.cppm."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
