#!/bin/bash

# SOUP Version Drift Detection Script Wrapper
# Detects mismatches between vcpkg.json overrides and FetchContent GIT_TAGs.
# See: https://github.com/kcenon/common_system/issues/409

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Check for Python 3
if command -v python3 &> /dev/null; then
    python3 "${SCRIPT_DIR}/check_version_drift.py" "$@"
elif command -v python &> /dev/null; then
    python "${SCRIPT_DIR}/check_version_drift.py" "$@"
else
    echo "Error: Python 3 is required but not found"
    exit 1
fi
