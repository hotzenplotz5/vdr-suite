#!/usr/bin/env python3
"""One-time HISENSE/VIDAA device-grant operation; never mutates SQLite.

Authenticate through the existing local HTTPS reverse proxy and the canonical
Public-v1 Device grant API. Password, session cookie and CSRF are memory-only.
"""
import argparse
import base64
import getpass
import http.client
import json
import os
import re
import socket
import sqlite3
import ssl
import sys
from urllib.parse import quote

DB = "/var/lib/vdr-suite/vdr-suite.db"
GRANTS = ("timers.view", "searchtimers.view")
SCOPES = ("channels.view", "epg.view", "media.live.play",
          "media.recording.play", "recordings.view")
DEVICE_RE = re.compile(r"^[A-Za-z0-9._:-]{1,256}$")
BACKEND_RE = re.compile(r"^[A-Za-z0-9._-]{1,128}$")


class Stop(Exception):
    pass


class LocalHTTPS(http.client.HTTPSConnection):
    """Loopback TCP, but real TLS verification and SNI for the user's domain."""

    def connect(self):
        raw = socket.create_connection(("127.0.0.1", 443), timeout=8)
        try:
            self.sock = self._context.wrap_socket(raw, server_hostname=self.host)
        except BaseException:
            raw.close()
            raise


def request(host, prefix, method, path, *, auth="", cookie="", csrf="",
            etag="", payload=None):
    # Requests never contain credentials in URL or query.
    headers = {"Accept": "application/json",
               "Cache-Control": "no-store",
               "Origin": "https://" + host}
    if auth:
        headers["Authorization"] = auth
    if cookie:
        headers["Cookie"] = cookie
    if csrf:
        headers["X-CSRF-Token"] = csrf
    if etag:
        headers["If-Match"] = etag
    body = None
    if payload is not None:
        body = json.dumps(payload, separators=(",", ":")).encode("utf-8")
        headers["Content-Type"] = "application/json"
    conn = LocalHTTPS(host, 443, timeout=10, context=ssl.create_default_context())
    try:
        conn.request(method, prefix + path, body=body, headers=headers)
        response = conn.getresponse()
        code = response.status
        head = {k.lower(): v for k, v in response.getheaders()}
        raw = response.read(131073)
        if len(raw) > 131072:
            raise Stop("API_RESPONSE_TOO_LARGE")
        try:
            data = json.loads(raw) if raw else {}
        except (ValueError, UnicodeError):
            data = {}
        return code, head, data
    finally:
        conn.close()


def read_devices(path):
    if not os.path.isfile(path):
        raise Stop("DB_NOT_FOUND: " + path)
    uri = "file:" + quote(os.path.abspath(path), safe="/") + "?mode=ro"
    con = sqlite3.connect(uri, uri=True, timeout=2)
    try:
        con.execute("PRAGMA query_only=ON")
        return con.execute(
            "SELECT device_id, display_name FROM security_devices "
            "WHERE active=1 AND revoked_at='' ORDER BY display_name, device_id"
        ).fetchall()
    finally:
        con.close()


def select_device(devices, explicit):
    if explicit:
        matching = [(did, label) for did, label in devices if did == explicit]
        if len(matching) != 1:
            raise Stop("DEVICE_NOT_ACTIVE_OR_NOT_FOUND")
    else:
        matching = [(did, label) for did, label in devices
                    if "hisense" in label.lower() or "vidaa" in label.lower()]
        if len(matching) != 1:
            print("ACTIVE_DEVICES_READ_ONLY:")
            for did, name in devices:
                print("  ", did, "|", name)
            raise Stop("HISENSE_NOT_UNAMBIGUOUS: rerun with --device-id EXACT_ID")
    did, name = matching[0]
    if not DEVICE_RE.fullmatch(did):
        raise Stop("DEVICE_ID_UNSAFE")
    print("TARGET_DEVICE:", did, "|", name)
    return did


def select_backend(items, explicit):
    if not isinstance(items, list):
        raise Stop("GRANT_SET_INVALID")
    choices = sorted({
        g["backendId"] for g in items
        if isinstance(g, dict) and g.get("permission") in SCOPES
        and isinstance(g.get("backendId"), str)
        and BACKEND_RE.fullmatch(g["backendId"]) and g["backendId"] != "*"
    })
    if explicit:
        if not BACKEND_RE.fullmatch(explicit) or explicit not in choices:
            raise Stop("BACKEND_NOT_ALREADY_AUTHORIZED_FOR_DEVICE")
        return explicit
    if len(choices) != 1:
        print("AUTHORIZED_BACKEND_CHOICES:", ", ".join(choices) or "(none)")
        raise Stop("BACKEND_NOT_UNAMBIGUOUS: rerun with --backend EXACT_BACKEND")
    return choices[0]


