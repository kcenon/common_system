#!/usr/bin/env python3
"""Central Actions entry points; resolve event data without shell interpolation."""
import argparse
import base64
import json
import os
from pathlib import Path
import subprocess
import sys

from coherence_dispatch import CHANGE, GitHubAPIError, api, correlation, validate_payload
from ecosystem_lock import digest, read, select, sha, validate, verify_run


def prepare(event, name, root, destination, head):
    context = {"repository": "common_system"}
    inputs = event.get("inputs", {})
    revision = sha(inputs.get("lock_revision") or head)
    repo, commit = None, None
    if name == "repository_dispatch":
        data = validate_payload(event.get("action"), event.get("client_payload"))
        if event["action"] != CHANGE:
            raise ValueError("central workflow accepts only change notifications")
        revision, repo, commit = data["lock_revision"], data["candidate_repository"], data["candidate_sha"]
    elif name == "pull_request":
        revision = sha(event["pull_request"]["base"]["sha"])
        repo, commit = "common_system", sha(event["pull_request"]["head"]["sha"])
    elif name == "push":
        repo, commit = "common_system", sha(head)
    elif name == "workflow_dispatch":
        repo, commit = inputs.get("candidate_repository") or None, inputs.get("candidate_sha") or None
    elif name != "schedule":
        raise ValueError(f"unsupported event {name}")
    destination.mkdir(parents=True, exist_ok=True)
    lock_path = destination / "input-lock.json"
    bootstrap = inputs.get("bootstrap") in (True, "true")
    if bootstrap:
        if name != "workflow_dispatch" or repo or commit:
            raise ValueError("bootstrap is an explicit manual tuple, without candidate overrides")
        candidate = validate(read(root / "ci/ecosystem-candidate.json"), allow_candidate=True)
        if candidate["status"] != "candidate":
            raise ValueError("bootstrap snapshot must be labeled candidate")
        candidate["base_lock_digest"] = digest(root / "ci/ecosystem-lock.json")
    else:
        try:
            item = api(f"repos/kcenon/common_system/contents/ci/ecosystem-lock.json?ref={revision}")
        except GitHubAPIError as exc:
            if name != "pull_request" or exc.status != 404:
                raise
            # The first lock cannot exist on the PR's base yet. Require the
            # proposed accepted lock and its original trusted Actions evidence;
            # an unvalidated candidate is never an automatic bootstrap input.
            lock_path.write_bytes((root / "ci/ecosystem-lock.json").read_bytes())
            lock = validate(read(lock_path))
            verify_run(lock["validation"])
            revision = sha(head)
        else:
            lock_path.write_bytes(base64.b64decode(item["content"]))
        candidate = select(read(lock_path), repo, commit)
        candidate["base_lock_digest"] = digest(lock_path)
    candidate["lock_revision"] = revision
    context.update(candidate_repository=repo, candidate_sha=commit, lock_revision=revision,
                   correlation=correlation(repo, commit, revision))
    (destination / "snapshot.json").write_text(json.dumps(candidate, indent=2) + "\n")
    (destination / "context.json").write_text(json.dumps(context, indent=2) + "\n")
    return candidate


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    prep = sub.add_parser("prepare")
    prep.add_argument("--root", type=Path, default=Path("."))
    prep.add_argument("--output", type=Path, default=Path("coherence-input"))
    result = sub.add_parser("result")
    result.add_argument("--context", type=Path, default=Path("coherence-input/context.json"))
    result.add_argument("--output", type=Path, default=Path("dispatch-result.json"))
    result.add_argument("--code", required=True)
    args = parser.parse_args()
    try:
        if args.command == "prepare":
            value = prepare(read(Path(os.environ["GITHUB_EVENT_PATH"])), os.environ["GITHUB_EVENT_NAME"],
                            args.root, args.output, os.environ["GITHUB_SHA"])
        else:
            value = read(args.context)
            value["raw_exit_code"] = int(args.code) if args.code else None
            value["run_url"] = f"https://github.com/{os.environ['GITHUB_REPOSITORY']}/actions/runs/{os.environ['GITHUB_RUN_ID']}"
            args.output.write_text(json.dumps(value, indent=2) + "\n")
        print(json.dumps(value, indent=2))
        return 0
    except (OSError, ValueError, KeyError, subprocess.SubprocessError) as exc:
        print(f"WORKFLOW INPUT FAILED: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
