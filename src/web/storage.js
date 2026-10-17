// SPDX-License-Identifier: GPL-3.0-only
// ScoreRehearsal, Copyright (C) 2026 Colja Carls
//
// Persistent storage of the web app. The app keeps its settings, sessions.json and the score library
// below /files/data (see GlobalConfiguration::userDataPath). That directory is an Emscripten IDBFS mount,
// i.e. a copy of the files in IndexedDB: loaded before the app starts, written back after changes.

export const DATA_DIR = "/files/data";
export const SESSIONS_FILE = DATA_DIR + "/sessions.json";

const WRITE_BACK_DELAY_MS = 800;

// Called from the preRun hook of the app module (FS and IDBFS come from pre.js).
// Delays the start of the app until the files are loaded from IndexedDB.
export function mountStorage(Module) {
    const FS = Module.FS;
    if (!FS || !Module.IDBFS) {
        console.warn("[storage] no IDBFS: settings and sessions are not kept");
        return;
    }

    FS.mkdirTree(DATA_DIR);
    FS.mount(Module.IDBFS, {}, DATA_DIR);

    const dependency = "scorerehearsal-storage";
    Module.addRunDependency?.(dependency);
    FS.syncfs(true, (err) => {
        if (err) {
            console.error("[storage] load failed", err);
        }
        Module.removeRunDependency?.(dependency);
    });
}

// Writes the files back to IndexedDB. Calls are merged and run one after the other,
// as IDBFS does not allow two synchronisations at the same time.
//
// readOnly: the presentation tab. It has a copy of the files from when it was started and must never write
// them back over the newer ones of the rehearsal tab.
export function createStorage(Module, { readOnly = false } = {}) {
    let timer = null;
    let running = false;
    let again = false;
    let waiters = [];

    function syncOnce() {
        const FS = Module.FS;
        if (readOnly || !FS || !Module.IDBFS) {
            return Promise.resolve();
        }
        return new Promise((resolve) => {
            FS.syncfs(false, (err) => {
                if (err) {
                    console.error("[storage] write back failed", err);
                }
                resolve();
            });
        });
    }

    async function run() {
        if (running) {
            again = true;
            return;
        }
        running = true;
        do {
            again = false;
            await syncOnce();
        } while (again);
        running = false;

        const done = waiters;
        waiters = [];
        done.forEach((resolve) => resolve());
    }

    const storage = {
        // Called by the app after it wrote a file (Module.persistStorage)
        persist() {
            if (readOnly) {
                return;
            }
            clearTimeout(timer);
            timer = setTimeout(run, WRITE_BACK_DELAY_MS);
        },

        // Writes back right now; resolves when everything is stored
        flush() {
            clearTimeout(timer);
            if (readOnly) {
                return Promise.resolve();
            }
            return new Promise((resolve) => {
                waiters.push(resolve);
                run();
            });
        },

        // Asks the browser not to delete the data when the device runs low on space.
        // Without it iPadOS may delete the data of a website that was not used for a week.
        async requestPersistence() {
            try {
                if (navigator.storage?.persist && !(await navigator.storage.persisted())) {
                    return await navigator.storage.persist();
                }
            } catch (e) {
                console.warn("[storage] persist() failed", e);
            }
            return false;
        },

        // The sessions as a file for the user (a backup against cleared site data)
        exportSessions() {
            let data;
            try {
                data = Module.FS.readFile(SESSIONS_FILE);
            } catch (e) {
                return false;
            }
            const url = URL.createObjectURL(new Blob([data], { type: "application/json" }));
            const link = document.createElement("a");
            link.href = url;
            link.download = "scorerehearsal-sessions.json";
            document.body.appendChild(link);
            link.click();
            link.remove();
            setTimeout(() => URL.revokeObjectURL(url), 10000);
            return true;
        },

        // Replaces sessions.json by the file of the user and restarts the app (it reads the file only once)
        async importSessions(file) {
            const text = await file.text();
            const parsed = JSON.parse(text); // throws for anything that is not JSON
            if (parsed === null || typeof parsed !== "object") {
                throw new Error("not a sessions file");
            }
            Module.FS.writeFile(SESSIONS_FILE, text);
            await storage.flush();
            location.reload();
        },
    };

    // Do not lose the last changes when the page is left (the delayed write back would not run)
    document.addEventListener("visibilitychange", () => {
        if (document.visibilityState === "hidden") {
            storage.flush();
        }
    });

    return storage;
}
