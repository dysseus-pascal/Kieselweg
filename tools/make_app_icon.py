#!/usr/bin/env python3
"""App-Symbol: der Abbiegepfeil.

Aufruf: make_app_icon.py <zielordner>          -> system_icon.png (25x25)
        make_app_icon.py --store <zielordner>  -> icon-144.png, icon-48.png

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

DER STORE NIMMT NICHTS AUS DER .pbw. Im Entwicklerportal liegen zwei eigene
Bilder, `icon_large` und `icon_small`; angefordert werden sie in festen Massen
(gross 80 und 144, klein 28 und 48), jeweils mit `exact` in der Adresse, also
erzwungen statt eingepasst - etwas Nicht-Quadratisches kommt verzogen zurueck.
Darum hier eine gefuellte Kachel: das grosse Symbol legt der Store fuer sein
Teilen-Bild durch eine abgerundete Maske, und ueber einer durchsichtigen
Strichzeichnung taete die nichts.

DIE FORM STEHT NUR EINMAL DA. Alle Masse gelten auf einem Raster von 25
Punkten und werden mit s hochgerechnet; mit s = 1 kommt Punkt fuer Punkt das
alte Bild heraus.
"""
import math
import os
import struct
import sys
import zlib

RASTER = 25                      # Bezugsraster, auf dem alle Masse gelten
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

# Store-Kachel. Der Wert stammt aus src/c/theme.h (KW_COLOR_BALKEN),
# nachgeschlagen in gcolor_definitions.h des SDK - nicht aus dem Gedaechtnis.
GRUND = (0x00, 0x55, 0xAA)       # GColorCobaltBlue
STRICH = (0xFF, 0xFF, 0xFF)      # weiss
FUELL = 0.72                     # wie viel der Kachel der Pfeil einnimmt
STORE_GROESSEN = (144, 48)


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


def pruefer(s):
    """Der Formtest, auf den Massstab s gebracht."""
    lw = LW * s
    stiel_x, stiel_unten = STIEL_X * s, STIEL_UNTEN * s
    knick_y, quer_bis = KNICK_Y * s, QUER_BIS * s
    kopf_x0, kopf_x1, kopf_halb = KOPF_X0 * s, KOPF_X1 * s, KOPF_HALB * s

    def inside(x, y):
        # Stiel und Querbalken als eine Linie mit Knick.
        if strecke(x, y, stiel_x, stiel_unten, stiel_x, knick_y) <= lw:
            return True
        if strecke(x, y, stiel_x, knick_y, quer_bis, knick_y) <= lw:
            return True
        # Der Kopf als volles Dreieck: Spitze rechts auf der Hoehe des Knicks.
        if kopf_x0 <= x <= kopf_x1:
            anteil = (kopf_x1 - x) / (kopf_x1 - kopf_x0)
            if abs(y - knick_y) <= kopf_halb * anteil:
                return True
        return False
    return inside


def raster(test, n):
    """Vierfach ueberabtasten, bei halber Deckung schneiden. Harte Kanten."""
    grid = []
    for py in range(n):
        row = []
        for px in range(n):
            hits = 0
            for sy in range(SS):
                for sx in range(SS):
                    if test(px + (sx + 0.5) / SS, py + (sy + 0.5) / SS):
                        hits += 1
            row.append(hits * 2 >= SS * SS)
        grid.append(row)
    return grid


def schreibe_uhr(dest):
    n = RASTER
    grid = raster(pruefer(1.0), n)
    rows = []
    for y in range(n):
        r = []
        for x in range(n):
            r += [0, 0, 0, 255] if grid[y][x] else [0, 0, 0, 0]
        rows.append(r)
    png(os.path.join(dest, "system_icon.png"), n, n, rows)
    punkte = sum(1 for r in grid for v in r if v)
    ys = [y for y in range(n) if any(grid[y])]
    print("system_icon.png: %d Punkte schwarz, %d hoch (Vorbild: 180 / 24)"
          % (punkte, (ys[-1] - ys[0] + 1) if ys else 0))


def schreibe_store(dest):
    for gross in STORE_GROESSEN:
        innen = int(round(gross * FUELL))
        grid = raster(pruefer(innen / float(RASTER)), innen)
        rand = (gross - innen) // 2
        rows = []
        for y in range(gross):
            r = []
            for x in range(gross):
                iy, ix = y - rand, x - rand
                treffer = 0 <= iy < innen and 0 <= ix < innen and grid[iy][ix]
                farbe = STRICH if treffer else GRUND
                r += [farbe[0], farbe[1], farbe[2], 255]
            rows.append(r)
        name = "icon-%d.png" % gross
        png(os.path.join(dest, name), gross, gross, rows)
        print("%s: Kachel %s, Pfeil weiss" % (name, "#%02X%02X%02X" % GRUND))


def main():
    args = sys.argv[1:]
    store = "--store" in args
    if store:
        args.remove("--store")
    ziel = args[0] if args else ("store" if store else ".")
    os.makedirs(ziel, exist_ok=True)
    if store:
        schreibe_store(ziel)
    else:
        schreibe_uhr(ziel)


if __name__ == "__main__":
    main()
