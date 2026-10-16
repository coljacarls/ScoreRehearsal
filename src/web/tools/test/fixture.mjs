// SPDX-License-Identifier: GPL-3.0-only
// ScoreRehearsal, Copyright (C) 2026 Colja Carls
//
// A fake build of the WebAssembly app, so the page, the loader and the service worker can be tested
// without Qt and Emscripten. The real qtloader.js of MuseScore's web build is used when it is checked out.

import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(here, "..", "..", "..", "..");
const upstreamDistr = path.join(repoRoot, "musescore", "src", "web", "appjs", "distr");

export function hasUpstreamLoader() {
    return fs.existsSync(path.join(upstreamDistr, "qtloader.js"));
}

// The page script of the stub stands in for ScoreRehearsal.js: it behaves like the Emscripten module that
// the app calls (preRun hooks, onStartApp from C++, onRuntimeInitialized) and keeps its state in window.
const STUB_APP = (marker) => `window.ScoreRehearsal_entry = async function (config) {
    const files = new Map();
    const calls = { mount: [], syncfs: [], persist: 0, startAudio: 0 };
    const instance = config;
    instance.marker = "${marker}";
    window.__instance = instance;
    window.__calls = calls;
    window.__files = files;
    window.__states = [];
    window.__presentationClosed = 0;

    instance.FS = {
        mkdirTree() {},
        mount(fs, opts, dir) { calls.mount.push(dir); },
        syncfs(populate, cb) { calls.syncfs.push(populate); setTimeout(() => cb(null), 5); },
        writeFile(p, data) { files.set(p, typeof data === "string" ? new TextEncoder().encode(data) : data); },
        readFile(p) { if (!files.has(p)) throw new Error("ENOENT " + p); return files.get(p); },
    };
    instance.IDBFS = {};
    instance._startAudioProcessing = () => { calls.startAudio++; };
    instance.ccall = () => {};
    instance.callMain = () => {};
    instance.onPresentationState = (json) => window.__states.push(json);
    instance.onPresentationClosed = () => { window.__presentationClosed++; };

    for (const hook of config.preRun) hook(instance);
    await instance.onStartApp();
    config.onRuntimeInitialized();
    return instance;
};
`;

const STUB_DRIVER = `const api = {
    inited: false,
    onInited: null,
    async setup() { setTimeout(() => { api.inited = true; api.onInited?.(); }, 10); },
    outputSpec: () => ({ sampleRate: 44100, samplesPerChannel: 128, audioChannelCount: 2 }),
    open() {}, resume() {}, suspend() {}, close() {},
};
export default api;
`;

// Writes the fake build and returns {build, audio, sound}. "marker" makes two builds differ.
export function makeFakeBuild(marker = "one") {
    const root = fs.mkdtempSync(path.join(os.tmpdir(), "scorerehearsal-fixture-"));
    const build = path.join(root, "app");
    const audio = path.join(root, "audio");
    fs.mkdirSync(path.join(build, "distr"), { recursive: true });
    fs.mkdirSync(audio, { recursive: true });

    fs.writeFileSync(path.join(build, "ScoreRehearsal.js"), STUB_APP(marker));
    fs.writeFileSync(path.join(build, "ScoreRehearsal.wasm"), Buffer.from([0, 0x61, 0x73, 0x6d, 1, 0, 0, 0]));
    fs.writeFileSync(path.join(build, "distr", "config.js"), "export default { MUSE_MODULE_AUDIO_WORKER: 'OFF' };\n");
    fs.writeFileSync(path.join(build, "distr", "audiodriver.js"), STUB_DRIVER);
    fs.writeFileSync(path.join(build, "distr", "audio_worklet_processor.js"), "// stub\n");

    if (hasUpstreamLoader()) {
        fs.copyFileSync(path.join(upstreamDistr, "qtloader.js"), path.join(build, "distr", "qtloader.js"));
    } else {
        throw new Error("musescore/src/web/appjs/distr/qtloader.js is needed: git submodule update --init");
    }

    fs.writeFileSync(path.join(audio, "MuseAudio.js"), "// stub\n");
    fs.writeFileSync(path.join(audio, "MuseAudio.wasm"), Buffer.from([0, 0x61, 0x73, 0x6d, 1, 0, 0, 0]));

    const sound = path.join(root, "MS Basic.sf3");
    fs.writeFileSync(sound, "sound font " + "x".repeat(1000));

    return { root, build, audio, sound };
}
