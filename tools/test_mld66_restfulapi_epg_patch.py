#!/usr/bin/env python3
"""Offline smoke test for the pinned RESTfulAPI empty-EPG-root patch.

The snippets below are copied from the two affected functions in upstream
yavdr/vdr-plugin-restfulapi at 79f9bd26370a08d6c5186c19cefa300c569045ea.
This verifies patch context + isolated C++ behavior, NOT a native MLD build.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

PATCH = (
    Path(__file__).resolve().parents[1]
    / "packaging/mld66/patches/0001-restfulapi-guard-empty-epg-image-root.patch"
)

HEADER = """#pragma once
#include <string>
using namespace std;
struct StringExtension {
  static string replace(string const& text, string const& substring,
                        string const& replacement);
};
struct Scraper2VdrService {
  string epgImagesDir;
  string cleanImagePath(string path);
};
"""

TOOLS = """#include "test_support.h"
string StringExtension::replace(string const& text, string const& substring, string const& replacement)
{
  std::string result = text;
  std::string::size_type pos = 0;
  while ( ( pos = result.find( substring, pos ) ) != std::string::npos ) {
    result.replace( pos, substring.length(), replacement );
    pos += replacement.length();
  }
  return result;
}
"""

SCRAPER = """#include "test_support.h"
string Scraper2VdrService::cleanImagePath(string path) {

  path = StringExtension::replace(path, epgImagesDir, "");
  path.erase(0, path.find_first_not_of("/"));
  return path;
};
"""

MAIN = """#include "test_support.h"
#include <cassert>
int main() {
  assert(StringExtension::replace("hello", "", "") == "hello");
  assert(StringExtension::replace("aba", "a", "z") == "zbz");
  Scraper2VdrService svc;
  svc.epgImagesDir = "";
  assert(svc.cleanImagePath("/posters/foo.jpg").empty());
  svc.epgImagesDir = "/var/cache/vdr/epgimages";
  assert(svc.cleanImagePath("/var/cache/vdr/epgimages/posters/foo.jpg")
         == "posters/foo.jpg");
}
"""


class RestfulapiEmptyEpgPatchTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which("patch") and shutil.which("g++"),
                         "requires GNU patch and g++")
    def test_patch_applies_and_fixed_functions_return(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for name, data in (
                ("test_support.h", HEADER),
                ("tools.cpp", TOOLS),
                ("scraper2vdr.cpp", SCRAPER),
                ("test_main.cpp", MAIN),
            ):
                (root / name).write_text(data, encoding="utf-8")

            patch = subprocess.run(
                ["patch", "--batch", "--fuzz=0", "-p1", "-d", str(root),
                 "-i", str(PATCH)],
                capture_output=True, text=True, check=False, timeout=10
            )
            self.assertEqual(patch.returncode, 0, patch.stdout + patch.stderr)
            build = subprocess.run(
                ["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                 str(root / "tools.cpp"), str(root / "scraper2vdr.cpp"),
                 str(root / "test_main.cpp"), "-o", str(root / "smoke")],
                capture_output=True, text=True, check=False, timeout=10
            )
            self.assertEqual(build.returncode, 0, build.stdout + build.stderr)
            run = subprocess.run(
                [str(root / "smoke")], capture_output=True, text=True,
                check=False, timeout=10
            )
            self.assertEqual(run.returncode, 0, run.stdout + run.stderr)


if __name__ == "__main__":
    unittest.main()
