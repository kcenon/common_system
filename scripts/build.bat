@echo off
setlocal enabledelayedexpansion

:: Build script for common_system on Windows
set PROJECT_NAME=common_system
set BUILD_DIR=build
set INSTALL_PREFIX=%USERPROFILE%\.local
set BUILD_TYPE=Release
set BUILD_TESTS=ON
set BUILD_EXAMPLES=ON
set BUILD_BENCHMARKS=OFF
set HEADER_ONLY=ON
set VERBOSE=OFF
set CLEAN_BUILD=OFF
set GENERATOR=Visual Studio 17 2022

:: Check for vcpkg
if not defined VCPKG_ROOT (
    if exist "%USERPROFILE%\vcpkg" (
        set VCPKG_ROOT=%USERPROFILE%\vcpkg
    ) else if exist "C:\vcpkg" (
        set VCPKG_ROOT=C:\vcpkg
    ) else if exist "C:\tools\vcpkg" (
        set VCPKG_ROOT=C:\tools\vcpkg
    )
)

:: Parse command line arguments
:parse_args
if "%~1"=="" goto :end_parse
if /i "%~1"=="--debug" (
    set BUILD_TYPE=Debug
    shift
    goto :parse_args
)
if /i "%~1"=="--release" (
    set BUILD_TYPE=Release
    shift
    goto :parse_args
)
if /i "%~1"=="--relwithdebinfo" (
    set BUILD_TYPE=RelWithDebInfo
    shift
    goto :parse_args
)
if /i "%~1"=="--no-tests" (
    set BUILD_TESTS=OFF
    shift
    goto :parse_args
)
if /i "%~1"=="--no-examples" (
    set BUILD_EXAMPLES=OFF
    shift
    goto :parse_args
)
if /i "%~1"=="--benchmarks" (
    set BUILD_BENCHMARKS=ON
    shift
    goto :parse_args
)
if /i "%~1"=="--static" (
    set HEADER_ONLY=OFF
    shift
    goto :parse_args
)
if /i "%~1"=="--clean" (
    set CLEAN_BUILD=ON
    shift
    goto :parse_args
)
if /i "%~1"=="--verbose" (
    set VERBOSE=ON
    shift
    goto :parse_args
)
if /i "%~1"=="--install-prefix" (
    set INSTALL_PREFIX=%~2
    shift
    shift
    goto :parse_args
)
if /i "%~1"=="--vcpkg-root" (
    set VCPKG_ROOT=%~2
    shift
    shift
    goto :parse_args
)
if /i "%~1"=="--vs2019" (
    set GENERATOR=Visual Studio 16 2019
    shift
    goto :parse_args
)
if /i "%~1"=="--vs2022" (
    set GENERATOR=Visual Studio 17 2022
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

:: Print configuration
echo ========================================
echo Building %PROJECT_NAME%
echo ========================================
echo Build Configuration:
echo   Build Type:        %BUILD_TYPE%
echo   Build Directory:   %BUILD_DIR%
echo   Install Prefix:    %INSTALL_PREFIX%
echo   Header Only:       %HEADER_ONLY%
echo   Build Tests:       %BUILD_TESTS%
echo   Build Examples:    %BUILD_EXAMPLES%
echo   Build Benchmarks:  %BUILD_BENCHMARKS%
echo   Generator:         %GENERATOR%
echo   Verbose:           %VERBOSE%
if defined VCPKG_ROOT (
    echo   vcpkg Root:        %VCPKG_ROOT%
)
echo ========================================

:: Check for required tools
echo Checking for required tools...

where cmake >nul 2>&1
if errorlevel 1 (
    echo Error: cmake is not installed or not in PATH
    echo Please install CMake from https://cmake.org/download/
    exit /b 1
)

:: Check for Visual Studio
if "%GENERATOR:~0,13%"=="Visual Studio" (
    echo Using %GENERATOR%
)

:: Check for vcpkg
if defined VCPKG_ROOT (
    if exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
        echo Found vcpkg at %VCPKG_ROOT%
        set CMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
    ) else (
        echo Warning: vcpkg toolchain file not found
        set CMAKE_TOOLCHAIN_FILE=
    )
) else (
    echo Warning: vcpkg not found, dependencies must be installed manually
    set CMAKE_TOOLCHAIN_FILE=
)

:: Clean build directory if requested
if "%CLEAN_BUILD%"=="ON" (
    echo Cleaning build directory...
    if exist "%BUILD_DIR%" (
        rmdir /s /q "%BUILD_DIR%"
    )
)

:: Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

:: Configure with CMake
echo Configuring with CMake...

set CMAKE_ARGS=-G"%GENERATOR%" ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL_PREFIX%" ^
    -DCOMMON_BUILD_TESTS=%BUILD_TESTS% ^
    -DCOMMON_BUILD_EXAMPLES=%BUILD_EXAMPLES% ^
    -DCOMMON_BUILD_BENCHMARKS=%BUILD_BENCHMARKS% ^
    -DCOMMON_HEADER_ONLY=%HEADER_ONLY% ^
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if defined CMAKE_TOOLCHAIN_FILE (
    set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_TOOLCHAIN_FILE="%CMAKE_TOOLCHAIN_FILE%"
)

if "%VERBOSE%"=="ON" (
    set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_VERBOSE_MAKEFILE=ON
)

cmake %CMAKE_ARGS% ..

if errorlevel 1 (
    echo CMake configuration failed
    cd ..
    exit /b 1
)

:: Build
echo Building...

if "%VERBOSE%"=="ON" (
    cmake --build . --config %BUILD_TYPE% --verbose
) else (
    cmake --build . --config %BUILD_TYPE%
)

if errorlevel 1 (
    echo Build failed
    cd ..
    exit /b 1
)

:: Run tests if built
if "%BUILD_TESTS%"=="ON" (
    echo Running tests...
    if exist "tests\%BUILD_TYPE%\common_system_tests.exe" (
        ctest -C %BUILD_TYPE% --output-on-failure
    ) else (
        echo No tests found to run
    )
)

cd ..

echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo To install the library, run:
echo   cd %BUILD_DIR% ^&^& cmake --build . --target install
echo.
echo To use in your project:
echo   find_package(common_system REQUIRED)
echo   target_link_libraries(your_target PRIVATE kcenon::common)
echo.

exit /b 0

:show_help
echo Usage: %~nx0 [options]
echo Options:
echo   --debug              Build in Debug mode
echo   --release            Build in Release mode (default)
echo   --relwithdebinfo     Build in RelWithDebInfo mode
echo   --no-tests           Don't build tests
echo   --no-examples        Don't build examples
echo   --benchmarks         Build benchmarks
echo   --static             Build as static library (default: header-only)
echo   --clean              Clean build directory before building
echo   --verbose            Enable verbose output
echo   --install-prefix DIR Set installation prefix (default: %%USERPROFILE%%\.local)
echo   --vcpkg-root DIR     Set vcpkg root directory
echo   --vs2019             Use Visual Studio 2019
echo   --vs2022             Use Visual Studio 2022 (default)
echo   --help               Show this help message
exit /b 0