import importlib.util
import json
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).parents[1]))
import check_port_sync as port


class PortSyncTests(unittest.TestCase):
    def files(self, version="1.0.0"):
        return {"portfile.cmake": "vcpkg_cmake_config_fixup(PACKAGE_NAME common_system CONFIG_PATH lib/cmake/common_system)",
                "usage": "target_link_libraries(main PRIVATE kcenon::common_system)",
                "vcpkg.json": json.dumps({"name":"kcenon-common-system","version-semver":version})}

    def test_equal_and_content_drift(self):
        self.assertTrue(port.compare(self.files(), self.files(), "common_system")["passed"])
        other = self.files()
        other["portfile.cmake"] += "\n# metadata difference\n"
        self.assertFalse(port.compare(self.files(),other,"common_system")["passed"])

    def test_staged_versions_keep_semantic_checks(self):
        result = port.compare(self.files("2.0.0"),self.files(),"common_system")
        self.assertTrue(result["passed"])
        self.assertEqual(result["classification"],"different-version-snapshots")
        bad = self.files("2.0.0")
        bad["portfile.cmake"] = bad["portfile.cmake"].replace("PACKAGE_NAME common_system","PACKAGE_NAME CommonSystem")
        self.assertFalse(port.compare(bad,self.files(),"common_system")["passed"])

    def test_config_usage_and_missing_versions_fail(self):
        for key, text in [("portfile.cmake","vcpkg_cmake_config_fixup(PACKAGE_NAME common_system CONFIG_PATH wrong)"),
                          ("usage","target_link_libraries(main PRIVATE Common::System)")]:
            bad = self.files(); bad[key] = text
            self.assertTrue(port.semantics(bad,"common_system"))
        with self.assertRaises(ValueError): port.version({})


if __name__ == "__main__": unittest.main()
