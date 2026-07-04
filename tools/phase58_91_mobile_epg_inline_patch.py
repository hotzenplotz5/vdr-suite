#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
app_path = ROOT / "web/frontend/app.js"
style_path = ROOT / "web/frontend/style.css"
mobile_helper = ROOT / "web/frontend/mobile-epg.js"


def replace_once(text, old, new, label):
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected 1 match, found {count}")
    return text.replace(old, new, 1)


app = app_path.read_text(encoding="utf-8")
style = style_path.read_text(encoding="utf-8")

old_render_detail = '''function renderEpgEventDetail(event, channel, sourceElement) {
  selectedEpgDetail = { event, channel };
  renderEpgSideDetail();
  alignEpgSideDetailToSource(sourceElement);

  const holder = detailDataElement.querySelector('[data-epg-side-detail="true"]');
  const desktop = window.matchMedia && window.matchMedia('(min-width: 1100px)').matches;

  if (holder && !desktop) {
    holder.scrollIntoView({ behavior: 'smooth', block: 'start' });
  }
}
'''

new_render_detail = '''function isMobileEpgLayout() {
  return window.matchMedia && window.matchMedia('(max-width: 720px)').matches;
}

function clearMobileEpgInlineDetails(focusElement) {
  detailDataElement.querySelectorAll('[data-epg-inline-detail="true"]').forEach(element => {
    element.remove();
  });

  detailDataElement.querySelectorAll('.epg-program-card-expanded').forEach(card => {
    card.classList.remove('epg-program-card-expanded');
  });

  detailDataElement.querySelectorAll('.epg-program-event.selected').forEach(button => {
    button.classList.remove('selected');
    button.removeAttribute('aria-expanded');
  });

  if (focusElement && typeof focusElement.focus === 'function') {
    try {
      focusElement.focus({ preventScroll: true });
    } catch (focusError) {
      void focusError;
      focusElement.focus();
    }
  }
}

function renderMobileEpgInlineDetail(event, channel, sourceElement) {
  const card = sourceElement ? sourceElement.closest('.epg-program-card') : null;
  const events = card ? card.querySelector('.epg-program-events') : null;

  if (!card || !events) {
    renderEpgSideDetail();
    alignEpgSideDetailToSource(sourceElement);
    return;
  }

  clearMobileEpgInlineDetails(null);

  card.classList.add('epg-program-card-expanded');
  sourceElement.classList.add('selected');
  sourceElement.setAttribute('aria-expanded', 'true');

  const inline = document.createElement('section');
  inline.className = 'epg-program-inline-detail';
  inline.dataset.epgInlineDetail = 'true';

  const back = document.createElement('button');
  back.type = 'button';
  back.className = 'epg-mobile-back-button';
  back.textContent = 'Zurück zur EPG-Liste';
  back.addEventListener('click', () => {
    selectedEpgDetail = null;
    clearMobileEpgInlineDetails(sourceElement);
    renderEpgSideDetail();
  });

  inline.appendChild(back);
  inline.appendChild(createEpgEventDetailCard(event, channel));

  events.insertAdjacentElement('afterend', inline);
  renderEpgSideDetail();

  window.setTimeout(() => {
    inline.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
  }, 30);
}

function renderEpgEventDetail(event, channel, sourceElement) {
  selectedEpgDetail = { event, channel };

  if (
    isMobileEpgLayout()
    && sourceElement
    && sourceElement.classList
    && sourceElement.classList.contains('epg-program-event')
  ) {
    renderMobileEpgInlineDetail(event, channel, sourceElement);
    return;
  }

  clearMobileEpgInlineDetails(null);
  renderEpgSideDetail();
  alignEpgSideDetailToSource(sourceElement);

  const holder = detailDataElement.querySelector('[data-epg-side-detail="true"]');
  const desktop = window.matchMedia && window.matchMedia('(min-width: 1100px)').matches;

  if (holder && !desktop) {
    holder.scrollIntoView({ behavior: 'smooth', block: 'start' });
  }
}
'''

if old_render_detail in app and "function renderMobileEpgInlineDetail" not in app:
    app = replace_once(app, old_render_detail, new_render_detail, "replace renderEpgEventDetail")

old_mobile_guard = '''  const nowSeconds = Math.floor(Date.now() / 1000);
  const bounds = epgTimelineBounds(nowSeconds);
  const limit = EPG_VISIBLE_CHANNEL_LIMIT;
'''

new_mobile_guard = '''  const nowSeconds = Math.floor(Date.now() / 1000);

  if (isMobileEpgLayout() && (epgProgramView === 'horizontal' || epgProgramView === 'vertical')) {
    epgProgramView = 'live';
    epgTimeAxisMode = 'horizontal';
  }

  const bounds = epgTimelineBounds(nowSeconds);
  const limit = isMobileEpgLayout() ? 8 : EPG_VISIBLE_CHANNEL_LIMIT;
'''

if old_mobile_guard in app:
    app = replace_once(app, old_mobile_guard, new_mobile_guard, "insert mobile EPG mode guard")

