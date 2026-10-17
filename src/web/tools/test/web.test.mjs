// SPDX-License-Identifier: GPL-3.0-only
// ScoreRehearsal, Copyright (C) 2026 Colja Carls
//
// Tests of the page, the loader, the service worker and the presentation tab in a real browser,
// against a fake build of the app (fixture.mjs). Run: npm test (needs playwright and its Chromium).

import assert from "node:assert/strict";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { after, before, describe, test } from "node:test";

import { chromium } from "playwright";

import { makeFakeBuild } from "./fixture.mjs";
import { packageWebApp } from "../package.mjs";
import { createServer } from "../serve.mjs";

const dist = fs.mkdtempSync(path.join(os.tmpdir(), "scorerehearsal-dist-"));
let browser;
let server;
let base;

function pack(marker) {
    const fake = makeFakeBuild(marker);
    return packageWebApp({ build: fake.build, audio: fake.audio, sound: fake.sound, out: dist, version: "9.9.9-" + marker });
}

before(async () => {
    pack("one");
    server = createServer(dist);
    await new Promise((resolve) => server.listen(0, resolve));
    base = `http://localhost:${server.address().port}/`;
    browser = await chromium.launch();
});

after(async () => {
    await browser?.close();
    server?.close();
});

async function newPage(context, search = "") {
    const page = await context.newPage();
    page.on("pageerror", (e) => assert.fail("page error: " + e.message));
    await page.goto(base + search);
    return page;
}

describe("packaging", () => {
    test("contains the page, the app, the audio engine and the sound font", () => {
        for (const name of ["index.html", "app.js", "manifest.webmanifest", "sw.js", "build-info.json", "ScoreRehearsal.js",
                            "ScoreRehearsal.wasm", "MuseAudio.js", "distr/qtloader.js", "sound/MS Basic.sf3", ".nojekyll"]) {
            assert.ok(fs.existsSync(path.join(dist, name)), name + " is missing");
        }
    });

    test("the service worker knows every file but itself", () => {
        const sw = fs.readFileSync(path.join(dist, "sw.js"), "utf8");
        assert.ok(!sw.includes("__VERSION__") && !sw.includes("__FILES__"));
        assert.match(sw, /9\.9\.9-one-[0-9a-f]{10}/);
        assert.ok(sw.includes('"url": "app.js"'));
        assert.ok(sw.includes('"url": "sound/MS%20Basic.sf3"'));
        assert.ok(!sw.includes('"url": "sw.js"'));
    });

    test("the entry function is found by its name in the app script", () => {
        const fake = makeFakeBuild("entry");
        fs.writeFileSync(path.join(fake.build, "ScoreRehearsal.js"), "var MuseScoreStudio_entry = (() => { return async () => {}; })();\n");
        const out = fs.mkdtempSync(path.join(os.tmpdir(), "scorerehearsal-entry-"));
        packageWebApp({ build: fake.build, audio: fake.audio, sound: fake.sound, out, version: "1.0.0" });
        assert.equal(JSON.parse(fs.readFileSync(path.join(out, "build-info.json"), "utf8")).entry, "MuseScoreStudio_entry");
    });

    test("the manifest points to existing icons", () => {
        const manifest = JSON.parse(fs.readFileSync(path.join(dist, "manifest.webmanifest"), "utf8"));
        assert.equal(manifest.display, "fullscreen");
        for (const icon of manifest.icons) {
            assert.ok(fs.existsSync(path.join(dist, icon.src)), icon.src);
        }
    });
});

