#!/usr/bin/env python3
import argparse
import base64
import json
import sys
import time
import urllib.error
import urllib.request


def normalize_base_path(value):
    if not value:
        return ""
    value = value.strip()
    if not value:
        return ""
    if not value.startswith("/"):
        value = "/" + value
    return value.rstrip("/")


def build_url(host, port, base_path, path):
    return f"http://{host}:{port}{normalize_base_path(base_path)}{path}"


def auth_header(user, password):
    if not user:
        return None

    token = f"{user}:{password or ''}".encode("utf-8")
    return "Basic " + base64.b64encode(token).decode("ascii")


def request_http(method, url, payload=None, user=None, password=None, timeout=5):
    data = None
    headers = {
        "Accept": "application/json, text/plain",
    }

    if payload is not None:
        data = json.dumps(payload, separators=(",", ":")).encode("utf-8")
        headers["Content-Type"] = "application/json"

    auth = auth_header(user, password)
    if auth:
        headers["Authorization"] = auth

    request = urllib.request.Request(
        url,
        data=data,
        headers=headers,
        method=method,
    )

    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            body = response.read().decode("utf-8", errors="replace")
            return response.getcode(), body
    except urllib.error.HTTPError as error:
        body = error.read().decode("utf-8", errors="replace")
        return error.code, body
    except urllib.error.URLError as error:
        return 0, str(error)


def extract_created_id(body):
    marker = "Id:"
    position = body.find(marker)
    if position < 0:
        return ""

    position += len(marker)

    while position < len(body) and body[position].isspace():
        position += 1

    end = position

    while end < len(body) and body[end].isdigit():
        end += 1

    return body[position:end]


def parse_json_object(body):
    try:
        parsed = json.loads(body)
    except json.JSONDecodeError:
        return None, "response is not valid JSON"

    if not isinstance(parsed, dict):
        return None, "response JSON is not an object"

    return parsed, ""


def read_searchtimers(host, port, base_path, user, password, timeout):
    list_url = build_url(host, port, base_path, "/searchtimers.json")

    status, body = request_http(
        "GET",
        list_url,
        None,
        user,
        password,
        timeout,
    )

    if status != 200:
        return status, body, [], f"readback request returned HTTP {status}"

    parsed, error = parse_json_object(body)
    if parsed is None:
        return status, body, [], error

    items = parsed.get("searchtimers", [])
    if not isinstance(items, list):
        return status, body, [], "readback JSON does not contain a searchtimers array"

    return status, body, items, ""


def item_id(item):
    return str(item.get("id", ""))


def item_search(item):
    return str(item.get("search", ""))


def find_searchtimer_by_id(items, native_id):
    for item in items:
        if not isinstance(item, dict):
            continue
        if item_id(item) == str(native_id):
            return item
    return None


def find_searchtimer_by_query(items, query):
    matches = []

    for item in items:
        if not isinstance(item, dict):
            continue
        if item_search(item) == query:
            matches.append(item)

    if len(matches) == 1:
        return matches[0], ""

    if len(matches) == 0:
        return None, f"created searchtimer with query {query!r} not found in readback"

    return None, f"query {query!r} matched {len(matches)} searchtimers; refusing ambiguous cleanup"


def delete_searchtimer(host, port, base_path, native_id, user, password, timeout):
    delete_url = build_url(
        host,
        port,
        base_path,
        "/searchtimers/" + str(native_id),
    )

    return request_http(
        "DELETE",
        delete_url,
        None,
        user,
        password,
        timeout,
    )


def cleanup_exact_query(args):
    status, body, items, error = read_searchtimers(
        args.host,
        args.port,
        args.base_path,
        args.user,
        args.password,
        args.timeout,
    )

    print("VDR-Suite native fuzzy cleanup")
    print(f"target={args.host}:{args.port}{normalize_base_path(args.base_path)}")
    print(f"query={args.cleanup_query}")
    print(f"readbackStatus={status}")

    if error:
        print("FAIL: " + error)
        return 1

    matches = []

    for item in items:
        if not isinstance(item, dict):
            continue
        if item_search(item) == args.cleanup_query:
            matches.append(item)

    if not matches:
        print("PASS: no matching SearchTimer found")
        return 0

    ok = True

    for item in matches:
        native_id = item_id(item)
        print("cleanupItem=" + json.dumps(item, sort_keys=True))
        delete_status, delete_body = delete_searchtimer(
            args.host,
            args.port,
            args.base_path,
            native_id,
            args.user,
            args.password,
            args.timeout,
        )

        print(f"deleteStatus={delete_status}")
        print(f"deleteBody={delete_body}")

        if delete_status != 200:
            ok = False

    if not ok:
        print("FAIL: one or more cleanup deletes failed")
        return 1

    print("PASS: cleanup completed")
    return 0


