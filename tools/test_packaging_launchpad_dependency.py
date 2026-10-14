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
        TARGET_URL: {"self_link": TARGET_URL, "dependencies_collection_link": PAGES},
        SOURCE_URL: {"self_link": SOURCE_URL},
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

    def test_named_lookup_json_null_falls_back_without_false_failure(self):
        called = []

        def loader(url):
            called.append(url)
            if url == TARGET_URL:
                return None
            if url.startswith(checker.API + "/archives?"):
                return {"self_link": TARGET_URL,
                        "dependencies_collection_link": PAGES}
            raise AssertionError("unexpected URL " + url)

        archive = checker.archive(checker.TARGET, loader)
        self.assertEqual(archive["self_link"], TARGET_URL)
        self.assertEqual(len(called), 2)

    def test_null_direct_and_named_explain_exact_api_shape(self):
        with self.assertRaisesRegex(ValueError, "direct=JSON null.*getByReference=JSON null"):
            checker.archive(checker.TARGET, lambda _: None)

    def test_dependency_can_link_alternate_api_version(self):
        def loader(url):
            return fake_responses()(url) if url != PAGES else {
                "entries": [{"dependency_link": SOURCE_URL.replace("/devel/", "/1.0/"),
                             "pin_priority": 500, "pocket": "Release"}],
                "next_collection_link": None,
            }
        result = checker.inspect(loader)
        self.assertTrue(result["dependency_found"])
        self.assertEqual(result["pin_priorities"], [500])

    def test_reject_foreign_archive_identity(self):
        with self.assertRaisesRegex(ValueError, "invalid Launchpad archive identity"):
            checker.api_archive_path("https://evil.example/~hotzenplotz5/+archive/ubuntu/vdr-suite")

    def test_reject_wrong_archive_returned_for_reference(self):
        def loader(url):
            return {"self_link": SOURCE_URL}
        with self.assertRaisesRegex(ValueError, "unexpected archive identity"):
            checker.archive(checker.TARGET, loader)

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
