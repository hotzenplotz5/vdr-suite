PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
SBINDIR ?= $(PREFIX)/sbin
SYSCONFDIR ?= /etc
DATADIR ?= $(PREFIX)/share/vdr-suite
DOCDIR ?= $(PREFIX)/share/doc/vdr-suite
MANDIR ?= $(PREFIX)/share/man
INSTALL ?= install

.PHONY: install install-runtime install-cli install-docs test-install-staging

install: install-runtime install-cli install-docs

install-runtime: daemon
	$(INSTALL) -d $(DESTDIR)$(SBINDIR)
	$(INSTALL) -m 0755 /tmp/vdr-suite-daemon $(DESTDIR)$(SBINDIR)/vdr-suite-daemon
	$(INSTALL) -d $(DESTDIR)$(SYSCONFDIR)/vdr-suite

install-cli: dashboard-cli
	$(INSTALL) -d $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 /tmp/vdr-suite-dashboard $(DESTDIR)$(BINDIR)/vdr-suite-dashboard

install-docs:
	$(INSTALL) -d $(DESTDIR)$(DOCDIR)
	$(INSTALL) -m 0644 README.md $(DESTDIR)$(DOCDIR)/README.md
	$(INSTALL) -d $(DESTDIR)$(DATADIR)

test-install-staging:
	rm -rf /tmp/vdr-suite-pkgroot
	$(MAKE) install DESTDIR=/tmp/vdr-suite-pkgroot PREFIX=/usr
	test -x /tmp/vdr-suite-pkgroot/usr/sbin/vdr-suite-daemon
	test -x /tmp/vdr-suite-pkgroot/usr/bin/vdr-suite-dashboard
	test -d /tmp/vdr-suite-pkgroot/etc/vdr-suite
	test -f /tmp/vdr-suite-pkgroot/usr/share/doc/vdr-suite/README.md
	test -d /tmp/vdr-suite-pkgroot/usr/share/vdr-suite
