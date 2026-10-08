.PHONY: check-addon-contract check-addon-deb check-addon-registry stage-addon build-addon-deb

# Optional add-ons are staged individually, never in base make install.
check-addon-contract:
	python3 tools/addons/addon_contract.py check
	python3 -m unittest discover -s tools/addons -p 'test_addon_contract.py'

# Local installed-package inventory: no daemon connection and no writes.
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

# Standalone C++ source implementation, deliberately not part of default install
# or metadata-only add-on package until backend security/activation is complete.
.PHONY: addon-media-import test-addon-media-import
addon-media-import:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -I modules/rectools/include \
		modules/rectools/src/MediaImport.cpp modules/rectools/src/main.cpp \
		-o "$(BUILD_DIR)/addons/vdr-suite-media-import"

test-addon-media-import:
	mkdir -p "$(BUILD_DIR)/addons"
	$(CXX) -std=c++17 -Wall -Wextra -Werror -I modules/rectools/include \
		modules/rectools/src/MediaImport.cpp modules/rectools/tests/test_media_import.cpp \
		-o "$(BUILD_DIR)/addons/test_media_import"
	"$(BUILD_DIR)/addons/test_media_import"

# This package contains the C++ prototype under private libexec. It advertises
# zero Suite capabilities and is neither installed nor started by this target.
.PHONY: build-media-tools-cpp-deb test-addon-media-tools-package
build-media-tools-cpp-deb: addon-media-import test-addon-media-import
	python3 tools/addons/build_media_tools_cpp_deb.py \
		--binary "$(BUILD_DIR)/addons/vdr-suite-media-import" \
		--output-dir "$(OUTPUT_DIR)" --maintainer "$(ADDON_MAINTAINER)"

test-addon-media-tools-package:
	python3 -m unittest discover -s tools/addons -p 'test_media_tools_cpp_deb.py'

test-ci-fast: test-addon-media-import
test-install-staging: test-addon-media-tools-package
