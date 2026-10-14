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
    # A named operation can return JSON null instead of an archive object.
    # Validate at the call site so a direct-resource fallback can be tried.
    return json.loads(raw)


def archive(reference, loader=fetch):
    """Use a public archive's canonical entry, falling back to getByReference.

    Both endpoints are documented Launchpad read-only GET routes. The named
    method can return JSON null for an unresolved reference; do not treat
    that result as evidence that a PPA dependency is missing.
    """
    locations = {
        TARGET: API + "/~hotzenplotz5/+archive/ubuntu/vdr-suite",
        DEPENDENCY: API + "/~seahawk1986-hotmail/+archive/ubuntu/vdr-2.8.3",
    }
    if reference not in locations:
        raise ValueError("unsupported archive reference")
    named = API + "/archives?" + urlencode(
        {"ws.op": "getByReference", "reference": reference})
    problems = []
    for label, url in (("direct", locations[reference]), ("getByReference", named)):
        try:
            document = loader(url)
        except (HTTPError, URLError, OSError, ValueError) as exc:
            if isinstance(exc, HTTPError):
                reason = "HTTP " + str(exc.code)
            else:
                reason = type(exc).__name__
            problems.append(label + "=" + reason)
            continue
        if isinstance(document, dict) and isinstance(document.get("self_link"), str):
            if api_archive_path(document["self_link"]) != api_archive_path(locations[reference]):
                problems.append(label + "=unexpected archive identity")
                continue
            return document
        actual = "null" if document is None else type(document).__name__
        problems.append(label + "=JSON " + actual)
    raise ValueError("archive lookup unresolved for " + reference + ": " +
                     ", ".join(problems))


def api_archive_path(link):
    """Compare version-independent archive identities, refusing foreign links."""
    parsed = urlparse(link)
    if parsed.scheme != "https" or parsed.hostname != "api.launchpad.net":
        raise ValueError("invalid Launchpad archive identity URL")
    parts = parsed.path.split("/", 2)
    if len(parts) != 3 or parts[1] not in ("devel", "1.0"):
        raise ValueError("unexpected Launchpad API version in archive link")
    return "/" + parts[2].rstrip("/")

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


def archive_dependency_lookup(target, source, loader=fetch):
    """Independent read-only lookup; failure is inconclusive, not absence."""
    url = target["self_link"] + "?" + urlencode({
        "ws.op": "getArchiveDependency",
        "dependency": source["self_link"],
    })
    try:
        item = loader(url)
    except (HTTPError, URLError, OSError, ValueError):
        return None
    if not isinstance(item, dict):
        return None
    try:
        if (api_archive_path(item["archive_link"]) !=
                api_archive_path(target["self_link"])):
            return None
        if (api_archive_path(item["dependency_link"]) !=
                api_archive_path(source["self_link"])):
            return None
    except (TypeError, KeyError, ValueError):
        return None
    return item


def inspect(loader=fetch):
    target = archive(TARGET, loader)
    source = archive(DEPENDENCY, loader)
    entries = dependency_records(target.get("dependencies_collection_link"), loader)
    # Publish only canonical *public archive paths* for diagnostics. Do not
    # log arbitrary API URL values or credentials returned by other services.
    observed = []
    for item in entries:
        if not isinstance(item, dict):
            continue
        dependency_link = item.get("dependency_link")
        if not isinstance(dependency_link, str):
            continue
        try:
            public_path = api_archive_path(dependency_link)
        except ValueError:
            public_path = "INVALID_ARCHIVE_IDENTITY"
        observed.append({
            "archive_path": public_path,
            "pin_priority": item.get("pin_priority"),
            "pocket": item.get("pocket")
        })
    matching = [
        record for record in entries
        if isinstance(record, dict)
        and isinstance(record.get("dependency_link"), str)
        and api_archive_path(record["dependency_link"]) ==
            api_archive_path(source["self_link"])
    ]
    direct_lookup = None
    if not matching:
        direct_lookup = archive_dependency_lookup(target, source, loader)
        if direct_lookup is not None:
            matching.append(direct_lookup)
    inconclusive = not matching and not entries
    result = {
        "target": TARGET,
        "expected_dependency": DEPENDENCY,
        "dependency_found": len(matching) > 0,
        "dependency_count": len(entries),
        "public_api_inconclusive": inconclusive,
        "verification_method": ("collection" if matching and direct_lookup is None else
                                "named_lookup" if direct_lookup is not None else "none"),
        "observed_dependencies": observed,
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
    parser.add_argument("--diagnose", action="store_true",
                        help="print only public archive identities and priorities on failure")
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
        if args.diagnose:
            print("OBSERVED_DEPENDENCY_COUNT=" + str(result["dependency_count"]))
            for item in result["observed_dependencies"]:
                print("OBSERVED_DEPENDENCY=" + item["archive_path"] +
                      " pin=" + str(item["pin_priority"]) +
                      " pocket=" + str(item["pocket"]))
        print("LAUNCHPAD_DEPENDENCY_CHECK=" +
              ("PASS" if result["dependency_found"] else
               "UNKNOWN (public API returned no dependencies; check authenticated UI)"
               if result["public_api_inconclusive"] else "FAIL"))
        if result["dependency_found"]:
            print("VERIFIED_BY=" + result["verification_method"])
        print("UBUNTU_VDR_ABI_VALIDATION=PENDING")
    return (0 if result["dependency_found"] else
            2 if result["public_api_inconclusive"] else 1)


if __name__ == "__main__":
    sys.exit(main())
