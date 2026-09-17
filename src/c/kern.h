#pragma once
#include <pebble.h>

/**
 * Den Kern einer Anweisung herausschaelen: Entfernungen werden zu '#'.
 *
 * WARUM DAS SEIN MUSS. Google Maps schreibt die Entfernung IN den Text:
 * "In 250 m rechts abbiegen". Beim naechsten Takt steht dort 200, dann 150 -
 * derselbe Schritt, jedes Mal ein anderer Text. Wer die Texte bloss
 * vergleicht, haelt jeden Takt fuer einen neuen Schritt und summt alle paar
 * hundert Meter Autofahrt. Genau so ist es passiert.
 *
 * Maskiert werden NUR Zahlen mit einer Laengeneinheit dahinter. "Ausfahrt 3"
 * und "Ausfahrt 4" bleiben verschieden - das sind wirklich zwei Schritte, und
 * wer sie zusammenwirft, verschweigt einen.
 *
 *   "In 250 m rechts abbiegen"  ->  "In # rechts abbiegen"
 *   "In 1,2 km rechts abbiegen" ->  "In # rechts abbiegen"
 *   "Ausfahrt 3 nehmen"         ->  "Ausfahrt 3 nehmen"
 */
void kern(const char *text, char *aus, size_t len);
