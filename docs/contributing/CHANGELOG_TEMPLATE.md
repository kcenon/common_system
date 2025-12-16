# CHANGELOG Template

This template provides a standardized CHANGELOG format for all KCENON ecosystem systems.

## Format Specification

All KCENON systems should follow [Keep a Changelog](https://keepachangelog.com/en/1.0.0/) format
and [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## Template

```markdown
# Changelog

All notable changes to the [SYSTEM_NAME] project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

> **Language:** **English** | [한국어](CHANGELOG_KO.md)

---

## [Unreleased]

### Added
- New features that have been added

### Changed
- Changes to existing functionality

### Deprecated
- Features that will be removed in upcoming releases

### Removed
- Features that have been removed

### Fixed
- Bug fixes

### Security
- Security-related changes

---

## [X.Y.Z] - YYYY-MM-DD

### Added
- **Feature Name** (#issue_number)
  - Detailed description of the feature
  - Sub-items for additional details

### Changed
- List of changes

### Deprecated
- List of deprecated items

### Removed
- **BREAKING**: Removed feature name
  - Migration path description

### Fixed
- Bug description (#issue_number)

### Security
- Security fix description

---

## Version Numbering

This project uses Semantic Versioning:
- **MAJOR** version: Incompatible API changes
- **MINOR** version: Backwards-compatible functionality additions
- **PATCH** version: Backwards-compatible bug fixes

---

## Migration Guides

### Migrating to X.Y.0

Detailed migration instructions...

---

## Support

- **Issues**: [GitHub Issues](https://github.com/kcenon/SYSTEM_NAME/issues)
- **Discussions**: [GitHub Discussions](https://github.com/kcenon/SYSTEM_NAME/discussions)
- **Email**: kcenon@naver.com

---

## License

This project is licensed under the BSD 3-Clause License - see the [LICENSE](LICENSE) file for details.

---

[Unreleased]: https://github.com/kcenon/SYSTEM_NAME/compare/vX.Y.Z...HEAD
[X.Y.Z]: https://github.com/kcenon/SYSTEM_NAME/releases/tag/vX.Y.Z
```

---

## Category Definitions

| Category | Usage |
|----------|-------|
| **Added** | New features |
| **Changed** | Changes in existing functionality |
| **Deprecated** | Soon-to-be removed features |
| **Removed** | Now removed features |
| **Fixed** | Bug fixes |
| **Security** | Vulnerability fixes |

---

## Best Practices

1. **Keep entries concise but informative**
   - Bad: "Fixed bug"
   - Good: "Fixed race condition in thread pool shutdown (#123)"

2. **Link to issues and PRs**
   - Reference issue numbers: `(#123)`
   - Link PRs when relevant: `(PR #456)`

3. **Highlight breaking changes**
   - Prefix with `**BREAKING**:`
   - Include migration path

4. **Use consistent formatting**
   - Bold for feature names: `**Feature Name**`
   - Code blocks for code references: `` `function_name()` ``

5. **Keep Unreleased section up to date**
   - Add entries as changes are merged
   - Move to versioned section on release

6. **Maintain comparison links**
   - Update footer links for each release
   - Unreleased should compare to latest tag

---

## Automation

Consider using conventional commits for automatic changelog generation:

```
feat: add new feature (#123)
fix: resolve bug in component (#124)
docs: update API documentation (#125)
refactor: simplify algorithm (#126)
perf: optimize query performance (#127)
test: add unit tests for module (#128)
chore: update dependencies (#129)
```

Tools like `conventional-changelog` can generate changelogs from these commits.

---

## Korean Version

For Korean translations, create `CHANGELOG_KO.md` with:
- Same structure and version numbers
- Translated descriptions
- Link to English version at top

```markdown
> **언어:** [English](CHANGELOG.md) | **한국어**
```

---

## Version in CMakeLists.txt

Ensure version in CHANGELOG matches CMakeLists.txt:

```cmake
project(system_name
    VERSION X.Y.Z.0
    DESCRIPTION "System description"
    LANGUAGES CXX
)
```

---

## Cross-Reference

When changes affect multiple systems:

1. Document in the originating system's CHANGELOG
2. Reference in dependent systems if breaking
3. Update `COMPATIBILITY.md` in common_system

Example:
```markdown
### Changed
- Updated to use common_system v2.0.0 (see common_system [CHANGELOG](../common_system/docs/CHANGELOG.md))
```
