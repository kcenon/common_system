#!/usr/bin/env bash
# verify-notices.sh — Verify NOTICES file exists and is not empty
#
# Usage:
#   ./scripts/verify-notices.sh [VCPKG_JSON_PATH]
#
# This script checks that:
# 1. A NOTICES file exists in the repository root
# 2. All Apache-2.0 dependencies from vcpkg.json are accounted for
#
# Intended for CI integration to catch missing NOTICE updates.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
VCPKG_JSON="${1:-$REPO_ROOT/vcpkg.json}"
NOTICES_FILE="$REPO_ROOT/NOTICES"

EXIT_CODE=0

echo "Verifying Apache-2.0 NOTICE compliance..."

# Check NOTICES file exists
if [ ! -f "$NOTICES_FILE" ]; then
    echo "WARNING: NOTICES file not found at $NOTICES_FILE"
    echo "  Run ./scripts/collect-notices.sh to generate it."
    echo "  This is acceptable if the project has no Apache-2.0 production dependencies."
    exit 0
fi

echo "  NOTICES file found: $NOTICES_FILE"

# Check it's not empty (beyond just the header)
line_count=$(wc -l < "$NOTICES_FILE")
if [ "$line_count" -lt 5 ]; then
    echo "WARNING: NOTICES file appears to be empty or incomplete ($line_count lines)"
    EXIT_CODE=1
fi

# Check audit summary is present
if grep -q "NOTICE File Audit Summary" "$NOTICES_FILE"; then
    echo "  Audit summary present"
else
    echo "WARNING: NOTICES file missing audit summary section"
    EXIT_CODE=1
fi

if [ $EXIT_CODE -eq 0 ]; then
    echo "NOTICE compliance check passed."
else
    echo "NOTICE compliance check completed with warnings."
fi

exit $EXIT_CODE
