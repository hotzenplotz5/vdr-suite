'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const sourcePath = path.join(__dirname, '..', 'home-recording-discovery.js');
const source = fs.readFileSync(sourcePath, 'utf8');

function functionBody(name, nextName) {
  const start = source.indexOf('  function ' + name + '(');
  const end = source.indexOf('  function ' + nextName + '(', start + 1);

  assert(start >= 0, name + ' must exist');
  assert(end > start, nextName + ' must follow ' + name);

  return source.slice(start, end);
}

/*
 * Presentation-only Series cover work must never enter the identity /
 * hierarchy projection functions. This is the permanent scope fence for the
 * regression that previously broke season grouping.
 */
[
  functionBody('seriesMemberProjection', 'buildSeriesProjection'),
  functionBody('buildSeriesProjection', 'seriesCountLabel')
].forEach(function (body) {
  assert(
    !/seriesCover|coverOverride|coverOverrides|set-series-cover|clear-series-cover/.test(body),
    'Series cover presentation must not enter Series hierarchy projection'
  );
});

assert(source.includes("'Cover ändern'"));
assert(source.includes("'set-series-cover-tmdb'"));
assert(source.includes("/recordings/metadata/search"));
assert(source.includes("/candidate-image?externalId="));
assert(
  !source.includes('function seriesCoverCandidates('),
  'Series cover picker must never derive candidates from episode artwork'
);
assert(source.includes("'clear-series-cover'"));
assert(source.includes('global.VdrSuiteBrowserSession'));
assert(source.includes('seriesCoverPosterUrl(series, backendId)'));

const context = {
  console,
  Promise,
  Map,
  Set,
  Date,
  JSON,
  Math,
  Number,
  String,
  Array,
  Object,
  encodeURIComponent,
  decodeURIComponent
};

context.window = context;
context.document = null;

context.VdrSuiteFrontendHelpers = {
  homeArtworkPreviewUrl(value) {
    return value;
  },

  isPublicRecordingMetadataImageUrl(value) {
    return String(value || '').startsWith(
      '/api/vdr/recordings/metadata/image?'
    );
  },

  recordingMetadataPosterUrl(value) {
    const images = Array.isArray(value && value.images)
      ? value.images
      : [];

    for (let index = 0; index < images.length; index += 1) {
      const entry = images[index];

      if (
        entry &&
        entry.orientation === 'portrait' &&
        entry.image &&
        entry.image.available === true
      ) {
        return String(entry.image.url || '');
      }
    }

    const preferred = value && value.preferredArtwork;

    return preferred && preferred.available === true
      ? String(preferred.url || '')
      : '';
  }
};

vm.runInNewContext(source, context, {
  filename: 'home-recording-discovery.js'
});

const runtime = context.VdrSuiteHomeRecordingDiscovery;
assert(runtime);
assert(runtime._test);

const api = runtime._test;

assert.strictEqual(typeof api.setSeriesCoverSettingsSnapshot, 'function');
assert.strictEqual(typeof api.seriesCoverPosterUrl, 'function');

function recording(index) {
  const padded = String(index).padStart(2, '0');

  return {
    id: 'recording-' + padded,
    recordingId: 'recording-' + padded,
    backendId: 'default',
    backendNativeId: 'native-' + padded,
    title: 'Serien/Testserie/S01E' + padded + ' Episode ' + index,
    path: 'Serien/Testserie/S01E' + padded + ' Episode ' + index,
    metadata: {
      provider: {},
      presentation: {}
    }
  };
}

function rich(index) {
  const padded = String(index).padStart(2, '0');
  const base =
    '/api/vdr/recordings/metadata/image?' +
    'backend=default&backendNativeId=native-' + padded;

  return {
    available: true,
    mediaType: 'episode',
    provider: 'tvscraper',
    providerId: 42,
    title: 'Testserie',
    seasonNumber: 1,
    episodeNumber: index,
    episodeName: 'Episode ' + index,
    preferredArtwork: {
      available: true,
      url: base + '&kind=preferred&index=0'
    },
    images: [
      {
        orientation: 'landscape',
        image: {
          available: true,
          url: base + '&kind=gallery&index=0'
        }
      },
      {
        orientation: 'portrait',
        image: {
          available: true,
          url: base + '&kind=gallery&index=1'
        }
      },
      {
        orientation: 'portrait',
        image: {
          available: true,
          url: base + '&kind=gallery&index=2'
        }
      }
    ]
  };
}

