#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        raise SystemExit(f"missing TimerIntent application file: {path}")
    return target.read_text(encoding="utf-8")

def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise SystemExit(f"missing {label}: {needle}")

makefile = read("Makefile")
source = read("core/timers/src/TimerIntentApplicationService.cpp")
test = read("core/timers/tests/test_timer_intent_application_service.cpp")
doc = read("docs/development/phase-64-timer-intent-application.md")
include = "include mk/phase64-timer-intent-application-tests.mk"
require(makefile, include, "TimerIntent application Make include")
if makefile.count(include) != 1:
    raise SystemExit("TimerIntent application Make include must occur once")
for needle in (
    "intentRepository_.create(request.intent)",
    "timerIntentSemanticIdentity(current.spec)",
    "TimerIntentState::active",
    "assignmentRepository_.findById(request.timerAssignmentId)",
    "scheduling_.schedulePrimary(schedulingRequest)",
    "assignment.timerAssignmentId != request.timerAssignmentId",
    "fulfillment_.beginProvisioning(",
    "TimerAssignmentFulfillmentStatus::alreadyProvisioning",
):
    require(source, needle, "restart-safe application entry point")
for needle in (
    "provisioningStarted",
    "alreadyProvisioning",
    "intentConflict",
    "noEligibleBackend",
):
    require(test, needle, "entry-point regression")
require(doc, "deterministic primary", "scheduling documentation")
require(doc, "restart-safe", "recovery documentation")
print("Phase 64 TimerIntent application guard passed")