def build_payload(query, tolerance):
    return {
        "search": query,
        "use_title": True,
        "use_subtitle": True,
        "use_description": True,
        "use_channel": 0,
        "channels": "",
        "channel_min": "",
        "channel_max": "",
        "use_time": 0,
        "start_time": 0,
        "stop_time": 0,
        "use_duration": 0,
        "duration_min": 0,
        "duration_max": 0,
        "use_dayofweek": 0,
        "dayofweek": 0,
        "avoid_repeats": 0,
        "allowed_repeats": 0,
        "repeats_within_days": 0,
        "compare_title": 1,
        "compare_subtitle": 1,
        "compare_summary": 1,
        "compare_categories": 0,
        "compare_time": 0,
        "use_series_recording": 0,
        "keep_recs": 0,
        "del_mode": 0,
        "search_timer_action": 0,
        "blacklist_mode": 0,
        "blacklist_ids": "",
        "mode": 5,
        "match_case": 0,
        "tolerance": tolerance,
        "summary_match": 0,
        "use_ext_epg_info": 0,
        "ext_epg_info": "",
        "ignore_missing_epg_cats": 0,
        "content_descriptors": "",
        "use_in_favorites": 0,
        "use_as_searchtimer_from": "",
        "use_as_searchtimer_til": "",
        "pause_on_recs": 0,
        "switch_min_before": 0,
        "unmute_sound_on_switch": 0,
        "del_recs_after_days": 0,
        "del_after_count_recs": 0,
        "del_after_days_of_first_rec": 0,
        "directory": "VDR-Suite-Validation",
        "priority": 0,
        "lifetime": 0,
        "margin_start": 0,
        "margin_stop": 0,
        "use_vps": 0,
        "use_as_searchtimer": 1,
    }


def validate_created_item(item, tolerance):
    validation_ok = True

    print("readbackItem=" + json.dumps(item, sort_keys=True))

    mode = int(item.get("mode", -1))
    actual_tolerance = int(item.get("tolerance", -1))

    if mode != 5:
        print(f"FAIL: expected readback mode=5, got mode={mode}")
        validation_ok = False
    else:
        print("PASS: readback mode=5")

    if actual_tolerance != tolerance:
        print(
            "FAIL: expected readback tolerance="
            + str(tolerance)
            + ", got tolerance="
            + str(actual_tolerance)
        )
        validation_ok = False
    else:
        print(f"PASS: readback tolerance={tolerance}")

    return validation_ok


def run_validation(args):
    query = f"{args.query_prefix} {int(time.time())}"
    payload = build_payload(query, args.tolerance)

    create_url = build_url(args.host, args.port, args.base_path, "/searchtimers")
    list_url = build_url(args.host, args.port, args.base_path, "/searchtimers.json")

    print("VDR-Suite native fuzzy real-backend validation")
    print(f"target={args.host}:{args.port}{normalize_base_path(args.base_path)}")
    print(f"createUrl={create_url}")
    print(f"readbackUrl={list_url}")
    print(f"query={query}")
    print("mode=5")
    print(f"tolerance={args.tolerance}")

    if not args.execute:
        print("")
        print("DRY RUN: no request sent.")
        print("Run again with --execute to create, read back and delete a temporary SearchTimer.")
        print("")
        print("Payload preview:")
        print(json.dumps(payload, indent=2, sort_keys=True))
        return 0

    status, body = request_http(
        "POST",
        create_url,
        payload,
        args.user,
        args.password,
        args.timeout,
    )

    print("")
    print(f"createStatus={status}")
    print(f"createBody={body}")

    if status != 200:
        print("FAIL: create request did not return HTTP 200")
        return 1

    created_id = extract_created_id(body)

    status, readback_body, items, error = read_searchtimers(
        args.host,
        args.port,
        args.base_path,
        args.user,
        args.password,
        args.timeout,
    )

    print(f"readbackStatus={status}")

    if error:
        print("FAIL: " + error)
        return 1

    item = None

    if created_id:
        print(f"createdId={created_id}")
        item = find_searchtimer_by_id(items, created_id)
        if item is None:
            print(f"FAIL: created searchtimer id {created_id} not found in readback")
            return 1
    else:
        print("create response did not contain Id; falling back to exact query readback")
        item, error = find_searchtimer_by_query(items, query)
        if item is None:
            print("FAIL: " + error)
            return 1
        created_id = item_id(item)
        print(f"createdIdFromReadback={created_id}")

    validation_ok = validate_created_item(item, args.tolerance)

    delete_ok = True

    if not args.keep_created:
        delete_status, delete_body = delete_searchtimer(
            args.host,
            args.port,
            args.base_path,
            created_id,
            args.user,
            args.password,
            args.timeout,
        )

        print(f"deleteStatus={delete_status}")
        print(f"deleteBody={delete_body}")

        if delete_status != 200:
            print(f"FAIL: delete failed for created searchtimer id {created_id}")
            delete_ok = False
    else:
        print(f"KEEP: created searchtimer id {created_id} was not deleted")

    if not validation_ok or not delete_ok:
        return 1

    print("")
    print("PASS: native fuzzy real-backend validation completed")
    return 0


def main():
    parser = argparse.ArgumentParser(
        description="Validate native RESTfulAPI/epgsearch fuzzy SearchTimer passthrough."
    )
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8002)
    parser.add_argument("--base-path", default="")
    parser.add_argument("--user", default="")
    parser.add_argument("--password", default="")
    parser.add_argument("--timeout", type=int, default=5)
    parser.add_argument("--tolerance", type=int, default=2)
    parser.add_argument("--query-prefix", default="VDR-Suite Native Fuzzy Validation")
    parser.add_argument("--execute", action="store_true")
    parser.add_argument("--keep-created", action="store_true")
    parser.add_argument("--cleanup-query", default="")
    args = parser.parse_args()

    if args.cleanup_query:
        return cleanup_exact_query(args)

    return run_validation(args)


if __name__ == "__main__":
    raise SystemExit(main())
