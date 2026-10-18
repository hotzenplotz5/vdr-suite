#!/usr/bin/env python3
"""P7 regression: preserve original schema tuples across additive runtime seeds."""
import unittest

from packaging_p7_runtime_schema_rehearsal import prior_rows_preserved


class PriorRowsPreservedTests(unittest.TestCase):
    def setUp(self):
        self.old = {
            "security_human_accounts": [("old-account", "old-actor", "Old")],
            "mutation_operations": [("old-operation", 1, "queued")],
        }

    def test_identical_first_open(self):
        self.assertTrue(prior_rows_preserved(self.old, self.old))

    def test_new_rows_on_second_open_do_not_invalidate_preservation(self):
        after = {
            "security_human_accounts": [
                ("old-account", "old-actor", "Old"),
                ("new-account", "new-actor", "New"),
            ],
            "mutation_operations": [
                ("old-operation", 1, "queued"),
                ("new-operation", 1, "queued"),
            ],
        }
        self.assertTrue(prior_rows_preserved(self.old, after))

    def test_mutated_original_row_fails(self):
        after = {
            **self.old,
            "mutation_operations": [("old-operation", 1, "succeeded")],
        }
        self.assertFalse(prior_rows_preserved(self.old, after))

    def test_missing_original_row_fails(self):
        after = {**self.old, "security_human_accounts": []}
        self.assertFalse(prior_rows_preserved(self.old, after))

    def test_missing_table_fails(self):
        self.assertFalse(prior_rows_preserved(
            self.old, {"security_human_accounts": self.old["security_human_accounts"]}))


if __name__ == "__main__":
    unittest.main()
