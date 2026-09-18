#!/usr/bin/env python3
"""Build an explicit eight-repository snapshot and retain raw, promotable evidence."""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import platform
import shlex
import shutil
import subprocess
import sys

from ecosystem_lock import CHECKS, REPOSITORIES, read, sha, validate, validate_evidence

# The profile exercises installed C++20 packages, SQLite storage, and sibling
# integration. Logger binding stays at runtime; there is no thread -> logger edge.
DEPENDENCIES = {
    "common_system": (), "thread_system": ("common_system",),
    "container_system": ("common_system",),
    "logger_system": ("common_system", "thread_system"),
    "network_system": ("common_system", "thread_system", "container_system"),
    "monitoring_system": ("common_system", "thread_system"),
    "database_system": ("common_system", "thread_system", "container_system", "monitoring_system"),
    "pacs_system": REPOSITORIES[:-1],
}
TARGETS = {repo: f"{repo}::{repo}" for repo in REPOSITORIES}
TARGETS.update(common_system="kcenon::common_system", logger_system="logger_system::logger")
HEADERS = dict(zip(REPOSITORIES, (
    "common/patterns/result.h", "thread/core/thread_pool.h", "container/container.h",
    "logger/core/logger.h", "network/network_system.h", "monitoring/core/performance_monitor.h",
    "database/database_manager.h", "pacs/core/dicom_dataset.h")))
CONSUMERS = {
    "common_system": "kcenon::common::Result<int> result(42); return result.value() == 42 ? 0 : 1;",
    "thread_system": "kcenon::thread::thread_pool pool; return 0;",
    "container_system": "kcenon::container::value_container container; container.set(\"answer\", 42); return container.get<int>(\"answer\").value() == 42 ? 0 : 1;",
    "logger_system": "kcenon::logger::logger logger(false); return 0;",
    "network_system": "kcenon::network::facade::tcp_facade facade; return facade.create_client({}).is_err() ? 0 : 1;",
    "monitoring_system": "kcenon::monitoring::performance_monitor monitor; return monitor.get_name().empty() ? 1 : 0;",
    "database_system": "auto context = std::make_shared<kcenon::database::database_context>(); kcenon::database::database_manager manager(context); auto built = kcenon::database::integrated::unified_database_system::create_builder().build(); return 0;",
    "pacs_system": "kcenon::pacs::core::dicom_dataset dataset; kcenon::pacs::storage::pacs_database_adapter database(\":memory:\"); auto result = database.connect(); return dataset.empty() && result.is_ok() ? 0 : 1;",
}
OPTIONS = {
    "common_system": {"COMMON_BUILD_TESTS": False, "COMMON_BUILD_EXAMPLES": False, "COMMON_BUILD_BENCHMARKS": False,
                      "COMMON_BUILD_INTEGRATION_TESTS": False, "COMMON_BUILD_DOCS": False},
    "thread_system": {"BUILD_TESTS": False, "BUILD_EXAMPLES": False, "BUILD_BENCHMARKS": False,
                      "THREAD_BUILD_INTEGRATION_TESTS": False, "BUILD_DOCUMENTATION": False,
                      "KCENON_WITH_COMMON_SYSTEM": True, "KCENON_WITH_LOGGER_SYSTEM": False},
    "container_system": {"BUILD_TESTS": False, "BUILD_CONTAINER_SAMPLES": False,
                         "BUILD_CONTAINER_EXAMPLES": False, "BUILD_BENCHMARKS": False,
                         "CONTAINER_BUILD_INTEGRATION_TESTS": False, "KCENON_WITH_COMMON_SYSTEM": True},
    "logger_system": {"BUILD_TESTS": False, "BUILD_SAMPLES": False, "BUILD_BENCHMARKS": False,
                      "LOGGER_BUILD_INTEGRATION_TESTS": False, "NO_VCPKG": True,
                      "KCENON_WITH_COMMON_SYSTEM": True, "KCENON_WITH_THREAD_SYSTEM": True},
    "network_system": {"BUILD_TESTS": False, "BUILD_EXAMPLES": False,
                       "NETWORK_BUILD_INTEGRATION_TESTS": False, "NETWORK_BUILD_BENCHMARKS": False,
                       "KCENON_WITH_COMMON_SYSTEM": True, "KCENON_WITH_THREAD_SYSTEM": True,
                       "KCENON_WITH_CONTAINER_SYSTEM": True, "KCENON_WITH_LOGGER_SYSTEM": False},
    "monitoring_system": {"MONITORING_BUILD_TESTS": False, "MONITORING_BUILD_EXAMPLES": False,
                          "MONITORING_BUILD_INTEGRATION_TESTS": False, "MONITORING_BUILD_BENCHMARKS": False,
                          "KCENON_WITH_COMMON_SYSTEM": True, "KCENON_WITH_THREAD_SYSTEM": True,
                          "KCENON_WITH_LOGGER_SYSTEM": False, "KCENON_WITH_NETWORK_SYSTEM": False},
    "database_system": {"USE_UNIT_TEST": False, "BUILD_DATABASE_SAMPLES": False,
                        "DATABASE_BUILD_INTEGRATION_TESTS": False, "DATABASE_BUILD_BENCHMARKS": False,
                        "USE_POSTGRESQL": False, "USE_SQLITE": True, "USE_OPENSSL": True,
                        "KCENON_WITH_COMMON_SYSTEM": True, "KCENON_WITH_THREAD_SYSTEM": True,
                        "KCENON_WITH_CONTAINER_SYSTEM": True, "KCENON_WITH_MONITORING_SYSTEM": True},
    "pacs_system": {"PACS_BUILD_TESTS": False, "PACS_BUILD_EXAMPLES": False, "PACS_BUILD_SAMPLES": False,
                    "PACS_BUILD_BENCHMARKS": False, "PACS_BUILD_CODECS": False, "PACS_FETCH_OPENJPH": False,
                    "PACS_WITH_REST_API": False, "PACS_BUILD_STORAGE": True, "PACS_WARNINGS_AS_ERRORS": False,
                    "KCENON_WITH_COMMON_SYSTEM": True, "KCENON_WITH_CONTAINER_SYSTEM": True,
                    "KCENON_WITH_NETWORK_SYSTEM": True},
}


