"""MT.I3.3: SQLite regression against the Core's ACTUAL batch scheduling SQL.

Extract the production query and Agent command UNIQUE INDEX definitions from
their existing C++ owners, rather than maintaining a test-only second query.
All databases are temporary fixtures; there is no VDR/Agent/daemon process.
"""
from __future__ import annotations

import ast
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest

REPO = Path(__file__).resolve().parents[2]
DAEMON = REPO / "core/daemon/src/DaemonRuntime.cpp"
ASSIGNMENT = REPO / "core/agent/src/BackendAgentMediaImportAssignment.cpp"

NOW = 1700000000
BATCH = "mb1_" + "a" * 32
GLOB = "mb1_" + "[0-9a-f]" * 32 + "_[0-9][0-9][0-9]"
QUOTED = r'"(?:\\.|[^"\\])*"'


def joined_cpp_literals(expression: str) -> str:
    pieces = re.findall(QUOTED, expression)
    if not pieces:
        raise AssertionError("no C++ SQL string literals")
    return "".join(ast.literal_eval(piece) for piece in pieces)


def production_schedule_sql() -> str:
    code = DAEMON.read_text(encoding="utf-8")
    method = code.split("void DaemonRuntime::progressDurableMediaImportBatches()", 1)[1]
    method = method.split("void DaemonRuntime::pollScheduledMediaImport()", 1)[0]
    # Keep the 32-hex/three-digit filter aligned with the query's C++ builder.
    assert 'std::string("mb1_")' in method
    assert "std::string(32U, '@')" in method
    assert '"_[0-9][0-9][0-9]"' in method
    assert 'ch == \'@\' ? "[0-9a-f]"' in method
    match = re.search(r"const char\* sql\s*=\s*((?:" + QUOTED + r"\s*)+);", method)
    if match is None:
        raise AssertionError("Core batch progression SQL not found")
    result = joined_cpp_literals(match.group(1))
    assert "FROM mutation_operations" in result
    assert "NOT EXISTS" in result
    return result


def production_agent_unique_indexes() -> tuple[str, str]:
    code = ASSIGNMENT.read_text(encoding="utf-8")
    body = code.split("bool BackendAgentCommandRepository::ensureMediaImportAssignmentSchema()", 1)[1]
    body = body.split("namespace vdrsuite::agent", 1)[0]
    expressions = re.findall(r"database_\.execute\(\s*((?:" + QUOTED + r"\s*)+)\)", body)
    if len(expressions) != 2:
        raise AssertionError("Expected two Core/Agent media import uniqueness indexes")
    statements = tuple(joined_cpp_literals(expr) for expr in expressions)
    assert all("CREATE UNIQUE INDEX" in statement for statement in statements)
    return statements


def operations_db(path: Path) -> sqlite3.Connection:
    db = sqlite3.connect(path)
    db.execute(
        "CREATE TABLE IF NOT EXISTS mutation_operations ("
        "operation_id TEXT PRIMARY KEY, actor_id TEXT NOT NULL, "
        "backend_id TEXT NOT NULL, action_family TEXT NOT NULL, "
        "state TEXT NOT NULL, idempotency_key TEXT NOT NULL, "
        "requested_at INTEGER NOT NULL, deadline INTEGER NOT NULL)"
    )
    return db


def add_batch(db: sqlite3.Connection, batch: str, count: int = 7,
              *, actor: str = "actor-a", backend: str = "backend-a",
              initial: str = "accepted") -> list[str]:
    ids = []
    for position in range(count):
        operation_id = actor + "/" + backend + "/" + batch + "/" + str(position)
        ids.append(operation_id)
        db.execute(
            "INSERT INTO mutation_operations VALUES (?,?,?,?,?,?,?,?)",
            (operation_id, actor, backend, "media.import", initial,
             batch + f"_{position:03d}", NOW - 10, NOW + 3600))
    db.commit()
    return ids


def ready(db: sqlite3.Connection, sql: str) -> list[str]:
    return [row[0] for row in db.execute(sql, (GLOB, NOW)).fetchall()]


def update(db: sqlite3.Connection, identifier: str, state: str) -> None:
    db.execute("UPDATE mutation_operations SET state=? WHERE operation_id=?",
               (state, identifier))
    db.commit()


