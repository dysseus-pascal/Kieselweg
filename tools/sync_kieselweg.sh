#!/bin/sh
# Quellen nach ~/kieselweg spiegeln und bauen (waf vertraegt keine Pfade mit
# Leerzeichen). Aufruf: sync_kieselweg.sh [<Quellordner>]; ohne Argument wird
# $KIESELWEG_SRC verwendet.
#
# Kein npm: diese App braucht kein Clay. Ihre Anzeige kommt nicht von einer
# Konfigseite, sondern von Kiesel-Helper ueber die klassische
# PebbleKit-Schnittstelle.
export PATH=$HOME/.local/bin:$PATH
SRC="${1:-$KIESELWEG_SRC}"
[ -d "$SRC" ] || { echo "Quellordner fehlt: '$SRC' (Argument oder KIESELWEG_SRC setzen)"; exit 1; }
DST=$HOME/kieselweg
mkdir -p "$DST"
rm -rf "$DST/src" "$DST/resources" "$DST/build"
cp -r "$SRC/src" "$SRC/resources" "$SRC/package.json" "$SRC/wscript" "$DST/"
cd "$DST" || exit 1
echo "Dateien in src/c: $(ls src/c | wc -l)"
pebble build 2>&1 | grep -iE 'error|warning: \.\./src|APP MEMORY|footprint in RAM|finished successfully|Build failed|Traceback'
