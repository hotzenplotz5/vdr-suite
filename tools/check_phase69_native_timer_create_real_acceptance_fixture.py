#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

fixture = ROOT / "tools/phase69-runtime-acceptance/prepare_native_timer_create_fixture.cpp"
phase69_make = ROOT / "mk/phase69-public-api-tests.mk"
ci_groups = ROOT / "mk/test-groups.mk"
daemon_sources = ROOT / "mk/daemon-sources.mk"
install_make = ROOT / "mk/install.mk"
doc = ROOT / "docs/development/phase-69c-native-timer-create-productive-runtime.md"

for path in [
    fixture,
    phase69_make,
    ci_groups,
    daemon_sources,
    install_make,
    doc,
]:
    if not path.is_file():
        raise SystemExit(
            "missing Timer CREATE real acceptance fixture boundary file: "
            + str(path.relative_to(ROOT))
        )

fixture_text = fixture.read_text(encoding="utf-8")
for marker in [
    "TimerIntentRepository intentRepository(database);",
    "TimerAssignmentRepository assignmentRepository(database);",
    "intentRepository.create(draft)",
    "intentRepository.update(",
    "TimerAssignmentSchedulingService scheduler(",
    "scheduler.schedulePrimary(scheduling)",
    "TimerAssignmentState::selected",
    "FIXTURE_PREPARATION=PASS",
    "--self-test",
]:
    if marker not in fixture_text:
        raise SystemExit(
            "Timer CREATE real acceptance fixture missing domain marker: "
            + marker
        )

for forbidden in [
    "INSERT INTO",
    "UPDATE timer_",
    "DELETE FROM",
    "REPLACE INTO",
    "sqlite3",
    "database.execute(",
    "std::system(",
]:
    if forbidden in fixture_text:
        raise SystemExit(
            "Timer CREATE real acceptance fixture must not bypass domain "
            "repositories/scheduler: " + forbidden
        )

make_text = phase69_make.read_text(encoding="utf-8")
for marker in [
    "test-phase69-native-timer-create-real-acceptance-fixture:",
    "prepare_native_timer_create_fixture.cpp",
    "phase69-native-timer-create-acceptance-fixture --self-test",
    "check_phase69_native_timer_create_real_acceptance_fixture.py",
]:
    if marker not in make_text:
        raise SystemExit(
            "Timer CREATE real acceptance fixture build/self-test marker missing: "
            + marker
        )

ci_text = ci_groups.read_text(encoding="utf-8")
if "test-phase69-native-timer-create-real-acceptance-fixture" not in ci_text:
    raise SystemExit(
        "Timer CREATE real acceptance fixture self-test must run in CI fast tests"
    )

for path in [daemon_sources, install_make]:
    text = path.read_text(encoding="utf-8")
    if "phase69-native-timer-create-acceptance-fixture" in text or        "prepare_native_timer_create_fixture.cpp" in text:
        raise SystemExit(
            "real acceptance fixture must remain outside production/install: "
            + str(path.relative_to(ROOT))
        )

doc_text = doc.read_text(encoding="utf-8")
for marker in [
    "Acceptance prerequisite gap",
    "domain-backed acceptance fixture",
    "TimerAssignmentSchedulingService",
    "no raw SQL fixture writes",
]:
    if marker not in doc_text:
        raise SystemExit(
            "productive Timer CREATE documentation missing acceptance-fixture marker: "
            + marker
        )

print("Phase-69.C real Timer CREATE acceptance fixture boundary check passed")
print("Boundary: acceptance creates the missing selected state through TimerIntentRepository + TimerAssignmentSchedulingService only")
