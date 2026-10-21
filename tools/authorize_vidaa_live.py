#!/usr/bin/env python3
"""Grant an already paired TV its explicit Public-v1 read/live permissions.

Uses the normal audited Device-grant API and a short-lived administrator browser
session. No SQLite edits, pairing reset, credential dumps or auth bypass.
Requires verified HTTPS, a real administrator login, CSRF and grant-set ETags.
"""
from __future__ import annotations

import argparse
import base64
import getpass
import json
import re
import ssl
import sys
import urllib.error
import urllib.parse
import urllib.request

DEVICE = re.compile(r"^device_[0-9a-f]{32}$")
BACKEND = re.compile(r"^[A-Za-z0-9._-]{1,128}$")
PERMISSIONS = ("channels.view", "epg.view", "media.live.play")


class Stop(RuntimeError):
    pass


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None


def validated_base(url: str) -> str:
    parts = urllib.parse.urlsplit(url)
    if (parts.scheme != "https" or not parts.hostname or
            parts.username is not None or parts.password is not None or
            parts.query or parts.fragment or parts.path.endswith("/api")):
        raise Stop("HTTPS_WEB_BASE_REQUIRED")
    return url.rstrip("/")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base-url", required=True,
                        help="verified HTTPS Web URL, e.g. https://host/vdr-suite")
    parser.add_argument("--device-id", required=True)
    parser.add_argument("--backend-id", default="default")
    parser.add_argument("--confirm", action="store_true",
                        help="authorize the three explicit grants")
    args = parser.parse_args(argv)

    base = validated_base(args.base_url)
    if not DEVICE.fullmatch(args.device_id):
        raise Stop("DEVICE_ID_INVALID")
    if not BACKEND.fullmatch(args.backend_id):
        raise Stop("BACKEND_ID_INVALID")

    print("DEVICE=" + args.device_id)
    print("BACKEND=" + args.backend_id)
    print("GRANTS=" + ",".join(PERMISSIONS))
    if not args.confirm:
        print("DRY_RUN=PASS_NO_CHANGES")
        return 0

    opener = urllib.request.build_opener(
        NoRedirect(), urllib.request.HTTPSHandler(context=ssl.create_default_context()))
    origin = urllib.parse.urlsplit(base)
    origin_header = origin.scheme + "://" + origin.netloc

    def request(method, path, headers=None, body=None):
        if not path.startswith("/api/"):
            raise Stop("UNSUPPORTED_PATH")
        url = base + path
        data = None if body is None else json.dumps(body, separators=(",", ":")).encode()
        req = urllib.request.Request(url, data=data, method=method)
        req.add_header("Accept", "application/json")
        if data is not None:
            req.add_header("Content-Type", "application/json")
        req.add_header("Origin", origin_header)
        for key, value in (headers or {}).items():
            req.add_header(key, value)
        try:
            with opener.open(req, timeout=12) as answer:
                if answer.status != 200:
                    raise Stop("UNEXPECTED_STATUS_" + str(answer.status))
                encoded = answer.read(128 * 1024 + 1)
                if len(encoded) > 128 * 1024:
                    raise Stop("RESPONSE_TOO_LARGE")
                return dict(answer.headers), json.loads(encoded.decode("utf-8"))
        except urllib.error.HTTPError as err:
            raise Stop("HTTP_" + str(err.code) + "_FROM_" + path.split("?")[0]) from None
        except (urllib.error.URLError, TimeoutError) as err:
            raise Stop("CONNECTION_OR_TLS_FAILURE") from None

    username = input("Admin-Anmeldename: ").strip()
    if not username or ":" in username or "\n" in username:
        raise Stop("LOGIN_NAME_INVALID")
    password = getpass.getpass("Admin-Passwort (nicht angezeigt): ")
    if not password:
        raise Stop("EMPTY_PASSWORD")
    basic = base64.b64encode((username + ":" + password).encode()).decode("ascii")
    del password

    headers, session = request("POST", "/api/security/browser-sessions",
                               {"Authorization": "Basic " + basic}, {})
    del basic
    cookie = headers.get("Set-Cookie", "").split(";", 1)[0]
    if not cookie.startswith("vdr_suite_session=") or "\n" in cookie:
        raise Stop("SESSION_COOKIE_MISSING")
    csrf = session.get("csrfToken")
    if not isinstance(csrf, str) or not csrf:
        raise Stop("SESSION_CSRF_MISSING")
    auth = {"Cookie": cookie, "X-CSRF-Token": csrf}
    resource = "/api/v1/devices/" + args.device_id + "/grants"
    try:
        for permission in PERMISSIONS:
            current_headers, current = request("GET", resource, auth)
            if current.get("deviceId") != args.device_id:
                raise Stop("DEVICE_ID_MISMATCH")
            items = current.get("items")
            if not isinstance(items, list):
                raise Stop("GRANT_SET_INVALID")
            if any(isinstance(g, dict) and
                   g.get("permission") == permission and
                   g.get("backendId") == args.backend_id for g in items):
                print("GRANT_ALREADY_ACTIVE=" + permission)
                continue
            etag = current_headers.get("ETag")
            if not isinstance(etag, str) or not etag.startswith('"'):
                raise Stop("GRANT_ETAG_MISSING")
            _, updated = request("POST", resource,
                dict(auth, **{"If-Match": etag}),
                {"permission": permission, "backendId": args.backend_id,
                 "active": True})
            grants = updated.get("items")
            if not isinstance(grants, list) or not any(
                    isinstance(g, dict) and g.get("permission") == permission and
                    g.get("backendId") == args.backend_id for g in grants):
                raise Stop("GRANT_NOT_CONFIRMED_" + permission)
            print("GRANT_CONFIRMED=" + permission)
        print("VIDAA_LIVE_RIGHTS=PASS")
    finally:
        try:
            request("POST", "/api/security/browser-sessions/logout", auth, {})
        except Stop:
            print("SESSION_LOGOUT=UNCONFIRMED", file=sys.stderr)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (Stop, ValueError, UnicodeError, json.JSONDecodeError) as error:
        print("RESULT=FAIL REASON=" + str(error), file=sys.stderr)
        raise SystemExit(1)
