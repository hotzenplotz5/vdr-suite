PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
SBINDIR ?= $(PREFIX)/sbin
SYSCONFDIR ?= /etc
DATADIR ?= $(PREFIX)/share/vdr-suite
DOCDIR ?= $(PREFIX)/share/doc/vdr-suite
MANDIR ?= $(PREFIX)/share/man
INSTALL ?= install

.PHONY: install install-runtime install-cli install-docs install-manpages test-install-staging

install: install-runtime install-cli install-docs install-manpages

install-runtime: daemon
	$(INSTALL) -d $(DESTDIR)$(SBINDIR)
	$(INSTALL) -m 0755 /tmp/vdr-suite-daemon $(DESTDIR)$(SBINDIR)/vdr-suite-daemon
	$(INSTALL) -d $(DESTDIR)$(SYSCONFDIR)/vdr-suite
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend
	$(INSTALL) -m 0644 web/frontend/index.html $(DESTDIR)$(DATADIR)/web/frontend/index.html
	$(INSTALL) -m 0644 web/frontend/app.js $(DESTDIR)$(DATADIR)/web/frontend/app.js
	$(INSTALL) -m 0644 web/frontend/style.css $(DESTDIR)$(DATADIR)/web/frontend/style.css

install-cli: dashboard-cli
	$(INSTALL) -d $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 /tmp/vdr-suite-dashboard $(DESTDIR)$(BINDIR)/vdr-suite-dashboard

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

test-install-staging:
	rm -rf /tmp/vdr-suite-pkgroot
	$(MAKE) install DESTDIR=/tmp/vdr-suite-pkgroot PREFIX=/usr
	test -x /tmp/vdr-suite-pkgroot/usr/sbin/vdr-suite-daemon
	test -x /tmp/vdr-suite-pkgroot/usr/bin/vdr-suite-dashboard
	test -d /tmp/vdr-suite-pkgroot/etc/vdr-suite
	test -f /tmp/vdr-suite-pkgroot/usr/share/doc/vdr-suite/README.md
	test -d /tmp/vdr-suite-pkgroot/usr/share/vdr-suite
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/index.html
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/app.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/style.css
	test -f /tmp/vdr-suite-pkgroot/usr/share/man/man8/vdr-suite-daemon.8
	test -f /tmp/vdr-suite-pkgroot/usr/share/man/man5/vdr-suite.conf.5
	test -f /tmp/vdr-suite-pkgroot/usr/share/man/man1/vdr-suite-dashboard.1
