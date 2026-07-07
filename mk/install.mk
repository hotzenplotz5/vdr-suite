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

test-ci-fast: test-systemd-unit-contract

test-systemd-unit-contract:
	python3 tools/check_systemd_unit_contract.py

install-runtime: daemon
	$(INSTALL) -d $(DESTDIR)$(SBINDIR)
	$(INSTALL) -m 0755 /tmp/vdr-suite-daemon $(DESTDIR)$(SBINDIR)/vdr-suite-daemon
	$(INSTALL) -d $(DESTDIR)$(SYSCONFDIR)/vdr-suite
	$(INSTALL) -d $(DESTDIR)$(CACHEDIR)/channel-logos
	$(INSTALL) -d $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand
	$(INSTALL) -d $(DESTDIR)$(STATEDIR)
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend/modules
	$(INSTALL) -m 0644 web/frontend/index.html $(DESTDIR)$(DATADIR)/web/frontend/index.html
	$(INSTALL) -m 0644 web/frontend/app.js $(DESTDIR)$(DATADIR)/web/frontend/app.js
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend/api
	$(INSTALL) -m 0644 web/frontend/api/client-api.js $(DESTDIR)$(DATADIR)/web/frontend/api/client-api.js
	$(INSTALL) -m 0644 web/frontend/channel-logos.js $(DESTDIR)$(DATADIR)/web/frontend/channel-logos.js
	$(INSTALL) -m 0644 web/frontend/modules/channels.js $(DESTDIR)$(DATADIR)/web/frontend/modules/channels.js
	$(INSTALL) -m 0644 web/frontend/modules/channels.js $(DESTDIR)$(DATADIR)/web/frontend/channel-browser.js
	if [ -f web/frontend/modules/recordings.js ]; then $(INSTALL) -m 0644 web/frontend/modules/recordings.js $(DESTDIR)$(DATADIR)/web/frontend/modules/recordings.js; fi
	if [ -f web/frontend/modules/recordings.js ]; then $(INSTALL) -m 0644 web/frontend/modules/recordings.js $(DESTDIR)$(DATADIR)/web/frontend/recording-browser.js; else $(INSTALL) -m 0644 web/frontend/recording-browser.js $(DESTDIR)$(DATADIR)/web/frontend/recording-browser.js; fi
	$(INSTALL) -m 0644 web/frontend/epg-cache.js $(DESTDIR)$(DATADIR)/web/frontend/epg-cache.js
	$(INSTALL) -m 0644 web/frontend/style.css $(DESTDIR)$(DATADIR)/web/frontend/style.css
	$(INSTALL) -m 0644 web/frontend/logo-vdr-suite.svg $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand/logo-vdr-suite.svg
	$(INSTALL) -m 0644 web/frontend/logo-vdr-suite-dark.svg $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand/logo-vdr-suite-dark.svg
	$(INSTALL) -m 0644 web/frontend/icon-vdr-suite.svg $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand/icon-vdr-suite.svg
	$(INSTALL) -m 0644 web/frontend/favicon.svg $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand/favicon.svg

install-cli: dashboard-cli
	$(INSTALL) -d $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 /tmp/vdr-suite-dashboard $(DESTDIR)$(BINDIR)/vdr-suite-dashboard
	$(INSTALL) -m 0755 tools/vdr_suite_logo_sync.py $(DESTDIR)$(BINDIR)/vdr-suite-logo-sync

install-docs:
	$(INSTALL) -d $(DESTDIR)$(DOCDIR)
	$(INSTALL) -m 0644 README.md $(DESTDIR)$(DOCDIR)/README.md
	$(INSTALL) -d $(DESTDIR)$(DATADIR)

install-manpages:
	$(INSTALL) -d $(DESTDIR)$(MANDIR)/man8
	$(INSTALL) -m 0644 docs/man/man8/vdr-suite-daemon.8 $(DESTDIR)$(MANDIR)/man8/vdr-suite-daemon.8
	$(INSTALL) -d $(DESTDIR)$(MANDIR)/man5
	$(INSTALL) -m 0644 docs/man/man5/vdr-suite.conf.5 $(DESTDIR)$(MANDIR)/man5/vdr-suite.conf.5
	$(INSTALL) -d $(DESTDIR)$(MANDIR)/man1
	$(INSTALL) -m 0644 docs/man/man1/vdr-suite-dashboard.1 $(DESTDIR)$(MANDIR)/man1/vdr-suite-dashboard.1

install-systemd:
	$(INSTALL) -d $(DESTDIR)$(SYSTEMDUNITDIR)
	$(INSTALL) -m 0644 packaging/systemd/vdr-suite-daemon.service $(DESTDIR)$(SYSTEMDUNITDIR)/vdr-suite-daemon.service

test-install-staging:
	rm -rf /tmp/vdr-suite-pkgroot
	$(MAKE) install DESTDIR=/tmp/vdr-suite-pkgroot PREFIX=/usr
	test -x /tmp/vdr-suite-pkgroot/usr/sbin/vdr-suite-daemon
	test -x /tmp/vdr-suite-pkgroot/usr/bin/vdr-suite-dashboard
	test -x /tmp/vdr-suite-pkgroot/usr/bin/vdr-suite-logo-sync
	test -d /tmp/vdr-suite-pkgroot/etc/vdr-suite
	test -d /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos
	test -d /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos/vdr-suite-brand
	test -d /tmp/vdr-suite-pkgroot/var/lib/vdr-suite
	test -f /tmp/vdr-suite-pkgroot/usr/share/doc/vdr-suite/README.md
	test -d /tmp/vdr-suite-pkgroot/usr/share/vdr-suite
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/index.html
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/app.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/channel-logos.js
	test -d /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/modules
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/modules/channels.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/channel-browser.js
	cmp -s /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/modules/channels.js /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/channel-browser.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recording-browser.js
	if [ -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/modules/recordings.js ]; then cmp -s /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/modules/recordings.js /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recording-browser.js; fi
	test -d /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/modules
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/epg-cache.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/style.css
	test -f /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos/vdr-suite-brand/logo-vdr-suite.svg
	test -f /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos/vdr-suite-brand/logo-vdr-suite-dark.svg
	test -f /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos/vdr-suite-brand/icon-vdr-suite.svg
	test -f /tmp/vdr-suite-pkgroot/var/cache/vdr-suite/channel-logos/vdr-suite-brand/favicon.svg
	test -f /tmp/vdr-suite-pkgroot/usr/share/man/man8/vdr-suite-daemon.8
	test -f /tmp/vdr-suite-pkgroot/usr/share/man/man5/vdr-suite.conf.5
	test -f /tmp/vdr-suite-pkgroot/usr/share/man/man1/vdr-suite-dashboard.1
	test -f /tmp/vdr-suite-pkgroot/lib/systemd/system/vdr-suite-daemon.service
