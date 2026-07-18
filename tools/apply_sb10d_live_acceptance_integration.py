#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(path: str, old: str, new: str) -> None:
    target = ROOT / path
    text = target.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(
            f"{path}: expected one occurrence, found {count}: {old!r}"
        )
    target.write_text(text.replace(old, new, 1), encoding="utf-8")


replace_once(
    "tools/run_sb10d_live_acceptance.py",
    '''        run(
            [
                "install",
''',
    '''        plugin_staged = True
        run(
            [
                "install",
''',
)

replace_once(
    "tools/run_sb10d_live_acceptance.py",
    '''        if not suitebridge_config_links():
            raise AcceptanceError("vdrctl did not create a Suite Bridge configuration link")
        plugin_staged = True

        run(["systemctl", "restart", VDR_SERVICE])
''',
    '''        if not suitebridge_config_links():
            raise AcceptanceError("vdrctl did not create a Suite Bridge configuration link")

        run(["systemctl", "restart", VDR_SERVICE])
''',
)

for anchor in (
    '''\ttest-suite-bridge-daemon-runtime-wiring \\
\ttest-fast \\
''',
    '''\ttest-suite-bridge-daemon-runtime-wiring \\
\ttest-backend-node \\
''',
):
    replacement = anchor.replace(
        "\ttest-suite-bridge-daemon-runtime-wiring \\\n",
        "\ttest-suite-bridge-daemon-runtime-wiring \\\n"
        "\ttest-sb10d-live-acceptance-contract \\\n",
    )
    replace_once("mk/test-groups.mk", anchor, replacement)

replace_once(
    "mk/test-groups.mk",
    '''\ttest-real-polling-stability \\
\ttest-suite-bridge-svdrp-transport-live
''',
    '''\ttest-real-polling-stability \\
\ttest-suite-bridge-svdrp-transport-live \\
\ttest-real-suite-bridge-embedded-runtime-live
''',
)

replace_once(
    "docs/architecture/suite-bridge-embedded-agent-runtime.md",
    '''---

## Back
''',
    '''---

## Controlled Acceptance Runner

The repository-owned runner executes the complete opt-in live sequence and always
attempts rollback:

```text
python3 tools/run_sb10d_live_acceptance.py
```

It refuses a pre-existing Suite Bridge installation, records only hashes for
channel, Timer, Recording and setup state, stops and later restores an active
`vdr-suite-daemon.service`, stages the plugin, starts the repository daemon with
SB.10d enabled, runs safe REST probes, restarts VDR, requires a changed plugin
epoch, verifies clean worker and daemon shutdown, removes the staged plugin and
configuration, restarts VDR, restores the original daemon-service state and
requires a clean worktree.

The runner uses no destructive VDR-Suite API operation.

---

## Back
''',
)

print("SB.10d controlled live acceptance integration prepared")
