// Phase 58.91: mobile EPG expandable cards.
// Loaded after app.js, channel-logos.js and channel-browser.js.

(function enableMobileEpgExpandableCards() {
  const mobileQuery = window.matchMedia ? window.matchMedia('(max-width: 720px)') : null;

  function isMobile() {
    return mobileQuery ? mobileQuery.matches : window.innerWidth <= 720;
  }

  function injectStyles() {
    if (document.getElementById('mobile-epg-expanded-card-style')) {
      return;
    }

    const style = document.createElement('style');
    style.id = 'mobile-epg-expanded-card-style';
    style.textContent = `
@media (max-width: 720px) {
  body.mobile-epg-mode {
    overflow-x: hidden;
  }

  body.mobile-epg-mode main,
  body.mobile-epg-mode .layout,
  body.mobile-epg-mode #detail,
  body.mobile-epg-mode #detail-data,
  body.mobile-epg-mode .epg-workbench,
  body.mobile-epg-mode .epg-workbench-main,
  body.mobile-epg-mode .epg-timeline-module,
  body.mobile-epg-mode .epg-program-grid,
  body.mobile-epg-mode .epg-program-card,
  body.mobile-epg-mode .epg-program-events,
  body.mobile-epg-mode .epg-program-event {
    box-sizing: border-box;
    width: 100% !important;
    max-width: 100% !important;
    min-width: 0 !important;
  }

  body.mobile-epg-mode .epg-workbench,
  body.mobile-epg-mode .epg-program-grid,
  body.mobile-epg-mode .epg-program-card,
  body.mobile-epg-mode .epg-program-events {
    display: grid !important;
    grid-template-columns: minmax(0, 1fr) !important;
  }

  body.mobile-epg-mode .epg-sidebar {
    display: none !important;
  }

  body.mobile-epg-mode .epg-program-card.mobile-epg-card-expanded {
    border-color: rgba(34, 211, 238, 0.72);
    box-shadow: 0 0 0 2px rgba(34, 211, 238, 0.22);
  }

  body.mobile-epg-mode .mobile-epg-expanded-detail {
    grid-column: 1 / -1;
    display: block;
    width: 100%;
    max-width: 100%;
    min-width: 0;
    margin-top: 0.65rem;
  }

  body.mobile-epg-mode .mobile-epg-expanded-detail .epg-event-detail,
  body.mobile-epg-mode .mobile-epg-expanded-detail .epg-event-detail-action-panel {
    box-sizing: border-box;
    width: 100%;
    max-width: 100%;
    min-width: 0;
  }

  body.mobile-epg-mode .mobile-epg-back-button {
    width: 100%;
    margin: 0 0 0.65rem;
    border-radius: 0.8rem;
    background: rgba(30, 41, 59, 0.98);
    border: 1px solid rgba(96, 165, 250, 0.42);
    color: #bfdbfe;
  }
}
`;
    document.head.appendChild(style);
  }

  function removeExpandedDetails() {
    document.querySelectorAll('.mobile-epg-expanded-detail').forEach(element => element.remove());
    document.querySelectorAll('.mobile-epg-card-expanded').forEach(element => element.classList.remove('mobile-epg-card-expanded'));
  }

  function expandCardFromDetail(sourceButton) {
    if (!isMobile() || !sourceButton) {
      return;
    }

    const card = sourceButton.closest('.epg-program-card');
    const events = card ? card.querySelector('.epg-program-events') : null;
    const detail = document.querySelector('.epg-side-detail .epg-event-detail');

    if (!card || !events || !detail) {
      return;
    }

    removeExpandedDetails();
    card.classList.add('mobile-epg-card-expanded');

    const holder = document.createElement('section');
    holder.className = 'mobile-epg-expanded-detail';

    const back = document.createElement('button');
    back.type = 'button';
    back.className = 'mobile-epg-back-button';
    back.textContent = 'Zurueck zur EPG-Liste';
    back.addEventListener('click', () => {
      holder.remove();
      card.classList.remove('mobile-epg-card-expanded');
      sourceButton.focus({ preventScroll: true });
    });

    holder.appendChild(back);
    holder.appendChild(detail);
    events.insertAdjacentElement('afterend', holder);

    window.setTimeout(() => {
      holder.scrollIntoView({ behavior: 'smooth', block: 'start' });
    }, 40);
  }

  injectStyles();

  document.addEventListener('click', event => {
    if (!isMobile()) {
      return;
    }

    const target = event.target;
    const sourceButton = target && target.closest ? target.closest('.epg-program-event') : null;
    if (!sourceButton) {
      return;
    }

    window.setTimeout(() => expandCardFromDetail(sourceButton), 30);
  });

  if (mobileQuery && typeof mobileQuery.addEventListener === 'function') {
    mobileQuery.addEventListener('change', () => {
      if (!isMobile()) {
        removeExpandedDetails();
      }
    });
  } else {
    window.addEventListener('resize', () => {
      if (!isMobile()) {
        removeExpandedDetails();
      }
    });
  }
})();
