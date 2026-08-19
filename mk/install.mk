PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
SBINDIR ?= $(PREFIX)/sbin
SYSCONFDIR ?= /etc
DATADIR ?= $(PREFIX)/share/vdr-suite
DOCDIR ?= $(PREFIX)/share/doc/vdr-suite
MANDIR ?= $(PREFIX)/share/man
LOCALSTATEDIR ?= /var
CACHEDIR ?= $(LOCALSTATEDIR)/cache/vdr-suite
STATEDIR ?= $(LOCALSTATEDIR)/lib/vdr-suite
SYSTEMDUNITDIR ?= /lib/systemd/system
INSTALL ?= install

.PHONY: install install-runtime install-cli install-docs install-manpages install-systemd test-install-staging test-systemd-unit-contract

install: install-runtime install-cli install-docs install-manpages install-systemd

test-systemd-unit-contract:
	python3 tools/check_systemd_unit_contract.py

install-runtime: daemon backend-agent backend-agent-enrollment backend-agent-admin backend-agent-command-admin
	$(INSTALL) -d $(DESTDIR)$(SBINDIR)
	$(INSTALL) -m 0755 $(BUILD_DIR)/vdr-suite-daemon $(DESTDIR)$(SBINDIR)/vdr-suite-daemon
	$(INSTALL) -m 0755 $(BUILD_DIR)/vdr-suite-backend-agent $(DESTDIR)$(SBINDIR)/vdr-suite-backend-agent
	$(INSTALL) -m 0755 $(BUILD_DIR)/vdr-suite-backend-agent-enroll $(DESTDIR)$(SBINDIR)/vdr-suite-backend-agent-enroll
	$(INSTALL) -m 0755 $(BUILD_DIR)/vdr-suite-backend-agent-admin $(DESTDIR)$(SBINDIR)/vdr-suite-backend-agent-admin
	$(INSTALL) -m 0755 $(BUILD_DIR)/vdr-suite-backend-agent-command-admin $(DESTDIR)$(SBINDIR)/vdr-suite-backend-agent-command-admin
	$(INSTALL) -d $(DESTDIR)$(SYSCONFDIR)/vdr-suite
	test -e $(DESTDIR)$(SYSCONFDIR)/vdr-suite/backend-agent.conf || \
		$(INSTALL) -m 0644 packaging/systemd/backend-agent.conf $(DESTDIR)$(SYSCONFDIR)/vdr-suite/backend-agent.conf
	$(INSTALL) -d $(DESTDIR)$(CACHEDIR)/channel-logos
	$(INSTALL) -d $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand
	$(INSTALL) -d $(DESTDIR)$(STATEDIR)
	$(INSTALL) -d -m 0700 $(DESTDIR)$(STATEDIR)/backend-agent
	$(INSTALL) -d -m 0700 $(DESTDIR)$(STATEDIR)/secrets
	$(INSTALL) -d -m 0700 $(DESTDIR)$(STATEDIR)/secrets/series-artwork
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend/modules
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend/platform
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend/locales
	rm -f \
		$(DESTDIR)$(DATADIR)/web/frontend/recording-browser.js \
		$(DESTDIR)$(DATADIR)/web/frontend/recording-artwork.js \
		$(DESTDIR)$(DATADIR)/web/frontend/recording-trash-ux.js \
		$(DESTDIR)$(DATADIR)/web/frontend/modules/recordings.js
	$(INSTALL) -m 0644 web/frontend/index.html $(DESTDIR)$(DATADIR)/web/frontend/index.html
	$(INSTALL) -m 0644 web/frontend/app.js $(DESTDIR)$(DATADIR)/web/frontend/app.js
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend/api
	$(INSTALL) -m 0644 web/frontend/api/client-api.js $(DESTDIR)$(DATADIR)/web/frontend/api/client-api.js
	$(INSTALL) -m 0644 web/frontend/api/genre-client-api.js $(DESTDIR)$(DATADIR)/web/frontend/api/genre-client-api.js
	$(INSTALL) -m 0644 web/frontend/platform/bootstrap.js $(DESTDIR)$(DATADIR)/web/frontend/platform/bootstrap.js
	$(INSTALL) -m 0644 web/frontend/platform/i18n.js $(DESTDIR)$(DATADIR)/web/frontend/platform/i18n.js
	$(INSTALL) -m 0644 web/frontend/platform/helpers.js $(DESTDIR)$(DATADIR)/web/frontend/platform/helpers.js
	$(INSTALL) -m 0644 web/frontend/platform/deferred-runtime-loader.js $(DESTDIR)$(DATADIR)/web/frontend/platform/deferred-runtime-loader.js
	$(INSTALL) -m 0644 web/frontend/settings-series-artwork.js $(DESTDIR)$(DATADIR)/web/frontend/settings-series-artwork.js
	$(INSTALL) -m 0644 web/frontend/locales/de.js $(DESTDIR)$(DATADIR)/web/frontend/locales/de.js
	$(INSTALL) -m 0644 web/frontend/locales/en.js $(DESTDIR)$(DATADIR)/web/frontend/locales/en.js
	$(INSTALL) -m 0644 web/frontend/channel-logos.js $(DESTDIR)$(DATADIR)/web/frontend/channel-logos.js
	$(INSTALL) -m 0644 web/frontend/channel-day-program.js $(DESTDIR)$(DATADIR)/web/frontend/channel-day-program.js
	$(INSTALL) -m 0644 web/frontend/channel-day-program-compat.js $(DESTDIR)$(DATADIR)/web/frontend/channel-day-program-compat.js
	cat \
		web/frontend/epg-metadata-detail.js \
		web/frontend/epg-searchtimer-actions.js \
		web/frontend/epg-metadata-detail-hook.js \
		web/frontend/epg-detail-desktop-focus.js \
		> $(DESTDIR)$(DATADIR)/web/frontend/.epg-searchtimer-actions.js.tmp
	chmod 0644 $(DESTDIR)$(DATADIR)/web/frontend/.epg-searchtimer-actions.js.tmp
	mv -f \
		$(DESTDIR)$(DATADIR)/web/frontend/.epg-searchtimer-actions.js.tmp \
		$(DESTDIR)$(DATADIR)/web/frontend/epg-searchtimer-actions.js
	$(INSTALL) -m 0644 web/frontend/epg-detail-owner.js $(DESTDIR)$(DATADIR)/web/frontend/epg-detail-owner.js
	$(INSTALL) -m 0644 web/frontend/modules/channels.js $(DESTDIR)$(DATADIR)/web/frontend/modules/channels.js
	$(INSTALL) -m 0644 web/frontend/modules/channels.js $(DESTDIR)$(DATADIR)/web/frontend/channel-browser.js
	$(INSTALL) -m 0644 web/frontend/modules/timers.js $(DESTDIR)$(DATADIR)/web/frontend/modules/timers.js
	$(INSTALL) -m 0644 web/frontend/modules/searchtimers.js $(DESTDIR)$(DATADIR)/web/frontend/modules/searchtimers.js
	$(INSTALL) -m 0644 web/frontend/modules/genres.js $(DESTDIR)$(DATADIR)/web/frontend/modules/genres.js
	$(INSTALL) -m 0644 web/frontend/modules/global-search.js $(DESTDIR)$(DATADIR)/web/frontend/modules/global-search.js
	$(INSTALL) -m 0644 web/frontend/epg-cache.js $(DESTDIR)$(DATADIR)/web/frontend/epg-cache.js
	$(INSTALL) -m 0644 web/frontend/style.css $(DESTDIR)$(DATADIR)/web/frontend/style.css
	$(INSTALL) -m 0644 web/frontend/logo-vdr-suite.svg $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand/logo-vdr-suite.svg
	$(INSTALL) -m 0644 web/frontend/logo-vdr-suite-dark.svg $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand/logo-vdr-suite-dark.svg
	$(INSTALL) -m 0644 web/frontend/icon-vdr-suite.svg $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand/icon-vdr-suite.svg
	$(INSTALL) -m 0644 web/frontend/favicon.svg $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand/favicon.svg
	$(INSTALL) -m 0644 web/frontend/assets/recording-genre-sprite.svg $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand/recording-genre-sprite.svg
	$(INSTALL) -m 0644 web/frontend/assets/recording-genre-thriller.svg $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand/recording-genre-thriller.svg
	$(INSTALL) -m 0644 web/frontend/assets/recording-genre-musik.svg $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand/recording-genre-musik.svg
	$(INSTALL) -m 0644 web/frontend/assets/recording-genre-drama.svg $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand/recording-genre-drama.svg
	$(INSTALL) -m 0644 web/frontend/assets/recording-genre-mystery.svg $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand/recording-genre-mystery.svg
	$(INSTALL) -m 0644 web/frontend/assets/recording-genre-scifi.svg $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand/recording-genre-scifi.svg
	$(INSTALL) -m 0644 web/frontend/assets/recording-genre-serien.svg $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand/recording-genre-serien.svg
	$(INSTALL) -m 0644 web/frontend/assets/recording-genre-western.svg $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand/recording-genre-western.svg
	$(INSTALL) -m 0644 web/frontend/assets/recording-genre-doku.svg $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand/recording-genre-doku.svg
	$(INSTALL) -m 0644 web/frontend/assets/recording-genre-action.svg $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand/recording-genre-action.svg
	$(INSTALL) -m 0644 web/frontend/assets/recording-genre-musical.svg $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand/recording-genre-musical.svg