app = app.replace(
    "  timeView.className = 'epg-view-button ' + (epgProgramView === 'horizontal' ? 'active' : '');\n",
    "  timeView.className = 'epg-view-button ' + (epgProgramView === 'horizontal' ? 'active' : '');\n  timeView.classList.add('epg-desktop-only');\n",
    1,
)

app = app.replace(
    "  verticalTimeView.className = 'epg-view-button ' + (epgProgramView === 'vertical' ? 'active' : '');\n",
    "  verticalTimeView.className = 'epg-view-button ' + (epgProgramView === 'vertical' ? 'active' : '');\n  verticalTimeView.classList.add('epg-desktop-only');\n",
    1,
)

old_hint_anchor = '''  header.appendChild(cacheStatus);

  const modeRow = document.createElement('div');
'''

new_hint_anchor = '''  header.appendChild(cacheStatus);

  if (isMobileEpgLayout()) {
    const mobileHint = addText(
      document.createElement('p'),
      'Mobile Ansicht: Zeitachsen sind deaktiviert. Sendung antippen, die Kachel klappt mit Details auf.'
    );
    mobileHint.className = 'epg-mobile-mode-note';
    header.appendChild(mobileHint);
  }

  const modeRow = document.createElement('div');
'''

if old_hint_anchor in app and "epg-mobile-mode-note" not in app:
    app = replace_once(app, old_hint_anchor, new_hint_anchor, "add mobile EPG hint")

app = app.replace("  previous.textContent = 'Vorherige 15';\n", "  previous.textContent = 'Vorherige ' + String(limit);\n", 1)
app = app.replace("  next.textContent = 'Nächste 15';\n", "  next.textContent = 'Nächste ' + String(limit);\n", 1)
app = app.replace(
    "  timePager.className = 'epg-time-window-pager';\n",
    "  timePager.className = 'epg-time-window-pager';\n  timePager.classList.add('epg-desktop-only');\n",
    1,
)

css_block = '''
/* Phase 58.91: mobile EPG inline expansion */
@media (max-width: 720px) {
  html,
  body {
    overflow-x: hidden;
  }

  .epg-desktop-only,
  .epg-time-window-pager,
  .epg-time-grid,
  .epg-vertical-time-grid {
    display: none !important;
  }

  .detail-card,
  #detail-data,
  .epg-workbench,
  .epg-workbench-main,
  .epg-timeline-module,
  .epg-program-grid,
  .epg-program-card,
  .epg-program-events,
  .epg-program-event {
    box-sizing: border-box;
    width: 100%;
    max-width: 100%;
    min-width: 0;
  }

  .epg-workbench {
    display: block;
    overflow-x: hidden;
  }

  .epg-sidebar {
    display: none !important;
  }

  .epg-program-grid {
    display: grid;
    grid-template-columns: minmax(0, 1fr);
    gap: 0.85rem;
  }

  .epg-program-card {
    display: grid;
    grid-template-columns: minmax(0, 1fr);
    gap: 0.72rem;
    padding: 0.72rem;
  }

  .epg-program-card-expanded {
    border-color: rgba(34, 211, 238, 0.78);
    box-shadow:
      0 0 0 2px rgba(34, 211, 238, 0.24),
      0 1rem 2rem rgba(2, 6, 23, 0.35);
  }

  .epg-program-channel {
    width: 100%;
  }

  .epg-program-events {
    display: grid;
    grid-template-columns: minmax(0, 1fr);
    gap: 0.62rem;
  }

  .epg-program-event.selected {
    border-color: rgba(45, 212, 191, 0.85);
    background:
      radial-gradient(circle at top left, rgba(20, 184, 166, 0.22), transparent 42%),
      linear-gradient(135deg, rgba(15, 23, 42, 0.98), rgba(30, 41, 59, 0.86));
  }

  .epg-program-inline-detail {
    grid-column: 1 / -1;
    width: 100%;
    max-width: 100%;
    min-width: 0;
    margin-top: 0.15rem;
  }

  .epg-program-inline-detail .epg-event-detail {
    width: 100%;
    max-width: 100%;
    min-width: 0;
    box-sizing: border-box;
  }

  .epg-mobile-back-button {
    width: 100%;
    margin: 0 0 0.65rem;
    border-radius: 0.78rem;
    background: rgba(30, 41, 59, 0.98);
    border: 1px solid rgba(96, 165, 250, 0.46);
    color: #bfdbfe;
  }

  .epg-mobile-mode-note {
    margin-top: 0.65rem;
    color: #7dd3fc;
    font-size: 0.9rem;
    line-height: 1.35;
  }

  .epg-program-inline-detail .epg-detail-actions,
  .epg-program-inline-detail .epg-event-detail .epg-detail-actions {
    grid-template-columns: minmax(0, 1fr);
  }
}
'''

if "Phase 58.91: mobile EPG inline expansion" not in style:
    style = style.rstrip() + "\n\n" + css_block.strip() + "\n"

app_path.write_text(app, encoding="utf-8")
style_path.write_text(style, encoding="utf-8")

if mobile_helper.exists():
    mobile_helper.unlink()

print("Phase 58.91 mobile EPG inline patch applied")
