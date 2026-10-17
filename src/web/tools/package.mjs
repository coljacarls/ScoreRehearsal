#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-only
// ScoreRehearsal, Copyright (C) 2026 Colja Carls
//
// Puts the web app together: the build of the WebAssembly app, the audio engine, the files of the page
// (src/web) and a service worker that knows all of them. The result can be served as it is.
//
// usage: node package.mjs --build <dir> --audio <dir> --out <dir> [--version <v>] [--sound <sf3>] [--qtloader-dir <dir>]
//   --build   output of the app build (ScoreRehearsal.js / MuseScoreStudio.js, the .wasm and distr/)
//   --audio   output of the MuseAudio build (MuseAudio.js, MuseAudio.wasm)
//   --sound   the sound font (default: musescore/share/sound/MS Basic.sf3)

import { createHash } from "node:crypto";
import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const webDir = path.resolve(here, "..");
const repoRoot = path.resolve(webDir, "..", "..");

function parseArgs(argv) {
    const args = {};
    for (let i = 0; i < argv.length; i += 2) {
        if (!argv[i].startsWith("--") || argv[i + 1] === undefined) {
            throw new Error("bad arguments near " + argv[i]);
        }
        args[argv[i].slice(2)] = argv[i + 1];
    }
    return args;
}

function copy(from, to) {
    fs.mkdirSync(path.dirname(to), { recursive: true });
    fs.copyFileSync(from, to);
}

function listFiles(dir, base = dir) {
    const result = [];
    for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
        const full = path.join(dir, entry.name);
        if (entry.isDirectory()) {
            result.push(...listFiles(full, base));
        } else {
            result.push(path.relative(base, full).split(path.sep).join("/"));
        }
    }
    return result.sort();
}

function hashOf(file) {
    return createHash("sha256").update(fs.readFileSync(file)).digest("hex").slice(0, 16);
}

function versionOf(args) {
    if (args.version) {
        return args.version;
    }
    if (process.env.SCOREREHEARSAL_VERSION) {
        return process.env.SCOREREHEARSAL_VERSION;
    }
    try {
        return execFileSync("git", ["describe", "--tags", "--match", "v[0-9]*", "--dirty"], { cwd: repoRoot })
            .toString().trim().replace(/^v/, "");
    } catch (e) {
        return "0.0.0-dev";
    }
}

// The name of the function that starts the module is made by Emscripten from the target name of CMake
// (MuseScoreStudio_entry), which is not the name of the file (ScoreRehearsal.js)
function entryNameOf(file, fallback) {
    const head = fs.readFileSync(file, "utf8").slice(0, 400000);
    const match = head.match(/\b(?:var|let|const)\s+([A-Za-z0-9_]+_entry)\b/) || head.match(/\b([A-Za-z0-9_]+_entry)\s*=/);
    return match ? match[1] : fallback;
}

export function packageWebApp(args) {
    const { build, audio, out } = args;
    if (!build || !audio || !out) {
        throw new Error("--build, --audio and --out are required");
    }

    const sound = args.sound || path.join(repoRoot, "musescore", "share", "sound", "MS Basic.sf3");
    const version = versionOf(args);

    // The Qt generated names depend on the target name
    const base = ["ScoreRehearsal", "MuseScoreStudio"].find((name) => fs.existsSync(path.join(build, name + ".js")));
    if (!base) {
        throw new Error(`no ScoreRehearsal.js or MuseScoreStudio.js in ${build}`);
    }
    if (!fs.existsSync(path.join(build, base + ".wasm"))) {
        throw new Error(`${base}.wasm is missing in ${build}`);
    }

    fs.rmSync(out, { recursive: true, force: true });
    fs.mkdirSync(out, { recursive: true });

    // The page
    for (const name of ["index.html", "app.js", "storage.js", "device.js", "presentation.js", "manifest.webmanifest"]) {
        copy(path.join(webDir, name), path.join(out, name));
    }
    for (const icon of fs.readdirSync(path.join(webDir, "icons"))) {
        copy(path.join(webDir, "icons", icon), path.join(out, "icons", icon));
    }

    // The app and the audio engine
    copy(path.join(build, base + ".js"), path.join(out, base + ".js"));
    copy(path.join(build, base + ".wasm"), path.join(out, base + ".wasm"));
    for (const name of ["MuseAudio.js", "MuseAudio.wasm"]) {
        copy(path.join(audio, name), path.join(out, name));
    }
    for (const name of ["qtloader.js", "audiodriver.js", "audio_worklet_processor.js", "config.js"]) {
        copy(path.join(build, "distr", name), path.join(out, "distr", name));
    }
    copy(sound, path.join(out, "sound", "MS Basic.sf3"));

    fs.writeFileSync(path.join(out, "build-info.json"), JSON.stringify({
        version,
        script: base + ".js",
        wasm: base + ".wasm",
        entry: entryNameOf(path.join(build, base + ".js"), base + "_entry"),
    }, null, 2) + "\n");

    // GitHub Pages: no Jekyll processing
    fs.writeFileSync(path.join(out, ".nojekyll"), "");

    // The service worker. The version changes with every content change, so a changed file always updates the app.
    const files = listFiles(out).filter((name) => name !== "sw.js" && name !== ".nojekyll");
    const fileInfos = files.map((name) => ({
        url: name.split("/").map(encodeURIComponent).join("/"),
        path: name,
        // The sound font does not change with the app: it has a cache of its own, named after its content
        cache: name.endsWith(".sf3") ? hashOf(path.join(out, name)) : "app",
    }));

    const entries = fileInfos;

    const contentHash = createHash("sha256");
    for (const file of fileInfos) {
        contentHash.update(file.path).update(hashOf(path.join(out, file.path)));
    }
    const swVersion = `${version}-${contentHash.digest("hex").slice(0, 10)}`;

    const template = fs.readFileSync(path.join(webDir, "sw.js"), "utf8");
    const worker = template
        .replace("__VERSION__", swVersion)
        .replace("__FILES__", JSON.stringify(entries.map(({ url, cache }) => ({ url, cache })), null, 2));
    fs.writeFileSync(path.join(out, "sw.js"), worker);

    return { out, version: swVersion, files: entries.length + 1 };
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
    try {
        const result = packageWebApp(parseArgs(process.argv.slice(2)));
        console.log(`packaged ${result.files} files, version ${result.version} -> ${result.out}`);
    } catch (e) {
        console.error("package.mjs: " + e.message);
        process.exit(1);
    }
}