const recordings = [];
const metadata = new Map();

for (let index = 1; index <= 10; index += 1) {
  const item = recording(index);
  recordings.push(item);
  metadata.set(item.backendNativeId, rich(index));
}

const members = recordings.map(function (item) {
  return api.seriesMemberProjection(
    item,
    metadata.get(item.backendNativeId),
    'default'
  );
});

const projection = api.buildSeriesProjection(members);

assert.strictEqual(projection.length, 1);

const series = projection[0];

function hierarchySnapshot(value) {
  return JSON.parse(JSON.stringify({
    key: value.key,
    title: value.title,
    seasons: value.seasons.map(function (season) {
      return {
        number: season.number,
        label: season.label,
        episodes: season.episodes.map(function (episode) {
          return {
            recordingId: episode.recording.recordingId,
            seriesKey: episode.seriesKey,
            seasonNumber: episode.seasonNumber,
            episodeNumber: episode.episodeNumber,
            episodeTitle: episode.episodeTitle
          };
        })
      };
    })
  }));
}

const hierarchyBefore = hierarchySnapshot(series);
const originalPoster = series.posterUrl;

assert.strictEqual(series.seasons.length, 1);
assert.strictEqual(series.seasons[0].number, 1);
assert.strictEqual(series.seasons[0].episodes.length, 10);

const manualCover =
  '/api/vdr/recordings/metadata/image?' +
  'backend=default&backendNativeId=native-01&kind=gallery&index=9';

api.setSeriesCoverSettingsSnapshot({
  backendId: 'default',
  coverOverrides: [
    {
      seriesKey: series.key,
      posterUrl: manualCover,
      revision: 3
    }
  ]
}, 'default');

assert.strictEqual(
  api.seriesCoverPosterUrl(series, 'default'),
  manualCover
);

/*
 * The underlying projection is immutable from the cover perspective.
 */
assert.strictEqual(series.posterUrl, originalPoster);
assert.deepStrictEqual(
  hierarchySnapshot(series),
  hierarchyBefore
);

const pickerBody = functionBody(
  'renderSeriesCoverPicker',
  'toggleSeriesCoverPicker'
);

assert(
  pickerBody.includes(
    'searchSeriesCoverCandidates'
  ),
  'Cover ändern must use TMDB Series search'
);

assert(
  !pickerBody.includes(
    'resolvedSeriesMetadata'
  ),
  'Cover ändern must not inspect episode metadata for candidates'
);

assert(
  !pickerBody.includes(
    'seriesCoverCandidates'
  ),
  'Cover ändern must not use episode artwork candidates'
);

assert.strictEqual(
  typeof api.seriesCoverCandidateImageUrl,
  'function'
);

const previewUrl =
  api.seriesCoverCandidateImageUrl(
    'default',
    {
      externalId: '1396',
      posterReference: '/series.jpg'
    }
  );

assert(
  previewUrl.includes(
    '/settings/series-artwork/candidate-image?'
  )
);

assert(
  previewUrl.includes(
    'externalId=1396'
  )
);

assert(
  previewUrl.includes(
    'posterReference=%2Fseries.jpg'
  )
);

api.setSeriesCoverSettingsSnapshot({
  backendId: 'house-b',
  coverOverrides: []
}, 'house-b');

assert.strictEqual(
  api.seriesCoverPosterUrl(series, 'default'),
  originalPoster,
  'an override from another backend must never leak into this Series'
);


/*
 * A manual SERIES assignment is presentation metadata.  It may replace title
 * and poster, but it must never erase already-known episode hierarchy from the
 * canonical Recording member.
 */
const manualHierarchyRecordings = [];
const manualHierarchyMembers = [];

