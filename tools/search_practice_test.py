#!/usr/bin/env python3

import argparse
import json
import sys
import urllib.error
import urllib.parse
import urllib.request


DEFAULT_BASE_URL = "http://127.0.0.1:8080"
DEFAULT_BACKEND = "default"
DEFAULT_QUERY = "Harrison Ford"
DEFAULT_TYPO_QUERY = "Harrisson Ford"
DEFAULT_LIMIT = 20
DEFAULT_TIMESPAN = 86400
DEFAULT_CHANNEL_EVENT_LIMIT = 50
DEFAULT_TIMEOUT = 5.0


class HttpResult:
    def __init__(self, method, url, status, body, error=None):
        self.method = method
        self.url = url
        self.status = status
        self.body = body
        self.error = error
        self.json_data = None

        if body:
            try:
                self.json_data = json.loads(body)
            except json.JSONDecodeError:
                self.json_data = None

    def ok(self):
        return self.error is None and 200 <= self.status < 300


def normalize_base_url(base_url):
    return base_url.rstrip("/")


def build_url(base_url, path, query):
    encoded = urllib.parse.urlencode(query, doseq=True)
    if encoded:
        return f"{normalize_base_url(base_url)}{path}?{encoded}"
    return f"{normalize_base_url(base_url)}{path}"


def http_json(method, base_url, path, query=None, timeout=DEFAULT_TIMEOUT):
    if query is None:
        query = {}

    url = build_url(base_url, path, query)
    data = None
    headers = {"Accept": "application/json"}

    if method == "POST":
        data = b"{}"
        headers["Content-Type"] = "application/json"

    request = urllib.request.Request(url, data=data, method=method, headers=headers)

    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            body = response.read().decode("utf-8", errors="replace")
            return HttpResult(method, url, response.status, body)
    except urllib.error.HTTPError as error:
        body = error.read().decode("utf-8", errors="replace")
        return HttpResult(method, url, error.code, body, str(error))
    except urllib.error.URLError as error:
        return HttpResult(method, url, 0, "", str(error))
    except TimeoutError as error:
        return HttpResult(method, url, 0, "", str(error))


def short_url(url):
    parsed = urllib.parse.urlparse(url)
    return urllib.parse.urlunparse(("", "", parsed.path, "", parsed.query, ""))


def iter_strings(value, path="$"):
    if isinstance(value, dict):
        for key, item in value.items():
            yield from iter_strings(item, f"{path}.{key}")
    elif isinstance(value, list):
        for index, item in enumerate(value):
            yield from iter_strings(item, f"{path}[{index}]")
    elif isinstance(value, str):
        yield path, value


def find_text_hits(value, term, max_hits=8):
    normalized_term = term.casefold()
    hits = []

    for path, text in iter_strings(value):
        if normalized_term in text.casefold():
            excerpt = text.strip().replace("\n", " ")
            if len(excerpt) > 160:
                excerpt = excerpt[:157] + "..."
            hits.append((path, excerpt))
            if len(hits) >= max_hits:
                break

    return hits


def find_key_values(value, wanted_keys, path="$"):
    found = []

    if isinstance(value, dict):
        for key, item in value.items():
            next_path = f"{path}.{key}"
            if key in wanted_keys:
                found.append((next_path, item))
            found.extend(find_key_values(item, wanted_keys, next_path))
    elif isinstance(value, list):
        for index, item in enumerate(value):
            found.extend(find_key_values(item, wanted_keys, f"{path}[{index}]"))

    return found


def first_list_size(value):
    if isinstance(value, list):
        return len(value)

    if isinstance(value, dict):
        preferred_keys = [
            "results",
            "events",
            "recordings",
            "items",
            "matches",
            "persons",
            "searchTimers",
        ]
        for key in preferred_keys:
            if isinstance(value.get(key), list):
                return len(value[key])
        for item in value.values():
            size = first_list_size(item)
            if size is not None:
                return size

    return None


def explicit_count(value):
    if not isinstance(value, dict):
        return None

    count_keys = [
        "totalCount",
        "resultCount",
        "matchCount",
        "count",
        "total",
        "eventCount",
    ]

    for key in count_keys:
        item = value.get(key)
        if isinstance(item, int):
            return item

    return None


def describe_count(value):
    if value is None:
        return "json=none"

    count = explicit_count(value)
    if count is not None:
        return f"count={count}"

    size = first_list_size(value)
    if size is not None:
        return f"items={size}"

    return "count=unknown"


def epg_input_summary(value):
    summaries = []
    for path, item in find_key_values(value, {"epgInput", "cacheStatus", "status", "available"}):
        if path.endswith(".epgInput") and isinstance(item, dict):
            summaries.append(f"{path}: {json.dumps(item, ensure_ascii=False, sort_keys=True)}")
    return summaries[:3]


