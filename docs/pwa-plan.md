# Plan: ScoreRehearsal Web (PWA)

Status: implemented, **not yet built or run with Qt for WebAssembly** (the environment it was written in
could not download Qt/Emscripten). See "Status" at the end.

## Decisions

- **Architecture (option A):** the whole app (C++/QML) is built with Qt for WebAssembly (Emscripten)
  and wrapped as a Progressive Web App. Desktop and web share one code base; web-specific code is
  guarded by `Q_OS_WASM`. The Windows build stays unchanged.
- **Target devices:** iPad (Safari), Android tablets (Chrome), Microsoft Surface (Edge/Chrome).
- **Hosting:** v1 on GitHub Pages (served under `/ScoreRehearsal/`), later on a self-hosted VPS.
  GitHub Pages cannot set custom HTTP headers (no COOP/COEP), so v1 is **single-threaded**
  (`wasm_singlethread`, as upstream does).
- **Score storage:** copies of opened scores may be kept in browser storage (OPFS) for a
  "recent scores" library. The original file is never written. The rule "the app never writes
  scores" in `AGENTS.md` is clarified accordingly when the web build lands.
- **Presentation window:** part of v1 (see below).

## Starting point upstream

The fork branch `rehearsal-app` already contains MuseScore's experimental web build:

- build configuration `APP-WEB` in `musescore/SetupConfigure.cmake` (threads off on WASM,
  `MUSE_MODULE_AUDIO_WORKMODE 2` = driver callback)
- `src/web/appshell`, `src/web/appjs`, `src/web/audioengine`: the audio engine is built separately
  (`MuseAudio.js`) and runs inside an **AudioWorklet** (`audiodriver.js`,
  `audio_worklet_processor.js`); a small JS API (`muapi.js`: `loadScoreFile`, `loadScoreData`,
  `startAudioProcessing`); MS Basic as default sound font
- CI workflow `.github/workflows/build_wasm.yml` (Qt 6.11.2, `wasm_singlethread`), manual trigger
  only, so it should be considered experimental

## App changes

1. **Opening files and session keys.** Sessions are keyed by file path today (see
   `rehearsalsessionstore.*`, "Locate score file" in `rehearsalpagemodel.cpp`). Browsers have no
   paths: on the web, sessions are keyed by a content hash. Opened scores are copied to OPFS and
   listed as recent scores. Chromium can additionally keep File System Access handles. Desktop
   sessions stay path-based.
2. **Persistent `sessions.json`.** `QStandardPaths::AppDataLocation` is in-memory on WASM by
   default: persist it (IDBFS/OPFS) and request `navigator.storage.persist()`. Add session
   export/import as a safety net against cleared site data.
3. **Audio.** An `AudioContext` needs a user gesture: add a "tap to start" step. Check tap-to-hear
   features (pitch pipe, chord scrubber, `offstreamplayback.h`) through the worklet RPC for latency.
4. **Single thread.** Score loading and layout block the UI; measure with large scores.
5. **Full screen and screen lock.** Fullscreen API (needs a gesture), `display: fullscreen` in the
   manifest, Wake Lock API during rehearsal.
6. **Startup file.** Replaces the command line argument: URL parameter, `launchQueue` with manifest
   `file_handlers` (Chromium desktop), share target (Android).

## Presentation window (v1)

Qt for WebAssembly cannot open a second native window. The web version opens a **second browser
tab** running the app in presentation mode (`?mode=presentation`):

- The presentation instance reads the score from OPFS (same origin), so no score data is sent.
- It starts without the audio engine and AudioWorklet to save memory.
- The rehearsal instance sends its state via `BroadcastChannel`: playback position,
  `soloPartIds`, laser highlight (`highlightRect`), `shadeBands` and presentation settings, i.e.
  the state `presentationpaintview.h` uses today. On desktop the in-process window stays as is;
  on WASM a small sync layer sits in between.
- Works without special hosting, so GitHub Pages is fine.
- Device limits: Surface works well (drag the tab to the projector). iPad: a separate Safari window
  on an external display needs Stage Manager (M1 iPads and newer), otherwise mirroring only.
  Android: depends on device and version, mostly mirroring.
- Later (VPS): presentation on a second device through a small WebSocket relay.

## PWA layer

- `manifest.webmanifest`: name, icons from `src/branding`, `display: fullscreen`, landscape,
  `file_handlers` for `.mscz` / `.mscx`.
- Service worker: precaches `.wasm`, JS, fonts and the sound font with a cache version per
  release, so the app works offline after the first load. The sound font has its own cache and
  survives app updates; the `.wasm` file is downloaded again on every app update. Updates install
  in the background and are only activated after the user confirms (never mid-rehearsal).
