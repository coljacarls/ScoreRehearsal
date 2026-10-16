// SPDX-License-Identifier: GPL-3.0-only
// ScoreRehearsal, Copyright (C) 2026 Colja Carls
//
// Linked with --pre-js into the app module (see src/cmake/App.cmake). It runs inside the module scope,
// where the Emscripten file system is visible. The page script (app.js) needs it for the persistent
// storage, and its own preRun hook must see it, so this hook is placed first.
Module["preRun"] = Module["preRun"] || [];
Module["preRun"].unshift(function () {
    Module["FS"] = FS;
    if (typeof IDBFS !== "undefined") {
        Module["IDBFS"] = IDBFS;
    }
    // Needed to delay the start of the app until the persistent storage is loaded
    if (typeof addRunDependency === "function" && !Module["addRunDependency"]) {
        Module["addRunDependency"] = addRunDependency;
        Module["removeRunDependency"] = removeRunDependency;
    }
});
