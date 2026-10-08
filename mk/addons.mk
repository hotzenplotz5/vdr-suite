.PHONY: check-addon-contract stage-addon

# Optional add-ons are staged individually, never in base make install.
check-addon-contract:
	python3 tools/addons/addon_contract.py check
	python3 -m unittest discover -s tools/addons -p 'test_*.py'

stage-addon:
	python3 tools/addons/addon_contract.py stage --module "$(MODULE)" --destdir "$(DESTDIR)" --prefix "$(PREFIX)"

# Reuse existing documentation and packaging regression CI entrypoints.
test-docs: check-addon-contract
test-install-staging: check-addon-contract
