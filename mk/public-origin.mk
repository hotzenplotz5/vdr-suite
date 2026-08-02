NGINXSNIPPETDIR ?= $(SYSCONFDIR)/nginx/snippets

.PHONY: install-public-origin-runtime install-nginx test-public-url-runtime test-public-origin-install-staging test-public-origin

install-runtime: install-public-origin-runtime

install-public-origin-runtime:
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend/platform
	$(INSTALL) -m 0644 web/frontend/platform/public-url.js $(DESTDIR)$(DATADIR)/web/frontend/platform/public-url.js

install: install-nginx

install-nginx:
	$(INSTALL) -d $(DESTDIR)$(NGINXSNIPPETDIR)
	$(INSTALL) -m 0644 packaging/nginx/vdr-suite.conf $(DESTDIR)$(NGINXSNIPPETDIR)/vdr-suite.conf

test-public-url-runtime:
	node web/frontend/tests/test_public_url_runtime.js

test-public-origin-install-staging:
	rm -rf /tmp/vdr-suite-public-origin-pkgroot
	$(MAKE) install-public-origin-runtime install-nginx \
		DESTDIR=/tmp/vdr-suite-public-origin-pkgroot PREFIX=/usr
	test -f /tmp/vdr-suite-public-origin-pkgroot/usr/share/vdr-suite/web/frontend/platform/public-url.js
	test -f /tmp/vdr-suite-public-origin-pkgroot/etc/nginx/snippets/vdr-suite.conf
	test "$$(stat -c '%a' /tmp/vdr-suite-public-origin-pkgroot/etc/nginx/snippets/vdr-suite.conf)" = 644
	node --check /tmp/vdr-suite-public-origin-pkgroot/usr/share/vdr-suite/web/frontend/platform/public-url.js
	grep -F 'location ^~ /vdr-suite/' /tmp/vdr-suite-public-origin-pkgroot/etc/nginx/snippets/vdr-suite.conf >/dev/null
	grep -F 'proxy_pass http://127.0.0.1:18080/;' /tmp/vdr-suite-public-origin-pkgroot/etc/nginx/snippets/vdr-suite.conf >/dev/null
	grep -F 'proxy_cookie_path / /vdr-suite/;' /tmp/vdr-suite-public-origin-pkgroot/etc/nginx/snippets/vdr-suite.conf >/dev/null
	! grep -E 'location[[:space:]]+[^;]*[[:space:]]/api(/|[[:space:]]|\{)' /tmp/vdr-suite-public-origin-pkgroot/etc/nginx/snippets/vdr-suite.conf >/dev/null

test-public-origin: test-public-url-runtime test-public-origin-install-staging
	python3 tools/check_public_origin_architecture.py

test-frontend-contracts: test-public-url-runtime
test-architecture: test-public-origin
