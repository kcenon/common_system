# vcpkg Features Documentation Template

> **Purpose**: Standard template for documenting vcpkg features across the kcenon ecosystem.
> Apply this template as a "## vcpkg Features" section in each repository's README.md.

## Template

````markdown
## vcpkg Features

```bash
vcpkg install <port-name>[feature1,feature2,...]
```

| Feature | Default | Description | Pulls In | Prerequisites |
|---------|:-------:|-------------|----------|---------------|
| feature | on/off  | What it enables | dependency-name | required-feature or -- |

### Feature Selection Guidance

- **Minimal install**: `vcpkg install <port-name>`
- **Production with TLS**: `vcpkg install <port-name>[ssl]`
- **Full install**: `vcpkg install <port-name>[feature1,feature2,...]`
````

## Field Descriptions

| Column | Description |
|--------|-------------|
| **Feature** | Feature name as used in `vcpkg install port[feature]` |
| **Default** | Whether the feature is included in a bare `vcpkg install port` |
| **Description** | One-line summary of what enabling the feature does |
| **Pulls In** | External dependencies added when the feature is selected |
| **Prerequisites** | Other features that must also be enabled (or `--` if none) |

## Conventions

- List features alphabetically, with `testing` last
- Mark default features as "on", optional features as "off"
- Use `--` for empty Prerequisites cells
- Include concrete `vcpkg install` examples for common deployment scenarios
- Keep descriptions concise (one line, no jargon)

## Applying to a Repository

1. Identify all features from the port's `vcpkg.json`
2. Determine default vs optional (check `"default-features"` or CMake defaults)
3. Fill in the table following the conventions above
4. Add a "Feature Selection Guidance" subsection with 2-4 scenario examples
5. Place the section after "Project Status" or "Architecture" and before "Getting Started"
