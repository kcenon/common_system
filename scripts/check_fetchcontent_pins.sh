#!/usr/bin/env bash
# check_fetchcontent_pins.sh — Verify no FetchContent declaration uses branch refs
#
# Usage:
#   ./scripts/check_fetchcontent_pins.sh [SEARCH_DIR]
#
# Scans CMakeLists.txt and *.cmake files for FetchContent_Declare blocks
# that reference `main`, `master`, or `HEAD` in GIT_TAG — these produce
# non-reproducible builds and violate the ecosystem versioning policy.
#
# See: VERSIONING.md § Consumer Guide

set -euo pipefail

SEARCH_DIR="${1:-.}"
EXIT_CODE=0
VIOLATIONS=()

echo "Checking FetchContent GIT_TAG pins..."
echo "Search directory: $SEARCH_DIR"
echo ""

# Find all CMake files
while IFS= read -r cmake_file; do
    # Look for GIT_TAG with branch names instead of version tags
    while IFS= read -r line; do
        line_num=$(echo "$line" | cut -d: -f1)
        content=$(echo "$line" | cut -d: -f2-)

        # Check for branch references
        if echo "$content" | grep -qiE 'GIT_TAG\s+(main|master|HEAD|develop)\b'; then
            VIOLATIONS+=("$cmake_file:$line_num: $content")
            EXIT_CODE=1
        fi
    done < <(grep -n -i 'GIT_TAG' "$cmake_file" 2>/dev/null || true)
done < <(find "$SEARCH_DIR" \( -name "CMakeLists.txt" -o -name "*.cmake" \) -not -path "*/build/*" -not -path "*/.git/*" 2>/dev/null)

if [ ${#VIOLATIONS[@]} -gt 0 ]; then
    echo "VIOLATIONS FOUND:"
    echo ""
    for v in "${VIOLATIONS[@]}"; do
        echo "  $v"
    done
    echo ""
    echo "FetchContent GIT_TAG must use versioned tags (e.g., v0.1.0), not branch names."
    echo "See VERSIONING.md for the ecosystem versioning policy."
else
    echo "All FetchContent GIT_TAG references use versioned tags."
fi

exit $EXIT_CODE
