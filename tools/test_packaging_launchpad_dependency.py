#!/usr/bin/env python3
"""No-network tests of the read-only Launchpad PPA dependency verification."""
import importlib.util
from pathlib import Path
import unittest

SCRIPT = Path(__file__).with_name("check_launchpad_ppa_dependency.py")
spec = importlib.util.spec_from_file_location("lp_dep_check", SCRIPT)
checker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checker)

TARGET_URL = checker.API + "/~hotzenplotz5/+archive/ubuntu/vdr-suite"
SOURCE_URL = checker.API + "/~seahawk1986-hotmail/+archive/ubuntu/vdr-2.8.3"
PAGES = TARGET_URL + "/dependencies"


def fake_responses(source_present=True, next_page=False):
    docs = {
        checker.API + "/archives?ws.op=getByReference&reference=ppa%3Ahotzenplotz5%2Fvdr-suite": {
            "self_link": TARGET_URL,
            "dependencies_collection_link": PAGES,
        },
        checker.API + "/archives?ws.op=getByReference&reference=ppa%3Aseahawk1986-hotmail%2Fvdr-2.8.3": {
            "self_link": SOURCE_URL,
        },
        PAGES: {
            "entries": [{"dependency_link": SOURCE_URL if source_present else
                         checker.API + "/~someone-else/+archive/ubuntu/other",
                         "pin_priority": 1001, "pocket": "Release"}],
            "next_collection_link": PAGES + "?page=2" if next_page else None,
        },
        PAGES + "?page=2": {
            "entries": [{"dependency_link": checker.API +
                         "/~someone-else/+archive/ubuntu/auxiliary",
                         "pin_priority": 500, "pocket": "Release"}],
            "next_collection_link": None,
        },
    }

    def loader(url):
        if url not in docs:
            raise AssertionError("unexpected URL: " + url)
        return docs[url]
    return loader


class LaunchpadDependencyTests(unittest.TestCase):
    def test_expected_archive_dependency_present(self):
        data = checker.inspect(fake_responses())
        self.assertTrue(data["dependency_found"])
        self.assertEqual(data["dependency_count"], 1)
        self.assertEqual(data["pin_priorities"], [1001])
        self.assertFalse(data["series_package_builds_verified"])

    def test_missing_dependency_fails_closed(self):
        data = checker.inspect(fake_responses(source_present=False))
        self.assertFalse(data["dependency_found"])
        self.assertEqual(data["pin_priorities"], [])

    def test_multiple_pages(self):
        data = checker.inspect(fake_responses(next_page=True))
        self.assertEqual(data["dependency_count"], 2)
        self.assertTrue(data["dependency_found"])

    def test_collection_rejects_missing_records(self):
        with self.assertRaisesRegex(ValueError, "has no entries"):
            checker.dependency_records(PAGES, lambda _: {})

    def test_rejects_pagination_cycle(self):
        with self.assertRaisesRegex(ValueError, "pagination"):
            checker.dependency_records(PAGES, lambda u: {
                "entries": [], "next_collection_link": u})

    def test_fetch_rejects_non_launchpad_hosts_without_network(self):
        for url in ("http://api.launchpad.net/devel/archives",
                    "https://example.org/", "file:///etc/passwd"):
            with self.subTest(url=url), self.assertRaises(ValueError):
                checker.fetch(url)


if __name__ == "__main__":
    unittest.main()
