#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(path: str, needle: str) -> None:
    if needle not in read(path):
        raise AssertionError(f"{path}: missing required contract: {needle}")


def require_count(path: str, needle: str, minimum: int) -> None:
    count = read(path).count(needle)
    if count < minimum:
        raise AssertionError(
            f"{path}: expected at least {minimum} occurrences of {needle}, "
            f"observed {count}"
        )


def forbid(path: str, needle: str) -> None:
    if needle in read(path):
        raise AssertionError(f"{path}: forbidden contract present: {needle}")


def main() -> int:
    repository_header = (
        "core/security/include/BrowserSessionCredentialRepository.h"
    )
    repository_source = (
        "core/security/src/BrowserSessionCredentialRepository.cpp"
    )
    authenticator = "core/security/include/BrowserSessionAuthenticator.h"
    test = "core/security/tests/test_browser_session_issuer_binding.cpp"
    makefile = "mk/security-sources.mk"
    document = (
        "docs/development/"
        "phase-62-slice-2t-browser-session-issuer-binding.md"
    )

    for path in (
        repository_header,
        repository_source,
        authenticator,
        test,
        makefile,
        document,
    ):
        if not (ROOT / path).is_file():
            raise AssertionError(f"missing Slice 2T contract file: {path}")

    require(repository_header, "findResolvedByTokenId(")
    require(repository_source, "LEFT JOIN security_credentials AS issuing")
    require(
        repository_source,
        "issuing.credential_id = browser.issued_from_credential_id",
    )
    require(repository_source, "issuing.actor_id = browser.actor_id")
    require(repository_source, "issuing.active <> 0")
    require(repository_source, "issuing.expires_at <= CURRENT_TIMESTAMP")
    require(repository_source, "issuing.revoked_at <> ''")
    require_count(
        authenticator,
        "repository_.findResolvedByTokenId(",
        2,
    )
    forbid(authenticator, "repository_.findByTokenId(tokenId)")

    require(test, "credential-issuer-active")
    require(test, "credential-issuer-expired")
    require(test, "credential-issuer-inactive")
    require(test, "credential-issuer-other-actor")
    require(test, "credential-issuer-missing")
    require(test, "rawAfterIssuerRevocation->active")
    require(test, "resolvedAfterIssuerRevocation->revoked")
    require(test, "request-issuer-revoked-get")
    require(test, "request-issuer-revoked-logout")
    require(test, "csrf_validation_failed")
    require(test, "PRAGMA foreign_keys = OFF")

    require(makefile, "test-security-browser-session-issuer-binding")
    require(makefile, "tools/check_browser_session_issuer_binding.py")
    require(
        makefile,
        "core/security/tests/test_browser_session_issuer_binding.cpp",
    )

    require(document, "issued_from_credential_id")
    require(document, "ordinary-route browser authentication")
    require(document, "CSRF verification")
    require(document, "no schema migration")
    require(document, "idle timeout")
    require(document, "cleanup")
    require(document, "security administration")
    require(
        "docs/development/index.md",
        "phase-62-slice-2t-browser-session-issuer-binding.md",
    )

    print("browser session issuer lifecycle binding contracts passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(
            "browser session issuer lifecycle architecture check failed: "
            f"{error}",
            file=sys.stderr,
        )
        raise SystemExit(1)