def prepare_asio(workspace, prefix):
    """Install the profile's pinned header-only ASIO prerequisite locally."""
    commit = "12e0ce9e0500bf0f247dbd1ae894272656456079"  # asio-1-30-2, peeled
    source = workspace / "asio-source"
    subprocess.run(["git", "init", "--quiet", str(source)], check=True)
    subprocess.run(["git", "-C", str(source), "fetch", "--depth=1",
                    "https://github.com/chriskohlhoff/asio.git", commit], check=True, timeout=300)
    subprocess.run(["git", "-C", str(source), "checkout", "--detach", "--quiet", commit], check=True)
    if output(["git", "rev-parse", "HEAD"], source) != commit:
        raise ValueError("ASIO prerequisite checkout mismatch")
    shutil.copytree(source / "asio/include", prefix / "include", dirs_exist_ok=True)
    config = prefix / "lib/cmake/asio"
    config.mkdir(parents=True)
    config.joinpath("asioConfig.cmake").write_text(
        'get_filename_component(_asio_prefix "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)\n'
        'if(NOT TARGET asio::asio)\n  add_library(asio::asio INTERFACE IMPORTED)\n'
        '  set_target_properties(asio::asio PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${_asio_prefix}/include"'
        ' INTERFACE_COMPILE_DEFINITIONS ASIO_STANDALONE)\nendif()\n'
        'set(asio_VERSION 1.30.2)\nset(asio_FOUND TRUE)\n')
    return {"version": "1.30.2", "sha": commit, "source": "https://github.com/chriskohlhoff/asio"}


def output(command, cwd=None):
    return subprocess.check_output(command, cwd=cwd, text=True, timeout=60).strip()


def run(command, cwd, log, timeout=3600):
    log.parent.mkdir(parents=True, exist_ok=True)
    print(f"{cwd.name}: {shlex.join(map(str, command))}", flush=True)
    # No credential is made available to candidate configure/build commands.
    env = {key: value for key, value in os.environ.items()
           if key not in ("GH_TOKEN", "GITHUB_TOKEN", "ECOSYSTEM_DISPATCH_TOKEN", "VCPKG_REGISTRY_PAT")}
    with log.open("w") as handle:
        handle.write(f"$ {shlex.join(map(str, command))}\n")
        handle.flush()
        try:
            code = subprocess.run(list(map(str, command)), cwd=cwd, env=env, stdout=handle,
                                  stderr=subprocess.STDOUT, timeout=timeout).returncode
        except subprocess.TimeoutExpired:
            handle.write("\nTIMEOUT\n")
            code = 124
    print(f"exit={code}; log={log}", flush=True)
    return code


def source_at(path, repo, commit):
    if not path.exists():
        path.mkdir(parents=True)
        subprocess.run(["git", "init", "--quiet", str(path)], check=True)
        subprocess.run(["git", "-C", str(path), "remote", "add", "origin",
                        f"https://github.com/kcenon/{repo}.git"], check=True)
        subprocess.run(["git", "-C", str(path), "fetch", "--depth=1", "origin", commit],
                       check=True, timeout=300)
        subprocess.run(["git", "-C", str(path), "checkout", "--detach", "--quiet", commit], check=True)
    actual = output(["git", "rev-parse", "HEAD"], path)
    if actual != commit:
        raise ValueError(f"{repo}: wanted {commit}, found {actual}; refusing to change an existing checkout")
    return bool(output(["git", "status", "--porcelain", "--untracked-files=normal"], path))


