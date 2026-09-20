#!/usr/bin/env python3
"""Resolve and promote immutable ecosystem snapshots. Common owns this policy."""
from __future__ import annotations
import argparse
import hashlib
import io
import json
from pathlib import Path
import re
import subprocess
import sys
import zipfile

REPOSITORIES = tuple(f"{name}_system" for name in
                     ("common", "thread", "container", "logger", "network", "monitoring", "database", "pacs"))
CHECKS = ("conformance", "version_drift", "configure", "build", "consumer")
PROFILES = ("ubuntu-24.04-gcc13", "macos-appleclang")


def sha(value):
    if not isinstance(value, str) or not re.fullmatch(r"[0-9a-f]{40}", value):
        raise ValueError(f"expected full immutable SHA, got {value!r}")
    return value


def read(path):
    try:
        value = json.loads(path.read_text())
    except (OSError, ValueError) as exc:
        raise ValueError(f"{path}: {exc}") from exc
    if not isinstance(value, dict):
        raise ValueError(f"{path}: expected an object")
    return value


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest() if path.exists() else None


def validate_repositories(repositories):
    if not isinstance(repositories, dict) or set(repositories) != set(REPOSITORIES):
        raise ValueError("snapshot must contain exactly the eight ecosystem repositories")
    for value in repositories.values():
        sha(value)


def validate_evidence(evidence, repositories, profile):
    if evidence.get("exploratory", False) is not False:
        raise ValueError("exploratory runs cannot qualify an accepted lock")
    if evidence.get("schema_version") != 1 or evidence.get("repositories") != repositories:
        raise ValueError("build evidence does not identify the exact source tuple")
    if evidence.get("profile") != profile or not evidence.get("toolchain"):
        raise ValueError("build profile/toolchain evidence is missing or mismatched")
    if set(evidence.get("results", {})) != set(REPOSITORIES):
        raise ValueError("build evidence must report all eight repositories")
    for repo in REPOSITORIES:
        result = evidence["results"][repo]
        if result.get("sha") != repositories[repo] or result.get("worktree_changes") is not False:
            raise ValueError(f"{repo}: source changed or SHA mismatch")
        for check in CHECKS:
            code = result.get("checks", {}).get(check)
            if type(code) is not int or code != 0:
                raise ValueError(f"{repo}/{check}: missing, skipped or failed ({code!r})")


def validate(lock, allow_candidate=False):
    if lock.get("schema_version") != 1 or lock.get("profile") not in PROFILES:
        raise ValueError("unsupported ecosystem lock schema/profile")
    validate_repositories(lock.get("repositories"))
    sha(lock.get("registry_sha"))
    status = lock.get("status")
    if status == "accepted":
        validate_evidence(lock.get("validation", {}), lock["repositories"], lock["profile"])
        if lock["validation"].get("registry_sha") != lock["registry_sha"]:
            raise ValueError("registry revision is not covered by the recorded snapshot")
        if not re.fullmatch(r"https://github.com/kcenon/common_system/actions/runs/\d+", lock["validation"].get("run_url", "")):
            raise ValueError("accepted lock lacks a central validation run")
    elif status != "candidate" or not allow_candidate:
        raise ValueError("no accepted last-green lock; bootstrap and validate an explicit candidate first")
    return lock


def select(lock, repository=None, candidate_sha=None):
    validate(lock)
    if bool(repository) != bool(candidate_sha):
        raise ValueError("candidate repository and SHA must be supplied together")
    selected = dict(lock["repositories"])
    if repository:
        if repository not in REPOSITORIES:
            raise ValueError("candidate repository is outside the ecosystem")
        selected[repository] = sha(candidate_sha)
    return dict(schema_version=1, status="candidate", profile=lock["profile"],
                repositories=selected, registry_sha=lock["registry_sha"])


