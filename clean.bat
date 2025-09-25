@echo off
setlocal enabledelayedexpansion

:: Clean script for common_system on Windows

echo ========================================
echo Cleaning common_system build artifacts
echo ========================================

:: Clean build directories
set BUILD_DIRS=build cmake-build-debug cmake-build-release cmake-build-relwithdebinfo

for %%d in (%BUILD_DIRS%) do (
    if exist "%%d" (
        echo Removing %%d...
        rmdir /s /q "%%d"
    )
)

:: Clean cache directories
if exist ".cache" (
    echo Removing .cache...
    rmdir /s /q ".cache"
)

if exist "vcpkg_installed" (
    echo Removing vcpkg_installed...
    rmdir /s /q "vcpkg_installed"
)

:: Clean temporary files
if exist "compile_commands.json" (
    echo Removing compile_commands.json...
    del /q "compile_commands.json"
)

:: Clean Visual Studio files
if exist ".vs" (
    echo Removing .vs directory...
    rmdir /s /q ".vs"
)

if exist "*.vcxproj.user" (
    echo Removing Visual Studio user files...
    del /q "*.vcxproj.user"
)

if exist "CMakeSettings.json" (
    echo Removing CMakeSettings.json...
    del /q "CMakeSettings.json"
)

:: Clean log files
if exist "*.log" (
    echo Removing log files...
    del /q "*.log"
)

:: Clean test outputs
if exist "tests\output" (
    echo Removing test output directory...
    rmdir /s /q "tests\output"
)

:: Clean benchmark results
if exist "benchmark_results" (
    echo Removing benchmark results...
    rmdir /s /q "benchmark_results"
)

:: Clean documentation if generated
if exist "docs\html" (
    echo Removing generated documentation...
    rmdir /s /q "docs\html"
)

if exist "docs\latex" (
    echo Removing generated LaTeX documentation...
    rmdir /s /q "docs\latex"
)

echo ========================================
echo Clean completed successfully!
echo ========================================