# Setup vcpkg Composite Action

Reusable GitHub Actions composite action that bootstraps vcpkg pinned to the kcenon ecosystem baseline with intelligent caching.

## Features

- Pins vcpkg to a specific commit (default: ecosystem baseline) for reproducible builds
- Caches vcpkg installation (excluding buildtrees, packages, downloads)
- Sets `VCPKG_ROOT` and `CMAKE_TOOLCHAIN_FILE` environment variables
- Cross-platform: Ubuntu, macOS, Windows

## Usage

### Basic (uses ecosystem default baseline)

```yaml
steps:
  - uses: actions/checkout@v4
  - uses: kcenon/common_system/.github/actions/setup-vcpkg@main
  - run: cmake -B build -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE
```

### With custom cache key

```yaml
steps:
  - uses: actions/checkout@v4
  - uses: kcenon/common_system/.github/actions/setup-vcpkg@main
    with:
      extra-cache-key: 'my-project'
```

### With custom vcpkg commit

```yaml
steps:
  - uses: actions/checkout@v4
  - uses: kcenon/common_system/.github/actions/setup-vcpkg@main
    with:
      vcpkg-commit: 'abc123def456'
```

### Using outputs

```yaml
steps:
  - uses: actions/checkout@v4
  - uses: kcenon/common_system/.github/actions/setup-vcpkg@main
    id: vcpkg
  - run: |
      echo "VCPKG_ROOT=${{ steps.vcpkg.outputs.vcpkg-root }}"
      echo "Toolchain=${{ steps.vcpkg.outputs.toolchain-file }}"
```

## Inputs

| Input | Required | Default | Description |
|-------|----------|---------|-------------|
| `vcpkg-commit` | No | Ecosystem baseline | vcpkg commit SHA to checkout |
| `manifest-dir` | No | `.` | Directory containing `vcpkg.json` |
| `extra-cache-key` | No | `''` | Additional cache key suffix |

## Outputs

| Output | Description |
|--------|-------------|
| `vcpkg-root` | Path to vcpkg installation |
| `toolchain-file` | Path to `vcpkg.cmake` |

## Environment Variables Set

- `VCPKG_ROOT` — vcpkg installation path
- `CMAKE_TOOLCHAIN_FILE` — path to vcpkg CMake toolchain
