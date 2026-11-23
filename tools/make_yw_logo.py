#!/usr/bin/env python3
"""Writes every YW Studio logo picture from the YW vector logo (white "YW" on a dark rounded tile).

    python3 tools/make_yw_logo.py      (from the repository root), then python3 tools/make_logo_data.py

Replaces the upstream brand pictures under the same file names, so the build and the other tools keep working.
"""
import os
from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# The YW logo outline in a 304 x 192 box
LOGO = [(261.0, 183.4), (220.5, 165.0), (198.0, 75.9), (174.0, 154.9), (135.4, 154.0), (111.0, 77.9), (89.2, 119.0),
        (89.5, 164.0), (53.0, 179.3), (52.5, 126.0), (7.0, 3.5), (41.6, 15.0), (71.0, 94.6), (97.6, 30.0), (134.0, 34.2),
        (155.0, 122.2), (179.5, 34.0), (216.0, 27.0), (240.0, 135.7), (262.6, 14.0), (296.8, 3.0)]
W, H = 304.0, 192.0
TILE = (14, 16, 19, 255)
SS = 4  # supersampling


def mark(size, fraction=0.86):
    """The white logo alone, centred, transparent background."""
    big = size * SS
    img = Image.new("RGBA", (big, big), (0, 0, 0, 0))
    s = big * fraction / W
    ox, oy = (big - W * s) / 2, (big - H * s) / 2
    ImageDraw.Draw(img).polygon([(ox + x * s, oy + y * s) for x, y in LOGO], fill=(255, 255, 255, 255))
    return img.resize((size, size), Image.LANCZOS)


def icon(size, fraction=0.70):
    """The app icon: the logo on the dark rounded tile."""
    big = size * SS
    img = Image.new("RGBA", (big, big), (0, 0, 0, 0))
    ImageDraw.Draw(img).rounded_rectangle([0, 0, big - 1, big - 1], radius=int(big * 0.2), fill=TILE)
    img = img.resize((size, size), Image.LANCZOS)
    img.alpha_composite(mark(size, fraction))
    return img


def placed(canvas, fraction):
    out = Image.new("RGBA", (canvas, canvas), (0, 0, 0, 0))
    side = int(round(canvas * fraction))
    out.alpha_composite(icon(side), ((canvas - side) // 2,) * 2)
    return out


def save(img, *path):
    full = os.path.join(ROOT, *path)
    os.makedirs(os.path.dirname(full), exist_ok=True)
    img.save(full)
    print("  " + os.path.join(*path))


def svg(tile):
    pts = " ".join("%.1f,%.1f" % (x + 104, y + 160) for x, y in LOGO)
    body = '<polygon fill="#ffffff" points="%s"/>' % pts
    if tile:
        body = '<rect width="512" height="512" rx="102" fill="#0e1013"/>' + body
    return '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512">%s</svg>\n' % body


def main():
    save(icon(1024), "resources", "lynx_badge.png")
    save(icon(256), "resources", "lynx_icon_256.png")
    save(mark(1024), "resources", "lynx_logo.png")
    save(icon(1024, 0.78), "resources", "brand", "lynx-share-logo-small.png")
    save(icon(1024, 0.78), "resources", "brand", "lynx-share-logo-tiny.png")
    save(mark(1024, 0.92), "resources", "brand", "lynx-share-mark-small.png")
    save(icon(512), "docs", "img", "logo.png")
    save(placed(1024, 824 / 1024), "resources", "macos", "lynx-icon-1024.png")
    save(placed(256, 824 / 1024), "resources", "macos", "lynx-icon-256.png")
    sizes = [16, 20, 24, 32, 40, 48, 64, 96, 128, 256]
    ico = [icon(s) for s in sizes]
    for name in ("LYNX.ico", "LYNX_doc.ico"):
        ico[-1].save(os.path.join(ROOT, "resources", name), format="ICO", sizes=[(s, s) for s in sizes], append_images=ico[:-1])
        print("  resources/" + name)
    for s in (16, 22, 24, 32, 48, 64, 128, 256, 512):
        save(icon(s), "resources", "linux", "icons", "hicolor", "%dx%d" % (s, s), "apps", "lynx.png")
    for name, tile in (("lynx-share-logo.svg", True), ("lynx-share-logo-small.svg", True), ("lynx-share-mark.svg", False)):
        with open(os.path.join(ROOT, "resources", "brand", name), "w") as f:
            f.write(svg(tile))
        print("  resources/brand/" + name)


if __name__ == "__main__":
    main()
