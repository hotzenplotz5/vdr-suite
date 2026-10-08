.PHONY: check-addon-contract check-addon-deb check-addon-registry stage-addon build-addon-deb

# Optional add-ons are staged individually, never in base make install.
check-addon-contract:
	python3 tools/addons/addon_contract.py check
	python3 -m unittest discover -s tools/addons -p 'test_addon_contract.py'

check-addon-registry:
	python3 -m unittest discover -s tools/addons -p 'test_addon_registry.py'

check-addon-deb:
	python3 -m unittest discover -s tools/addons -p 'test_addon_deb.py'

stage-addon:
	python3 tools/addons/addon_contract.py stage --module "$(MODULE)" --destdir "$(DESTDIR)" --prefix "$(PREFIX)"

# Build a standalone, explicitly nonfunctional Debian metadata scaffold package.
# No install, dependency changes or daemon access; caller owns output directory.
build-addon-deb:
	python3 tools/addons/build_deb.py --module "$(MODULE)" --output-dir "$(OUTPUT_DIR)" --maintainer "$(ADDON_MAINTAINER)"

# Reuse existing documentation and packaging regression CI entrypoints.
test-docs: check-addon-contract check-addon-registry
test-install-staging: check-addon-contract check-addon-deb
