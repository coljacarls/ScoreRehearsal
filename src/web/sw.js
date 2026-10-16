// SPDX-License-Identifier: GPL-3.0-only
// ScoreRehearsal, Copyright (C) 2026 Colja Carls
//
// Service worker of the web app (template: tools/package.mjs fills in VERSION and FILES).
//
// - All files of the app are stored when the worker is installed, so the app starts without network.
// - Files with a "cache" of their own (the sound font) survive app updates as long as the file is the same.
// - A new version is installed in the background and only activated when the user agrees (message
//   SKIP_WAITING), never in the middle of a rehearsal.
// - Share target: a score shared to the installed app (POST /share) is kept for the page and the page is opened.

const VERSION = "__VERSION__";
const FILES = __FILES__; // [{url, cache}]: cache is "app" or the name of an own cache that lives as long as the file

const APP_CACHE = "scorerehearsal-app-" + VERSION;
const SHARE_CACHE = "scorerehearsal-share";
const PREFIX = "scorerehearsal-";

const scope = new URL(self.registration.scope);
const absolute = (url) => new URL(url, scope).href;

const cacheNameOf = (file) => (file.cache === "app" ? APP_CACHE : PREFIX + "file-" + file.cache);

self.addEventListener("install", (event) => {
    event.waitUntil((async () => {
        for (const file of FILES) {
            const cache = await caches.open(cacheNameOf(file));
            if (file.cache !== "app" && (await cache.match(absolute(file.url)))) {
                continue; // an unchanged big file is not loaded again
            }
            // "reload": not the HTTP cache, as that could have the previous version
            const response = await fetch(new Request(absolute(file.url), { cache: "reload" }));
            if (!response.ok) {
                throw new Error(`${file.url}: ${response.status}`);
            }
            await cache.put(absolute(file.url), response);
        }
    })());
});

self.addEventListener("activate", (event) => {
    event.waitUntil((async () => {
        const keep = new Set([APP_CACHE, SHARE_CACHE, ...FILES.map(cacheNameOf)]);
        for (const name of await caches.keys()) {
            if (name.startsWith(PREFIX) && !keep.has(name)) {
                await caches.delete(name);
            }
        }
        await self.clients.claim();
    })());
});

self.addEventListener("message", (event) => {
    if (event.data?.type === "SKIP_WAITING") {
        self.skipWaiting();
    }
});

async function fromCaches(request, options) {
    for (const file of FILES) {
        const cache = await caches.open(cacheNameOf(file));
        const match = await cache.match(request, options);
        if (match) {
            return match;
        }
    }
    return undefined;
}

async function handleShare(request) {
    try {
        const form = await request.formData();
        const file = form.get("score");
        if (file && typeof file !== "string") {
            const cache = await caches.open(SHARE_CACHE);
            await cache.put("shared-score", new Response(file, {
                headers: { "x-file-name": encodeURIComponent(file.name) },
            }));
        }
    } catch (e) {
        // the page then simply opens without a score
    }
    return Response.redirect(absolute("./?shared=1"), 303);
}

self.addEventListener("fetch", (event) => {
    const request = event.request;
    const url = new URL(request.url);

    if (url.origin !== scope.origin) {
        return;
    }

    if (request.method === "POST" && url.href === absolute("share")) {
        event.respondWith(handleShare(request));
        return;
    }

    if (request.method !== "GET") {
        return;
    }

    // Every start of the app (also with ?mode=presentation or ?shared=1) is the same page
    if (request.mode === "navigate") {
        event.respondWith((async () => {
            return (await fromCaches(absolute("index.html"))) ||
                   (await fetch(request).catch(() => undefined)) ||
                   Response.error();
        })());
        return;
    }

    event.respondWith((async () => {
        return (await fromCaches(request)) || fetch(request);
    })());
});