- Relative paths everywhere so moving from `/ScoreRehearsal/` on GitHub Pages to the VPS only
  needs a new deployment.
- Show the app shell first and load the sound font with a progress bar.
- Note for iPad users: install the app to the home screen; Safari deletes data of plain websites
  after about 7 days without use.

## Build and integration

- `src/cmake/Configure.cmake`: add an `if(OS_IS_WASM)` block that mirrors the web flags of the
  `APP-WEB` configuration. `APP-WEB` and `APP-EXTERNAL` are mutually exclusive today; if
  `src/web/audioengine` depends on `MUE_CONFIGURATION_IS_APPWEB`, add one tiny, generic hook to
  the fork.
- App-specific web entry (HTML page, loader, branding without MuseScore names) lives in this repo
  under `src/web/`, not in the fork.
- Local build script `build-web.sh`.
- CI: Linux job with Qt 6.11.2 `wasm_singlethread` and the emsdk version Qt requires; deploy with
  `actions/deploy-pages` (no `gh-pages` branch, so large binaries stay out of git history).
- Size: `-Os`/`-Oz`, no debug info in releases, only needed Qt modules. Brotli on the VPS.
- GPL: hosting the app distributes it; the About dialog links to the source (this repo and the fork).
- Upstream merge checklist gets an extra step: re-check the `APP-WEB` block and build the web target.

## Phases

0. **Spike (go/no-go):** build the upstream `APP-WEB` viewer from the fork, load and play an
   `.mscz` on all three device classes. Measure transfer size, stored size, load time, time to
   first playback, memory (also with two tabs on iPad), audio dropouts and tap-to-hear latency.
1. **Build integration** (Configure.cmake, web entry, `build-web.sh`).
2. **Platform changes** (app changes 1–6).
3. **PWA layer** (manifest, service worker, offline, install).
4. **Presentation window** (presentation mode, BroadcastChannel sync, open via `window.open`).
5. **CI and release** (GitHub Pages deployment, About link).
6. **VPS** (COOP/COEP and multi-threading measurement, Brotli, optional second-device presentation).

## Risks

- The upstream web build is experimental and may change with every upstream merge.
- Single-thread performance with large scores.
- Memory limits on iPad, especially with two instances.
- Size of the first download.

## Status

What exists, and how far it was checked:

| Part | Where | Checked |
|---|---|---|
| Build switches for WASM | `src/cmake/Configure.cmake`, `App.cmake`, fork hook `MUE_BUILD_APPJS_MODULE` | not built |
| Page, loader, storage, wake lock, full screen | `src/web/*.js`, `index.html` | tested in Chromium against a fake build |
| Service worker, offline start, update flow | `src/web/sw.js`, `tools/package.mjs` | tested in Chromium (offline reload, update, sound font cache kept) |
| Presentation tab sync | `presentation.js` | tested in Chromium (score hand-over, state, close) |
| C++ (`WebBridge`, session library, remote playback of the paint view) | `src/appshell/qml/MuseScore/AppShell` | **not compiled** |
| QML (`PresentationView/Page`, publisher in `RehearsalPage.qml`) | same | **not run** |
| CI / GitHub Pages | `.github/workflows/web.yml` | not run; first run will need adjustments |
| Phase 0 measurements (size, load time, memory, latency) | | **not done**: needs a real build and the devices |

Differences to the plan above:

- The persistent storage is Emscripten's **IDBFS** (IndexedDB), not OPFS: Qt's WebAssembly build has no
  WASMFS. The result for the user is the same.
- The score reaches the presentation tab as bytes through the `BroadcastChannel` (not through the
  library): no stale copy, no waiting for a write to finish, and `window.open` stays inside the click.
  A score change closes the presentation tab (it shows the score it was opened for).
- The manifest says `"orientation": "any"` (a music stand holds a tablet in portrait, too).
- `MS Basic.sf3` is about 51 MB. It is downloaded once and has a cache of its own, so app updates do
  not load it again. Consider a smaller sound font before the first public release.
- Sessions are stored by the content hash of the score (library path), so renaming a file keeps the session
  and two different files with the same name do not share one.

Open points: the first real build (Qt/Emscripten flags, `FS`/`IDBFS` export via `pre.js`, embind
bindings, memory), measuring Phase 0 on iPad/Android/Surface, whether `QSettings` reaches
`/files/data` (the page writes the folder back every 20 s as a safety net), and the session
import/export in the menu.
