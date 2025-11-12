@echo off
setlocal enabledelayedexpansion

:: Test script for common_system on Windows

set BUILD_DIR=build
set BUILD_TYPE=Debug
set VERBOSE=OFF
set COVERAGE=OFF
set BENCHMARK=OFF
set FILTER=
set OUTPUT_FORMAT=console

:: Parse command line arguments
:parse_args
if "%~1"=="" goto :end_parse
if /i "%~1"=="--release" (
    set BUILD_TYPE=Release
    shift
    goto :parse_args
)
if /i "%~1"=="--verbose" (
    set VERBOSE=ON
    shift
    goto :parse_args
)
if /i "%~1"=="-v" (
    set VERBOSE=ON
    shift
    goto :parse_args
)
if /i "%~1"=="--coverage" (
    set COVERAGE=ON
    shift
    goto :parse_args
)
if /i "%~1"=="--benchmark" (
    set BENCHMARK=ON
    shift
    goto :parse_args
)
if /i "%~1"=="--filter" (
    set FILTER=%~2
    shift
    shift
    goto :parse_args
)
if /i "%~1"=="--junit" (
    set OUTPUT_FORMAT=junit
    shift
    goto :parse_args
)
if /i "%~1"=="--build-dir" (
    set BUILD_DIR=%~2
    shift
    shift
    goto :parse_args
)
if /i "%~1"=="--help" (
    goto :show_help
)
echo Unknown option: %~1
echo Use --help for usage information
exit /b 1

:end_parse

echo ========================================
echo Running common_system tests
echo ========================================
echo Configuration:
echo   Build Type:     %BUILD_TYPE%
echo   Build Dir:      %BUILD_DIR%
echo   Verbose:        %VERBOSE%
echo   Coverage:       %COVERAGE%
echo   Benchmarks:     %BENCHMARK%
if defined FILTER (
    echo   Filter:         %FILTER%
)
echo ========================================

:: Check if build directory exists
if not exist "%BUILD_DIR%" (
    echo Error: Build directory '%BUILD_DIR%' not found
    echo Please run build.bat first
    exit /b 1
)

cd "%BUILD_DIR%"

:: Check if tests are built
set TEST_EXE=
if exist "tests\%BUILD_TYPE%\common_system_tests.exe" (
    set TEST_EXE=tests\%BUILD_TYPE%\common_system_tests.exe
) else if exist "tests\common_system_tests.exe" (
    set TEST_EXE=tests\common_system_tests.exe
)

if not defined TEST_EXE (
    echo Tests not found. Building tests...
    cmake --build . --config %BUILD_TYPE% --target common_system_tests
    if errorlevel 1 (
        echo Failed to build tests
        cd ..
        exit /b 1
    )
)

:: Run unit tests
echo Running unit tests...

set CTEST_ARGS=-C %BUILD_TYPE%

if "%VERBOSE%"=="ON" (
    set CTEST_ARGS=%CTEST_ARGS% --verbose
) else (
    set CTEST_ARGS=%CTEST_ARGS% --output-on-failure
)

if defined FILTER (
    set CTEST_ARGS=%CTEST_ARGS% -R "%FILTER%"
)

if "%OUTPUT_FORMAT%"=="junit" (
    set CTEST_ARGS=%CTEST_ARGS% --output-junit test_results.xml
)

:: Run tests
ctest %CTEST_ARGS%
set TEST_RESULT=%errorlevel%

:: Run benchmarks if requested
if "%BENCHMARK%"=="ON" (
    echo Running benchmarks...

    set BENCH_EXE=
    if exist "benchmarks\%BUILD_TYPE%\common_system_benchmarks.exe" (
        set BENCH_EXE=benchmarks\%BUILD_TYPE%\common_system_benchmarks.exe
    ) else if exist "benchmarks\common_system_benchmarks.exe" (
        set BENCH_EXE=benchmarks\common_system_benchmarks.exe
    )

    if defined BENCH_EXE (
        !BENCH_EXE! --benchmark_out=benchmark_results.json --benchmark_out_format=json
    ) else (
        echo Benchmarks not found
    )
)

:: Generate coverage report if requested
if "%COVERAGE%"=="ON" (
    echo Coverage reporting is not yet implemented for Windows
    echo Consider using WSL or a CI/CD pipeline for coverage analysis
)

cd ..

:: Print summary
echo ========================================
if %TEST_RESULT% equ 0 (
    echo All tests passed!
) else (
    echo Some tests failed!
)
echo ========================================

:: Show test report location if JUnit format was used
if "%OUTPUT_FORMAT%"=="junit" (
    if exist "%BUILD_DIR%\test_results.xml" (
        echo Test results saved to: %BUILD_DIR%\test_results.xml
    )
)

exit /b %TEST_RESULT%

:show_help
echo Usage: %~nx0 [options]
echo Options:
echo   --release        Run tests in Release mode
echo   --verbose, -v    Enable verbose output
echo   --coverage       Generate code coverage report (Linux/Mac only)
echo   --benchmark      Run benchmarks
echo   --filter PATTERN Filter tests by pattern
echo   --junit          Output in JUnit XML format
echo   --build-dir DIR  Specify build directory (default: build)
echo   --help           Show this help message
exit /b 0