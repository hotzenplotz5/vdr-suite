.PHONY: test-phase65d-continuous-fmp4-mse-backpressure

test-phase65d-continuous-fmp4-mse-backpressure:
	node --check web/frontend/api/session-frontend-sync.js
	node web/frontend/tests/test_phase65d_continuous_fmp4_mse_backpressure.js

# Keep the real browser transport regression in the normal frontend CI path.
test-frontend-i18n: test-phase65d-continuous-fmp4-mse-backpressure
