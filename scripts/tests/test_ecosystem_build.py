import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).parents[1]))
import ecosystem_build as build
import coherence_workflow as workflow
from unittest.mock import patch


class EcosystemBuildTests(unittest.TestCase):
    def test_dependency_order_and_no_logger_cycle(self):
        seen = set()
        for repository in build.REPOSITORIES:
            self.assertLessEqual(set(build.DEPENDENCIES[repository]),seen)
            seen.add(repository)
        self.assertFalse(build.OPTIONS["thread_system"]["KCENON_WITH_LOGGER_SYSTEM"])
        self.assertTrue(build.OPTIONS["database_system"]["USE_SQLITE"])

    def test_failed_command_retains_real_exit_and_log(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            code = build.run([sys.executable,"-c","print('raw evidence'); raise SystemExit(7)"], root, root/"command.log")
            self.assertEqual(code,7)
            self.assertIn("raw evidence",(root/"command.log").read_text())

    def test_existing_checkout_must_match_pin(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            subprocess.run(["git","init","-q",str(root)],check=True)
            subprocess.run(["git","-C",str(root),"-c","user.name=Test","-c","user.email=test@example.invalid",
                            "-c","commit.gpgsign=false","commit","--allow-empty","-qm","fixture"],check=True)
            with self.assertRaisesRegex(ValueError,"refusing to change"):
                build.source_at(root,"common_system","a"*40)

    def test_invalid_dispatch_cannot_select_a_default_branch(self):
        with tempfile.TemporaryDirectory() as tmp, patch.object(workflow,"api") as api:
            with self.assertRaises(ValueError):
                workflow.prepare({"action":"wrong","client_payload":{}},"repository_dispatch",Path(tmp),Path(tmp),"a"*40)
            api.assert_not_called()

    def test_final_source_audit_retains_dirty_input_and_observes_new_head(self):
        entry = {"sha": "a"*40, "worktree_changes": True}
        with patch.object(build, "output", side_effect=["", "b"*40]):
            build.record_source_state(entry, Path("source"))
        self.assertTrue(entry["worktree_changes"])
        self.assertEqual(entry["sha"], "b"*40)
        with patch.object(build, "output", side_effect=OSError("checkout removed")):
            build.record_source_state(entry, Path("source"))
        self.assertIsNone(entry["worktree_changes"])
        self.assertIn("checkout removed", entry["error"])

    def test_bootstrap_is_manual_and_explicit(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root/"ci").mkdir()
            candidate = {"schema_version":1,"status":"candidate","profile":"macos-appleclang",
                         "repositories":{repo:"a"*40 for repo in build.REPOSITORIES},"registry_sha":"b"*40}
            (root/"ci/ecosystem-candidate.json").write_text(json.dumps(candidate))
            selected = workflow.prepare({"inputs":{"bootstrap":"true"}},"workflow_dispatch",root,root/"output","a"*40)
            self.assertEqual(selected["status"],"candidate")
            self.assertIsNone(selected["base_lock_digest"])
            with self.assertRaises(ValueError):
                workflow.prepare({"inputs":{"bootstrap":"true"}},"push",root,root/"bad","a"*40)


if __name__ == "__main__": unittest.main()
