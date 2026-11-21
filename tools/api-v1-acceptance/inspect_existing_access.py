#!/usr/bin/env python3
"""Read-only Public-v1 Account/Device grant inventory for an existing administrator.

Only POSTs: create and revoke this operator's temporary Browser Session.
All resource requests: authenticated bounded GET. No credential issuance,
pairing, grant changes, VDR writes, config edits, or secret output.
"""
from __future__ import annotations

import argparse
import getpass
import re
import sys

from runner import Blocked, ConfigError, HttpClient, Problem

BACKEND = "default"
SAFE_ID = re.compile(r"[A-Za-z0-9_.-]{1,128}\Z")
SAFE_GRANT = re.compile(r"[A-Za-z][A-Za-z0-9_.-]{0,80}\Z")
IMPORTANT = {
    "role.admin", "timers.view", "searchtimers.view", "recordings.view",
    "epg.view", "channels.view", "media.recording.play", "media.live.play",
    "accounts.grants.view", "devices.grants.view",
}


def safe_label(value):
    if not isinstance(value, str):
        return "(unbenannt)"
    return re.sub(r"[^A-Za-z0-9 äöüÄÖÜß._-]", "?", value)[:64] or "(unbenannt)"


def safe_resource_id(value):
    if (not isinstance(value, str) or not SAFE_ID.fullmatch(value)
            or value in (".", "..")):
        raise Blocked("server returned an invalid resource identity")
    return value


def grant_summary(document):
    """Return only recognized grant names/scopes, never raw JSON or secrets."""
    items = document.get("items") if isinstance(document, dict) else None
    if not isinstance(items, list):
        raise Problem("wrong_json_structure", "grant collection has no items list")
    result = []
    for item in items:
        if not isinstance(item, dict):
            raise Problem("wrong_json_structure", "grant collection item not object")
        perm, scope = item.get("permission"), item.get("backendId")
        if (not isinstance(perm, str) or not SAFE_GRANT.fullmatch(perm)
                or not isinstance(scope, str) or scope != "*"
                and not SAFE_ID.fullmatch(scope)):
            raise Problem("wrong_json_structure", "invalid permission/scope representation")
        result.append((perm, scope))
    return sorted(set(result))


def get_collection(client, name, path):
    try:
        data, _ = client.expect(path)
    except (Blocked, Problem) as e:
        print(name + ": " + e.kind + ", HTTP " + str(e.actual or "?"))
        return None
    items = data.get("items")
    if not isinstance(items, list) or len(items) > 100:
        print(name + ": invalid bounded collection")
        return None
    page = data.get("page") if isinstance(data.get("page"), dict) else {}
    if page.get("hasMore"):
        print(name + ": WARNING — more than 100 rows; this inventory is partial")
    return items


def inspect(client):
    print("=== VORHANDENE HUMAN ACCOUNTS UND GRANTS ===")
    accounts = get_collection(client, "Accounts", "/api/v1/accounts?limit=100")
    if accounts is not None:
        print("Account-Eintraege in dieser Seite:", len(accounts))
        for index, item in enumerate(accounts, 1):
            if not isinstance(item, dict):
                continue
            label = safe_label(item.get("displayName"))
            print("Account #" + str(index) + ": " + label +
                  " | active=" + str(item.get("active") is True))
            try:
                ident = safe_resource_id(item.get("accountId"))
                path = "/api/v1/accounts/" + ident + "/grants"
                data, _ = client.expect(path)
                grants = grant_summary(data)
            except (Blocked, Problem) as e:
                print("  Grants: " + e.kind + " HTTP " + str(e.actual or "?"))
                continue
            for perm, scope in grants:
                if perm in IMPORTANT:
                    print("  " + perm + "@" + scope)
            if not grants:
                print("  (keine Grants)")
    print()
    print("=== VORHANDENE DEVICES UND GRANTS ===")
    devices = get_collection(client, "Devices", "/api/v1/devices?limit=100")
    if devices is not None:
        print("Device-Eintraege in dieser Seite:", len(devices))
        for index, item in enumerate(devices, 1):
            if not isinstance(item, dict):
                continue
            active = item.get("active") is True and item.get("revoked") is not True
            print("Device #" + str(index) + ": " +
                  safe_label(item.get("displayName")) +
                  " | active=" + str(active))
            if not active:
                continue
            try:
                ident = safe_resource_id(item.get("deviceId"))
                data, _ = client.expect("/api/v1/devices/" + ident + "/grants")
                grants = grant_summary(data)
            except (Blocked, Problem) as e:
                print("  Grants: " + e.kind + " HTTP " + str(e.actual or "?"))
                continue
            for perm, scope in grants:
                if perm in IMPORTANT:
                    print("  " + perm + "@" + scope)
            if not grants:
                print("  (keine Grants)")
            relevant = set(grants)
            for perm in ("searchtimers.view", "timers.view",
                         "media.recording.play"):
                covered = (perm, BACKEND) in relevant or (perm, "*") in relevant
                print("  " + perm + "@" + BACKEND + ": " +
                      ("grant vorhanden" if covered else "kein Grant"))
    print()
    print("=== EINORDNUNG ===")
    print("Admin-Browser-Session ist NICHT dasselbe wie ein Device-Credential.")
    print("Grants allein beweisen keine Verfuegbarkeit des einmaligen Device-Secrets.")
    print("Es wurden keine Device-Authentifizierungen vorgetaeuscht.")


def main(argv=None):
    parser = argparse.ArgumentParser(description="Inspect existing Public-v1 permissions only")
    parser.add_argument("--admin-user", default="vdr-suite")
    parser.add_argument("--origin", default="http://127.0.0.1:18080")
    args = parser.parse_args(argv)
    if not sys.stdin.isatty():
        print("BLOCKED: interactive terminal required")
        return 2
    client = HttpClient(args.origin)
    password = getpass.getpass("VDR-Suite Passwort (" + args.admin_user + "): ")
    try:
        client.open_browser_session(args.admin_user, password)
    except (Blocked, Problem, ConfigError) as e:
        print("ADMIN-LOGIN: BLOCKED (" +
              (e.kind if isinstance(e, (Blocked, Problem)) else "configuration") + ")")
        return 2
    finally:
        del password
    try:
        inspect(client)
        return 0
    except (Blocked, Problem, ConfigError) as e:
        print("INVENTUR: " +
              (e.kind if isinstance(e, (Blocked, Problem)) else "configuration_error"))
        return 2
    finally:
        try:
            client.close_browser_session()
            print("Admin-Session-Abmeldung: erfolgreich")
        except (Blocked, Problem):
            print("WARNUNG: Admin-Session-Abmeldung nicht bestaetigt")


if __name__ == "__main__":
    sys.exit(main())
