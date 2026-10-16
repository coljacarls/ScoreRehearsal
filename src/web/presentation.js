// SPDX-License-Identifier: GPL-3.0-only
// ScoreRehearsal, Copyright (C) 2026 Colja Carls
//
// The presentation window of the web app: a second browser tab that runs the app in presentation mode
// (?mode=presentation). Both tabs talk through a BroadcastChannel, which works without any server.
//
//   presentation tab                          rehearsal tab
//   ----------------                          -------------
//   {type: "hello"}                  ------>
//                                    <------  {type: "score", name, bytes}   (the open score)
//                                    <------  {type: "state", json}          (position, solo, laser, ...)
//   {type: "closed"}                 ------>  (the tab was closed)
//   {type: "close"}  <------------------------ (the button in the rehearsal tab)
//
// The C++ side (webbridge.cpp) calls Module.openPresentationTab, publishPresentationState and
// closePresentationTab and receives Module.onPresentationState and Module.onPresentationClosed.

const CHANNEL_NAME = "scorerehearsal-presentation";
const WINDOW_NAME = "scorerehearsal-presentation";
const HELLO_INTERVAL_MS = 500;
const HELLO_TIMEOUT_MS = 20000;

export function isPresentationMode(search = location.search) {
    return new URLSearchParams(search).get("mode") === "presentation";
}

// ---------------------------------------------------------------------------------------------------------------------
// Rehearsal tab
// ---------------------------------------------------------------------------------------------------------------------

export function installRehearsalSide(Module) {
    const channel = new BroadcastChannel(CHANNEL_NAME);
    let score = null; // {name, bytes} of the score the presentation tab is opened for
    let lastState = null;

    function sendScore() {
        if (score) {
            channel.postMessage({ type: "score", name: score.name, bytes: score.bytes });
        }
        if (lastState) {
            channel.postMessage({ type: "state", json: lastState });
        }
    }

    channel.onmessage = (event) => {
        const message = event.data;
        if (message?.type === "hello") {
            sendScore();
        } else if (message?.type === "closed") {
            Module.onPresentationClosed?.();
        }
    };

    // Must be called from the click handler: browsers block windows that are not opened by a user gesture.
    Module.openPresentationTab = (path) => {
        try {
            const name = String(path).split("/").pop();
            score = { name, bytes: Module.FS.readFile(path) };
        } catch (e) {
            console.error("[presentation] cannot read the score", e);
            return false;
        }

        const url = new URL(location.href);
        url.search = "?mode=presentation";
        url.hash = "";
        const tab = window.open(url.href, WINDOW_NAME);
        if (!tab) {
            console.warn("[presentation] the window was blocked");
            return false;
        }
        // A tab that is already open (same name) does not say hello again
        sendScore();
        return true;
    };

    Module.publishPresentationState = (json) => {
        lastState = json;
        channel.postMessage({ type: "state", json });
    };

    Module.closePresentationTab = () => {
        channel.postMessage({ type: "close" });
        score = null;
        lastState = null;
    };

    // The score of the presentation tab is the one that was current when it was opened
    Module.setPresentationScore = (path) => {
        if (!score) {
            return;
        }
        try {
            score = { name: String(path).split("/").pop(), bytes: Module.FS.readFile(path) };
        } catch (e) {
            console.error("[presentation] cannot read the score", e);
        }
    };
}

// ---------------------------------------------------------------------------------------------------------------------
// Presentation tab
// ---------------------------------------------------------------------------------------------------------------------

// Asks the rehearsal tab for the score. Resolves with {name, bytes}, rejects if there is no rehearsal tab.
export function requestScore() {
    const channel = new BroadcastChannel(CHANNEL_NAME);

    return new Promise((resolve, reject) => {
        const hello = () => channel.postMessage({ type: "hello" });
        const interval = setInterval(hello, HELLO_INTERVAL_MS);
        const timeout = setTimeout(() => {
            clearInterval(interval);
            channel.close();
            reject(new Error("no rehearsal window answered"));
        }, HELLO_TIMEOUT_MS);

        channel.onmessage = (event) => {
            const message = event.data;
            if (message?.type === "score") {
                clearInterval(interval);
                clearTimeout(timeout);
                channel.close();
                resolve({ name: message.name, bytes: message.bytes });
            }
        };

        hello();
    });
}

// Passes the state of the rehearsal tab on to the app and tells the rehearsal tab when this tab is closed.
export function installPresentationSide(Module) {
    const channel = new BroadcastChannel(CHANNEL_NAME);
    let latest = null;
    let retry = null;

    function deliver() {
        if (latest === null) {
            return;
        }
        if (typeof Module.onPresentationState === "function") {
            const json = latest;
            latest = null;
            Module.onPresentationState(json);
        } else if (!retry) {
            // The app is not started yet (embind registers the function when the module runs)
            retry = setInterval(() => {
                if (typeof Module.onPresentationState === "function") {
                    clearInterval(retry);
                    retry = null;
                    deliver();
                }
            }, 250);
        }
    }

    channel.onmessage = (event) => {
        const message = event.data;
        if (message?.type === "state") {
            latest = message.json;
            deliver();
        } else if (message?.type === "close") {
            window.close();
        }
    };

    window.addEventListener("pagehide", () => channel.postMessage({ type: "closed" }));
}
