#!/usr/bin/env python3
"""Lager t1revpro-logo-{1024,512,256}.png fra SVG-en.

Quick Look (qlmanage) tegner SVG-en med WebKit (blur og glød blir riktige),
men fyller hjørnene hvite. Dette skriptet gjør alt utenfor den avrundede
firkanten (x/y 32–992, rx 220) gjennomsiktig med myk kant, og bruker
bakgrunnsfargen i kantpikslene så det ikke blir lys rand.
Ren Python 3 + macOS-verktøyene qlmanage og sips.
"""
import math
import os
import struct
import subprocess
import zlib

MAPPE = os.path.dirname(os.path.abspath(__file__))
SVG = os.path.join(MAPPE, "t1revpro-logo.svg")
N = 1024


def les_png(sti):
    d = open(sti, "rb").read()
    i, idat = 8, b""
    while i < len(d):
        n = struct.unpack(">I", d[i:i + 4])[0]
        t = d[i + 4:i + 8]
        c = d[i + 8:i + 8 + n]
        if t == b"IHDR":
            w, h, bd, ct = struct.unpack(">IIBB", c[:10])
            assert bd == 8 and ct == 6, "forventer RGBA 8 bit"
        if t == b"IDAT":
            idat += c
        i += 12 + n
    raw = zlib.decompress(idat)
    bpp, stride = 4, w * 4
    ut = bytearray(h * stride)
    forrige = bytearray(stride)
    pos = 0
    for y in range(h):
        f = raw[pos]
        rad = bytearray(raw[pos + 1:pos + 1 + stride])
        pos += 1 + stride
        for x in range(stride):
            a = rad[x - bpp] if x >= bpp else 0
            b = forrige[x]
            c = forrige[x - bpp] if x >= bpp else 0
            if f == 1:
                rad[x] = (rad[x] + a) & 255
            elif f == 2:
                rad[x] = (rad[x] + b) & 255
            elif f == 3:
                rad[x] = (rad[x] + ((a + b) >> 1)) & 255
            elif f == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                rad[x] = (rad[x] + pr) & 255
        ut[y * stride:(y + 1) * stride] = rad
        forrige = rad
    return w, h, ut


def skriv_png(sti, w, h, px):
    stride = w * 4
    raw = b"".join(b"\x00" + bytes(px[y * stride:(y + 1) * stride]) for y in range(h))

    def chunk(t, c):
        return struct.pack(">I", len(c)) + t + c + struct.pack(">I", zlib.crc32(t + c) & 0xffffffff)

    with open(sti, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(raw, 9)))
        f.write(chunk(b"IEND", b""))


def dekning(x, y):
    """Andel av pikselen (x, y) som ligger inne i den avrundede firkanten."""
    lo, hi, r = 32.0, 992.0, 220.0
    cx = min(max(x + 0.5, lo + r), hi - r)
    cy = min(max(y + 0.5, lo + r), hi - r)
    if lo <= x + 0.5 <= hi and lo <= y + 0.5 <= hi and (cx == x + 0.5 or cy == y + 0.5):
        # rett kant: avstand til kanten
        d = min(x + 0.5 - lo, hi - x - 0.5, y + 0.5 - lo, hi - y - 0.5)
    else:
        d = r - math.hypot(x + 0.5 - cx, y + 0.5 - cy)
    return min(1.0, max(0.0, d + 0.5))


def main():
    png_inn = os.path.join(MAPPE, "t1revpro-logo.svg.png")
    subprocess.run(["qlmanage", "-t", "-s", str(N), "-o", MAPPE, SVG],
                   check=True, capture_output=True)
    w, h, px = les_png(png_inn)
    assert w == N and h == N
    for y in range(h):
        # bakgrunnsgradient (#1B2C44 -> #0B1524) innenfor firkanten
        t = min(1.0, max(0.0, (y - 32) / 960.0))
        bg = (round(0x1B + (0x0B - 0x1B) * t), round(0x2C + (0x15 - 0x2C) * t), round(0x44 + (0x24 - 0x44) * t))
        for x in range(w):
            k = dekning(x, y)
            if k >= 1.0:
                continue
            i = (y * w + x) * 4
            if k <= 0.0:
                px[i:i + 4] = b"\x00\x00\x00\x00"
            else:
                px[i:i + 4] = bytes((bg[0], bg[1], bg[2], round(255 * k)))
    ut = os.path.join(MAPPE, "t1revpro-logo-1024.png")
    skriv_png(ut, w, h, px)
    os.remove(png_inn)
    for s in (512, 256):
        subprocess.run(["sips", "-z", str(s), str(s), ut, "--out",
                        os.path.join(MAPPE, f"t1revpro-logo-{s}.png")], check=True, capture_output=True)
    print("Laget t1revpro-logo-1024/512/256.png")


if __name__ == "__main__":
    main()
