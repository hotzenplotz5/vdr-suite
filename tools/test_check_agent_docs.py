#!/usr/bin/env python3
"""Isolated fixture tests for advisory agent-documentation checker."""

import contextlib
import io
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

import check_agent_docs as checker


class AgentDocAuditTests(unittest.TestCase):
    def setUp(self):
        self.temp = TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.write('AGENTS.md', '# Agent rules\n')
        self.write('docs/development/agent-rule-register.md',
                   '| ID | Rule |\n| --- | --- |\n| AG-000 | Start |\n| AG-001 | Consent |\n')

    def write(self, name, data):
        file = self.root / name
        file.parent.mkdir(parents=True, exist_ok=True)
        file.write_text(data, encoding='utf-8')

    def run_scan(self):
        return checker.scan(self.root)[0]

    def test_valid_links_and_one_row_per_rule(self):
        self.write('docs/development/target.md', '# A\n')
        self.write('docs/development/agent-guide.md',
                   '[target](target.md#section) [external](https://example.org/x)\n')
        self.assertEqual(self.run_scan(), [])

    def test_duplicate_registry_id_reports_both_lines(self):
        self.write('docs/development/agent-rule-register.md',
                   '| AG-001 | a |\n| AG-001 | b |\n')
        finding, = self.run_scan()
        self.assertEqual(finding.kind, 'DUPLICATE_ID')
        self.assertEqual(finding.line, 2)
        self.assertIn('line 1', finding.explanation)

    def test_nested_agents_broken_link_and_traversal(self):
        self.write('modules/AGENTS.md',
                   '[missing](../docs/no-such-file.md)\n'
                   '[escape](../../etc/shadow)\n')
        self.assertEqual([x.kind for x in self.run_scan()],
                         ['BROKEN_LINK', 'BROKEN_LINK'])

    def test_stale_command_but_historical_mentions_are_allowed(self):
        self.write('docs/development/agent-history.md',
                   'Historisch war `sudo -u yavdr` falsch.\n'
                   '~~~python\nsudo -u yavdr\n~~~\n'
                   '~~~bash\nsudo -u yavdr make daemon\n~~~\n')
        finding, = self.run_scan()
        self.assertEqual(finding.kind, 'OBSOLETE_COMMAND')
        self.assertEqual(finding.line, 6)

    def test_obsolete_normative_sentence_only(self):
        self.write('AGENTS.md',
                   'This explains why `No root compilation.` was removed.\n'
                   '- No root compilation.\n')
        finding, = self.run_scan()
        self.assertEqual(finding.kind, 'OBSOLETE_RULE')
        self.assertEqual(finding.line, 2)

    def test_exit_zero_even_with_findings(self):
        self.write('AGENTS.md', '[missing](missing.md)\n')
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            rc = checker.main(['--root', str(self.root)])
        self.assertEqual(rc, 0)
        self.assertIn('AGENT_DOC_AUDIT=WARNINGS', output.getvalue())
        self.assertIn('ADVISORY_ONLY=YES', output.getvalue())

    def test_ignores_generated_hidden_path(self):
        self.write('.build/AGENTS.md', '[missing](nope.md)\n')
        self.assertEqual(self.run_scan(), [])


if __name__ == '__main__':
    unittest.main()
