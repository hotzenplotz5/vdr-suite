#!/usr/bin/env python3

import base64
import hashlib
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CHUNK_PATTERN = (
    ROOT
    / "web/frontend/assets/recording-genre-poster-sprite.jpg.b64.*"
)
EXPECTED_SIZE = 51173
EXPECTED_SHA256 = (
    "5c92421c839c8fc5b6ebc06723fbeaf15a80392350036efb2c4c56a8f597f5f1"
)
SVG_PREFIX = (
    '<svg xmlns="http://www.w3.org/2000/svg" width="384" height="384" '
    'viewBox="0 0 384 384"><image width="384" height="384" '
    'href="data:image/jpeg;base64,'
)
SVG_SUFFIX = '"/></svg>\n'


def main() -> int:
    if len(sys.argv) != 2:
        print(
            "usage: build_recording_genre_artwork.py OUTPUT",
            file=sys.stderr,
        )
        return 2

    chunks = sorted(ROOT.glob(str(CHUNK_PATTERN.relative_to(ROOT))))
    if not chunks:
        print("recording genre artwork chunks not found", file=sys.stderr)
        return 1

    encoded = "".join(
        chunk.read_text(encoding="ascii").strip()
        for chunk in chunks
    )

    try:
        image = base64.b64decode(encoded, validate=True)
    except ValueError as error:
        print(f"invalid recording genre artwork base64: {error}", file=sys.stderr)
        return 1

    digest = hashlib.sha256(image).hexdigest()
    if len(image) != EXPECTED_SIZE or digest != EXPECTED_SHA256:
        print(
            "recording genre artwork payload mismatch: "
            f"size={len(image)} sha256={digest}",
            file=sys.stderr,
        )
        return 1

    if not image.startswith(b"\xff\xd8\xff"):
        print("recording genre artwork is not a JPEG", file=sys.stderr)
        return 1

    output = Path(sys.argv[1])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(
        SVG_PREFIX + encoded + SVG_SUFFIX,
        encoding="ascii",
    )

    print(
        "recording genre artwork built: "
        f"{output} ({len(image)} JPEG bytes)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
