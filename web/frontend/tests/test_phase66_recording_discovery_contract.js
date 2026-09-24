
assert(initialRandomGenre.includes('fetchSeriesRecordingMetadata('));
assert(
  initialRandomGenre.indexOf('renderRecordingRail(') <
    initialRandomGenre.indexOf('fetchSeriesRecordingMetadata('),
  'Random Genre must render Recording data before bounded Metadata artwork enrichment'
);
assert(!initialSeries.includes('waitForSeriesRepresentatives('));
assert(!initialSeries.includes('startSeriesCompletion('));
assert(initialSeries.includes('resolvedSeriesMetadata('));
assert(initialSeries.includes('prefetchSeriesRepresentativeMetadata('));
assert(
  initialSeries.indexOf('applySeriesProjection(') <
    initialSeries.indexOf('prefetchSeriesRepresentativeMetadata('),
  'Series must publish the Recording-derived projection before representative Metadata artwork enrichment'
);
assert(!initialSeries.includes('waitForSeriesRepresentatives('));
assert(!initialSeries.includes('startSeriesCompletion('));
assert(!seriesScan.includes('prefetchSeriesRepresentativeMetadata('));
assert(initialRandomGenre.includes("renderRecordingRail("));
assert(
  /applySeriesProjection\(\s*recordings,\s*backendId\s*\)/.test(initialSeries),
  'Series must publish an initial Recording-derived projection without Rich Metadata'
);

// Below-the-fold discovery is bounded, deferred, and each rail settles independently.
assert(source.includes('new global.IntersectionObserver'));
assert(source.includes("rootMargin: '320px 0px'"));
assert(source.includes('Promise.allSettled(loads)'));
assert(source.includes('loadNewly(client, backendId, generation, {'));
assert(source.includes('loadGenres(client, backendId, generation, {'));
assert(source.includes("loadSeries(client, backendId, generation, [{id: 'series'}]"));
assert(source.includes('loadFolders(client, backendId, generation, {'));
assert(source.includes('parallelHomeResume'));
assert(source.includes('includeSeries: !parallelSeries'));
assert(source.includes('state.seriesAvailable === true'));
assert(source.includes('retainVisible: true'));
assert(source.includes("'Neu aufgenommene Inhalte sind vorübergehend nicht verfügbar.'"));
assert(source.includes("'Genres sind vorübergehend nicht verfügbar.'"));
assert(source.includes("'Aufnahmeordner sind vorübergehend nicht verfügbar.'"));

// The discovery runtime is itself deferred from the established production loader.
assert(bootstrap.includes("loadVdrSuiteDeferredRuntime("));
assert(bootstrap.includes("'/frontend/home-recording-discovery.js'"));
assert(httpPaths.includes('{"/frontend/home-recording-discovery.js", "home-recording-discovery.js"'));
assert(httpPaths.includes('{"/frontend/platform/deferred-runtime-loader.js", "platform/deferred-runtime-loader.js", "application/javascript; charset=utf-8", "home-recording-discovery-bootstrap.js"}'));

// 66.4 stays on its existing composition path and is not repurposed as the 66.5 owner.
assert(httpPaths.includes('{"/frontend/home-live-hero.js", "home-live-hero.js", "application/javascript; charset=utf-8", "home-continue-watching.js"}'));
assert(!continueWatching.includes('VdrSuiteHomeRecordingDiscovery'));

// No new playback/history/recommendation owner is introduced by this follow-up.
assert(!source.includes('MediaSession'));
assert(!source.includes('navigator.mediaSession'));
assert(!source.includes('localStorage'));
assert(!source.includes('/history'));
assert(!source.includes('recommendation'));
assert(!source.includes('ranking'));

// The ordinary frontend and packaging gates include the actual production assets.
assert(makefile.includes('include mk/phase66-recording-discovery.mk'));