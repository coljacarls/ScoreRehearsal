// SPDX-License-Identifier: GPL-3.0-only
// ScoreRehearsal, Copyright (C) 2026 Colja Carls
//
// Starts the app in the browser: loads the WebAssembly module, mounts the persistent storage, starts the
// audio (an AudioWorklet that runs the sound engine) and connects the app to the page (file dialog, full
// screen, presentation tab). Based on the viewer of MuseScore's web build (musescore/src/web/appjs).

import qtLoad from "./distr/qtloader.js";
import AudioDriver from "./distr/audiodriver.js";
import config from "./distr/config.js";

import { mountStorage, createStorage } from "./storage.js";
import { createWakeLock, toggleFullscreen, isFullscreen } from "./device.js";
import {
    isPresentationMode,
    installRehearsalSide,
    installPresentationSide,
    requestScore,
} from "./presentation.js";

const presentation = isPresentationMode();
const $ = (id) => document.getElementById(id);

const ui = {
    splash: $("splash"),
    status: $("status"),
    progress: $("progress"),
    start: $("start"),
    screen: $("screen"),
    error: $("error"),
    errorText: $("error-text"),
    update: $("update"),
};

function setStatus(text, fraction) {
    ui.status.textContent = text;
    if (fraction === undefined) {
        ui.progress.removeAttribute("value"); // indeterminate
    } else {
        ui.progress.value = Math.min(1, Math.max(0, fraction));
    }
}

function showError(text) {
    ui.splash.hidden = true;
    ui.screen.hidden = true;
    ui.errorText.textContent = text;
    ui.error.hidden = false;
}

// ---------------------------------------------------------------------------------------------------------------------
// Service worker: offline use and updates
// ---------------------------------------------------------------------------------------------------------------------

function registerServiceWorker() {
    if (!("serviceWorker" in navigator)) {
        return;
    }

    navigator.serviceWorker.register("sw.js").then((registration) => {
        const offer = (worker) => {
            // An update is never activated by itself: that would reload the page in the middle of a rehearsal
            ui.update.hidden = false;
            $("update-button").onclick = () => worker.postMessage({ type: "SKIP_WAITING" });
        };

        if (registration.waiting && navigator.serviceWorker.controller) {
            offer(registration.waiting);
        }
        registration.addEventListener("updatefound", () => {
            const worker = registration.installing;
            worker?.addEventListener("statechange", () => {
                if (worker.state === "installed" && navigator.serviceWorker.controller) {
                    offer(worker);
                }
            });
        });
    }).catch((e) => console.warn("[sw] registration failed", e));

    let reloading = false;
    navigator.serviceWorker.addEventListener("controllerchange", () => {
        if (!reloading && !ui.update.hidden) {
            reloading = true;
            location.reload();
        }
    });
}

// ---------------------------------------------------------------------------------------------------------------------
// Build information (written by tools/package.mjs)
// ---------------------------------------------------------------------------------------------------------------------

async function loadBuildInfo() {
    try {
        const response = await fetch("build-info.json");
        if (response.ok) {
            return await response.json();
        }
    } catch (e) {
        // fall through
    }
    return { version: "dev", script: "ScoreRehearsal.js", wasm: "ScoreRehearsal.wasm", entry: "ScoreRehearsal_entry" };
}

function loadScript(src) {
    return new Promise((resolve, reject) => {
        const script = document.createElement("script");
        script.src = src;
        script.onload = resolve;
        script.onerror = () => reject(new Error("cannot load " + src));
        document.head.appendChild(script);
    });
}

