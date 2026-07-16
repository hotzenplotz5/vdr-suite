.PHONY: \
	test-ci-fast \
	test-ci-frontend \
	test-ci-packaging \
	test-vdr \
	test-all \
	test-manual-real \
	test-make-test-manifest \
	test-make-test-manifest-strict

# Public test entrypoints live in this file only. Individual test recipes remain
# close to their domain until the follow-up Makefile split is complete.

test-ci-fast: \
	check-vdr-linkage-contracts \
	test-make-test-manifest \
	test-fast \
	test-api-router \
	test-restful-api-vdr-timer-action-executor \
	test-search-timer-preview-result-json-serializer \
	test-search-timer-preview-service \
	test-search-timer-preview-epg-cache \
	test-vdr-snapshot-read-service-searchtimer-preview-epg-cache \
	test-api-router-searchtimer-preview-epg-cache \
	test-runtime-config \
	test-epg-event-repository \
	test-epg-cache-service \
	test-epg-cache-controller \
	test-api-router-epg-cache-routes \
	test-vdr-snapshot-builder-startup-snapshot \
	test-search-timer-preview-epg-cache-refresh-service \
	test-search-timer-preview-epg-cache-refresh-controller \
	test-search-timer-preview-epg-cache-stale-guard \
	test-search-timer-preview-epg-cache-change-invalidator \
	test-snapshot-change-feed-preview-epg-cache-invalidation \
	test-api-router-searchtimer-preview-epg-cache-refresh-route \
	test-api-router-searchtimer-preview-refresh-then-preview \
	test-daemon-runtime-shutdown-resets \
	test-http-listener-bind-failure-handling \
	test-http-listener-partial-request-timeout \
	test-real-vdr-acceptance-manifest \
	test-phase-map-coverage \
	test-github-update-safety-handoff \
	test-recording-mutation-safety-policy \
	test-restfulapi-recording-trash-contract \
	test-systemd-unit-contract


test-ci-frontend: test-frontend-contracts


test-ci-packaging: \
	test-systemd-unit-contract \
	test-install-staging