describe("app", () => {
    test("loads, mounts the storage and starts the audio on the first tap", async () => {
        const context = await browser.newContext();
        const page = await newPage(context);

        await page.waitForSelector("#start", { state: "visible" });
        assert.equal(await page.locator("#splash").isHidden(), true);

        const before = await page.evaluate(() => ({ ...window.__calls }));
        assert.deepEqual(before.mount, ["/files/data"]);
        assert.deepEqual(before.syncfs.slice(0, 1), [true], "the files are loaded from IndexedDB first");
        assert.equal(before.startAudio, 0, "no sound before a tap");

        await page.click("#start");
        assert.equal(await page.evaluate(() => window.__calls.startAudio), 1);
        assert.equal(await page.locator("#start").isHidden(), true);
        await context.close();
    });

    test("writes the changes back to the storage", async () => {
        const context = await browser.newContext();
        const page = await newPage(context);
        await page.waitForSelector("#start");

        await page.evaluate(() => window.__instance.persistStorage());
        await page.waitForFunction(() => window.__calls.syncfs.includes(false));
        await context.close();
    });

    test("exports the sessions as a file and imports them again", async () => {
        const context = await browser.newContext();
        const page = await newPage(context);
        await page.waitForSelector("#start");

        await page.evaluate(() => window.__instance.FS.writeFile("/files/data/sessions.json", '{"sessions":[1]}'));
        const download = page.waitForEvent("download");
        await page.evaluate(() => window.__instance.exportSessions());
        const file = await download;
        assert.equal(file.suggestedFilename(), "scorerehearsal-sessions.json");
        assert.equal(fs.readFileSync(await file.path(), "utf8"), '{"sessions":[1]}');
        await context.close();
    });

    test("restores the sessions from a file", async () => {
        const context = await browser.newContext();
        const page = await newPage(context);
        await page.waitForSelector("#start");

        await page.evaluate(() => {
            const write = window.__instance.FS.writeFile;
            window.__instance.FS.writeFile = (path, data) => {
                sessionStorage.setItem("written:" + path, typeof data === "string" ? data : new TextDecoder().decode(data));
                write(path, data);
            };
            const button = document.createElement("button");
            button.id = "restore";
            button.style.cssText = "position:fixed;top:0;left:0;z-index:99";
            button.textContent = "restore";
            button.onclick = () => window.__instance.importSessions();
            document.body.appendChild(button);
        });

        const chooser = page.waitForEvent("filechooser");
        await page.click("#restore");
        await Promise.all([
            page.waitForEvent("load"), // the app is restarted: it reads the file once
            (await chooser).setFiles({ name: "sessions.json", mimeType: "application/json", buffer: Buffer.from('{"version":1,"sessions":[]}') }),
        ]);
        assert.equal(await page.evaluate(() => sessionStorage.getItem("written:/files/data/sessions.json")), '{"version":1,"sessions":[]}');
        await context.close();
    });

    test("a cancelled file dialog is reported with an empty name", async () => {
        const context = await browser.newContext();
        const page = await newPage(context);
        await page.waitForSelector("#start");

        const result = page.evaluate(() => new Promise((resolve) => {
            window.__instance.openFileDialog((name, data) => resolve({ name, length: data.length }));
            document.querySelector("input[type=file]").dispatchEvent(new Event("cancel"));
        }));
        assert.deepEqual(await result, { name: "", length: 1 });
        await context.close();
    });

    test("a chosen score is passed to the app with its name", async () => {
        const context = await browser.newContext();
        const page = await newPage(context);
        await page.waitForSelector("#start");

        // a real click: browsers only open the file dialog for a user gesture
        await page.evaluate(() => {
            window.__chosen = new Promise((resolve) => {
                const button = document.createElement("button");
                button.id = "choose";
                button.style.cssText = "position:fixed;top:0;left:0;z-index:99";
                button.textContent = "choose";
                button.onclick = () => window.__instance.openFileDialog((name, data) => resolve({ name, bytes: Array.from(data) }));
                document.body.appendChild(button);
            });
        });
        const chooser = page.waitForEvent("filechooser");
        await page.click("#choose");
        await (await chooser).setFiles({ name: "Mass in B/minor.mscz", mimeType: "application/octet-stream", buffer: Buffer.from([1, 2, 3]) });
        const waiting = page.evaluate(() => window.__chosen);
        assert.deepEqual(await waiting, { name: "Mass in B_minor.mscz", bytes: [1, 2, 3] });
        await context.close();
    });
});

