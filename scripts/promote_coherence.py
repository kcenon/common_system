#!/usr/bin/env python3
"""Prepare gate promotion or require emitted checks, only at a clean green develop HEAD."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import tempfile

from ecosystem_lock import REPOSITORIES
from run_coherence import run_check

STATUS_NAMES = ("cross-system conformance linter", "SOUP Version Drift Detection")


def github(endpoint, method="GET", data=None):
    command = ["gh", "api", endpoint, "--method", method]
    if data is not None:
        command += ["--input", "-"]
    result = subprocess.run(command, input=json.dumps(data) if data is not None else None,
                            text=True, capture_output=True, timeout=60, check=True)
    return json.loads(result.stdout) if result.stdout else None


def required_payload(existing, runs):
    additions = []
    for name in STATUS_NAMES:
        matches = [run for run in runs if run.get("name") == name and run.get("app", {}).get("slug") == "github-actions"]
        latest = max(matches, key=lambda run: run.get("id", 0)) if matches else {}
        if latest.get("status") != "completed" or latest.get("conclusion") != "success":
            raise ValueError(f"current develop HEAD has no successful emitted check: {name}")
        additions.append({"context": name, "app_id": latest["app"]["id"]})
    checks = list(existing.get("checks", []))
    for context in existing.get("contexts", []):
        if not any(item["context"] == context for item in checks):
            checks.append({"context": context, "app_id": -1})
    for addition in additions:
        if not any(item["context"] == addition["context"] for item in checks):
            checks.append(addition)
    return {"strict": existing.get("strict", True), "checks": checks}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("prepare", "require"))
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--repository", choices=REPOSITORIES, required=True)
    parser.add_argument("--apply", action="store_true", help="actually update develop's required checks after all evidence passes")
    args = parser.parse_args()
    try:
        root = args.root.resolve()
        if subprocess.check_output(["git", "status", "--porcelain"], cwd=root, text=True).strip():
            raise ValueError("promotion requires a complete clean checkout")
        head = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip()
        endpoint = f"repos/kcenon/{args.repository}/branches/develop"
        branch = github(endpoint)
        if branch["commit"]["sha"] != head:
            raise ValueError("checkout is not the current landed develop HEAD; remediation must land first")
        origin = subprocess.check_output(["git", "remote", "get-url", "origin"], cwd=root, text=True).strip()
        if not origin.endswith(f"kcenon/{args.repository}.git") and not origin.endswith(f"kcenon/{args.repository}"):
            raise ValueError("checkout origin does not match the requested repository")
        reports = {}
        with tempfile.TemporaryDirectory() as tmp:
            for check in ("conformance", "version-drift"):
                output = Path(tmp) / f"{check}.json"
                extra = ["--no-matrix"] if check == "version-drift" and args.repository != "common_system" else []
                if run_check(root, check, output, extra, strict=True):
                    raise ValueError(f"raw {check} failed; gate cannot be promoted")
                reports[check] = json.loads(output.read_text())
            subprocess.run([sys.executable,"-m","unittest","discover","-s","scripts/tests"], cwd=root, check=True)
        if github(endpoint)["commit"]["sha"] != head:
            raise ValueError("develop advanced while validating; start again at the new HEAD")
        config_path = root / "ci/coherence.json"
        config = json.loads(config_path.read_text())
        if args.command == "prepare":
            for check in reports:
                config["modes"][check] = "enforcing"
            config_path.write_text(json.dumps(config, indent=2) + "\n")
            (root / "ci/coherence-promotion.json").write_text(json.dumps(
                {"schema_version":1,"repository":args.repository,"develop_sha":head,"reports":reports}, indent=2) + "\n")
            print("Prepared local gate promotion; review and land it separately. Release policy remains independent.")
        else:
            if any(config["modes"][check] != "enforcing" for check in reports):
                raise ValueError("enforcing policy has not landed on develop")
            runs = github(f"repos/kcenon/{args.repository}/commits/{head}/check-runs?per_page=100")["check_runs"]
            # PATCH only required statuses on protected branches, preserving all
            # other review/admin/restriction settings. An unprotected branch has
            # no such settings to preserve. Ruleset-only protection is not bypassed.
            if branch.get("protected"):
                existing = github(endpoint + "/protection").get("required_status_checks") or {}
                payload = required_payload(existing, runs)
                target, method = endpoint + "/protection/required_status_checks", "PATCH"
            else:
                payload = {"required_status_checks":required_payload({},runs),"enforce_admins":False,
                           "required_pull_request_reviews":None,"restrictions":None}
                target, method = endpoint + "/protection", "PUT"
            print(json.dumps({"method":method,"endpoint":target,"body":payload},indent=2))
            if args.apply:
                if github(endpoint)["commit"]["sha"] != head:
                    raise ValueError("develop advanced before settings update")
                github(target, method, payload)
                print("Applied required checks on develop; verify settings and a representative PR.")
            else:
                print("Settings preview only; pass --apply after reviewing this exact change.")
        return 0
    except (OSError, ValueError, KeyError, subprocess.SubprocessError) as exc:
        print(f"GATE PROMOTION FAILED: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__": sys.exit(main())
