# COM-007: Conan 패키지 매니저 지원

**Status**: DONE
**Priority**: MEDIUM
**Category**: BUILD
**Estimated Duration**: 4-5h
**Dependencies**: None

---

## 개요

Conan 패키지 매니저 지원을 추가하여 배포 옵션을 확장합니다.

## 현재 상태

**현재 지원**:
- vcpkg (`vcpkg.json`, `vcpkg-configuration.json`)
- CMake FetchContent
- 수동 설치

**누락**:
- Conan 지원

## 작업 항목

### 1. conanfile.py 작성
```python
# conanfile.py
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout

class CommonSystemConan(ConanFile):
    name = "common_system"
    version = "1.0.0"
    license = "BSD-3-Clause"
    url = "https://github.com/kcenon/common_system"
    description = "Header-only C++ common utilities"
    settings = "os", "compiler", "build_type", "arch"

    def layout(self):
        cmake_layout(self)

    def package(self):
        # Header-only: copy headers only
        ...
```

### 2. 테스트 패키지
```
test_package/
├── conanfile.py
├── CMakeLists.txt
└── example.cpp
```

### 3. CI 통합
- [ ] Conan 빌드 테스트 워크플로우
- [ ] Conan Center 등록 준비 (선택)

## 수락 기준

- [ ] `conan create .` 성공
- [ ] test_package 통과
- [ ] README에 Conan 설치 방법 추가
- [ ] CI에서 Conan 빌드 테스트
