'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const frontendRoot = path.join(__dirname, '..');
const repositoryRoot = path.join(frontendRoot, '..', '..');
const fastOwner = fs.readFileSync(path.join(frontendRoot, 'api', 'session-frontend-sync.js'), 'utf8');
const fallbackOwner = fs.readFileSync(path.join(frontendRoot, 'api', 'recording-fallback-controls.js'), 'utf8');
const continueSync = fs.readFileSync(path.join(frontendRoot, 'api', 'continue-watching-sync.js'), 'utf8');
const playbackResponse = fs.readFileSync(path.join(repositoryRoot, 'api', 'rest', 'src', 'MediaPlaybackContractResponse.cpp'), 'utf8');

// Continue Watching must consume the canonical Recording owner's resume truth.
assert(continueSync.includes("return typeof owner.canResume === 'function' && owner.canResume() === true;"));

// The server already exposes absolute Recording restart/resume capability through
// mediaSession.playback.resume; the fast owner must consume that same truth rather
// than deriving a second capability from seek controls or browser behavior.
assert(playbackResponse.includes('const auto restart = objectField(response.body, "resume", playback->begin, playback->end);'));
assert(fastOwner.includes('const resume = playback && playback.resume;'));
assert(fastOwner.includes('resumeSupported = Boolean(resume && resume.supported === true && durationSeconds > 0);'));
assert(fastOwner.includes('canResume: function () {'));
assert(fastOwner.includes("return (stopped || destroyed) ? stoppedResumeSupported : resumeSupported;"));
assert(fastOwner.includes('if (fallbackPanel) return false;'));

// The existing HLS owner remains its own canonical transport owner and already
// publishes the same fail-closed resume contract.
assert(fallbackOwner.includes('const resume = playback && playback.resume;'));
assert(fallbackOwner.includes('canResume: function () { return stopped ? stoppedResumeSupported : resumeSupported; }'));

// Stop must capture the active absolute position before the owner changes to the
// stopped state; otherwise Continue Watching observes only the original seek base
// during the lifecycle flush (zero for an ordinary start-from-beginning playback).
const stopStart = fastOwner.indexOf('function stopPlayback()');
assert(stopStart >= 0);
const capture = fastOwner.indexOf('stoppedPositionSeconds = Math.max(0, Math.floor(positionSeconds()));', stopStart);
const stoppedTransition = fastOwner.indexOf('stopped = true;', stopStart);
assert(capture >= 0 && stoppedTransition >= 0 && capture < stoppedTransition);
assert(fastOwner.includes('if (stopped || destroyed) return stoppedPositionSeconds;'));
assert(fastOwner.includes('stoppedResumeSupported = Boolean(resumeSupported && stoppedPositionSeconds > 0);'));

// Natural completion must never retain resumable truth.
const endedStart = fastOwner.indexOf("video.addEventListener('ended'");
assert(endedStart >= 0);
const completionClear = fastOwner.indexOf('stoppedResumeSupported = false;', endedStart);
const completionStopped = fastOwner.indexOf('stopped = true;', endedStart);
assert(completionClear >= 0 && completionStopped >= 0 && completionClear < completionStopped);

console.log('phase66 fast recording Continue Watching owner contract ok');
