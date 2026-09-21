.PHONY: install-manual-recording-metadata-runtime

install-runtime: install-manual-recording-metadata-runtime

install-manual-recording-metadata-runtime:
	$(INSTALL) -d -m 0750 \
		$(DESTDIR)$(CACHEDIR)/recording-metadata/posters
