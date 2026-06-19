# Fuzzing

libFuzzer harnesses for `common_system` foundation decoding surfaces.

The fuzz targets are **excluded from the default build**. They are compiled only
when `BUILD_FUZZERS=ON` is passed to CMake, and they require a Clang toolchain
because libFuzzer and the fuzzer sanitizer mode are Clang features.

## Targets

| Target                | Surface under test |
|-----------------------|--------------------|
| `result_error_fuzzer` | `error::get_error_message` switch lookup, `error::get_category_name` range bucketing, `common_error_category::message`, `error_info` construction, and the `Result<T>` error branch (`make_error<T>` / `is_err` / `error` / `value_or`). |

## Build

```bash
cmake -B build-fuzz -G Ninja \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DBUILD_FUZZERS=ON \
    -DCOMMON_SYSTEM_BUILD_TESTS=OFF \
    -DCOMMON_SYSTEM_BUILD_EXAMPLES=OFF
cmake --build build-fuzz
```

## Run

```bash
# Replay the seed corpus and continue fuzzing.
./build-fuzz/fuzz/result_error_fuzzer fuzz/corpus/result_error

# Time-boxed run (used by CI).
./build-fuzz/fuzz/result_error_fuzzer -max_total_time=300 fuzz/corpus/result_error
```

The harness is built with `-fsanitize=fuzzer,address`, so any memory error,
signed-overflow-driven out-of-bounds access, or unhandled allocation fault in
the decoding path will abort with a sanitizer report and a reproducer file.

## Corpus

`corpus/result_error/` holds seed inputs. Each seed encodes a 4-byte
little-endian signed error code followed by an arbitrary message/module tail,
covering success, the common error codes, category range boundaries, the
reserved range, and the `INT_MIN` / `INT_MAX` extremes.
