#!/usr/bin/env python3
"""Read-only Launchpad check: does VDR-Suite's PPA depend on Seahawk VDR 2.8.3?

This reads public HTTPS JSON. It never authenticates, edits PPAs, installs
software, signs packages, changes APT sources, or touches the local VDR.
"""
import argparse
import json
import sys
from urllib.parse import urlencode, urlparse
from urllib.request import Request, urlopen
from urllib.error import HTTPError, URLError

API = "https://api.launchpad.net/devel"
TARGET = "ppa:hotzenplotz5/vdr-suite"
DEPENDENCY = "ppa:seahawk1986-hotmail/vdr-2.8.3"
MAX_BYTES = 2 * 1024 * 1024
MAX_PAGES = 20


def fetch(url):
    parsed = urlparse(url)
    if parsed.scheme != "https" or parsed.hostname != "api.launchpad.net":
        raise ValueError("unexpected Launchpad API URL (refusing network request)")
    request = Request(url, headers={"Accept": "application/json",
                                    "User-Agent": "vdr-suite-packaging-verify/1"})
    with urlopen(request, timeout=12) as response:
        if urlparse(response.geturl()).hostname != "api.launchpad.net":
            raise ValueError("Launchpad API redirected to an unexpected host")
        raw = response.read(MAX_BYTES + 1)
    if len(raw) > MAX_BYTES:
        raise ValueError("oversize API response")
    document = json.loads(raw)
    if not isinstance(document, dict):
        raise ValueError("unexpected Launchpad API response")
    return document


def archive(reference, loader=fetch):
    url = API + "/archives?" + urlencode(
        {"ws.op": "getByReference", "reference": reference})
    doc = loader(url)
    if not isinstance(doc.get("self_link"), str):
        raise ValueError("PPA lookup has no self_link: " + reference)
    return doc


def dependency_records(collection_url, loader=fetch):
    if not collection_url:
        raise ValueError("archive has no dependencies_collection_link")
    seen = set()
    entries = []
    url = collection_url
    while url:
        if url in seen or len(seen) >= MAX_PAGES:
            raise ValueError("unexpected Launchpad dependency pagination")
        seen.add(url)
        doc = loader(url)
        batch = doc.get("entries")
        if not isinstance(batch, list):
            raise ValueError("Launchpad dependency collection has no entries")
        entries.extend(batch)
        url = doc.get("next_collection_link")
    return entries


def inspect(loader=fetch):
    target = archive(TARGET, loader)
    source = archive(DEPENDENCY, loader)
    entries = dependency_records(target.get("dependencies_collection_link"), loader)
    matching = [
        record for record in entries
        if isinstance(record, dict)
        and record.get("dependency_link", "").rstrip("/") ==
            source["self_link"].rstrip("/")
    ]
    result = {
        "target": TARGET,
        "expected_dependency": DEPENDENCY,
        "dependency_found": len(matching) > 0,
        "dependency_count": len(entries),
        "pin_priorities": [x.get("pin_priority") for x in matching],
        "pockets": [x.get("pocket") for x in matching],
        "series_package_builds_verified": False,
        "note": ("Archive link verification only: confirms neither availability "
                 "of VDR 2.8.3 binaries for noble/resolute nor actual build selection.")
    }
    return result


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", action="store_true", help="machine-readable output")
    args = parser.parse_args(argv)
    try:
        result = inspect()
    except (HTTPError, URLError, OSError, ValueError, KeyError,
            TypeError, json.JSONDecodeError) as exc:
        print("LAUNCHPAD_DEPENDENCY_CHECK=UNKNOWN (" + str(exc) + ")", file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps(result, indent=2, sort_keys=True))
    else:
        print("TARGET_PPA=" + TARGET)
        print("EXPECTED_BUILD_DEPENDENCY=" + DEPENDENCY)
        print("DEPENDENCY_PIN_PRIORITIES=" + ",".join(map(str, result["pin_priorities"])))
        print("LAUNCHPAD_DEPENDENCY_CHECK=" +
              ("PASS" if result["dependency_found"] else "FAIL"))
        print("UBUNTU_VDR_ABI_VALIDATION=PENDING")
    return 0 if result["dependency_found"] else 1


if __name__ == "__main__":
    sys.exit(main())
