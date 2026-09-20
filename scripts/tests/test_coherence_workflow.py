import base64
import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).parents[1]))
import coherence_workflow as workflow
from coherence_dispatch import GitHubAPIError, correlation, payload, CHANGE
from ecosystem_lock import CHECKS, REPOSITORIES, digest


class CoherenceWorkflowTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        (self.root / "ci").mkdir()
        self.destination = self.root / "output"
        self.base, self.head, self.merge = "a" * 40, "b" * 40, "c" * 40
        self.event = {"pull_request": {"base": {"sha": self.base}, "head": {"sha": self.head}}}
        repositories = {repo: format(i + 1, "040x") for i, repo in enumerate(REPOSITORIES)}
        self.accepted = {
            "schema_version": 1, "status": "accepted", "profile": "ubuntu-24.04-gcc13",
            "repositories": repositories, "registry_sha": "d" * 40,
            "validation": {
                "schema_version": 1, "repositories": repositories,
                "profile": "ubuntu-24.04-gcc13", "registry_sha": "d" * 40,
                "toolchain": {"compiler": "gcc-13"},
                "run_url": "https://github.com/kcenon/common_system/actions/runs/123",
                "orchestrator_sha": "e" * 40,
                "results": {repo: {"sha": commit, "worktree_changes": False,
                                   "checks": {check: 0 for check in CHECKS}}
                            for repo, commit in repositories.items()},
            },
        }
        self.local_lock = self.root / "ci/ecosystem-lock.json"
        self.local_lock.write_text(json.dumps(self.accepted) + "\n")

    def prepare(self, event=None, name="pull_request"):
        return workflow.prepare(self.event if event is None else event, name, self.root,
                                self.destination, self.merge)

    def response(self, lock):
        return {"content": base64.b64encode(json.dumps(lock).encode()).decode()}

    def assert_selection(self, selected, revision):
        expected = dict(self.accepted["repositories"], common_system=self.head)
        self.assertEqual(selected["repositories"], expected)
        self.assertEqual(selected["registry_sha"], self.accepted["registry_sha"])
        self.assertEqual(selected["status"], "candidate")
        self.assertEqual(selected["lock_revision"], revision)
        self.assertEqual(selected["base_lock_digest"], digest(self.destination / "input-lock.json"))
        self.assertEqual(json.loads((self.destination / "snapshot.json").read_text()), selected)
        context = json.loads((self.destination / "context.json").read_text())
        self.assertEqual(context["lock_revision"], revision)
        self.assertEqual(context["candidate_sha"], self.head)
        self.assertEqual(context["correlation"], correlation("common_system", self.head, revision))

    def test_existing_base_lock_wins_over_pr_lock(self):
        self.local_lock.write_text("invalid PR lock must not replace the base lock")
        with patch.object(workflow, "api", return_value=self.response(self.accepted)) as api, \
             patch.object(workflow, "verify_run") as verify:
            selected = self.prepare()
        api.assert_called_once_with(
            f"repos/kcenon/common_system/contents/ci/ecosystem-lock.json?ref={self.base}")
        verify.assert_not_called()
        self.assert_selection(selected, self.base)

    def test_first_lock_pr_requires_original_actions_evidence(self):
        with patch.object(workflow, "api", side_effect=GitHubAPIError("missing lock", 404)), \
             patch.object(workflow, "verify_run") as verify:
            selected = self.prepare()
        verify.assert_called_once_with(self.accepted["validation"])
        self.assert_selection(selected, self.merge)
        self.assertEqual(selected["base_lock_digest"], digest(self.local_lock))
        self.assertEqual(json.loads(self.local_lock.read_text()), self.accepted)

    def test_first_lock_pr_rejects_missing_or_candidate_only_lock(self):
        self.local_lock.unlink()
        with patch.object(workflow, "api", side_effect=GitHubAPIError("missing lock", 404)), \
             patch.object(workflow, "verify_run") as verify:
            with self.assertRaises(FileNotFoundError):
                self.prepare()
            self.local_lock.write_text(json.dumps(dict(self.accepted, status="candidate")))
            with self.assertRaisesRegex(ValueError, "no accepted"):
                self.prepare()
        verify.assert_not_called()
        self.assertFalse((self.destination / "snapshot.json").exists())

    def test_first_lock_pr_rejects_failed_raw_evidence(self):
        bad = copy.deepcopy(self.accepted)
        bad["validation"]["results"]["pacs_system"]["checks"]["consumer"] = 1
        self.local_lock.write_text(json.dumps(bad))
        with patch.object(workflow, "api", side_effect=GitHubAPIError("missing lock", 404)), \
             patch.object(workflow, "verify_run") as verify:
            with self.assertRaisesRegex(ValueError, "pacs_system/consumer"):
                self.prepare()
        verify.assert_not_called()

    def test_first_lock_pr_rejects_unverifiable_artifact(self):
        with patch.object(workflow, "api", side_effect=GitHubAPIError("missing lock", 404)), \
             patch.object(workflow, "verify_run", side_effect=ValueError("artifact mismatch")):
            with self.assertRaisesRegex(ValueError, "artifact mismatch"):
                self.prepare()
        self.assertFalse((self.destination / "snapshot.json").exists())

    def test_other_api_errors_never_use_pr_lock(self):
        for status in (401, 403, 429, 500, None):
            with self.subTest(status=status):
                error = GitHubAPIError("request failed", status)
                with patch.object(workflow, "api", side_effect=error), \
                     patch.object(workflow, "verify_run") as verify:
                    with self.assertRaises(GitHubAPIError) as raised:
                        self.prepare()
                self.assertIs(raised.exception, error)
                verify.assert_not_called()
        self.assertFalse((self.destination / "snapshot.json").exists())

    def test_invalid_base_lock_never_uses_pr_lock(self):
        for invalid in (dict(self.accepted, status="candidate"), {"schema_version": 0}):
            with self.subTest(invalid=invalid):
                with patch.object(workflow, "api", return_value=self.response(invalid)), \
                     patch.object(workflow, "verify_run") as verify:
                    with self.assertRaises(ValueError):
                        self.prepare()
                verify.assert_not_called()

    def test_missing_lock_on_other_events_never_uses_checkout_lock(self):
        notification = {"action": CHANGE, "client_payload": payload("thread_system", self.head, self.base, CHANGE)}
        for name, event in (("push", {}), ("schedule", {}), ("workflow_dispatch", {}),
                            ("repository_dispatch", notification)):
            with self.subTest(name=name):
                with patch.object(workflow, "api", side_effect=GitHubAPIError("missing lock", 404)), \
                     patch.object(workflow, "verify_run") as verify:
                    with self.assertRaises(GitHubAPIError):
                        self.prepare(event, name)
                verify.assert_not_called()
        self.assertFalse((self.destination / "snapshot.json").exists())


if __name__ == "__main__":
    unittest.main()
