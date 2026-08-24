#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "vdr-plugin-suite-bridge/suitebridge_live_source.h"


def require(condition: bool, message: str) -> None:
    if not condition:
        print(f"ERROR: {message}", file=sys.stderr)
        raise SystemExit(1)


def main() -> int:
    source = SOURCE.read_text(encoding="utf-8")

    required = (
        "Channels->SwitchTo(channel->Number())",
        "device = cDevice::ActualDevice()",
        "device->IsTunedToTransponder(channel)",
        "device->AttachReceiver(receiver.get())",
        "device->Detach(receiver.get())",
    )
    for fragment in required:
        require(fragment in source, f"missing VDR live-switch contract: {fragment}")

    forbidden = (
        "cDevice::GetDevice(channel, LIVEPRIORITY, false)",
        "device->SwitchChannel(channel, false)",
    )
    for fragment in forbidden:
        require(fragment not in source, f"SuiteBridge must not bypass VDR live switching: {fragment}")

    switch_pos = source.index("Channels->SwitchTo(channel->Number())")
    actual_device_pos = source.index("device = cDevice::ActualDevice()")
    attach_pos = source.index("device->AttachReceiver(receiver.get())")
    require(
        switch_pos < actual_device_pos < attach_pos,
        "SuiteBridge must switch through VDR before resolving ActualDevice and attaching",
    )

    detach_pos = source.index("device->Detach(receiver.get())")
    receiver_reset_pos = source.index("receiver.reset();", detach_pos)
    require(
        detach_pos < receiver_reset_pos,
        "SuiteBridge must detach the VDR receiver before destroying it",
    )

    print("suite bridge VDR live-switch contract ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
