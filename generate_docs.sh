#!/bin/bash

# Documentation Generation Script
# Generates Doxygen documentation for all systems

set -e

SYSTEMS="common_system thread_system logger_system monitoring_system container_system database_system network_system"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(dirname "$SCRIPT_DIR")"

echo "=================================="
echo "System Documentation Generator"
echo "=================================="
echo ""

# Check if doxygen is installed
if ! command -v doxygen &> /dev/null; then
    echo "Error: doxygen is not installed"
    echo "Install with: brew install doxygen (macOS) or apt-get install doxygen (Linux)"
    exit 1
fi

echo "Doxygen version: $(doxygen --version)"
echo ""

SUCCESS_COUNT=0
SKIP_COUNT=0
FAIL_COUNT=0

for system in $SYSTEMS; do
    SYSTEM_PATH="$SOURCE_DIR/$system"
    
    if [ ! -d "$SYSTEM_PATH" ]; then
        echo "⚠️  $system: Directory not found, skipping"
        SKIP_COUNT=$((SKIP_COUNT + 1))
        continue
    fi
    
    cd "$SYSTEM_PATH"
    
    if [ ! -f "Doxyfile" ]; then
        echo "⚠️  $system: No Doxyfile found, skipping"
        SKIP_COUNT=$((SKIP_COUNT + 1))
        continue
    fi
    
    echo "📚 Generating documentation for $system..."
    
    if doxygen Doxyfile > /dev/null 2>&1; then
        echo "✅ $system: Documentation generated successfully"
        echo "   Output: $SYSTEM_PATH/docs/api/html/index.html"
        SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
    else
        echo "❌ $system: Documentation generation failed"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
    
    echo ""
done

echo "=================================="
echo "Summary"
echo "=================================="
echo "✅ Success: $SUCCESS_COUNT"
echo "⚠️  Skipped: $SKIP_COUNT"
echo "❌ Failed:  $FAIL_COUNT"
echo ""

if [ $SUCCESS_COUNT -gt 0 ]; then
    echo "Documentation available in:"
    for system in $SYSTEMS; do
        DOC_PATH="$SOURCE_DIR/$system/docs/api/html/index.html"
        if [ -f "$DOC_PATH" ]; then
            echo "  - $system: file://$DOC_PATH"
        fi
    done
fi

exit 0