def toolchain(profile):
    compiler = os.environ.get("CXX", "c++")
    version = output([compiler, "--version"])
    if profile == "ubuntu-24.04-gcc13":
        release = Path("/etc/os-release").read_text() if Path("/etc/os-release").exists() else ""
        if 'VERSION_ID="24.04"' not in release or output([compiler, "-dumpversion"]) != "13":
            raise ValueError("ubuntu-24.04-gcc13 requires Ubuntu 24.04 and GCC 13")
    elif platform.system() != "Darwin" or "Apple clang" not in version:
        raise ValueError("macos-appleclang requires Darwin and Apple Clang")
    return {"compiler": version, "cmake": output(["cmake", "--version"]),
            "ninja": output(["ninja", "--version"]), "platform": platform.platform()}


def prelude(repo, prefix, path):
    lines = ["include_guard(GLOBAL)"]
    for dep in DEPENDENCIES[repo]:
        # Load the actual installed package, before legacy discovery can silently
        # substitute a system installation or an unpinned FetchContent revision.
        lines += [f'find_package({dep} CONFIG REQUIRED PATHS "{prefix}/lib/cmake/{dep}" "{prefix}/share/{dep}" NO_DEFAULT_PATH)',
                  f'if(NOT TARGET {TARGETS[dep]})',
                  f'  message(FATAL_ERROR "Pinned {dep} did not export {TARGETS[dep]}")', "endif()"]
    # Record effective values at directory end (after dependency detection).
    lines += ["function(coherence_effective_options)"]
    for key, value in OPTIONS[repo].items():
        if key.startswith("KCENON_WITH_") or (repo == "database_system" and key == "USE_SQLITE"):
            test = f"NOT {key}" if value else key
            lines += [f"  if({test})", f'    message(FATAL_ERROR "Profile option {key} was silently changed")', "  endif()"]
    if repo == "database_system":
        lines += ["  get_target_property(_definitions database COMPILE_DEFINITIONS)"]
        for feature in ("USE_SQLITE", "USE_CONTAINER_SYSTEM", "USE_THREAD_SYSTEM"):
            lines += [f'  if(NOT "{feature}" IN_LIST _definitions)',
                      f'    message(FATAL_ERROR "Database profile did not enable {feature}")', "  endif()"]
    lines += ["endfunction()", "cmake_language(DEFER CALL coherence_effective_options)"]
    path.write_text("\n".join(lines) + "\n")


def consumer(repo, prefix, root, jobs):
    source = root / "consumer-src"
    source.mkdir()
    source.joinpath("CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.24)\nproject(ecosystem_consumer LANGUAGES CXX)\n"
        "set(CMAKE_CXX_STANDARD 20)\nset(CMAKE_CXX_STANDARD_REQUIRED ON)\n"
        f'find_package({repo} CONFIG REQUIRED PATHS "{prefix}/lib/cmake/{repo}" NO_DEFAULT_PATH)\n'
        f'if(NOT TARGET {TARGETS[repo]})\n  message(FATAL_ERROR "Missing exported target")\nendif()\n'
        f"add_executable(consumer main.cpp)\ntarget_link_libraries(consumer PRIVATE {TARGETS[repo]})\n"
        + ("target_link_libraries(consumer PRIVATE database_system::integrated_database)\n" if repo == "database_system" else ""))
    extra = {"network_system": "#include <kcenon/network/facade/tcp_facade.h>\n",
             "database_system": "#include <kcenon/database/core/database_context.h>\n#include <kcenon/database/integrated/unified_database_system.h>\n",
             "pacs_system": "#include <kcenon/pacs/storage/pacs_database_adapter.h>\n"}.get(repo, "")
    source.joinpath("main.cpp").write_text(f"#include <kcenon/{HEADERS[repo]}>\n{extra}int main() {{ {CONSUMERS[repo]} }}\n")
    build = root / "consumer-build"
    code = run(["cmake", "-S", source, "-B", build, "-G", "Ninja", f"-DCMAKE_PREFIX_PATH={prefix}"],
               root, root / "consumer-configure.log", 600)
    if not code:
        code = run(["cmake", "--build", build, "--parallel", jobs], root, root / "consumer-build.log", 600)
    if not code:
        code = run([build / "consumer"], root, root / "consumer-run.log", 60)
    return code


