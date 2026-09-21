#!/usr/bin/env python3
from pathlib import Path


def require(text: str, needle: str, source: str) -> None:
    if needle not in text:
        raise SystemExit(f"missing Phase 65 response-header contract in {source}: {needle}")


api_path = Path("api/rest/include/DashboardController.h")
server_header_path = Path("core/http/include/TestHttpServer.h")
server_source_path = Path("core/http/src/TestHttpServer.cpp")

api = api_path.read_text(encoding="utf-8")
server_header = server_header_path.read_text(encoding="utf-8")
server_source = server_source_path.read_text(encoding="utf-8")

require(api, "std::map<std::string, std::string> headers;", str(api_path))
require(
    server_header,
    "const std::map<std::string, std::string>& headers) const;",
    str(server_header_path),
)
require(server_header, "response.headers);", str(server_header_path))
require(server_source, "apiResponse.headers));", str(server_source_path))
require(server_source, "response.headers = headers;", str(server_source_path))
require(
    server_source,
    'response.headers["Content-Type"] = contentType;',
    str(server_source_path),
)

copy_position = server_source.index("response.headers = headers;")
content_type_position = server_source.index(
    'response.headers["Content-Type"] = contentType;'
)
if copy_position > content_type_position:
    raise SystemExit(
        "ApiResponse headers must be copied before the typed Content-Type field "
        "so generic headers cannot override the typed response content type"
    )

print("phase65 api response header contract passed")
