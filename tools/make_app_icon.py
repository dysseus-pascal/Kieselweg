#!/usr/bin/env python3
"""App-Symbol: der Abbiegepfeil (25x25).

Aufruf: make_app_icon.py <zielordner>
Erzeugt system_icon.png - schwarze Linien auf durchsichtigem Grund.

MASSSTAB IST DAS SYSTEMSYMBOL, wie bei den Geschwistern: die Uhr-Kachel von
"Watchfaces" im Starter wurde Punkt fuer Punkt nachgemessen - 24 von 25
Punkten hoch, Linien 2 bis 3 Punkte stark, rund 180 schwarze Punkte. Eine
duennere Linie sieht daneben aus wie ein Versehen.

NUR LINIEN, KEINE FLAECHE, und keine ~bw-Fassung. Der Starter zeichnet Symbole
einfarbig; eine farbige Flaeche kam dort als grauer Fleck heraus. Der Pfeilkopf
ist die eine Ausnahme: ein Dreieck, das nur aus Kontur bestuende, waere bei
acht Punkten Breite ein Knaeuel.

WARUM EIN ABBIEGEPFEIL UND KEIN WEGWEISER: das Symbol steht in einer Liste
neben Kapsel, Glas, Flieger und HRV. Es muss bei 25 Punkten sagen "hier geht
es um Richtung", und das tut ein Pfeil in jeder Kultur sofort.
"""
import math
import os
import struct
import sys
import zlib

W = H = 25
SS = 4                           # Ueberabtastung je Achse
LINE = 3.0                       # Strichstaerke in Punkten
LW = LINE / 2.0 - 0.15           # halbe Strichstaerke, minus Rundungsluft

# Der Weg: von unten herauf, dann nach rechts. Der Knick sitzt bewusst nicht
# in der Mitte - ein Pfeil, der unten laenger ist als oben, liest sich als
# "von hier nach dort" und nicht als Ecke.
STIEL_X = 6.5
STIEL_UNTEN = 23.5
KNICK_Y = 8.5
QUER_BIS = 14.0

# Der Kopf. Etwas hoeher als die Strichstaerke, sonst verschwindet die Spitze.
KOPF_X0, KOPF_X1 = 12.5, 23.0
KOPF_HALB = 7.5


def png(path, w, h, rows):
    """Minimaler PNG-Schreiber, 8 Bit RGBA, ohne Fremdbibliothek."""
    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xffffffff)
    raw = b"".join(b"\x00" + bytes(r) for r in rows)
    out = b"\x89PNG\r\n\x1a\n"
    out += chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
    out += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(out)


def strecke(x, y, x0, y0, x1, y1):
    """Abstand zur Strecke - so ist die Linie auch im Knick gleich stark."""
    dx, dy = x1 - x0, y1 - y0
    laenge = dx * dx + dy * dy
    if laenge == 0:
        t = 0.0
    else:
        t = max(0.0, min(1.0, ((x - x0) * dx + (y - y0) * dy) / laenge))
    return math.hypot(x - (x0 + t * dx), y - (y0 + t * dy))


def inside(x, y):
    # Stiel und Querbalken als eine Linie mit Knick.
    if strecke(x, y, STIEL_X, STIEL_UNTEN, STIEL_X, KNICK_Y) <= LW:
        return True
    if strecke(x, y, STIEL_X, KNICK_Y, QUER_BIS, KNICK_Y) <= LW:
        return True
    # Der Kopf als volles Dreieck: Spitze rechts auf der Hoehe des Knicks.
    if KOPF_X0 <= x <= KOPF_X1:
        anteil = (KOPF_X1 - x) / (KOPF_X1 - KOPF_X0)
        if abs(y - KNICK_Y) <= KOPF_HALB * anteil:
            return True
    return False


def raster(test):
    """Vierfach ueberabtasten, bei halber Deckung schneiden. Harte Kanten."""
    grid = []
    for py in range(H):
        row = []
        for px in range(W):
            hits = 0
            for sy in range(SS):
                for sx in range(SS):
                    if test(px + (sx + 0.5) / SS, py + (sy + 0.5) / SS):
                        hits += 1
            row.append(hits * 2 >= SS * SS)
        grid.append(row)
    return grid


def write(dest, grid):
    rows = []
    for y in range(H):
        r = []
        for x in range(W):
            r += [0, 0, 0, 255] if grid[y][x] else [0, 0, 0, 0]
        rows.append(r)
    png(os.path.join(dest, "system_icon.png"), W, H, rows)
    n = sum(1 for r in grid for v in r if v)
    ys = [y for y in range(H) if any(grid[y])]
    print("system_icon.png: %d Punkte schwarz, %d hoch (Vorbild: 180 / 24)"
          % (n, (ys[-1] - ys[0] + 1) if ys else 0))


if __name__ == "__main__":
    ziel = sys.argv[1] if len(sys.argv) > 1 else "."
    os.makedirs(ziel, exist_ok=True)
    write(ziel, raster(inside))