def build(snapshot, workspace, source_root, jobs, allow_dirty=False):
    validate(snapshot, allow_candidate=True)
    workspace.mkdir(parents=True, exist_ok=False)
    prefix = workspace / "install"
    report = {"schema_version": 1, "profile": snapshot["profile"], "repositories": snapshot["repositories"],
              "registry_sha": snapshot["registry_sha"], "toolchain": toolchain(snapshot["profile"]),
              "run_url": os.environ.get("ECOSYSTEM_RUN_URL"),
              "orchestrator_sha": os.environ.get("ECOSYSTEM_ORCHESTRATOR_SHA"), "results": {}}
    for repo in REPOSITORIES:
        report["results"][repo] = {"sha": snapshot["repositories"][repo], "worktree_changes": None,
                                   "checks": {name: None for name in CHECKS},
                                   "dependencies": {dep: snapshot["repositories"][dep] for dep in DEPENDENCIES[repo]}}
    def save():
        (workspace / "evidence.json").write_text(json.dumps(report, indent=2) + "\n")
    save()
    report["toolchain"]["asio"] = prepare_asio(workspace, prefix)
    save()
    for repo in REPOSITORIES:
        entry = report["results"][repo]
        source = source_root / repo
        logs = workspace / repo
        logs.mkdir()
        try:
            dirty = source_at(source, repo, entry["sha"])
            entry["worktree_changes"] = dirty
            if dirty and not allow_dirty:
                raise ValueError("worktree is dirty; --allow-dirty is exploratory and cannot qualify a lock")
            entry["checks"]["conformance"] = run([sys.executable, "scripts/conformance_lint.py", "--root", "."],
                                                     source, logs / "conformance.log", 180)
            version_command = [sys.executable, "scripts/check_version_drift.py", "--project-root", ".", "--no-color", "--verbose"]
            if repo != "common_system":
                version_command.append("--no-matrix")
            entry["checks"]["version_drift"] = run(version_command, source, logs / "version-drift.log", 180)
            failed = [dep for dep in DEPENDENCIES[repo] if report["results"][dep]["checks"]["consumer"] != 0]
            if failed:
                raise ValueError(f"build blocked by failed dependencies: {', '.join(failed)}")
            hook = logs / "pinned-packages.cmake"
            prelude(repo, prefix, hook)
            command = ["cmake", "-S", source, "-B", logs / "build", "-G", "Ninja",
                       "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_INSTALL_LIBDIR=lib", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
                       f"-DCMAKE_INSTALL_PREFIX={prefix}", f"-DCMAKE_PREFIX_PATH={prefix}",
                       f"-DCMAKE_PROJECT_INCLUDE={hook}", "-DCMAKE_FIND_PACKAGE_PREFER_CONFIG=ON",
                       "-DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF", "-DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF"]
            command += [f"-D{key}={'ON' if value else 'OFF'}" for key, value in OPTIONS[repo].items()]
            command += [f"-DFETCHCONTENT_SOURCE_DIR_{dep.upper()}={source_root / dep}" for dep in REPOSITORIES if dep != repo]
            entry["checks"]["configure"] = run(command, source, logs / "configure.log", 1200)
            if entry["checks"]["configure"] == 0:
                entry["checks"]["build"] = run(["cmake", "--build", logs / "build", "--target", "install", "--parallel", jobs],
                                                  source, logs / "build.log")
            if entry["checks"]["build"] == 0:
                entry["checks"]["consumer"] = consumer(repo, prefix, logs, jobs)
        except (OSError, ValueError, subprocess.SubprocessError) as exc:
            entry["error"] = str(exc)
            print(f"{repo}: {exc}", file=sys.stderr, flush=True)
        finally:
            if (source / ".git").exists():
                entry["worktree_changes"] = bool(output(["git", "status", "--porcelain", "--untracked-files=normal"], source))
                entry["sha"] = output(["git", "rev-parse", "HEAD"], source)
            save()
    try:
        validate_evidence(report, snapshot["repositories"], snapshot["profile"])
        return 0
    except ValueError as exc:
        print(f"Snapshot is NOT eligible for promotion: {exc}", file=sys.stderr)
        return 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", type=Path, required=True)
    parser.add_argument("--work-dir", type=Path, required=True, help="must not exist")
    parser.add_argument("--source-root", type=Path, help="existing pinned checkouts; otherwise clone into work-dir/sources")
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--allow-dirty", action="store_true", help="exploratory only; always fails promotion eligibility")
    args = parser.parse_args()
    try:
        root = args.work_dir.resolve()
        return build(read(args.snapshot), root, args.source_root.resolve() if args.source_root else root / "sources",
                     args.jobs, args.allow_dirty)
    except (OSError, ValueError, subprocess.SubprocessError) as exc:
        print(f"ECOSYSTEM BUILD FAILED: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