test-vdr: \
	test-backend-node \
	test-backend-registry \
	test-backend-registry-service \
	test-backend-registry-json-serializer \
	test-vdr-config \
	test-external-vdr-adapter \
	test-vdr-adapter-factory \
	test-vdr-service \
	test-vdr-overview-service \
	test-vdr-overview-json-serializer \
	test-vdr-snapshot-builder \
	test-polling-service \
	test-vdr-change-state \
	test-vdr-change-event \
	test-change-detection-service \
	test-snapshot-refresh-decision-service \
	test-snapshot-refresh-planner \
	test-snapshot-update-plan \
	test-snapshot-cache \
	test-snapshot-cache-service \
	test-snapshot-change-feed \
	test-snapshot-change-feed-json-serializer \
	test-snapshot-access-service \
	test-mock-vdr-adapter \
	test-json-string-decoder \
	test-restful-api-status-mapper \
	test-restful-api-event-mapper \
	test-restful-api-channel-mapper \
	test-restful-api-recording-mapper \
	test-vdr-recording-cache-repository \
	test-vdr-recording-query-service-cache \
	test-vdr-recording-folder-controller \
	test-restful-api-timer-mapper \
	test-vdr-timer-operation-request \
	test-vdr-timer-action-result \
	test-vdr-timer-action-result-json-serializer \
	test-vdr-timer-action-executor-interface \
	test-mock-vdr-timer-action-executor \
	test-vdr-timer-action-service \
	test-restful-api-vdr-timer-action-request-builder \
	test-restful-api-vdr-timer-action-executor \
	test-vdr-timer-action-request-parser \
	test-vdr-timer-action-controller \
	test-vdr-timer-action-executor-adapter-registry \
	test-restful-api-vdr-timer-action-executor-adapter \
	test-vdr-timer-action-execution-service \
	test-restful-api-vdr-adapter \
	test-restful-api-change-state-adapter \
	test-person-query-matcher \
	test-person-query-result-json-serializer \
	test-person-search-service \
	test-recording-person-search-result \
	test-recording-person-search-service \
	test-recording-person-search-result-json-serializer \
	test-recording-person-search-controller \
	test-person-query-controller \
	test-genre-classification \
	test-genre-resolver \
	test-genre-resolution-json-serializer \
	test-canonical-genre-registry \
	test-genre-localization \
	test-epg-search-request \
	test-epg-search-matcher \
	test-epg-search-result \
	test-search-timer \
	test-search-timer-preview-epg-cache \
	test-vdr-snapshot-read-service-searchtimer-preview-epg-cache \
	test-search-timer-discovery \
	test-search-timer-discovery-service \
	test-search-timer-discovery-static-provider \
	test-restfulapi-search-timer-discovery-provider-contract \
	test-searchtimer-discovery-runtime-wiring \
	test-daemon-runtime-shutdown-resets \
	test-http-listener-bind-failure-handling \
	test-http-listener-partial-request-timeout \
	test-real-vdr-acceptance-manifest \
	test-github-update-safety-handoff \
	test-recording-mutation-safety-policy \
	test-search-timer-automation-evaluation-plan \
	test-search-timer-automation-match-candidate \
	test-search-timer-automation-duplicate-detection \
	test-search-timer-automation-candidate-timer-proposal \
	test-search-timer-automation-dry-run-result-json-serializer \
	test-search-timer-automation-read-only-service \
	test-search-timer-automation-preview-controller \
	test-search-timer-automation-daemon-scheduling-plan \
	test-search-timer-automation-safety-review \
	test-restful-api-search-timer-command-executor \
	test-search-timer-runtime-mutation-policy-executor \
	test-search-timer-create-request-parser \
	test-search-timer-discovery-json-serializer \
	test-search-timer-discovery-controller \
	test-search-timer-workflow-execution-result-json-serializer \
	test-search-timer-workflow-backend-readback-verification-result \
	test-search-timer-workflow-create-readback-verification-service \
	test-search-timer-workflow-update-readback-verification-service \
	test-search-timer-workflow-delete-absence-verification-service \
	test-search-timer-workflow-command-dispatch-service \
	test-search-timer-workflow-end-to-end-verified-execution \
	test-search-timer-workflow-real-execution-policy \
	test-search-timer-workflow-real-execution-enablement-switch \
	test-search-timer-workflow-backend-write-allowlist \
	test-search-timer-workflow-backend-write-permission-gate \
	test-search-timer-workflow-production-policy-gate \
	test-search-timer-workflow-production-executor-hardening-plan \
	test-search-timer-workflow-production-executor-hardening-plan-json-serializer \
	test-search-timer-workflow-real-execution-readiness-review \
	test-search-timer-workflow-real-execution-readiness-review-json-serializer \
	test-search-timer-workflow-guarded-executor-invocation \
	test-search-timer-workflow-executor-invocation-kill-switch \
	test-search-timer-workflow-controlled-test-executor-invocation \
	test-search-timer-workflow-controlled-invocation-audit-trail \
	test-search-timer-workflow-executor-result-mapper \
	test-search-timer-workflow-command-request-mapper \
	test-search-timer-workflow-execution-service \
	test-search-timer-workflow-execution-plan-json-serializer \
	test-search-timer-workflow-planning-service \
	test-search-timer-workflow-execution-plan \
	test-search-timer-workflow-validation-request-parser \
	test-search-timer-workflow-validation-result-json-serializer \
	test-search-timer-workflow-validation-service \
	test-search-timer-workflow-request \
	test-search-timer-query \
	test-search-timer-result \
	test-search-timer-service-interface \
	test-epg-person-search-result \
	test-epg-search-result-json-serializer \
	test-epg-search-service \
	test-vdr-domain-objects


test-all: \
	test \
	test-vdr \
	test-ci-frontend \
	test-ci-packaging \
	test-restfulapi-recording-trash-contract \
	test-make-test-manifest


# These targets require an explicitly configured real VDR and are never part of CI.
test-manual-real: \
	real-vdr-regression \
	test-real-restfulapi-integration \
	test-real-snapshot-builder \
	test-real-change-state \
	test-real-polling-initial-snapshot \
	test-real-polling-stability


test-make-test-manifest:
	python3 tools/check_make_test_manifest.py


test-make-test-manifest-strict:
	python3 tools/check_make_test_manifest.py --strict
