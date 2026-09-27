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

    def registry_files(self, repository):
        directory = Path(__file__).parent / "fixtures/registry-packages" / repository
        return {name: (directory / name).read_text() for name in ("portfile.cmake", "vcpkg.json", "usage")}

    def source_files(self, repository):
        return {name: text.replace("common_system", repository).replace("kcenon-common-system", "kcenon-" + repository.replace("_", "-"))
                for name, text in self.files("2.0.0").items()}

    def test_reviewed_legacy_releases_keep_their_exported_names(self):
        for repository in ("container_system", "logger_system", "network_system"):
            with self.subTest(repository=repository):
                result = port.compare(self.source_files(repository), self.registry_files(repository), repository)
                self.assertTrue(result["passed"], result)
                self.assertEqual(result["classification"], "different-version-snapshots")
                self.assertEqual(len(result["registry_contract"]["source_revision"]), 40)

    def test_network_usage_must_name_the_released_target(self):
        registry = self.registry_files("network_system")
        source = self.source_files("network_system")
        result = port.compare(source, registry, "network_system")
        self.assertTrue(result["passed"], result)
        registry["usage"] = registry["usage"].replace("NetworkSystem::network-all", "NetworkSystem::NetworkSystem")
        self.assertTrue(port.compare(source, registry, "network_system")["passed"])
        registry["usage"] = registry["usage"].replace("NetworkSystem::NetworkSystem", "NetworkSystem::missing")
        result = port.compare(source, registry, "network_system")
        self.assertFalse(result["passed"])
        self.assertIn("reviewed registry release", result["semantic_errors"]["registry"][0])

    def test_legacy_contract_is_not_applied_to_current_source(self):
        registry = self.registry_files("container_system")
        result = port.compare(registry, registry, "container_system")
        self.assertFalse(result["passed"])
        self.assertTrue(result["semantic_errors"]["source"])
        self.assertEqual(result["semantic_errors"]["registry"], [])

    def test_registry_contract_requires_the_reviewed_source_archive(self):
        original = self.registry_files("container_system")
        for old, new in (("kcenon/container_system", "kcenon/thread_system"),
                         ('"v${VERSION}"', '"main"'),
                         ("SHA512 3cf", "SHA512 4cf")):
            with self.subTest(change=new):
                registry = dict(original)
                registry["portfile.cmake"] = registry["portfile.cmake"].replace(old, new)
                with self.assertRaisesRegex(ValueError, "reviewed source archive"):
                    port.compare(self.source_files("container_system"), registry, "container_system")

    def test_legacy_package_config_and_every_usage_target_are_checked(self):
        original = self.registry_files("container_system")
        for file, old, new in (
            ("portfile.cmake", "PACKAGE_NAME ContainerSystem", "PACKAGE_NAME Wrong"),
            ("portfile.cmake", "CONFIG_PATH lib/cmake/ContainerSystem", "CONFIG_PATH lib/cmake/Wrong"),
            ("usage", "find_package(ContainerSystem", "find_package(Wrong"),
            ("usage", "ContainerSystem::container)", "ContainerSystem::container ContainerSystem::missing)"),
            ("usage", "    target_link_libraries", "    # target_link_libraries"),
        ):
            with self.subTest(change=new):
                registry = dict(original)
                registry[file] = registry[file].replace(old, new)
                self.assertFalse(port.compare(self.source_files("container_system"), registry, "container_system")["passed"])

    def test_unreviewed_versions_do_not_inherit_legacy_names(self):
        registry = self.registry_files("container_system")
        manifest = json.loads(registry["vcpkg.json"])
        manifest["version-semver"] = "0.1.1"
        registry["vcpkg.json"] = json.dumps(manifest)
        self.assertFalse(port.compare(self.source_files("container_system"), registry, "container_system")["passed"])

    def test_port_revision_preserves_archive_contract_but_not_byte_equality(self):
        registry = self.registry_files("container_system")
        manifest = json.loads(registry["vcpkg.json"])
        manifest["port-version"] += 1
        registry["vcpkg.json"] = json.dumps(manifest)
        self.assertTrue(port.compare(self.source_files("container_system"), registry, "container_system")["passed"])
        canonical = self.files()
        changed = dict(canonical, usage=canonical["usage"] + "\n# changed guidance\n")
        self.assertFalse(port.compare(canonical, changed, "common_system")["passed"])


if __name__ == "__main__": unittest.main()
