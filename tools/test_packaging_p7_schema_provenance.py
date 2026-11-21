#!/usr/bin/env python3
"""P7: verify old-column fixture DDL against byte-exact historic C++ owners.

The test reads only two vendored, Git-blob-verified historical source files
and one explicitly synthetic SQL fixture. No live SQLite DB is accessed.
"""
import ast
import hashlib
from pathlib import Path
import re
import unittest

HERE = Path(__file__).resolve().parent / "fixtures" / "packaging"
FIXTURE = HERE / "p7-pre-revision-security-operations.sql"
SOURCES = (
    ("security_human_accounts",
     HERE / "historical" / "HumanAccountRepository-20260930.cpp",
     "9ad863d9af74ba725436db782c0ae88e192d45ed",
     "revision"),
    ("mutation_operations",
     HERE / "historical" / "MutationOperationRepository-20260811.cpp",
     "ac4106441bce3fbe77cd26921c01805fccf73429",
     "expected_resource_fingerprint"),
)


def git_blob_sha1(raw):
    return hashlib.sha1(
        b"blob " + str(len(raw)).encode("ascii") + b"\x00" + raw
    ).hexdigest()


def owner_create_table_sql(source, table):
    # The earlier ensureSchema() SQL is assembled from C++ string literals.
    # Restrict extraction to the function body; fail on unexpected layout.
    match = re.search(r"::ensureSchema\(\)\s*\{(.*?)\n\}", source, re.S)
    if match is None:
        raise ValueError("historical ensureSchema body missing: " + table)
    tokens = re.findall(r'"(?:\\.|[^"\\])*"', match.group(1))
    if not tokens:
        raise ValueError("historical ensureSchema has no SQL: " + table)
    sql = "".join(ast.literal_eval(token) for token in tokens)
    result = re.findall(
        r"CREATE TABLE IF NOT EXISTS " + re.escape(table) +
        r" \([\s\S]*?\);", sql
    )
    if len(result) != 1:
        raise ValueError("ambiguous historical table DDL: " + table)
    return result[0]


def fixture_create_table_sql(fixture, table):
    result = re.findall(
        r"CREATE TABLE " + re.escape(table) + r" \([\s\S]*?\);",
        fixture
    )
    if len(result) != 1:
        raise ValueError("ambiguous synthetic table DDL: " + table)
    return result[0]


def normalized_ddl(sql):
    # Whitespace outside quoted values is immaterial for these specific
    # literal old schemas: compare *every* table constraint, not just columns.
    # Both verified DDL definitions have no whitespace-sensitive SQL literals.
    return re.sub(r"\s+", "", sql).casefold().replace("ifnotexists", "")


class HistoricalCppSchemaProvenanceTests(unittest.TestCase):
    def test_old_column_ddl_is_identical_to_historical_repository_sources(self):
        fixture = FIXTURE.read_text(encoding="utf-8")
        for table, source_file, git_sha, added_column in SOURCES:
            with self.subTest(table=table):
                raw = source_file.read_bytes()
                self.assertEqual(git_blob_sha1(raw), git_sha,
                                 "historical C++ source provenance mismatch")
                source = raw.decode("utf-8")
                old_ddl = owner_create_table_sql(source, table)
                fixture_ddl = fixture_create_table_sql(fixture, table)
                self.assertNotIn(added_column, normalized_ddl(old_ddl))
                self.assertEqual(normalized_ddl(fixture_ddl),
                                 normalized_ddl(old_ddl))

    def test_modified_old_schema_fails_provenance_gate(self):
        table, source_file, git_sha, _ = SOURCES[0]
        raw = source_file.read_bytes()
        self.assertEqual(git_blob_sha1(raw), git_sha)
        self.assertNotEqual(git_blob_sha1(raw + b"\n"), git_sha)
        fixture = FIXTURE.read_text(encoding="utf-8")
        original = fixture_create_table_sql(fixture, table)
        changed = original.replace("active INTEGER NOT NULL DEFAULT 1",
                                   "active INTEGER NOT NULL DEFAULT 0")
        self.assertNotEqual(normalized_ddl(original), normalized_ddl(changed))


if __name__ == "__main__":
    unittest.main()