def print_result(label, term, result):
    status = result.status if result.status else "ERR"
    state = "OK" if result.ok() else "FAIL"
    print(f"[{state}] {label}: {result.method} {short_url(result.url)} -> {status}")

    if result.error and not result.body:
        print(f"      error: {result.error}")
        return

    if result.json_data is None:
        if result.body:
            excerpt = result.body.replace("\n", " ")[:180]
            print(f"      body: {excerpt}")
        return

    print(f"      {describe_count(result.json_data)}")

    for summary in epg_input_summary(result.json_data):
        print(f"      {summary}")

    hits = find_text_hits(result.json_data, term)
    if not hits:
        print("      text hits: 0")
        return

    print(f"      text hits: {len(hits)} shown")
    for path, excerpt in hits:
        print(f"        - {path}: {excerpt}")


def practice_endpoints(args, term):
    common_window = {
        "backend": args.backend,
        "from": args.from_time,
        "timespan": args.timespan,
        "limit": args.limit,
    }

    return [
        (
            "searchtimer-preview",
            "GET",
            "/api/vdr/searchtimers/preview",
            {
                "backend": args.backend,
                "query": term,
                "limit": args.limit,
            },
        ),
        (
            "epg-search-exact",
            "GET",
            "/api/epg/search",
            dict(common_window, query=term),
        ),
        (
            "epg-search-fuzzy",
            "GET",
            "/api/epg/search",
            dict(common_window, query=term, mode="fuzzy", tolerance=args.tolerance),
        ),
        (
            "recordings-title-query",
            "GET",
            "/api/vdr/recordings/query",
            {
                "backend": args.backend,
                "title": term,
                "limit": args.limit,
            },
        ),
        (
            "recordings-scan-window",
            "GET",
            "/api/vdr/recordings/query",
            {
                "backend": args.backend,
                "limit": args.limit,
            },
        ),
        (
            "recording-person-search",
            "GET",
            "/api/vdr/recordings/persons/search",
            {
                "backend": args.backend,
                "name": term,
                "limit": args.limit,
            },
        ),
    ]


def refresh_cache(args):
    query = {
        "backend": args.backend,
        "from": args.from_time,
        "timespan": args.timespan,
        "limit": args.limit,
        "channelEventLimit": args.channel_event_limit,
    }

    result = http_json(
        "POST",
        args.base_url,
        "/api/vdr/searchtimers/preview/cache/refresh",
        query,
        args.timeout,
    )

    print_result("preview-cache-refresh", "", result)
    return result.ok()


def run_practice(args):
    print("VDR-Suite Search Practice Test")
    print(f"base-url: {normalize_base_url(args.base_url)}")
    print(f"backend:  {args.backend}")
    print(f"window:   from={args.from_time} timespan={args.timespan}")
    print("")

    if args.refresh_cache:
        print("== Cache refresh ==")
        refresh_cache(args)
        print("")

    exit_code = 0

    for term in args.terms:
        print(f"== Query: {term} ==")
        for label, method, path, query in practice_endpoints(args, term):
            result = http_json(method, args.base_url, path, query, args.timeout)
            print_result(label, term, result)
            if not result.ok():
                exit_code = 1
        print("")

    return exit_code


def parse_args():
    parser = argparse.ArgumentParser(
        description="Run an operator search practice test against a running VDR-Suite daemon."
    )
    parser.add_argument("--base-url", default=DEFAULT_BASE_URL)
    parser.add_argument("--backend", default=DEFAULT_BACKEND)
    parser.add_argument("--query", default=DEFAULT_QUERY)
    parser.add_argument("--typo", default=DEFAULT_TYPO_QUERY)
    parser.add_argument("--term", action="append", default=[])
    parser.add_argument("--limit", type=int, default=DEFAULT_LIMIT)
    parser.add_argument("--from", dest="from_time", type=int, default=-1)
    parser.add_argument("--timespan", type=int, default=DEFAULT_TIMESPAN)
    parser.add_argument("--channel-event-limit", type=int, default=DEFAULT_CHANNEL_EVENT_LIMIT)
    parser.add_argument("--tolerance", default="2")
    parser.add_argument("--timeout", type=float, default=DEFAULT_TIMEOUT)
    parser.add_argument("--refresh-cache", action="store_true")

    args = parser.parse_args()

    terms = []
    for value in [args.query, args.typo] + args.term:
        value = value.strip()
        if value and value not in terms:
            terms.append(value)

    args.terms = terms
    return args


def main():
    args = parse_args()
    return run_practice(args)


if __name__ == "__main__":
    raise SystemExit(main())
