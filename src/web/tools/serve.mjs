#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-only
// ScoreRehearsal, Copyright (C) 2026 Colja Carls
//
// A small static file server for trying the web app locally: node serve.mjs <dir> [port]
// localhost counts as a secure context, so the service worker and the install prompt work.

import fs from "node:fs";
import http from "node:http";
import path from "node:path";
import { fileURLToPath } from "node:url";

const TYPES = {
    ".html": "text/html; charset=utf-8",
    ".js": "text/javascript; charset=utf-8",
    ".mjs": "text/javascript; charset=utf-8",
    ".json": "application/json",
    ".webmanifest": "application/manifest+json",
    ".wasm": "application/wasm",
    ".png": "image/png",
    ".svg": "image/svg+xml",
    ".sf3": "application/octet-stream",
};

export function createServer(root, { headers = {} } = {}) {
    const base = path.resolve(root);

    return http.createServer((request, response) => {
        const url = new URL(request.url, "http://localhost");
        let relative = decodeURIComponent(url.pathname);
        if (relative.endsWith("/")) {
            relative += "index.html";
        }

        const file = path.resolve(base, "." + relative);
        if (file !== base && !file.startsWith(base + path.sep)) {
            response.writeHead(403).end("forbidden");
            return;
        }

        fs.readFile(file, (error, data) => {
            if (error) {
                response.writeHead(404, { "content-type": "text/plain" }).end("not found");
                return;
            }
            response.writeHead(200, {
                "content-type": TYPES[path.extname(file)] || "application/octet-stream",
                "cache-control": "no-cache",
                ...headers,
            });
            response.end(data);
        });
    });
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
    const [dir, port = "8080"] = process.argv.slice(2);
    if (!dir) {
        console.error("usage: serve.mjs <dir> [port]");
        process.exit(1);
    }
    createServer(dir).listen(Number(port), () => console.log(`http://localhost:${port}/`));
}
