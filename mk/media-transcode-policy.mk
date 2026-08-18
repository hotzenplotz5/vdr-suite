.PHONY: install-media-transcode-calibrator test-phase65-media-transcode-calibrator-install

# Keep the calibrator available with install-runtime as an operator tool. The
# daemon itself never benchmarks during playback or startup.
install-runtime: install-media-transcode-calibrator

install-media-transcode-calibrator:
	$(INSTALL) -d $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 tools/vdr_suite_media_calibrate.py \
		$(DESTDIR)$(BINDIR)/vdr-suite-media-calibrate

test-phase65-media-transcode-calibrator-install:
	rm -rf /tmp/vdr-suite-media-calibrator-install
	$(MAKE) install-media-transcode-calibrator \
		DESTDIR=/tmp/vdr-suite-media-calibrator-install PREFIX=/usr
	test -x /tmp/vdr-suite-media-calibrator-install/usr/bin/vdr-suite-media-calibrate
	/tmp/vdr-suite-media-calibrator-install/usr/bin/vdr-suite-media-calibrate \
		--help >/dev/null
	rm -rf /tmp/vdr-suite-media-calibrator-install
