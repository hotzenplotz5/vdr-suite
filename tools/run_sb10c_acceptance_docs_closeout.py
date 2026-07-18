#!/usr/bin/env python3

from pathlib import Path

updater = Path(__file__).with_name("update_sb10c_acceptance_docs.py")
text = updater.read_text(encoding="utf-8")

old = '''replace_once(
    ROADMAP,
    "- [SB.10b Local SVDRP Transport](../../docs/architecture/suite-bridge-svdrp-transport.md)\\n",
    "- [SB.10b Local SVDRP Transport](../../docs/architecture/suite-bridge-svdrp-transport.md)\\n- [SB.10c Observation Lifecycle](../../docs/architecture/suite-bridge-observation-lifecycle.md)\\n",
)
'''

new = '''replace_once(
    ROADMAP,
    "- [SB.9 Capability Discovery](SB-9-capability-discovery.md)\\n- [SB.10b Local SVDRP Transport](../../docs/architecture/suite-bridge-svdrp-transport.md)\\n",
    "- [SB.9 Capability Discovery](SB-9-capability-discovery.md)\\n- [SB.10b Local SVDRP Transport](../../docs/architecture/suite-bridge-svdrp-transport.md)\\n- [SB.10c Observation Lifecycle](../../docs/architecture/suite-bridge-observation-lifecycle.md)\\n",
)
'''

if text.count(old) != 1:
    raise RuntimeError("SB.10c roadmap navigation updater block is not unique")

patched = text.replace(old, new, 1)
namespace = {"__name__": "__main__", "__file__": str(updater)}

try:
    exec(compile(patched, str(updater), "exec"), namespace)
except Exception as error:
    print(f"SB10C_CLOSEOUT_ERROR: {error}", flush=True)
    raise SystemExit(1)