install-cli: dashboard-cli
	$(INSTALL) -d $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(BUILD_DIR)/vdr-suite-dashboard $(DESTDIR)$(BINDIR)/vdr-suite-dashboard
	$(INSTALL) -m 0755 tools/vdr_suite_logo_sync.py $(DESTDIR)$(BINDIR)/vdr-suite-logo-sync

install-docs:
	$(INSTALL) -d $(DESTDIR)$(DOCDIR)
	$(INSTALL) -m 0644 README.md $(DESTDIR)$(DOCDIR)/README.md
	$(INSTALL) -d $(DESTDIR)$(DATADIR)

install-manpages:
	$(INSTALL) -d $(DESTDIR)$(MANDIR)/man8
	$(INSTALL) -m 0644 docs/man/man8/vdr-suite-daemon.8 $(DESTDIR)$(MANDIR)/man8/vdr-suite-daemon.8
	$(INSTALL) -m 0644 docs/man/man8/vdr-suite-backend-agent.8 $(DESTDIR)$(MANDIR)/man8/vdr-suite-backend-agent.8
	$(INSTALL) -m 0644 docs/man/man8/vdr-suite-backend-agent-enroll.8 $(DESTDIR)$(MANDIR)/man8/vdr-suite-backend-agent-enroll.8
	$(INSTALL) -m 0644 docs/man/man8/vdr-suite-backend-agent-admin.8 $(DESTDIR)$(MANDIR)/man8/vdr-suite-backend-agent-admin.8
	$(INSTALL) -d $(DESTDIR)$(MANDIR)/man5
	$(INSTALL) -m 0644 docs/man/man5/vdr-suite.conf.5 $(DESTDIR)$(MANDIR)/man5/vdr-suite.conf.5
	$(INSTALL) -d $(DESTDIR)$(MANDIR)/man1
	$(INSTALL) -m 0644 docs/man/man1/vdr-suite-dashboard.1 $(DESTDIR)$(MANDIR)/man1/vdr-suite-dashboard.1