for (let index = 1; index <= 10; index += 1) {
  const padded = String(index).padStart(2, '0');

  const automaticMetadata = {
    available: true,
    status: 'ready',
    provider: 'tvscraper',
    mediaType: 'series',
    providerId: -74205,
    title: 'Band of Brothers',
    episodeName: index === 1
      ? 'Currahee'
      : 'Folge ' + index,
    seasonNumber: 1,
    episodeNumber: index
  };

  const recording = {
    recordingId: 'manual-hierarchy-' + padded,
    backendId: 'default',
    backendNativeId: 'manual-hierarchy-native-' + padded,
    path:
      '/srv/vdr/video/Serien/' +
      'Band_Of_Brothers_-_Wir_waren_wie_Brüder/' +
      padded + '_' +
      (index === 1 ? 'Currahee' : 'Folge_' + padded) +
      '/2016-04-' + padded + '.00.00.1-0.rec',
    title:
      'Band_Of_Brothers_-_Wir_waren_wie_Brüder/' +
      padded + '_' +
      (index === 1 ? 'Currahee' : 'Folge_' + padded),
    metadata: {
      provider: {}
    },
    seriesMetadata: automaticMetadata
  };

  manualHierarchyRecordings.push(recording);

  const rich = index === 1
    ? {
        available: true,
        status: 'ready',
        provider: 'manual',
        mediaType: 'series',
        providerId: 0,
        title: 'Band of Brothers',
        episodeName: '',
        seasonNumber: 0,
        episodeNumber: 0,
        preferredArtwork: {
          available: true,
          url:
            '/api/vdr/recordings/metadata/image?' +
            'backend=default&backendNativeId=' +
            encodeURIComponent(recording.backendNativeId) +
            '&kind=preferred&index=0'
        },
        manualAssignment: {
          active: true,
          relationshipLocked: true
        }
      }
    : automaticMetadata;

  manualHierarchyMembers.push(
    api.seriesMemberProjection(
      recording,
      rich,
      'default'
    )
  );
}

const manualHierarchyProjection =
  api.buildSeriesProjection(manualHierarchyMembers);

assert.strictEqual(
  manualHierarchyProjection.length,
  1,
  'manual and automatic Band of Brothers members must remain one Series'
);

const manualHierarchySeries =
  manualHierarchyProjection[0];

assert.deepStrictEqual(
  Array.from(
    manualHierarchySeries.seasons,
    function (season) {
      return season.number;
    }
  ),
  [1],
  'manual Series presentation must preserve embedded episode hierarchy'
);

assert.strictEqual(
  manualHierarchySeries.seasons[0].episodes.length,
  10,
  'manual Series presentation must not move one episode into Staffel unbekannt'
);

assert.strictEqual(
  manualHierarchySeries.seasons[0].episodes[0].seasonNumber,
  1,
  'manual Series presentation must retain Currahee season 1'
);

assert.strictEqual(
  manualHierarchySeries.seasons[0].episodes[0].episodeNumber,
  1,
  'manual Series presentation must retain Currahee episode 1'
);

assert.strictEqual(
  manualHierarchySeries.seasons[0].episodes[0].episodeTitle,
  'Currahee',
  'manual Series presentation must retain the automatic episode title'
);

assert(
  manualHierarchySeries.seasons[0].episodes[0].posterUrl.includes(
    'kind=preferred&index=0'
  ),
  'manual Series presentation poster must still win visually'
);

console.log(
  'manual Series presentation preserves canonical episode hierarchy ok'
);

assert(
  !functionBody(
    'updateSeriesCoverOverride',
    'renderSeriesCoverPicker'
  ).includes('/recordings/metadata/assign'),
  'Series cover selection must not call Recording assignment'
);



const tmdbSeriesCover =
  '/api/backends/default/settings/series-artwork/image' +
  '?seriesKey=' +
  encodeURIComponent('folder:series/battlestar-galactica') +
  '&revision=7';

const tmdbSeries = {
  key: 'folder:series/battlestar-galactica',
  posterUrl: '/automatic-series-cover.jpg'
};

api.setSeriesCoverSettingsSnapshot(
  {
    backendId: 'default',
    coverOverrides: [
      {
        seriesKey: tmdbSeries.key,
        posterUrl: tmdbSeriesCover,
        revision: 7
      }
    ]
  },
  'default'
);

assert.strictEqual(
  api.seriesCoverPosterUrl(
    tmdbSeries,
    'default'
  ),
  tmdbSeriesCover,
  'TMDB persisted Series cover image URL must survive frontend allowlist'
);

api.setSeriesCoverSettingsSnapshot(
  {
    backendId: 'default',
    coverOverrides: [
      {
        seriesKey: tmdbSeries.key,
        posterUrl:
          'https://example.invalid/evil.jpg',
        revision: 8
      }
    ]
  },
  'default'
);

assert.strictEqual(
  api.seriesCoverPosterUrl(
    tmdbSeries,
    'default'
  ),
  tmdbSeries.posterUrl,
  'external Series cover URLs must remain rejected'
);

console.log(
  'TMDB persisted Series cover image URL allowlist ok'
);

console.log('home Series cover override and hierarchy invariance ok');