def verify_run(evidence):
    match = re.fullmatch(r"https://github.com/kcenon/common_system/actions/runs/(\d+)", evidence.get("run_url", ""))
    if not match:
        raise ValueError("promotion requires a completed central Actions run")
    run = json.loads(subprocess.check_output(["gh", "api", f"repos/kcenon/common_system/actions/runs/{match[1]}"],
                                             text=True, timeout=60))
    if (run.get("conclusion") != "success" or run.get("status") != "completed"
            or run.get("event") not in ("workflow_dispatch", "push")
            or run.get("path", "").split("@")[0] != ".github/workflows/ecosystem-cross-build.yml"
            or run.get("head_sha") != sha(evidence.get("orchestrator_sha"))):
        raise ValueError("run is not a successful trusted cross-build of the recorded orchestrator")
    # The submitted JSON must be the actual report emitted by that run, not a
    # locally invented tuple accompanied by an unrelated green Actions URL.
    artifacts = json.loads(subprocess.check_output(["gh", "api",
        f"repos/kcenon/common_system/actions/runs/{match[1]}/artifacts"], text=True, timeout=60))
    found = [item for item in artifacts.get("artifacts", [])
             if item.get("name") == "ecosystem-evidence" and not item.get("expired")]
    if len(found) != 1:
        raise ValueError("promotion requires one unexpired ecosystem-evidence artifact")
    archive = subprocess.check_output(["gh", "api",
        f"repos/kcenon/common_system/actions/artifacts/{found[0]['id']}/zip"], timeout=60)
    if len(archive) > 4 * 1024 * 1024:
        raise ValueError("unexpectedly large evidence archive")
    with zipfile.ZipFile(io.BytesIO(archive)) as bundle:
        members = [item for item in bundle.infolist() if item.filename.endswith("/evidence.json") or item.filename == "evidence.json"]
        if len(members) != 1 or members[0].file_size > 1024 * 1024:
            raise ValueError("evidence artifact lacks a unique bounded report")
        if json.loads(bundle.read(members[0])) != evidence:
            raise ValueError("submitted evidence differs from the actual Actions artifact")


def promote(candidate, evidence, current_digest):
    validate(candidate, allow_candidate=True)
    if candidate.get("status") != "candidate" or candidate.get("base_lock_digest") != current_digest:
        raise ValueError("candidate is stale relative to the currently accepted lock")
    validate_evidence(evidence, candidate["repositories"], candidate["profile"])
    if evidence.get("registry_sha") != candidate["registry_sha"]:
        raise ValueError("candidate registry revision differs from validation evidence")
    verify_run(evidence)
    return dict(schema_version=1, status="accepted", profile=candidate["profile"],
                repositories=candidate["repositories"], registry_sha=candidate["registry_sha"],
                validation=evidence)


def write_promotion(candidate, evidence, path):
    """Serialize local promotions and reject a lock changed during verification."""
    path.parent.mkdir(parents=True, exist_ok=True)
    mutex = path.with_suffix(".json.promoting")
    try:
        handle = mutex.open("x")
    except FileExistsError as exc:
        raise ValueError(f"another promotion owns {mutex}; inspect it before retrying") from exc
    temporary = path.with_suffix(".json.tmp")
    try:
        with handle:
            before = digest(path)
            result = promote(candidate, evidence, before)
            if digest(path) != before:
                raise ValueError("accepted lock changed while promotion was being verified")
            temporary.write_text(json.dumps(result, indent=2) + "\n")
            temporary.replace(path)
    finally:
        temporary.unlink(missing_ok=True)
        mutex.unlink()


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    check = sub.add_parser("check")
    check.add_argument("file", type=Path)
    check.add_argument("--allow-candidate", action="store_true")
    resolve = sub.add_parser("resolve")
    resolve.add_argument("--lock", type=Path, default=Path("ci/ecosystem-lock.json"))
    resolve.add_argument("--bootstrap", type=Path, help="explicit reviewed candidate snapshot")
    resolve.add_argument("--candidate-repo", choices=REPOSITORIES)
    resolve.add_argument("--candidate-sha")
    resolve.add_argument("--output", type=Path, required=True)
    promotion = sub.add_parser("promote")
    promotion.add_argument("--candidate", type=Path, required=True)
    promotion.add_argument("--evidence", type=Path, required=True)
    promotion.add_argument("--lock", type=Path, default=Path("ci/ecosystem-lock.json"))
    args = parser.parse_args(argv)
    try:
        if args.command == "check":
            validate(read(args.file), args.allow_candidate)
            print("Valid ecosystem snapshot")
        elif args.command == "resolve":
            if args.bootstrap:
                if args.candidate_repo or args.candidate_sha:
                    raise ValueError("bootstrap tuple cannot be mixed with a candidate override")
                result = validate(read(args.bootstrap), allow_candidate=True)
                if result["status"] != "candidate":
                    raise ValueError("bootstrap input must be labeled candidate")
            else:
                result = select(read(args.lock), args.candidate_repo, args.candidate_sha)
            result["base_lock_digest"] = digest(args.lock)
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(json.dumps(result, indent=2) + "\n")
            print(json.dumps(result, indent=2))
        else:
            write_promotion(read(args.candidate), read(args.evidence), args.lock)
            print(f"Prepared accepted lock: {args.lock}; review before committing")
        return 0
    except (ValueError, KeyError, TypeError, OSError, subprocess.SubprocessError) as exc:
        print(f"ECOSYSTEM LOCK FAILED: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
