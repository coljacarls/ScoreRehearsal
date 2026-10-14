#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
#
# ScoreRehearsal
# Copyright (C) 2026 Colja Carls
#
# Generates the ScoreRehearsal application icon (ScoreRehearsal.ico and
# scorerehearsal_logo.png) without any external dependencies: a rounded
# square with a play triangle in front of three staff lines.
#
# Usage: python make_icon.py [output-dir]

import math
import os
import struct
import sys
import zlib

BG = (31, 93, 140)        # deep blue
BG_DARK = (22, 66, 100)   # bottom gradient
FG = (255, 255, 255)      # white
LINE = (170, 205, 235)    # staff lines

SUPERSAMPLE = 4


def rounded_rect_dist(px, py, w, h, r):
    # signed distance to a rounded rectangle centered at the origin
    qx = abs(px) - (w / 2 - r)
    qy = abs(py) - (h / 2 - r)
    ox = max(qx, 0.0)
    oy = max(qy, 0.0)
    return math.hypot(ox, oy) + min(max(qx, qy), 0.0) - r


def point_in_triangle(px, py, a, b, c):
    def sign(p1, p2, p3):
        return (p1[0] - p3[0]) * (p2[1] - p3[1]) - (p2[0] - p3[0]) * (p1[1] - p3[1])
    d1 = sign((px, py), a, b)
    d2 = sign((px, py), b, c)
    d3 = sign((px, py), c, a)
    has_neg = d1 < 0 or d2 < 0 or d3 < 0
    has_pos = d1 > 0 or d2 > 0 or d3 > 0
    return not (has_neg and has_pos)


def render(size):
    """Returns rows of RGBA tuples for an icon of the given size."""
    s = size * SUPERSAMPLE
    corner = s * 0.22
    rows = []
    # triangle (play symbol), slightly right of center
    ta = (s * 0.40, s * 0.28)
    tb = (s * 0.40, s * 0.72)
    tc = (s * 0.76, s * 0.50)
    # staff lines
    line_ys = [s * 0.38, s * 0.50, s * 0.62]
    line_h = max(1.0, s * 0.035)
    line_x0 = s * 0.20
    line_x1 = s * 0.80

    for y in range(s):
        row = []
        for x in range(s):
            px = x + 0.5 - s / 2
            py = y + 0.5 - s / 2
            d = rounded_rect_dist(px, py, s * 0.94, s * 0.94, corner)
            if d > 0:
                row.append((0, 0, 0, 0))
                continue
            t = (y + 0.5) / s
            r = BG[0] + (BG_DARK[0] - BG[0]) * t
            g = BG[1] + (BG_DARK[1] - BG[1]) * t
            b = BG[2] + (BG_DARK[2] - BG[2]) * t
            color = (r, g, b)
            for ly in line_ys:
                if abs(y + 0.5 - ly) <= line_h / 2 and line_x0 <= x + 0.5 <= line_x1:
                    color = LINE
            if point_in_triangle(x + 0.5, y + 0.5, ta, tb, tc):
                color = FG
            row.append((color[0], color[1], color[2], 255))
        rows.append(row)

    # downsample
    out = []
    n = SUPERSAMPLE * SUPERSAMPLE
    for y in range(size):
        orow = []
        for x in range(size):
            r = g = b = a = 0
            for sy in range(SUPERSAMPLE):
                for sx in range(SUPERSAMPLE):
                    pr, pg, pb, pa = rows[y * SUPERSAMPLE + sy][x * SUPERSAMPLE + sx]
                    r += pr * pa
                    g += pg * pa
                    b += pb * pa
                    a += pa
            if a == 0:
                orow.append((0, 0, 0, 0))
            else:
                orow.append((int(r / a), int(g / a), int(b / a), int(a / n)))
        out.append(orow)
    return out


def png_bytes(pixels):
    size = len(pixels)
    raw = bytearray()
    for row in pixels:
        raw.append(0)  # filter: none
        for r, g, b, a in row:
            raw.extend((r, g, b, a))

    def chunk(kind, data):
        c = struct.pack(">I", len(data)) + kind + data
        return c + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)

    ihdr = struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0)
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr)
            + chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b""))


def ico_bytes(pngs):
    # pngs: list of (size, png data); PNG-compressed entries (Vista+)
    header = struct.pack("<HHH", 0, 1, len(pngs))
    entries = b""
    data = b""
    offset = 6 + 16 * len(pngs)
    for size, png in pngs:
        s = 0 if size >= 256 else size
        entries += struct.pack("<BBBBHHII", s, s, 0, 0, 1, 32, len(png), offset)
        data += png
        offset += len(png)
    return header + entries + data


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))
    pngs = []
    for size in (16, 32, 48, 256):
        pngs.append((size, png_bytes(render(size))))
    with open(os.path.join(out_dir, "ScoreRehearsal.ico"), "wb") as f:
        f.write(ico_bytes(pngs))
    with open(os.path.join(out_dir, "scorerehearsal_logo.png"), "wb") as f:
        f.write(pngs[-1][1])
    print("written ScoreRehearsal.ico and scorerehearsal_logo.png to", out_dir)


if __name__ == "__main__":
    main()
