# AGENTS.md — ScoreRehearsal

Read-only rehearsal player for MuseScore files, built on MuseScore Studio
components. GPL-3.0-only. No MuseScore name/logo in the app.

## Layout
- `src/appshell` app shell (C++/QML); `src/cmake/Configure.cmake` module switches
- `musescore/` git submodule = fork coljacarls/MuseScore, branch `rehearsal-app`
  (core rehearsal patches + generic `app-external` hooks); `musescore/muse/` = fork
  coljacarls/muse_framework

## Rules
- Never push to musescore/* (remotes `upstream` are read-only). Push only to the
  coljacarls forks and this repo.
- App-specific code belongs here, not in the fork. Keep fork hooks tiny/generic.
- The app never writes scores: no save, autosave, migration or save prompts.
- Build: `build.cmd install Release`; run `build\Release\install\bin\ScoreRehearsal.exe`.
- Update MuseScore: merge upstream into the fork branch, bump the submodule,
  re-check `src/cmake/Configure.cmake`.
- Talk to the maintainer in German; code and docs in English.
