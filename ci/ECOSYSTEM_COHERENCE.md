# Ecosystem coherence operations

This implements the remaining orchestration in [#701](https://github.com/kcenon/common_system/issues/701).
An implementation branch, an advisory job conclusion, and a candidate snapshot
are not an accepted last-green lock. Common owns snapshot resolution, promotion,
cross-builds and port audits; downstream repositories only run local gates and
send/receive validated notifications.

## Source copies and option compatibility

Make shared validator changes in common and commit the tested revision first.
For each downstream complete checkout, run:

```sh
python3 scripts/sync_coherence.py --target ../thread_system --source-revision FULL_COMMON_SHA
python3 scripts/sync_coherence.py --target ../thread_system --source-revision FULL_COMMON_SHA --check
```

The generated `ci/coherence-source.json` identifies every copied file by source
path and SHA256. Repository configuration and workflow files remain local.
Run `python3 -m unittest discover -s scripts/tests -v` in each repository.

`KCENON_WITH_<REPOSITORY>` is the canonical CMake sibling option. Explicit
canonical values win conflicts with a warning; legacy-only values and defaults
retain their behavior, including OFF and existing-cache reconfiguration. The
helper does not cache inferred canonical defaults or FORCE parent/cache values.
Ports send both spellings while they still reference releases predating the
helper. This preserves installation of historical archives. Thread's deprecated
logger option remains disabled; logger uses thread as an optional backend.

## Bootstrap and reproduce a snapshot

`ci/ecosystem-lock.json` is deliberately absent until all eight repositories
have passing evidence. Create a reviewed `ci/ecosystem-candidate.json` with this
schema and resolve every chosen source and registry branch to a full SHA once:

```json
{
  "schema_version": 1,
  "status": "candidate",
  "profile": "ubuntu-24.04-gcc13",
  "repositories": {
    "common_system": "FULL_40_HEX_SHA",
    "thread_system": "FULL_40_HEX_SHA",
    "container_system": "FULL_40_HEX_SHA",
    "logger_system": "FULL_40_HEX_SHA",
    "network_system": "FULL_40_HEX_SHA",
    "monitoring_system": "FULL_40_HEX_SHA",
    "database_system": "FULL_40_HEX_SHA",
    "pacs_system": "FULL_40_HEX_SHA"
  },
  "registry_sha": "FULL_40_HEX_SHA"
}
```

The placeholders are documentation, not valid pins. The resolver rejects them,
branch names, abbreviated SHAs and missing repositories. For first bootstrap:

```sh
python3 scripts/ecosystem_lock.py resolve --bootstrap ci/ecosystem-candidate.json --output candidate.json
python3 scripts/ecosystem_build.py --snapshot candidate.json --work-dir /tmp/ecosystem-build --jobs 2
```

Use a fresh work directory. Existing source checkouts can be supplied with
`--source-root`; their HEADs must match the tuple and they must be clean.
`--allow-dirty` is for local diagnosis and cannot qualify a lock. No resolver
falls back to `main` or `develop`. Once an accepted lock exists, omit bootstrap
and optionally supply `--candidate-repo thread_system --candidate-sha FULL_SHA`.
All other source and registry pins remain unchanged.

The Ubuntu profile uses Ubuntu 24.04, GCC 13, CMake/Ninja and native OpenSSL,
SQLite, ICU, zlib, LZ4 and curl development packages. The macOS profile uses
Apple Clang with the equivalent native packages. ASIO 1.30.2 is installed into
the isolated prefix from its pinned upstream commit. Tool versions and ASIO
identity are recorded with each result. Thread also emits simdutf provenance.
This C++20 header profile builds common, thread, container, logger, network,
monitoring, database, then PACS, installing each before compiling, linking and
running its public-header consumer. Dependencies are imported from that prefix
before legacy discovery. Database enables SQLite and container/thread/monitoring
integration; PACS enables file-backed SQLite storage and every pinned sibling. Optional image
codecs, REST frameworks, PostgreSQL and C++ modules are outside this profile.
Logger binding remains at runtime where that is the repository's default.
The existing in-memory SQLite path in PACS delegates to an unimplemented DAL
backend and is outside this profile, as are the domain backend projects in #684.

Every raw gate/configure/build/consumer result is recorded, including explicit
missing results after a failed prerequisite. A failed, skipped, cancelled,
dirty or advisory-failed repository cannot qualify. The `skip-cross-build` PR
label skips execution and fails lock eligibility; it never advances a lock.
The runner audits every source checkout again after the last build and retains
any earlier dirty observation. `--allow-dirty` marks the whole run exploratory,
even when its final worktrees happen to be clean.

For hosted validation, dispatch `ecosystem-cross-build.yml` at the reviewed implementation ref with
`bootstrap=true`. After the complete run succeeds, download its
`ecosystem-evidence` artifact and prepare a promotion:

```sh
python3 scripts/ecosystem_lock.py promote --candidate candidate.json --evidence evidence.json --lock ci/ecosystem-lock.json
```

Promotion verifies the exact raw tuple, profile, clean worktrees, successful
central Actions run, orchestrator SHA and matching original report artifact.
It rejects evidence from PR runs and stale lock digests. Review the resulting
lock in a PR. Failed candidates leave the accepted lock unchanged. Roll back
by restoring a previously reviewed accepted lock and reproducing its tuple.
The lock-containing revision is distinct from all source revisions it records.
Concurrent local promotions are serialized by a `.json.promoting` sidecar. If
an interrupted process leaves that file behind, confirm the process has ended
before removing it and retrying against the current accepted lock.

## Dispatch routes and credentials

All default branches are `main`. Receivers must be delivered there before
repository dispatch works; merging only into `develop` does not activate them.
Preserve newer default-branch workflow updates when delivering these files.

| Route | Event | Receiver workflows |
| --- | --- | --- |
| common to seven downstream repositories | `coherence-check-v1` | `coherence-receiver.yml` |
| downstream to common | `coherence-change-v1` | `ecosystem-cross-build.yml`, `port-sync-check.yml` |

Payloads contain schema version, allowlisted origin/candidate repository, full
candidate SHA, lock-containing common revision, and a deterministic correlation
ID. Requests also identify the receiver's exact selected SHA. Notifications and
results are separate: receivers never notify, preventing fan-out loops. Retries
reuse correlated runs; concurrent duplicate checks have no mutation side effects.

The selected automation identity is an existing fine-grained personal access
token. Configure it as `ECOSYSTEM_DISPATCH_TOKEN` in each sending repository.
It needs target repository contents write (repository_dispatch) and actions read
(run/artifact inspection). A GitHub App installation token can be supplied through
the same environment after an App-token setup step; do not store a short-lived
installation token as a permanent secret. No such credential was present during
initial implementation. A default `GITHUB_TOKEN` cannot perform cross-repository
dispatch. The separate release sync still needs `VCPKG_REGISTRY_PAT`.

Credentialed senders run only on trusted branch events, never fork PR builds.
Receivers check out policy at their workflow revision, validate the payload and
accepted lock, then check out the intended candidate/locked commit with a read
token and no persisted checkout credential. Candidate builds receive no dispatch
credential. Fork PRs retain their local checks without secrets.

Senders wait for matching receiver runs and verify the `coherence-result`
artifact's correlation, tuple and raw zero exit. An HTTP 204, green advisory
wrapper, missing artifact or timeout is not passing evidence. Run links and
errors are retained in dispatch artifacts. After repairing a failed receiver,
rerun that Actions run or dispatch a new candidate/lock identity.

## Release and port boundaries

`tag-reality.yml` audits published releases/manual tags. The reusable registry
sync checks independent release identity before generation, then checks the
stored generated port hash before installation and registry PR publication. It
uses validators from a reviewed immutable common revision, so an old tag need
not already contain the new scripts. Existing independent archive redownload
and vcpkg installation checks remain in place. The policy input is advisory
until that repository's release track is promoted.

These release-triggered checks detect an already published source tag; the
enforcing boundary they can prevent is registry sync/publication. A release
producer wanting to gate source publication must run identity validation before
publishing its release, using the independently recorded merge SHA. Never move
an existing tag or require an archive to contain its own checksum.

Port sync fetches source overlays and registry files at explicit snapshot SHAs.
It retains byte comparisons, package/config path checks, usage target checks,
and version comparisons. Different version snapshots are reported as staged
differences, while semantic failures still fail raw validation. Same-version
content drift remains a finding. The scheduled/manual workflow stays advisory;
dispatch result acceptance still requires raw zero. The existing default-branch
scheduled-failure reporter is retained.

## Enforcement and required statuses

After remediation lands, rerun raw conformance and version drift against the
then-current clean `develop` HEAD. Record its SHA and reports, promote
`ci/coherence.json` in a separate change, and verify warning/failure fixtures.
Release promotion separately requires independently verifiable release records
and matching port/archive evidence. Do not promote it from local-only fixtures.

Use the guarded common-owned helper after landing remediation:

```sh
python3 scripts/promote_coherence.py prepare --root ../thread_system --repository thread_system
# Review and land the separate policy change, then use a clean checkout of it.
python3 scripts/promote_coherence.py require --root ../thread_system --repository thread_system
# The same command with --apply updates the previewed develop protection.
```

The helper rejects dirty or unlanded commits, reruns raw gates and fixtures,
checks that develop did not advance, and requires successful emitted Actions
statuses before preparing a settings update. For an existing protected branch,
it patches only required status checks and preserves existing entries. A
ruleset-only protection that cannot be read through the branch protection API
requires a separate review of that ruleset; it is not bypassed.

Only after the enforcing checks are delivered and emit reliably, add
`cross-system conformance linter` and `SOUP Version Drift Detection` to the
appropriate branch's required-check settings, preserving existing checks and
rules. Those gate workflows have no top-level path filter. Unrelated required
checks and review policies must remain intact. Until the settings API is
actually updated, record required status as pending rather than complete.