install-systemd:
	$(INSTALL) -d $(DESTDIR)$(SYSTEMDUNITDIR)
	$(INSTALL) -m 0644 packaging/systemd/vdr-suite-daemon.service $(DESTDIR)$(SYSTEMDUNITDIR)/vdr-suite-daemon.service
	$(INSTALL) -m 0644 packaging/systemd/vdr-suite-backend-agent.service $(DESTDIR)$(SYSTEMDUNITDIR)/vdr-suite-backend-agent.service
	$(INSTALL) -d $(DESTDIR)$(SYSCONFDIR)/default
	test -e $(DESTDIR)$(SYSCONFDIR)/default/vdr-suite-daemon || \
		$(INSTALL) -m 0644 packaging/systemd/vdr-suite-daemon.default $(DESTDIR)$(SYSCONFDIR)/default/vdr-suite-daemon

test-install-staging:
	rm -rf /tmp/vdr-suite-pkgroot
	$(MAKE) install DESTDIR=/tmp/vdr-suite-pkgroot PREFIX=/usr
	test -x /tmp/vdr-suite-pkgroot/usr/sbin/vdr-suite-daemon
	test -x /tmp/vdr-suite-pkgroot/usr/sbin/vdr-suite-backend-agent
	test -x /tmp/vdr-suite-pkgroot/usr/sbin/vdr-suite-backend-agent-enroll
	test -x /tmp/vdr-suite-pkgroot/usr/sbin/vdr-suite-backend-agent-admin
	test -x /tmp/vdr-suite-pkgroot/usr/sbin/vdr-suite-backend-agent-command-admin
	test -x /tmp/vdr-suite-pkgroot/usr/bin/vdr-suite-dashboard
	test -x /tmp/vdr-suite-pkgroot/usr/bin/vdr-suite-logo-sync
	test -d /tmp/vdr-suite-pkgroot/etc/vdr-suite
	test -f /tmp/vdr-suite-pkgroot/etc/vdr-suite/backend-agent.conf
	grep -F 'CONTROL_PLANE_URL=https://' /tmp/vdr-suite-pkgroot/etc/vdr-suite/backend-agent.conf >/dev/null
	! grep -E -i '(token|password|credential_secret|authorization|cookie|csrf)=' /tmp/vdr-suite-pkgroot/etc/vdr-suite/backend-agent.conf >/dev/null
	test -f /tmp/vdr-suite-pkgroot/etc/default/vdr-suite-daemon
	grep -F 'VDR_SUITE_SUITE_BRIDGE_ENABLED=true' /tmp/vdr-suite-pkgroot/etc/default/vdr-suite-daemon >/dev/null
	test -d /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos
	test -d /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos/vdr-suite-brand
	test -d /tmp/vdr-suite-pkgroot/var/lib/vdr-suite
	test -d /tmp/vdr-suite-pkgroot/var/lib/vdr-suite/backend-agent
	test "$$(stat -c '%a' /tmp/vdr-suite-pkgroot/var/lib/vdr-suite/backend-agent)" = 700
	test -d /tmp/vdr-suite-pkgroot/var/lib/vdr-suite/secrets/series-artwork
	test "$$(stat -c '%a' /tmp/vdr-suite-pkgroot/var/lib/vdr-suite/secrets/series-artwork)" = 700
	test -f /tmp/vdr-suite-pkgroot/usr/share/doc/vdr-suite/README.md
	test -d /tmp/vdr-suite-pkgroot/usr/share/vdr-suite
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/index.html
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/app.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/api/genre-client-api.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/channel-logos.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/settings-series-artwork.js
	! grep -F '/frontend/channel-day-program.js' /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/platform/deferred-runtime-loader.js >/dev/null
	! grep -F '/frontend/channel-day-program-compat.js' /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/platform/deferred-runtime-loader.js >/dev/null
	grep -F '/frontend/epg-searchtimer-actions.js' /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/platform/deferred-runtime-loader.js >/dev/null
	grep -F '/frontend/epg-detail-owner.js' /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/platform/deferred-runtime-loader.js >/dev/null
	grep -F '/frontend/modules/genres.js' /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/platform/deferred-runtime-loader.js >/dev/null
	grep -F '/frontend/settings-series-artwork.js' /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/platform/deferred-runtime-loader.js >/dev/null
	! grep -F '/frontend/epg-metadata-detail.js' /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/platform/deferred-runtime-loader.js >/dev/null
	! grep -F '/frontend/epg-metadata-detail-hook.js' /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/platform/deferred-runtime-loader.js >/dev/null
	! grep -F '/frontend/epg-detail-desktop-focus.js' /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/platform/deferred-runtime-loader.js >/dev/null
	! grep -F '/frontend/recording-trash-ux.js' /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/platform/deferred-runtime-loader.js >/dev/null
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/channel-day-program.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/channel-day-program-compat.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/epg-searchtimer-actions.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/epg-detail-owner.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/modules/genres.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/modules/global-search.js
	node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/epg-searchtimer-actions.js
	node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/epg-detail-owner.js
	node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/modules/genres.js
	node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/modules/global-search.js
	node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/settings-series-artwork.js
	grep -F 'global.VdrSuiteEpgMetadataDetail = Object.freeze' /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/epg-searchtimer-actions.js >/dev/null
	grep -F 'global.VdrSuiteEpgSearchTimerActions = Object.freeze' /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/epg-searchtimer-actions.js >/dev/null
	grep -F 'global.VdrSuiteEpgMetadataDetailHook = Object.freeze' /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/epg-searchtimer-actions.js >/dev/null
	grep -F 'global.VdrSuiteEpgDetailDesktopFocus = Object.freeze' /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/epg-searchtimer-actions.js >/dev/null
	grep -F 'global.VdrSuiteSeriesArtworkSettings = Object.freeze' /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/settings-series-artwork.js >/dev/null
	! test -e /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recording-trash-ux.js
	test -d /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/modules
	test -d /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/platform
	test -d /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/locales
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/platform/bootstrap.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/platform/i18n.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/platform/helpers.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/platform/deferred-runtime-loader.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/locales/de.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/locales/en.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/modules/channels.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/channel-browser.js
	cmp -s /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/modules/channels.js /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/channel-browser.js
	! test -e /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/modules/recordings.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/modules/timers.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/modules/searchtimers.js
	! test -e /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recording-browser.js
	! test -e /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recording-artwork.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/epg-cache.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/style.css
	test -f /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos/vdr-suite-brand/logo-vdr-suite.svg
	test -f /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos/vdr-suite-brand/logo-vdr-suite-dark.svg
	test -f /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos/vdr-suite-brand/icon-vdr-suite.svg
	test -f /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos/vdr-suite-brand/favicon.svg
	test -f /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos/vdr-suite-brand/recording-genre-sprite.svg
	test -f /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos/vdr-suite-brand/recording-genre-thriller.svg
	test -f /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos/vdr-suite-brand/recording-genre-musik.svg
	test -f /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos/vdr-suite-brand/recording-genre-drama.svg
	test -f /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos/vdr-suite-brand/recording-genre-mystery.svg
	test -f /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos/vdr-suite-brand/recording-genre-scifi.svg
	test -f /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos/vdr-suite-brand/recording-genre-serien.svg
	test -f /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos/vdr-suite-brand/recording-genre-western.svg
	test -f /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos/vdr-suite-brand/recording-genre-doku.svg
	test -f /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos/vdr-suite-brand/recording-genre-action.svg
	test -f /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos/vdr-suite-brand/recording-genre-musical.svg
	test -f /tmp/vdr-suite-pkgroot/usr/share/man/man8/vdr-suite-daemon.8
	test -f /tmp/vdr-suite-pkgroot/usr/share/man/man8/vdr-suite-backend-agent.8
	test -f /tmp/vdr-suite-pkgroot/usr/share/man/man8/vdr-suite-backend-agent-enroll.8
	test -f /tmp/vdr-suite-pkgroot/usr/share/man/man8/vdr-suite-backend-agent-admin.8
	test -f /tmp/vdr-suite-pkgroot/usr/share/man/man5/vdr-suite.conf.5
	test -f /tmp/vdr-suite-pkgroot/usr/share/man/man1/vdr-suite-dashboard.1
	test -f /tmp/vdr-suite-pkgroot/lib/systemd/system/vdr-suite-daemon.service
	test -f /tmp/vdr-suite-pkgroot/lib/systemd/system/vdr-suite-backend-agent.service