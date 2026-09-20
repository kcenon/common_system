#!/usr/bin/env python3
"""Copy or verify shared coherence files at an explicit reviewed common revision."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

FILES = {f"scripts/{name}": f"scripts/{name}" for name in (
    "conformance_lint.py", "check_version_drift.py", "run_coherence.py", "check_tag_reality.py", "coherence_dispatch.py",
    "tests/test_version_drift.py", "tests/test_gate_policy.py", "tests/test_tag_reality.py",
    "tests/test_dependency_options.py", "tests/test_coherence_dispatch.py")}
FILES["cmake/template/dependency_options.cmake"] = "cmake/KcenonDependencyOptions.cmake"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target", type=Path, required=True)
    parser.add_argument("--source-revision", required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    from ecosystem_lock import sha
    try:
        revision = sha(args.source_revision)
        manifest = {"schema_version": 1, "source_repository": "kcenon/common_system", "source_revision": revision, "files": {}}
        differences = []
        for source, target in FILES.items():
            content = subprocess.check_output(["git", "show", f"{revision}:{source}"], cwd=root)
            destination = args.target / target
            manifest["files"][target] = {"source": source, "sha256": hashlib.sha256(content).hexdigest()}
            if args.check:
                if not destination.exists() or destination.read_bytes() != content:
                    differences.append(target)
            else:
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(content)
        path = args.target / "ci/coherence-source.json"
        if args.check:
            if not path.exists() or json.loads(path.read_text()) != manifest:
                differences.append("ci/coherence-source.json")
            if differences:
                raise ValueError("shared files differ: " + ", ".join(differences))
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(json.dumps(manifest, indent=2) + "\n")
        print(f"{'Verified' if args.check else 'Updated'} {args.target} from common_system@{revision}")
        return 0
    except (OSError, ValueError, subprocess.SubprocessError) as exc:
        print(f"COHERENCE SYNC FAILED: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
