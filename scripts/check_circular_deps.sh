#!/bin/bash

# Circular Dependency Detection Script Wrapper for common_system
# This script wraps the Python implementation for cross-platform compatibility

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Check for Python 3
if command -v python3 &> /dev/null; then
    python3 "${SCRIPT_DIR}/check_circular_deps.py" "$@"
elif command -v python &> /dev/null; then
    python "${SCRIPT_DIR}/check_circular_deps.py" "$@"
else
    echo "Error: Python 3 is required but not found"
    exit 1
fi
