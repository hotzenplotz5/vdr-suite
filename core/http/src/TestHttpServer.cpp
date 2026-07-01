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
    h2 { margin: 0; font-size: 1.35rem; }
    h3 { margin: 0 0 0.45rem; font-size: 1.1rem; }
    button {
      border: 0;
      border-radius: 999px;
      padding: 0.55rem 0.85rem;
      background: #2563eb;
      color: #dbeafe;
      font-weight: 700;
      cursor: pointer;
    }
    button:disabled { background: #475569; color: #cbd5e1; cursor: default; }
    .status { margin: 0 0 1rem; color: #cbd5e1; font-size: 1.1rem; }
    .layout { display: grid; gap: 1rem; max-width: 70rem; }
    .backend-list { display: grid; gap: 1rem; }
    .backend-card {
      padding: 1rem;
      border: 1px solid #334155;
      border-radius: 1rem;
      background: #1e293b;
      box-shadow: 0 1rem 2rem rgba(0, 0, 0, 0.18);
      cursor: pointer;
    }
    .backend-card:focus { outline: 3px solid #60a5fa; outline-offset: 3px; }
    .backend-card.selected { border-color: #60a5fa; background: #24324a; }
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
    .detail-card {
      padding: 1rem;
      border: 1px solid #334155;
      border-radius: 1rem;
      background: #111827;
    }
    .detail-header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 1rem;
      margin-bottom: 0.75rem;
    }
    .detail-meta { color: #cbd5e1; margin-bottom: 0.75rem; }
    .module-nav {
      display: flex;
      flex-wrap: wrap;
      gap: 0.5rem;
      margin: 0 0 0.85rem;
    }
    .module-tab {
      background: #1e293b;
      color: #cbd5e1;
      border: 1px solid #334155;
    }
    .module-tab.active {
      background: #2563eb;
      color: #dbeafe;
      border-color: #60a5fa;
    }
    .metric-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(8.5rem, 1fr));
      gap: 0.75rem;
    }
    .metric-card,
    .module-placeholder {
      padding: 0.9rem;
      border: 1px solid #334155;
      border-radius: 0.85rem;
      background: #020617;
    }
    .module-placeholder {
      grid-column: 1 / -1;
      color: #cbd5e1;
    }
    .metric-value {
      font-size: 1.85rem;
      line-height: 1;
      font-weight: 900;
      color: #bfdbfe;
    }
    .metric-label {
      margin-top: 0.35rem;
      color: #cbd5e1;
      font-size: 0.95rem;
    }
    .error { color: #fecaca; }
  </style>
</head>
<body>
  <header><h1>VDR-Suite Frontend</h1></header>
  <main>
    <p id="status" class="status">Lade Backend-Auswahl...</p>
    <section class="layout">
      <section id="backends" class="backend-list" aria-label="Backend-Auswahl"></section>
      <section id="detail" class="detail-card" aria-live="polite">
        <div class="detail-header">
          <h2>Backend-Details</h2>
          <button id="refresh-detail" type="button" disabled>Aktualisieren</button>
        </div>
        <div id="detail-meta" class="detail-meta">Noch kein Backend ausgewählt.</div>
        <nav id="module-nav" class="module-nav" aria-label="Backend-Module">
          <button type="button" class="module-tab active" data-module="overview">Übersicht</button>
          <button type="button" class="module-tab" data-module="channels">Kanäle</button>
          <button type="button" class="module-tab" data-module="timers">Timer</button>
          <button type="button" class="module-tab" data-module="recordings">Aufnahmen</button>
          <button type="button" class="module-tab" data-module="searchtimers">SearchTimer</button>
        </nav>
        <section id="detail-data" class="metric-grid">Klicke auf eine Backend-Karte.</section>
      </section>
    </section>
  </main>
  <script>
    const statusElement = document.getElementById('status');
    const backendsElement = document.getElementById('backends');
    const detailMetaElement = document.getElementById('detail-meta');
    const detailDataElement = document.getElementById('detail-data');
    const refreshDetailButton = document.getElementById('refresh-detail');
    let selectedBackendId = '';
    let selectedBackend = null;
    let selectedModule = 'overview';
    let currentSnapshot = null;

    const moduleLabels = {
      overview: 'Übersicht',
      channels: 'Kanäle',
      timers: 'Timer',
      recordings: 'Aufnahmen',
      searchtimers: 'SearchTimer'
    };

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

    function valueOrZero(value) {
      return Number.isFinite(Number(value)) ? Number(value) : 0;
    }

    function createMetric(label, value) {
      const card = document.createElement('article');
      card.className = 'metric-card';
      const valueElement = addText(document.createElement('div'), String(value));
      valueElement.className = 'metric-value';
      const labelElement = addText(document.createElement('div'), label);
      labelElement.className = 'metric-label';
      card.appendChild(valueElement);
      card.appendChild(labelElement);
      return card;
    }

    function renderSnapshotMetrics(data) {
      detailDataElement.replaceChildren();
      detailDataElement.appendChild(createMetric('Snapshot', data.snapshotAvailable ? 'ja' : 'nein'));
      detailDataElement.appendChild(createMetric('Kanäle', valueOrZero(data.channelCount)));
      detailDataElement.appendChild(createMetric('Events', valueOrZero(data.eventCount)));
      detailDataElement.appendChild(createMetric('Timer', valueOrZero(data.timerCount)));
      detailDataElement.appendChild(createMetric('Aufnahmen', valueOrZero(data.recordingCount)));
    }

    function renderModulePlaceholder(moduleName, data) {
      const countMap = {
        channels: valueOrZero(data.channelCount),
        timers: valueOrZero(data.timerCount),
        recordings: valueOrZero(data.recordingCount),
        searchtimers: 0
      };
      const endpointMap = {
        channels: '/api/vdr/channels',
        timers: '/api/vdr/timers',
        recordings: '/api/vdr/recordings',
        searchtimers: '/api/searchtimers'
      };
      detailDataElement.replaceChildren();
      const box = document.createElement('section');
      box.className = 'module-placeholder';
      box.appendChild(addText(document.createElement('h3'), moduleLabels[moduleName] || moduleName));
      box.appendChild(addText(
        document.createElement('p'),
        'Modul vorbereitet. Aktueller Snapshot-Zähler: ' + String(countMap[moduleName] || 0) + '. Nächster Schritt: Liste aus ' + endpointMap[moduleName] + ' rendern.'
      ));
      detailDataElement.appendChild(box);
    }

    function renderSelectedModule(data) {
      if (selectedModule === 'overview') {
        renderSnapshotMetrics(data);
        return;
      }

      renderModulePlaceholder(selectedModule, data);
    }

    function selectModule(moduleName) {
      selectedModule = moduleName;
      document.querySelectorAll('.module-tab').forEach(button => {
        button.classList.toggle('active', button.dataset.module === moduleName);
      });

      if (currentSnapshot) {
        renderSelectedModule(currentSnapshot);
      }
    }

    function markSelected(backendId) {
      selectedBackendId = backendId;
      document.querySelectorAll('.backend-card').forEach(card => {
        card.classList.toggle('selected', card.dataset.backendId === backendId);
      });
    }

    function loadBackendDetails(backend) {
      selectedBackend = backend;
      selectedModule = 'overview';
      selectModule('overview');
      const selector = backend.frontendSelector || backend;
      const backendId = selector.id || backend.backendId || 'default';
      markSelected(backendId);
      refreshDetailButton.disabled = true;
      detailMetaElement.className = 'detail-meta';
      detailMetaElement.textContent = 'Lade Details für ' + (selector.label || backend.backendName || backendId) + '...';
      detailDataElement.replaceChildren();

      fetch('/api/backends/' + encodeURIComponent(backendId) + '/snapshot')
        .then(response => {
          if (!response.ok) {
            throw new Error('HTTP ' + response.status);
          }
          return response.json();
        })
        .then(data => {
          currentSnapshot = data;
          detailMetaElement.textContent = 'Details für ' + (selector.label || backend.backendName || backendId);
          renderSelectedModule(data);
          refreshDetailButton.disabled = false;
        })
        .catch(error => {
          currentSnapshot = null;
          detailMetaElement.className = 'detail-meta error';
          detailMetaElement.textContent = 'Details konnten nicht geladen werden: ' + error.message;
          detailDataElement.replaceChildren();
          refreshDetailButton.disabled = false;
        });
    }

    function renderBackend(backend) {
      const selector = backend.frontendSelector || backend;
      const backendId = selector.id || backend.backendId || 'default';
      const card = document.createElement('article');
      card.className = 'backend-card';
      card.dataset.backendId = backendId;
      card.tabIndex = 0;
      card.setAttribute('role', 'button');
      card.setAttribute('aria-label', 'Backend ' + (selector.label || backend.backendName || backendId) + ' auswählen');

      const header = document.createElement('div');
      header.className = 'backend-header';

      const titleBlock = document.createElement('div');
      const title = addText(document.createElement('div'), selector.label || backend.backendName || backendId || 'Unbekanntes Backend');
      title.className = 'backend-title';
      const subtitle = addText(
        document.createElement('div'),
        'ID: ' + backendId + ' · Zugriff: ' + (selector.accessMode || backend.accessMode || '-')
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

      card.addEventListener('click', () => loadBackendDetails(backend));
      card.addEventListener('keydown', event => {
        if (event.key === 'Enter' || event.key === ' ') {
          event.preventDefault();
          loadBackendDetails(backend);
        }
      });

      return card;
    }

    document.querySelectorAll('.module-tab').forEach(button => {
      button.addEventListener('click', () => selectModule(button.dataset.module));
    });

    refreshDetailButton.addEventListener('click', () => {
      if (selectedBackend) {
        loadBackendDetails(selectedBackend);
      }
    });

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
        if (backends.length > 0) {
          loadBackendDetails(backends[0]);
        }
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
