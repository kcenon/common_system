#!/usr/bin/env python3
"""Compare pinned source/registry ports, retaining content and semantic checks."""
from __future__ import annotations
import argparse
import base64
import hashlib
import json
from pathlib import Path
import re
import sys

from coherence_dispatch import api
from ecosystem_lock import REPOSITORIES, read, validate


def fetch(repository, revision, directory):
    result = {}
    for filename in ("portfile.cmake", "vcpkg.json", "usage"):
        item = api(f"repos/kcenon/{repository}/contents/{directory}/{filename}?ref={revision}")
        if item.get("type") != "file" or item.get("encoding") != "base64":
            raise ValueError(f"{repository}@{revision}/{directory}/{filename}: not an available file")
        result[filename] = base64.b64decode(item["content"]).decode()
    return result


def version(manifest):
    values = [manifest[key] for key in ("version-semver", "version", "version-string", "version-date") if key in manifest]
    if len(values) != 1 or not isinstance(values[0], str):
        raise ValueError("port manifest must contain exactly one version field")
    return values[0], manifest.get("port-version", 0)


def semantics(files, repository):
    errors = []
    port = files["portfile.cmake"]
    # Ignore comments; do not let a commented legacy spelling satisfy the check.
    port = re.sub(r"#[^\n]*", "", port)
    package = re.findall(r'\bPACKAGE_NAME\s+"?([^\s)"]+)', port)
    config = re.findall(r'\bCONFIG_PATH\s+"?([^\s)"]+)', port)
    if len(package) != 1 or package[0] != repository:
        errors.append(f"PACKAGE_NAME must be canonical {repository}")
    if len(config) != 1 or config[0] != f"lib/cmake/{repository}":
        errors.append(f"CONFIG_PATH must be lib/cmake/{repository}")
    targets = re.findall(r"target_link_libraries\([^)]*?\bPRIVATE\s+([^\s)]+)", files["usage"], re.S)
    if not targets or any(not re.fullmatch(r"[a-z_]+::[a-z_]+", target) for target in targets):
        errors.append("usage must document a snake_case namespace::target")
    manifest = json.loads(files["vcpkg.json"])
    version(manifest)
    expected_name = "kcenon-" + repository.replace("_", "-")
    if manifest.get("name") != expected_name:
        errors.append(f"manifest name must be {expected_name}")
    return errors


def compare(source, registry, repository):
    source_version = version(json.loads(source["vcpkg.json"]))
    registry_version = version(json.loads(registry["vcpkg.json"]))
    differences = [name for name in source if source[name] != registry[name]]
    errors = {"source": semantics(source, repository), "registry": semantics(registry, repository)}
    # A staged overlay version is informative. It is not evidence that a
    # registry release is wrong or that the candidate has already been released.
    staged = source_version != registry_version
    return {"source_version": source_version, "registry_version": registry_version,
            "classification": "different-version-snapshots" if staged else "same-version",
            "different_files": differences, "semantic_errors": errors,
            "file_sha256": {side: {name: hashlib.sha256(text.encode()).hexdigest() for name, text in files.items()}
                            for side, files in (("source", source), ("registry", registry))},
            "passed": not any(errors.values()) and (staged or not differences)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--advisory", action="store_true")
    args = parser.parse_args()
    report = {"schema_version": 1, "results": {}, "raw_exit_code": 2}
    try:
        snapshot = validate(read(args.snapshot), allow_candidate=True)
        report.update(repositories=snapshot["repositories"], registry_sha=snapshot["registry_sha"])
        for repository in REPOSITORIES:
            port = "kcenon-" + repository.replace("_", "-")
            try:
                source = fetch(repository, snapshot["repositories"][repository], f"vcpkg-ports/{port}")
                registry = fetch("vcpkg-registry", snapshot["registry_sha"], f"ports/{port}")
                report["results"][repository] = compare(source, registry, repository)
            except (ValueError, KeyError, OSError) as exc:
                report["results"][repository] = {"passed": False, "error": str(exc)}
        report["raw_exit_code"] = 0 if all(item["passed"] for item in report["results"].values()) else 1
    except (ValueError, KeyError, OSError) as exc:
        report["error"] = str(exc)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    if report["raw_exit_code"] and args.advisory:
        print("::warning::Port sync has raw failures; inspect the pinned report")
    return 0 if args.advisory else report["raw_exit_code"]


if __name__ == "__main__":
    sys.exit(main())
