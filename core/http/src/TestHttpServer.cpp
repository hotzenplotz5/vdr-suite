#include "TestHttpServer.h"

#include "ApiRouter.h"
#include "DashboardController.h"

#include <cstdlib>
#include <cctype>
#include <string>

namespace {

std::string toLowerAscii(const std::string& value)
{
    std::string lowered;
    lowered.reserve(value.size());

    for (const char character : value)
    {
        lowered.push_back(
            static_cast<char>(
                std::tolower(
                    static_cast<unsigned char>(character))));
    }

    return lowered;
}

std::string headerValue(
    const HttpServerRequest& request,
    const std::string& headerName)
{
    const std::string wanted =
        toLowerAscii(headerName);

    for (const auto& header : request.headers)
    {
        if (toLowerAscii(header.first) == wanted)
        {
            return header.second;
        }
    }

    return "";
}

std::string expectedAuthorizationHeader()
{
    const char* configured =
        std::getenv("VDR_SUITE_BASIC_AUTH");

    if (configured != nullptr && configured[0] != '\0')
    {
        return configured;
    }

    return "Basic YWRtaW46dmRyLXN1aXRl";
}

bool isAuthorized(const HttpServerRequest& request)
{
    return headerValue(request, "Authorization") ==
        expectedAuthorizationHeader();
}

HttpServerResponse makeUnauthorizedResponse()
{
    HttpServerResponse response;

    response.statusCode = 401;
    response.headers["Content-Type"] = "application/json";
    response.headers["WWW-Authenticate"] =
        "Basic realm=\"VDR-Suite\", charset=\"UTF-8\"";
    response.body = "{\"error\":\"authentication required\"}";

    return response;
}

HttpServerResponse makeFrontendShellResponse()
{
    HttpServerResponse response;

    response.statusCode = 200;
    response.headers["Content-Type"] = "text/html; charset=utf-8";
    response.body = R"FRONTEND(<!doctype html>
<html lang="de">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>VDR-Suite Frontend</title>
  <style>
    :root { color-scheme: dark; }
    body {
      margin: 0;
      font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      background: #0f172a;
      color: #f8fafc;
    }
    header {
      padding: 1.25rem;
      background: #020617;
      border-bottom: 1px solid #334155;
    }
    main { padding: 1.25rem; }
    h1 { margin: 0; font-size: clamp(2rem, 7vw, 3.5rem); }
    .status { margin: 0 0 1rem; color: #cbd5e1; font-size: 1.1rem; }
    .backend-list { display: grid; gap: 1rem; max-width: 60rem; }
    .backend-card {
      padding: 1rem;
      border: 1px solid #334155;
      border-radius: 1rem;
      background: #1e293b;
      box-shadow: 0 1rem 2rem rgba(0, 0, 0, 0.18);
    }
    .backend-header {
      display: flex;
      justify-content: space-between;
      gap: 1rem;
      align-items: flex-start;
      margin-bottom: 0.75rem;
    }
    .backend-title { font-size: 1.35rem; font-weight: 800; }
    .backend-subtitle { color: #cbd5e1; margin-top: 0.2rem; }
    .status-pill {
      padding: 0.25rem 0.65rem;
      border-radius: 999px;
      background: #14532d;
      color: #bbf7d0;
      font-weight: 700;
      white-space: nowrap;
    }
    .status-pill.offline { background: #7f1d1d; color: #fecaca; }
    .badge-row { display: flex; flex-wrap: wrap; gap: 0.5rem; }
    .badge {
      padding: 0.35rem 0.55rem;
      border-radius: 999px;
      background: #334155;
      color: #e2e8f0;
      font-size: 0.95rem;
    }
    .badge.enabled { background: #1d4ed8; color: #dbeafe; }
    .badge.disabled { background: #475569; color: #cbd5e1; }
    .error { color: #fecaca; }
  </style>
</head>
<body>
  <header><h1>VDR-Suite Frontend</h1></header>
  <main>
    <p id="status" class="status">Lade Backend-Auswahl...</p>
    <section id="backends" class="backend-list"></section>
  </main>
  <script>
    const statusElement = document.getElementById('status');
    const backendsElement = document.getElementById('backends');

    function addText(element, text) {
      element.textContent = text;
      return element;
    }

    function createBadge(label, enabled) {
      const badge = document.createElement('span');
      badge.className = 'badge ' + (enabled ? 'enabled' : 'disabled');
      badge.textContent = label + ': ' + (enabled ? 'ja' : 'nein');
      return badge;
    }

    function renderBackend(backend) {
      const selector = backend.frontendSelector || backend;
      const card = document.createElement('article');
      card.className = 'backend-card';

      const header = document.createElement('div');
      header.className = 'backend-header';

      const titleBlock = document.createElement('div');
      const title = addText(document.createElement('div'), selector.label || backend.backendName || backend.backendId || 'Unbekanntes Backend');
      title.className = 'backend-title';
      const subtitle = addText(
        document.createElement('div'),
        'ID: ' + (selector.id || backend.backendId || '-') + ' · Zugriff: ' + (selector.accessMode || backend.accessMode || '-')
      );
      subtitle.className = 'backend-subtitle';
      titleBlock.appendChild(title);
      titleBlock.appendChild(subtitle);

      const status = addText(document.createElement('div'), backend.online ? 'online' : 'offline');
      status.className = 'status-pill' + (backend.online ? '' : ' offline');

      header.appendChild(titleBlock);
      header.appendChild(status);
      card.appendChild(header);

      const badges = document.createElement('div');
      badges.className = 'badge-row';
      badges.appendChild(createBadge('Schreiben', Boolean(selector.canWrite)));
      badges.appendChild(createBadge('Aufnahmen', Boolean(selector.canWriteRecordings)));
      badges.appendChild(createBadge('Timer', Boolean(selector.canWriteTimers)));
      badges.appendChild(createBadge('SearchTimer', Boolean(selector.canWriteSearchTimers)));
      card.appendChild(badges);

      return card;
    }

    fetch('/api/backends')
      .then(response => {
        if (!response.ok) {
          throw new Error('HTTP ' + response.status);
        }
        return response.json();
      })
      .then(data => {
        const backends = Array.isArray(data.backends) ? data.backends : [];
        statusElement.textContent = backends.length + ' Backend(s) gefunden';
        backendsElement.replaceChildren();
        backends.forEach(backend => backendsElement.appendChild(renderBackend(backend)));
      })
      .catch(error => {
        statusElement.className = 'status error';
        statusElement.textContent = 'Backend-Auswahl konnte nicht geladen werden: ' + error.message;
      });
  </script>
</body>
</html>)FRONTEND";

    return response;
}

}

TestHttpServer::TestHttpServer(ApiRouter& apiRouter)
    : apiRouter_(apiRouter)
{
}

HttpServerResponse TestHttpServer::handleRequest(
    const HttpServerRequest& request) const
{
    if (!isAuthorized(request))
    {
        return makeUnauthorizedResponse();
    }

    if (request.method == "GET" &&
        (request.path == "/" || request.path == "/frontend"))
    {
        return makeFrontendShellResponse();
    }

    ApiResponse apiResponse;

    if (request.method == "GET")
    {
        apiResponse =
            apiRouter_.handleGet(request.path);
    }
    else if (request.method == "POST")
    {
        apiResponse =
            apiRouter_.handlePost(
                request.path,
                request.body);
    }
    else
    {
        return mapApiResponse(
            405,
            "application/json",
            "{\"error\":\"method not allowed\"}");
    }

    return mapApiResponse(
        apiResponse.statusCode,
        apiResponse.contentType,
        apiResponse.body);
}

HttpServerResponse TestHttpServer::mapApiResponse(
    int statusCode,
    const std::string& contentType,
    const std::string& body) const
{
    HttpServerResponse response;

    response.statusCode = statusCode;
    response.headers["Content-Type"] = contentType;
    response.body = body;

    return response;
}
