#!/usr/bin/env python3
"""CI-only real daemon HTTP / cookie / first-admin roundtrip.

No package installation, root access, service edits, or live database access.
The spawned daemon receives a fresh temporary SQLite file and is stopped
before that directory is removed.
"""
import base64
import http.client
import ipaddress
import json
import os
from pathlib import Path
import signal
import socket
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
DAEMON = ROOT / ".build" / "vdr-suite-daemon"
PORT = 18080
LOGIN = "mld66-ci-admin"
PASSWORD = "mld66-isolated-test-password"


def local_lan_address():
    candidates = {info[4][0] for info in socket.getaddrinfo(
        socket.gethostname(), None, socket.AF_INET, socket.SOCK_STREAM)}
    for address in sorted(candidates):
        ip = ipaddress.ip_address(address)
        if ip.is_private and not ip.is_loopback and not ip.is_link_local:
            return address
    raise RuntimeError("CI runner has no directly attached private IPv4 address")


def request(host, method, path, *, payload=None, headers=None):
    headers = dict(headers or {})
    body = json.dumps(payload) if payload is not None else None
    if payload is not None:
        headers["Content-Type"] = "application/json"
    connection = http.client.HTTPConnection(host, PORT, timeout=4)
    try:
        connection.request(method, path, body=body, headers=headers)
        response = connection.getresponse()
        data = response.read()
        return response.status, dict(response.getheaders()), data
    finally:
        connection.close()


def expect(host, method, path, status, **kwargs):
    actual, headers, body = request(host, method, path, **kwargs)
    if actual != status:
        # Do not print credentials or cookie material.
        raise AssertionError(
            f"{method} {path}: expected HTTP {status}, received {actual}")
    return headers, json.loads(body) if body else {}


def launch(logfile, database):
    environment = os.environ.copy()
    environment["VDR_SUITE_DATABASE_PATH"] = str(database)
    environment["VDR_SUITE_SECURITY_DATABASE_PATH"] = str(database)
    process = subprocess.Popen(
        [str(DAEMON)], cwd=str(ROOT), env=environment,
        stdout=logfile, stderr=subprocess.STDOUT)
    try:
        for _ in range(80):
            if process.poll() is not None:
                raise RuntimeError(f"isolated CI daemon exited: {process.returncode}")
            try:
                status, _, _ = request("127.0.0.1", "GET",
                                       "/api/security/first-admin/status")
                if status in (200, 503):
                    return process
            except (OSError, http.client.HTTPException):
                pass
            time.sleep(0.2)
        raise RuntimeError("isolated CI daemon did not start on port 18080")
    except BaseException:
        stop(process)
        raise


def stop(process):
    if process is None or process.poll() is not None:
        return
    process.send_signal(signal.SIGTERM)
    try:
        process.wait(timeout=8)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait(timeout=8)


