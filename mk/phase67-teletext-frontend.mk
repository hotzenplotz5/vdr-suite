.PHONY: test-phase67-teletext-frontend

test-phase67-teletext-frontend:
	node --check web/frontend/teletext-view.js
	node --check web/frontend/live-tv-view.js
	node --check web/frontend/api/client-api.js
	node web/frontend/tests/test_phase67_teletext_view.js
	python3 tools/check_phase67_teletext_frontend.py
