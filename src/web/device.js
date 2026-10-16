// SPDX-License-Identifier: GPL-3.0-only
// ScoreRehearsal, Copyright (C) 2026 Colja Carls
//
// Device features that browsers only allow after a user gesture: full screen and keeping the screen on.

export function isFullscreen() {
    return !!(document.fullscreenElement || document.webkitFullscreenElement);
}

export function toggleFullscreen() {
    const root = document.documentElement;
    if (isFullscreen()) {
        (document.exitFullscreen || document.webkitExitFullscreen)?.call(document);
    } else {
        // Not available on every device (e.g. iPhone); the installed app is full screen anyway
        const request = root.requestFullscreen || root.webkitRequestFullscreen;
        request?.call(root, { navigationUI: "hide" })?.catch?.(() => {});
    }
}

// Keeps the screen on while the app is visible. The browser releases the lock when the page is hidden.
export function createWakeLock() {
    let sentinel = null;
    let wanted = false;

    async function acquire() {
        if (!wanted || sentinel || !("wakeLock" in navigator) || document.visibilityState !== "visible") {
            return;
        }
        try {
            sentinel = await navigator.wakeLock.request("screen");
            sentinel.addEventListener("release", () => {
                sentinel = null;
            });
        } catch (e) {
            // denied (e.g. low battery): the screen may turn off, nothing else is lost
            console.warn("[wakelock]", e.name);
        }
    }

    document.addEventListener("visibilitychange", acquire);

    return {
        set(keepAwake) {
            wanted = !!keepAwake;
            if (wanted) {
                acquire();
            } else if (sentinel) {
                sentinel.release();
                sentinel = null;
            }
        },
    };
}
