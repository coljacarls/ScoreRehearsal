# AGENTS.md — ScoreRehearsal

Read-only rehearsal player for MuseScore files, built on MuseScore Studio
components. GPL-3.0-only. No MuseScore name/logo in the app.

## Layout
- `src/appshell` app shell (C++/QML); `src/cmake/Configure.cmake` module switches
- `src/web` web version (PWA: page, loader, service worker, tools, tests); `build-web.sh`; plan in
  `docs/pwa-plan.md`; web specific C++ is guarded by `Q_OS_WASM` (see `webbridge.*`)
- `musescore/` git submodule = fork coljacarls/MuseScore, branch `rehearsal-app`
  (core rehearsal patches + generic `app-external` hooks); `musescore/muse/` = fork
  coljacarls/muse_framework

## Rules
- Never push to musescore/* (remotes `upstream` are read-only). Push only to the
  coljacarls forks and this repo.
- App-specific code belongs here, not in the fork. Keep fork hooks tiny/generic.
- Sessions (per score file: faders, fader groups, view mode, playhead measure) are stored per user in
  `sessions.json` (app data folder); see `rehearsalsessionstore.*`.
- The app never writes scores: no save, autosave, migration or save prompts. (The web version keeps
  a copy of opened scores in its browser library; the original file is never written.)
- Build: `build.cmd install Release`; run `build\Release\install\bin\ScoreRehearsal.exe`.
- Web: `./build-web.sh all` (needs Qt for WebAssembly + Emscripten, see docs/DEVELOPMENT.md);
  JS tests without Qt: `cd src/web && npm test`.
- Update MuseScore: merge upstream into the fork branch, bump the submodule,
  re-check `src/cmake/Configure.cmake`.
- Code, comments and docs in English.