describe("offline and updates", () => {
    test("starts without network after the first visit", async () => {
        const context = await browser.newContext();
        const page = await newPage(context);
        await page.waitForSelector("#start");
        await page.evaluate(() => navigator.serviceWorker.ready);
        await page.reload(); // now the page is controlled by the worker
        await page.waitForSelector("#start");
        assert.ok(await page.evaluate(() => !!navigator.serviceWorker.controller));

        await context.setOffline(true);
        await page.reload();
        await page.waitForSelector("#start", { state: "visible" });

        // the sound font is there, too
        const sound = await page.evaluate(async () => (await fetch("sound/MS%20Basic.sf3")).status);
        assert.equal(sound, 200);

        // a start with a parameter is the same page
        await page.goto(base + "?shared=1");
        await page.waitForSelector("#start", { state: "visible" });
        await context.close();
    });

    test("an update waits for the user and keeps the sound font", async () => {
        const context = await browser.newContext();
        const page = await newPage(context);
        await page.waitForSelector("#start");
        await page.evaluate(() => navigator.serviceWorker.ready);
        await page.reload();
        await page.waitForSelector("#start");

        const cachesBefore = await page.evaluate(() => caches.keys());
        const soundCache = cachesBefore.find((name) => name.startsWith("scorerehearsal-file-"));
        assert.ok(soundCache, "the sound font has a cache of its own: " + cachesBefore);

        pack("two"); // a new version appears on the server
        await page.evaluate(async () => (await navigator.serviceWorker.getRegistration()).update());
        await page.waitForSelector("#update", { state: "visible" });

        // still the old version, the user was not interrupted
        assert.equal(await page.evaluate(() => window.__instance.marker), "one");

        await Promise.all([page.waitForEvent("load"), page.click("#update-button")]);
        await page.waitForSelector("#start");
        assert.equal(await page.evaluate(() => window.__instance.marker), "two");

        const cachesAfter = await page.evaluate(() => caches.keys());
        assert.ok(cachesAfter.includes(soundCache), "the sound font stays");
        assert.equal(cachesAfter.filter((name) => name.startsWith("scorerehearsal-app-")).length, 1, "the old app cache is gone");

        pack("one");
        await context.close();
    });
});

describe("presentation tab", () => {
    test("gets the score and the state of the rehearsal tab and tells when it is closed", async () => {
        const context = await browser.newContext();
        const rehearsal = await newPage(context);
        await rehearsal.waitForSelector("#start");

        await rehearsal.evaluate(() => {
            window.__instance.FS.writeFile("/interactive/Song.mscz", new Uint8Array([7, 8, 9]));
            const button = document.createElement("button");
            button.id = "present";
            button.textContent = "present";
            button.style.cssText = "position:fixed;top:0;left:0;z-index:99";
            button.onclick = () => window.__instance.openPresentationTab("/interactive/Song.mscz");
            document.body.appendChild(button);
        });

        const popupPromise = context.waitForEvent("page");
        await rehearsal.click("#present");
        const presentation = await popupPromise;
        presentation.on("pageerror", (e) => assert.fail("page error: " + e.message));
        await presentation.waitForFunction(() => window.__instance?.startupScore);

        const startup = await presentation.evaluate(() => ({
            score: window.__instance.startupScore,
            bytes: Array.from(window.__files.get("/interactive/Song.mscz")),
            mode: window.__instance.webMode,
        }));
        assert.deepEqual(startup, { score: { path: "/interactive/Song.mscz", title: "Song" }, bytes: [7, 8, 9], mode: "presentation" });
        assert.equal(await presentation.locator("#start").isHidden(), true, "no sound in the presentation tab");
        assert.deepEqual(await presentation.evaluate(() => window.__instance.arguments), ["/interactive/Song.mscz"],
                         "the app opens the score like a file from the command line");

        // the presentation tab has old copies of the files: it never writes them back
        await presentation.evaluate(() => window.__instance.persistStorage());
        await presentation.waitForTimeout(1500);
        assert.ok(!(await presentation.evaluate(() => window.__calls.syncfs)).includes(false));

        await rehearsal.evaluate(() => window.__instance.publishPresentationState('{"secs":12.5}'));
        await presentation.waitForFunction(() => window.__states.length > 0);
        assert.equal(await presentation.evaluate(() => window.__states.at(-1)), '{"secs":12.5}');

        await presentation.close();
        await rehearsal.waitForFunction(() => window.__presentationClosed > 0);
        await context.close();
    });

    test("is closed from the rehearsal tab", async () => {
        const context = await browser.newContext();
        const rehearsal = await newPage(context);
        await rehearsal.waitForSelector("#start");
        await rehearsal.evaluate(() => {
            window.__instance.FS.writeFile("/interactive/A.mscz", new Uint8Array([1]));
            const button = document.createElement("button");
            button.id = "present";
            button.style.cssText = "position:fixed;top:0;left:0;z-index:99";
            button.textContent = "x";
            button.onclick = () => window.__instance.openPresentationTab("/interactive/A.mscz");
            document.body.appendChild(button);
        });
        const popupPromise = context.waitForEvent("page");
        await rehearsal.click("#present");
        const presentation = await popupPromise;
        await presentation.waitForFunction(() => window.__instance?.startupScore);

        const closed = presentation.waitForEvent("close");
        await rehearsal.evaluate(() => window.__instance.closePresentationTab());
        await closed;
        await context.close();
    });
});