// Reads the file once to show the progress (and to have it in the cache); the loader of Qt then gets it from there
async function download(url, label) {
    const response = await fetch(url);
    if (!response.ok) {
        throw new Error(`${url}: ${response.status}`);
    }
    const total = Number(response.headers.get("content-length")) || 0;
    if (!response.body || !total) {
        await response.arrayBuffer();
        return;
    }

    const reader = response.body.getReader();
    let received = 0;
    for (;;) {
        const { done, value } = await reader.read();
        if (done) {
            break;
        }
        received += value.length;
        // content-length is the compressed size, the reader counts the decoded bytes
        setStatus(label, Math.min(0.99, received / total));
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// The score the app starts with
// ---------------------------------------------------------------------------------------------------------------------

// A file that the operating system passed to the installed app (manifest file_handlers)
function launchedFile() {
    return new Promise((resolve) => {
        if (!("launchQueue" in window)) {
            resolve(null);
            return;
        }
        const timeout = setTimeout(() => resolve(null), 400);
        window.launchQueue.setConsumer(async (params) => {
            clearTimeout(timeout);
            try {
                const handle = params.files?.[0];
                if (!handle) {
                    resolve(null);
                    return;
                }
                const file = await handle.getFile();
                resolve({ name: file.name, bytes: new Uint8Array(await file.arrayBuffer()) });
            } catch (e) {
                console.warn("[launch] cannot read the file", e);
                resolve(null);
            }
        });
    });
}

// A file that was shared to the app (manifest share_target); the service worker keeps it in a cache
async function sharedFile() {
    if (!new URLSearchParams(location.search).has("shared") || !("caches" in window)) {
        return null;
    }
    try {
        const cache = await caches.open("scorerehearsal-share");
        const response = await cache.match("shared-score");
        if (!response) {
            return null;
        }
        const name = decodeURIComponent(response.headers.get("x-file-name") || "score.mscz");
        const bytes = new Uint8Array(await response.arrayBuffer());
        await cache.delete("shared-score");
        history.replaceState(null, "", location.pathname);
        return { name, bytes };
    } catch (e) {
        console.warn("[share] cannot read the file", e);
        return null;
    }
}

function safeFileName(name) {
    return String(name).replace(/[\\/:*?"<>|]/g, "_") || "score.mscz";
}

// ---------------------------------------------------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------------------------------------------------

async function main() {
    registerServiceWorker();
    setStatus(presentation ? "Waiting for the rehearsal window…" : "Loading…");

    const build = await loadBuildInfo();

    let startupScore = null;
    if (presentation) {
        startupScore = await requestScore();
    } else {
        startupScore = (await sharedFile()) || (await launchedFile());
    }

    setStatus("Loading…", 0);
    await download(build.wasm, "Loading the app…");
    await loadScript(build.script);

    const entry = window[build.entry];
    if (typeof entry !== "function") {
        throw new Error(`${build.script} does not define ${build.entry}`);
    }

    // The app opens this file like a file from the command line
    const startupPath = startupScore ? "/interactive/" + safeFileName(startupScore.name) : null;

    const Module = {
        config, // static configuration of the audio, read by the C++ side
        webMode: presentation ? "presentation" : "rehearsal",
        soundFont: new URL("sound/MS%20Basic.sf3", document.baseURI).href,
        arguments: startupPath ? [startupPath] : [],
        print: (text) => console.log(text),
        printErr: (text) => console.warn(text),

        qt: {
            entryFunction: entry,
            containerElements: [ui.screen],
            onLoaded: () => {
                ui.splash.hidden = true;
                ui.screen.hidden = false;
                audioStartable();
            },
            onExit: (exit) => {
                const detail = exit.code !== undefined ? ` (code ${exit.code})` : "";
                showError((exit.text || "The app has stopped.") + detail);
            },
        },
    };

    const storage = createStorage(Module, { readOnly: presentation });
    const wakeLock = createWakeLock();

    // Called from C++ (AppJsModule::onStartApp) when the app is initialised
    Module.onStartApp = async () => {
        await setupAudio(Module);
    };

    // The same JS side as in the viewer of the web build: the app talks to the audio worklet through these channels
    setupRpc(Module);

    // ---- called from C++ (src/appshell/qml/MuseScore/AppShell/webbridge.cpp) ----
    Module.persistStorage = () => storage.persist();
    Module.toggleFullscreen = toggleFullscreen;
    Module.isFullscreen = isFullscreen;
    Module.exportSessions = () => storage.exportSessions();
    Module.importSessions = () => pickFile(".json", (file) => storage.importSessions(file));
    Module.openFileDialog = openFileDialog;

    if (presentation) {
        installPresentationSide(Module);
    } else {
        installRehearsalSide(Module);
    }

    Module.preRun = [
        (instance) => {
            mountStorage(instance);
            if (startupScore) {
                instance.FS.mkdirTree("/interactive");
                instance.FS.writeFile(startupPath, startupScore.bytes);
                instance.startupScore = { path: startupPath, title: startupPath.split("/").pop().replace(/\.[^.]*$/, "") };
            }
        },
    ];

    // Start button: browsers allow sound only after a user gesture
    ui.start.addEventListener("click", () => {
        startAudio(Module);
        wakeLock.set(true);
        storage.requestPersistence();
        ui.start.hidden = true;
        ui.screen.focus();
    });

    if (presentation) {
        // No sound, no start button: only keep the screen on
        document.addEventListener("pointerdown", () => wakeLock.set(true), { once: true });
    }

    if (!presentation) {
        // QSettings and other files are written by the app without telling the page
        setInterval(() => storage.persist(), 20000);
    }

    await qtLoad(Module);
}

// ---------------------------------------------------------------------------------------------------------------------
// Audio
// ---------------------------------------------------------------------------------------------------------------------

let audioReady = false;
let appLoaded = false;

function audioStartable() {
    appLoaded = true;
    updateStartButton();
}

function updateStartButton() {
    if (presentation) {
        return;
    }
    // Enabled only when the driver is ready: the audio context must be resumed inside the click
    if (appLoaded && audioReady) {
        ui.start.hidden = false;
    }
}

function setupRpc(Module) {
    // Main <=> Worker: port1 - main, port2 - worker
    Module.main_worker_rpcChannel = new MessageChannel();
    Module.main_worker_rpcSend = (data) => Module.main_worker_rpcChannel.port1.postMessage(data);
    Module.main_worker_rpcListen = () => {}; // replaced by the C++ side
    Module.main_worker_rpcChannel.port1.onmessage = (event) => Module.main_worker_rpcListen(event.data);

    // Worker <=> Driver (processor)
    Module.driver_worker_rpcChannel = new MessageChannel();
}

// The presentation tab plays nothing, but the audio module still asks for a driver
const silentDriver = {
    inited: true,
    outputSpec: () => ({ sampleRate: 44100, samplesPerChannel: 128, audioChannelCount: 2 }),
    open() {},
    resume() {},
    suspend() {},
    close() {},
};

async function setupAudio(Module) {
    if (presentation) {
        Module.driver = silentDriver;
        return;
    }

    Module.driver = AudioDriver;
    AudioDriver.onInited = () => {
        Module.ccall("addSoundFont", "", ["string"], [Module.soundFont]);
        audioReady = true;
        updateStartButton();
    };
    await AudioDriver.setup(Module.config, Module.main_worker_rpcChannel.port2);
}

function startAudio(Module) {
    if (presentation || !Module._startAudioProcessing) {
        return;
    }
    Module._startAudioProcessing();
}

// ---------------------------------------------------------------------------------------------------------------------
// File dialog (called by WebInteractive::selectOpeningFile)
// ---------------------------------------------------------------------------------------------------------------------

function pickFile(accept, onFile) {
    const input = document.createElement("input");
    input.type = "file";
    input.accept = accept;
    input.style.display = "none";
    input.onchange = async () => {
        const file = input.files?.[0];
        input.remove();
        if (file) {
            try {
                await onFile(file);
            } catch (e) {
                console.error("[file]", e);
                alert("The file could not be used: " + e.message);
            }
        }
    };
    // iOS Safari does not report a cancelled dialog; the app simply stays as it is
    document.body.appendChild(input);
    input.click();
}

function openFileDialog(callback) {
    const input = document.createElement("input");
    input.type = "file";
    input.accept = ".mscz,.mscx";
    input.style.display = "none";
    input.onchange = async () => {
        const file = input.files?.[0];
        input.remove();
        if (!file) {
            callback("", new Uint8Array(1)); // an empty name is a cancel (the C++ side reads the first byte)
            return;
        }
        callback(safeFileName(file.name), new Uint8Array(await file.arrayBuffer()));
    };
    input.addEventListener("cancel", () => {
        input.remove();
        callback("", new Uint8Array(1)); // an empty name is a cancel (the C++ side reads the first byte)
    });
    document.body.appendChild(input);
    input.click();
}

main().catch((e) => {
    console.error(e);
    showError(e.message || String(e));
});
