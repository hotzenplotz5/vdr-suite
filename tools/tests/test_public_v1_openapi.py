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

    def errors_for(self, document=None, source=None):
        return guard.validate(
            self.document if document is None else document,
            self.source if source is None else source,
        )

    def assert_rejected(self, document=None, source=None, reason=""):
        errors = self.errors_for(document, source)
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

    def test_partial_coverage_cannot_claim_complete(self):
        changed = copy.deepcopy(self.document)
        changed["x-vdr-suite-coverage"] = "complete"
        self.assert_rejected(changed, reason="partial coverage")


if __name__ == "__main__":
    unittest.main()