def main():
    if (os.environ.get("GITHUB_ACTIONS") != "true"
            or os.environ.get("RUNNER_ENVIRONMENT") != "github-hosted"
            or os.geteuid() == 0):
        raise RuntimeError("This first-admin test requires a non-root GitHub-hosted runner")
    if not DAEMON.is_file() or not os.access(DAEMON, os.X_OK):
        raise RuntimeError("Expected built daemon missing: .build/vdr-suite-daemon")
    # Refuse to touch any server already listening on either test address.
    host = local_lan_address()
    for address in ("127.0.0.1", host):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.settimeout(0.3)
            if sock.connect_ex((address, PORT)) == 0:
                raise RuntimeError("Port 18080 already in use; no test started")
    origin = f"http://{host}:{PORT}"
    basic = "Basic " + base64.b64encode(
        f"{LOGIN}:{PASSWORD}".encode()).decode("ascii")
    with tempfile.TemporaryDirectory(prefix="vdr-suite-mld66-ci-") as temp:
        database = Path(temp) / "isolated.sqlite"
        with (Path(temp) / "daemon.log").open("wb") as log:
            process = None
            try:
                process = launch(log, database)
                _, before = expect(host, "GET",
                    "/api/security/first-admin/status", 200)
                assert before["unclaimed"] is True

                initial = {"loginName": LOGIN, "displayName": "CI admin",
                           "password": PASSWORD}
                expect(host, "POST", "/api/security/first-admin/claim", 201,
                       payload=initial, headers={"Origin": origin})
                _, claimed = expect(host, "GET",
                    "/api/security/first-admin/status", 200)
                assert claimed["unclaimed"] is False
                expect(host, "POST", "/api/security/first-admin/claim", 409,
                       payload=initial, headers={"Origin": origin})

                expect(host, "POST", "/api/security/browser-sessions", 401,
                       headers={"Authorization": "Basic " + base64.b64encode(
                         f"{LOGIN}:wrong-password".encode()).decode("ascii"),
                         "Origin": origin})
                login_headers, login = expect(
                    host, "POST", "/api/security/browser-sessions", 200,
                    headers={"Authorization": basic, "Origin": origin})
                cookie = login_headers["Set-Cookie"]
                assert "; HttpOnly" in cookie and "; SameSite=Strict" in cookie
                assert "; Secure" not in cookie, "direct LAN HTTP cookie unexpectedly Secure"
                pair = cookie.split(";", 1)[0]
                assert pair.startswith("vdr_suite_session=")
                _, current = expect(
                    host, "GET", "/api/security/browser-sessions/current", 200,
                    headers={"Cookie": pair})
                assert current["csrfToken"] == login["csrfToken"]
                expect(host, "GET", "/api/security/browser-sessions/current", 401)

                logout_headers, _ = expect(
                    host, "POST", "/api/security/browser-sessions/logout", 204,
                    headers={"Cookie": pair,
                             "X-CSRF-Token": login["csrfToken"],
                             "Origin": origin})
                expired = logout_headers["Set-Cookie"]
                assert "Max-Age=0" in expired and "; Secure" not in expired
                expect(host, "GET", "/api/security/browser-sessions/current", 401,
                       headers={"Cookie": pair})

                relogin_headers, _ = expect(
                    host, "POST", "/api/security/browser-sessions", 200,
                    headers={"Authorization": basic, "Origin": origin})
                retained = relogin_headers["Set-Cookie"].split(";", 1)[0]
                # Forwarding headers or mismatched origins must never relax Secure.
                forwarded_headers, _ = expect(
                    host, "POST", "/api/security/browser-sessions", 200,
                    headers={"Authorization": basic, "Origin": origin,
                             "X-Forwarded-For": "203.0.113.5"})
                assert "; Secure" in forwarded_headers["Set-Cookie"]
                mismatched_headers, _ = expect(
                    host, "POST", "/api/security/browser-sessions", 200,
                    headers={"Authorization": basic,
                             "Origin": "http://public.example.invalid"})
                assert "; Secure" in mismatched_headers["Set-Cookie"]
                # Loopback/proxy remains conservative even without a TLS listener.
                proxy_headers, _ = expect(
                    "127.0.0.1", "POST", "/api/security/browser-sessions", 200,
                    headers={"Authorization": basic,
                             "Origin": "http://127.0.0.1:18080"})
                assert "; Secure" in proxy_headers["Set-Cookie"]

                stop(process)
                process = None
                process = launch(log, database)
                _, after_restart = expect(host, "GET",
                    "/api/security/first-admin/status", 200)
                assert after_restart["unclaimed"] is False
                expect(host, "GET", "/api/security/browser-sessions/current", 200,
                       headers={"Cookie": retained})
                expect(host, "POST", "/api/security/first-admin/claim", 409,
                       payload=initial, headers={"Origin": origin})
                print("MLD CI real HTTP session roundtrip passed (throwaway SQLite)")
            finally:
                stop(process)


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(f"MLD CI HTTP roundtrip FAILED: {type(error).__name__}: {error}",
              file=sys.stderr)
        sys.exit(1)