class DurableBatchSqlTests(unittest.TestCase):
    def setUp(self) -> None:
        self.sql = production_schedule_sql()

    def test_seven_items_survive_db_reopen_and_only_advance_after_success(self):
        with tempfile.TemporaryDirectory(prefix="mt-i33-core-restart-") as temp:
            filename = Path(temp) / "durable.db"
            db = operations_db(filename)
            ids = add_batch(db, BATCH)
            self.assertEqual(ready(db, self.sql), ids[:1])
            update(db, ids[0], "dispatching")
            self.assertEqual(ready(db, self.sql), [])
            update(db, ids[0], "executed_unverified")
            self.assertEqual(ready(db, self.sql), [])
            update(db, ids[0], "succeeded")
            db.close()  # Simulated daemon process/database reopen.
            reopened = operations_db(filename)
            self.addCleanup(reopened.close)
            self.assertEqual(ready(reopened, self.sql), ids[1:2])
            for index in range(1, 6):
                update(reopened, ids[index], "succeeded")
                self.assertEqual(ready(reopened, self.sql), ids[index + 1:index + 2])
            update(reopened, ids[6], "succeeded")
            self.assertEqual(ready(reopened, self.sql), [])
            self.assertEqual(reopened.execute(
                "SELECT COUNT(*) FROM mutation_operations").fetchone()[0], 7)

    def test_unknown_failure_and_deadline_block_followup_without_replay(self):
        for state in ("outcome_unknown", "failed_verified",
                      "failed_before_dispatch", "cancelled", "dispatching"):
            with self.subTest(state=state), tempfile.TemporaryDirectory() as temp:
                db = operations_db(Path(temp) / "operations.db")
                ids = add_batch(db, BATCH, 3)
                update(db, ids[0], state)
                self.assertEqual(ready(db, self.sql), [])
                self.assertEqual(db.execute(
                    "SELECT COUNT(*) FROM mutation_operations").fetchone()[0], 3)
                db.close()
        with tempfile.TemporaryDirectory() as temp:
            db = operations_db(Path(temp) / "operations.db")
            add_batch(db, BATCH, 2)
            db.execute("UPDATE mutation_operations SET deadline=?", (NOW,))
            db.commit()
            self.assertEqual(ready(db, self.sql), [])
            db.close()

    def test_other_batches_and_actors_do_not_block_independent_work(self):
        with tempfile.TemporaryDirectory() as temp:
            db = operations_db(Path(temp) / "operations.db")
            first = add_batch(db, BATCH, 2, actor="actor-a")
            other = add_batch(db, BATCH, 2, actor="actor-b")
            separate = add_batch(db, "mb1_" + "b" * 32, 2, actor="actor-a")
            self.assertEqual(set(ready(db, self.sql)),
                             {first[0], other[0], separate[0]})
            update(db, first[0], "outcome_unknown")
            self.assertEqual(set(ready(db, self.sql)), {other[0], separate[0]})
            db.execute(
                "INSERT INTO mutation_operations VALUES (?,?,?,?,?,?,?,?)",
                ("not-a-batch", "actor-a", "backend-a", "media.import",
                 "accepted", "mb1_" + "c" * 31 + "_000",
                 NOW - 10, NOW + 3600))
            db.commit()
            self.assertNotIn("not-a-batch", ready(db, self.sql))
            db.close()

    def test_real_agent_unique_indexes_reject_duplicate_command_and_readback(self):
        statements = production_agent_unique_indexes()
        with tempfile.TemporaryDirectory() as temp:
            db = sqlite3.connect(Path(temp) / "commands.db")
            db.execute(
                "CREATE TABLE backend_agent_commands ("
                "backend_id TEXT, operation_id TEXT, command_type TEXT)"
            )
            for statement in statements:
                db.execute(statement)
            for kind in ("addon.media.import", "addon.media.import.readback"):
                with self.subTest(kind=kind):
                    args = ("backend-a", "same-operation", kind)
                    db.execute("INSERT INTO backend_agent_commands VALUES (?,?,?)", args)
                    with self.assertRaises(sqlite3.IntegrityError):
                        db.execute("INSERT INTO backend_agent_commands VALUES (?,?,?)", args)
                    # A different backend is a different idempotency scope.
                    db.execute("INSERT INTO backend_agent_commands VALUES (?,?,?)",
                               ("backend-b", "same-operation", kind))
            db.close()


if __name__ == "__main__":
    unittest.main()
