#!/usr/bin/env python3
"""Regression tests: a damaged audited spec/source MUST fail the checker."""

import copy
import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import check_public_v1_openapi as guard


class OpenApiDriftTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.document = json.loads((ROOT / "api/openapi.json").read_text(encoding="utf-8"))
        cls.source = (ROOT / "api/rest/src/PublicApiRuntime.cpp").read_text(encoding="utf-8")
        cls.device_source = (ROOT / "core/daemon/src/RecordingMediaHttpRuntime.cpp").read_text(encoding="utf-8")
        cls.security_source = (ROOT / "core/security/include/SecurityHttpGate.h").read_text(encoding="utf-8")

    def errors_for(self, document=None, source=None, device_source=None, security_source=None):
        return guard.validate(
            self.document if document is None else document,
            self.source if source is None else source,
            self.device_source if device_source is None else device_source,
            self.security_source if security_source is None else security_source,
        )

    def assert_rejected(self, document=None, source=None, reason="", device_source=None, security_source=None):
        errors = self.errors_for(document, source, device_source, security_source)
        self.assertTrue(any(reason in e for e in errors), errors)

    def test_current_audited_contract_and_handlers(self):
        self.assertEqual([], self.errors_for())

    def test_missing_path(self):
        changed = copy.deepcopy(self.document)
        del changed["paths"]["/api/v1/backends"]
        self.assert_rejected(changed, reason="audited path inventory changed")

    def test_missing_post(self):
        changed = copy.deepcopy(self.document)
        del changed["paths"]["/api/v1/devices/{deviceId}/grants"]["post"]
        self.assert_rejected(changed, reason="documented methods changed")

    def test_wrong_handler(self):
        source = self.source.replace(
            "if (publicDeviceCredentialRotationPath(",
            "if (obsoleteDeviceCredentialRotationPath(",
            1,
        )
        self.assert_rejected(source=source, reason="source route handler anchor disappeared")

    def test_missing_recording_browse_route(self):
        changed = copy.deepcopy(self.document)
        del changed["paths"]["/api/v1/recordings"]
        self.assert_rejected(changed, reason="audited path inventory changed")

    def test_missing_epg_handler(self):
        changed_source = self.source.replace(
            "if (path == PublicEpgNowNextPath)",
            "if (path == ObsoleteEpgNowNextPath)",
            1,
        )
        self.assert_rejected(source=changed_source, reason="source route handler anchor disappeared")

    def test_dangling_schema(self):
        changed = copy.deepcopy(self.document)
        changed["paths"]["/api/v1/accounts"]["get"]["responses"]["200"]["content"]["application/json"]["schema"] = {
            "$ref": "#/components/schemas/DeletedAccountCollection",
        }
        self.assert_rejected(changed, reason="missing referenced schema")

    def test_wrong_path_parameter(self):
        changed = copy.deepcopy(self.document)
        changed["paths"]["/api/v1/accounts/{accountId}"]["get"]["parameters"][0]["name"] = "actorId"
        self.assert_rejected(changed, reason="path variable mismatch")

    def test_missing_if_match(self):
        changed = copy.deepcopy(self.document)
        parameters = changed["paths"]["/api/v1/devices/{deviceId}/credentials/{credentialId}/rotate"]["post"]["parameters"]
        for param in parameters:
            if param.get("name") == "If-Match":
                param["required"] = False
        self.assert_rejected(changed, reason="strong If-Match requirement drifted")

    def test_missing_idempotency(self):
        changed = copy.deepcopy(self.document)
        del changed["paths"]["/api/v1/accounts"]["post"]["parameters"][0]["required"]
        self.assert_rejected(changed, reason="Idempotency-Key requirement drifted")

    def test_security_scope_weakened(self):
        changed = copy.deepcopy(self.document)
        changed["paths"]["/api/v1/accounts/{accountId}/grants"]["post"]["x-vdr-suite-authentication"]["actorRequired"] = False
        self.assert_rejected(changed, reason="actorRequired does not match")

    def test_missing_device_tracks_route(self):
        changed = copy.deepcopy(self.document)
        del changed["paths"]["/api/v1/recording-playback-sessions/{sessionId}/tracks"]
        self.assert_rejected(changed, reason="audited path inventory changed")

    def test_missing_device_stop_body(self):
        changed = copy.deepcopy(self.document)
        del changed["paths"]["/api/v1/recording-playback-sessions/{sessionId}/stop"]["post"]["requestBody"]
        self.assert_rejected(changed, reason="required POST body missing")

    def test_device_source_handler_removed(self):
        removed = self.device_source.replace(
            'const std::string tracksSuffix = "/tracks";',
            'const std::string obsoleteTracksSuffix = "/tracks";',
            1,
        )
        self.assert_rejected(device_source=removed, reason="Device source route handler anchor disappeared")

    def test_device_composition_missing(self):
        removed = self.device_source.replace(
            "apiRouter.setDeviceRecordingPlaybackHandler(",
            "apiRouter.setObsoleteDeviceRecordingPlaybackHandler(",
            1,
        )
        self.assert_rejected(device_source=removed, reason="Device Recording playback composition handler disappeared")

    def test_device_security_gate_removed(self):
        removed = self.security_source.replace(
            "isPublicDeviceRecordingSubtitleSelection",
            "isObsoleteDeviceRecordingSubtitleSelection",
        )
        self.assert_rejected(security_source=removed, reason="Device SecurityHttpGate admission marker disappeared")

    def test_device_grant_documentation_weakened(self):
        changed = copy.deepcopy(self.document)
        changed["paths"]["/api/v1/recording-playback-sessions"]["post"]["x-vdr-suite-authentication"]["authorization"] = "Browser allowed"
        self.assert_rejected(changed, reason="Device playback ownership/play grant boundary missing")

    def test_device_subtitle_no_request_body(self):
        changed = copy.deepcopy(self.document)
        del changed["paths"]["/api/v1/recording-playback-sessions/{sessionId}/subtitles"]["post"]["requestBody"]
        self.assert_rejected(changed, reason="required POST body missing")

    def test_partial_coverage_cannot_claim_complete(self):
        changed = copy.deepcopy(self.document)
        changed["x-vdr-suite-coverage"] = "complete"
        self.assert_rejected(changed, reason="partial coverage")


if __name__ == "__main__":
    unittest.main()
