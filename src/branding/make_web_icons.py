#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
#
# ScoreRehearsal
# Copyright (C) 2026 Colja Carls
#
# Generates the icons of the web app (PWA manifest, Apple touch icon, favicon) into src/web/icons
# from the same drawing as the desktop icon (make_icon.py). Needs Pillow for the maskable icon.
#
# Usage: python make_web_icons.py [output-dir]

import io
import os
import sys

from PIL import Image

import make_icon


def render_image(size):
    return Image.open(io.BytesIO(make_icon.png_bytes(make_icon.render(size)))).convert("RGBA")


def maskable(size):
    # Full bleed background; the drawing stays inside the 80 % safe zone (circle) of the mask
    background = tuple(make_icon.BG) + (255,)
    canvas = Image.new("RGBA", (size, size), background)
    inner = int(size * 0.7)
    canvas.alpha_composite(render_image(inner), ((size - inner) // 2, (size - inner) // 2))
    return canvas


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "..", "web", "icons")
    os.makedirs(out_dir, exist_ok=True)

    big = render_image(512)
    big.save(os.path.join(out_dir, "icon-512.png"))
    big.resize((192, 192), Image.LANCZOS).save(os.path.join(out_dir, "icon-192.png"))
    big.resize((180, 180), Image.LANCZOS).save(os.path.join(out_dir, "apple-touch-icon.png"))
    big.resize((32, 32), Image.LANCZOS).save(os.path.join(out_dir, "favicon-32.png"))
    maskable(512).save(os.path.join(out_dir, "icon-maskable-512.png"))


if __name__ == "__main__":
    main()
