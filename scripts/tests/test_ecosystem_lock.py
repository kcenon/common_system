import copy
import importlib.util
import json
import io
import zipfile
from pathlib import Path
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("lock", Path(__file__).parents[1] / "ecosystem_lock.py")
lock = importlib.util.module_from_spec(spec)
spec.loader.exec_module(lock)


class EcosystemLockTests(unittest.TestCase):
    def setUp(self):
        self.repos = {repo: format(i+1, "040x") for i, repo in enumerate(lock.REPOSITORIES)}
        self.evidence = {"schema_version": 1, "repositories": self.repos, "profile": lock.PROFILES[0],
                         "toolchain": {"compiler": "gcc-13"}, "run_url": "https://github.com/kcenon/common_system/actions/runs/123",
                         "orchestrator_sha": "a"*40, "results": {}}
        self.evidence["registry_sha"] = "b"*40
        for repo in lock.REPOSITORIES:
            self.evidence["results"][repo] = {"sha": self.repos[repo], "worktree_changes": False,
                                               "checks": {name: 0 for name in lock.CHECKS}}
        self.accepted = {"schema_version": 1, "status": "accepted", "profile": lock.PROFILES[0],
                         "repositories": self.repos, "registry_sha": "b"*40, "validation": self.evidence}

    def test_candidate_substitution_preserves_every_sibling(self):
        for changed in ["common_system", "thread_system"]:
            selected = lock.select(self.accepted, changed, "c"*40)
            self.assertEqual(selected["repositories"][changed], "c"*40)
            for repo in self.repos:
                if repo != changed:
                    self.assertEqual(selected["repositories"][repo], self.repos[repo])

    def test_branches_partial_shas_and_missing_members_fail(self):
        for value in ["main", "develop", "abc123", "x"*40]:
            bad = copy.deepcopy(self.accepted)
            bad["repositories"]["thread_system"] = value
            with self.assertRaises(ValueError): lock.validate(bad)
        bad = copy.deepcopy(self.accepted)
        del bad["repositories"]["pacs_system"]
        with self.assertRaises(ValueError): lock.validate(bad)

    def test_bootstrap_is_explicit_not_last_green(self):
        candidate = lock.select(self.accepted)
        with self.assertRaises(ValueError): lock.validate(candidate)
        lock.validate(candidate, allow_candidate=True)

    def test_failed_skipped_advisory_missing_or_dirty_cannot_promote(self):
        for code in [1, None, "skipped", "advisory", False]:
            evidence = copy.deepcopy(self.evidence)
            evidence["results"]["pacs_system"]["checks"]["build"] = code
            with self.assertRaises(ValueError): lock.validate_evidence(evidence, self.repos, lock.PROFILES[0])
        evidence = copy.deepcopy(self.evidence)
        evidence["results"]["common_system"]["worktree_changes"] = True
        with self.assertRaises(ValueError): lock.validate_evidence(evidence, self.repos, lock.PROFILES[0])

    def test_promotion_requires_run_and_current_lock_digest(self):
        candidate = dict(lock.select(self.accepted), base_lock_digest="old")
        run = {"status":"completed", "conclusion":"success", "event":"workflow_dispatch",
               "path":".github/workflows/ecosystem-cross-build.yml", "head_sha":"a"*40}
        archive = io.BytesIO()
        with zipfile.ZipFile(archive, "w") as bundle:
            bundle.writestr("cross-build/evidence.json", json.dumps(self.evidence))
        def response(command, **kwargs):
            if command[-1].endswith("/zip"): return archive.getvalue()
            if command[-1].endswith("/artifacts"):
                return json.dumps({"artifacts": [{"id": 7, "name": "ecosystem-evidence", "expired": False}]})
            return json.dumps(run)
        with patch.object(lock.subprocess, "check_output", side_effect=response):
            self.assertEqual(lock.promote(candidate,self.evidence,"old")["status"],"accepted")
            with self.assertRaises(ValueError): lock.promote(candidate,self.evidence,"new")
            invented = copy.deepcopy(self.evidence)
            invented["toolchain"]["compiler"] = "invented"
            with self.assertRaisesRegex(ValueError, "actual Actions artifact"):
                lock.promote(candidate, invented, "old")
        for event,conclusion in [("pull_request","success"),("workflow_dispatch","failure")]:
            run.update(event=event,conclusion=conclusion)
            with patch.object(lock.subprocess, "check_output", return_value=json.dumps(run)):
                with self.assertRaises(ValueError): lock.promote(candidate,self.evidence,"old")


if __name__ == "__main__":
    unittest.main()
