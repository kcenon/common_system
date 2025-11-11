# Namespace Migration Guide

## Issue

The common_system include path is `kcenon/common/*`, but some headers declare `namespace common` instead of `namespace kcenon::common`. This creates inconsistency where:

- Include path suggests: `kcenon::common::`
- Actual namespace: `::common::`

This leads to:
1. External modules use inconsistent references (`::common::interfaces` vs `kcenon::common::`)
2. Adapters need workarounds (see thread_system using `::common::interfaces`)
3. Name collision risk with other libraries using `common` namespace

## Solution

### Phase 1: Alias (Current) - Backward Compatible

`common.h` now provides a namespace alias:
```cpp
namespace common = kcenon::common;
```

**This means:**
- Existing code using `::common::` continues to work
- New code can use `kcenon::common::` (recommended)
- Both refer to the same types

### Phase 2: Migration (Future)

Individual headers will be updated from:
```cpp
namespace common {
    // ...
}
```

To:
```cpp
namespace kcenon {
namespace common {
    // ...
}} // namespace kcenon::common
```

## Recommended Usage

### For New Code
```cpp
#include <kcenon/common/patterns/result.h>

using kcenon::common::Result;  // Preferred
using kcenon::common::error_info;
```

### For Existing Code
```cpp
// Option 1: Keep current (works via alias)
#include <kcenon/common/patterns/result.h>

using common::Result;  // Still works
using common::error_info;

// Option 2: Migrate gradually (recommended)
using kcenon::common::Result;
using kcenon::common::error_info;
```

### For Library Authors

If your library uses common_system:

```cpp
// Preferred: Use fully qualified namespace
kcenon::common::Result<T> my_function();

// Also works: Alias
common::Result<T> my_function();

// Recommended: Import specific types
namespace mylib {
    using Result = kcenon::common::Result;
    using VoidResult = kcenon::common::VoidResult;

    Result<T> my_function();
}
```

## Migration Checklist

### Phase 1 (Non-Breaking)
- [x] Add namespace alias in common.h
- [x] Document recommended usage
- [ ] Update examples to use kcenon::common
- [ ] Update tests to use kcenon::common
- [ ] Add deprecation warnings (future)

### Phase 2 (Breaking Change - Major Version)
- [ ] Update all headers to use kcenon::common
- [ ] Remove backward compatibility alias
- [ ] Update all dependent systems
- [ ] Bump major version

## Impact on Dependent Systems

### thread_system
Currently uses: `::common::interfaces::...`
```cpp
// thread_system/include/kcenon/thread/adapters/common_executor_adapter.h:38
namespace common::interfaces { ... }
```

**Action:** Can continue using `common::` (via alias) or migrate to `kcenon::common::`

### logger_system
Check for any `common::` usage and ensure consistent reference.

### monitoring_system
Same as logger_system - verify namespace references.

## Timeline

- **v1.0.x**: Alias provided, both namespaces work
- **v1.1.0**: Deprecation warnings added
- **v2.0.0**: Only `kcenon::common` supported (breaking change)

## References

- Include path: `kcenon/common/*`
- Target namespace: `kcenon::common::`
- Compatibility alias: `namespace common = kcenon::common;`