def assert_grants(data, device_id):
    if not isinstance(data, dict) or data.get("deviceId") != device_id:
        raise Stop("DEVICE_GRANTS_WRONG_TARGET")
    if not isinstance(data.get("items"), list):
        raise Stop("DEVICE_GRANTS_ITEMS_INVALID")
    allowed = data.get("supportedPermissions")
    if not isinstance(allowed, list) or not all(p in allowed for p in GRANTS):
        raise Stop("INSTALLED_BACKEND_DOES_NOT_SUPPORT_REQUESTED_GRANTS")
    return data["items"]


def included(items, permission, backend):
    return any(isinstance(g, dict) and g.get("permission") == permission
               and g.get("backendId") == backend for g in items)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="hotzenplotz-v6.zapto.org",
                        help="TLS certificate hostname of local Nginx (not an IP)")
    parser.add_argument("--database", default=DB)
    parser.add_argument("--device-id", default="")
    parser.add_argument("--backend", default="")
    parser.add_argument("--check-only", action="store_true")
    args = parser.parse_args()
    if not re.fullmatch(r"[A-Za-z0-9.-]{1,253}", args.host):
        raise Stop("INVALID_TLS_HOST")
    print("DB_MODE=READ_ONLY, API_TARGET=127.0.0.1:443, TLS_SNI=" + args.host)
    devices = read_devices(args.database)
    device_id = select_device(devices, args.device_id)

    # Probe only; no Basic auth nor Session creation until route is established.
    prefix = None
    for attempt in ("", "/vdr-suite"):
        status, _, _ = request(args.host, attempt, "GET", "/api/v1/capabilities")
        if status != 404:
            prefix = attempt
            break
    if prefix is None:
        raise Stop("PUBLIC_API_ROUTE_NOT_FOUND")
    print("API_PREFIX=" + (prefix or "/"))

    login = input("VDR-Suite Admin-Benutzer: ").strip()
    if not login or ":" in login:
        raise Stop("INVALID_LOGIN")
    passwd = getpass.getpass("Admin-Passwort (nur lokal, unsichtbar): ")
    if not passwd:
        raise Stop("EMPTY_PASSWORD")
    basic = "Basic " + base64.b64encode(
        (login + ":" + passwd).encode("utf-8")).decode("ascii")
    del passwd
    status, headers, data = request(args.host, prefix, "POST",
                                    "/api/security/browser-sessions",
                                    auth=basic, payload={})
    del basic
    if status != 200:
        raise Stop("ADMIN_LOGIN_HTTP_" + str(status))
    csrf = data.get("csrfToken")
    value = headers.get("set-cookie", "").split(";", 1)[0]
    if not isinstance(csrf, str) or not csrf or not value.startswith(
            "vdr_suite_session="):
        raise Stop("ADMIN_SESSION_INVALID")
    url = "/api/v1/devices/" + quote(device_id, safe="") + "/grants"
    try:
        status, head, data = request(args.host, prefix, "GET", url,
                                     cookie=value)
        if status != 200:
            raise Stop("ADMIN_GRANTS_GET_HTTP_" + str(status))
        items = assert_grants(data, device_id)
        backend = select_backend(items, args.backend)
        missing = [p for p in GRANTS if not included(items, p, backend)]
        print("GRANT_TARGET_SCOPE:", backend)
        print("CURRENT: timers.view=", included(items, GRANTS[0], backend),
              "searchtimers.view=", included(items, GRANTS[1], backend))
        print("PENDING:", ", ".join(missing) if missing else "(none)")
        if args.check_only or not missing:
            print("GRANTS_CHECK=PASS; MUTATION=NONE")
            return
        if input("Genau diese fehlenden Rechte vergeben? [ja]: ").strip() != "ja":
            raise Stop("NOT_CONFIRMED")
        etag = head.get("etag", "")
        if not etag.startswith('"') or not etag.endswith('"'):
            raise Stop("DEVICE_GRANTS_STRONG_ETAG_MISSING")
        for permission in missing:
            status, head, data = request(args.host, prefix, "POST", url,
                cookie=value, csrf=csrf, etag=etag,
                payload={"permission": permission, "backendId": backend,
                         "active": True})
            if status != 200:
                raise Stop("GRANT_POST_" + permission + "_HTTP_" + str(status))
            items = assert_grants(data, device_id)
            if not included(items, permission, backend):
                raise Stop("GRANT_POST_NOT_CONFIRMED_" + permission)
            etag = head.get("etag", "")
            if not etag.startswith('"') or not etag.endswith('"'):
                raise Stop("GRANT_POST_ETAG_INVALID")
            print("CONFIRMED:", permission + "@" + backend)
        print("HISENSE_GRANTS=PASS")
    finally:
        # Session logout is a separate, authenticated, CSRF-protected action.
        status, _, _ = request(args.host, prefix, "POST",
                              "/api/security/browser-sessions/logout",
                              cookie=value, csrf=csrf, payload={})
        print("ADMIN_LOGOUT_HTTP=" + str(status))


if __name__ == "__main__":
    try:
        main()
    except (Stop, OSError, sqlite3.Error, ssl.SSLError, ValueError) as error:
        print("STOP=" + str(error), file=sys.stderr)
        sys.exit(1)
