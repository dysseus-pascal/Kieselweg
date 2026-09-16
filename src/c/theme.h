#pragma once
#include <pebble.h>

// Schwarz auf Weiss im Stil der Pebble-Timeline, wie bei den Geschwistern.
// Eine Navigationsanzeige liest man im Gehen und im Vorbeischauen - da zaehlt
// Kontrast mehr als Farbe. Der einzige Farbtupfer ist der Balken, der die
// verbleibende Entfernung zeigt.
#define KW_COLOR_BG        GColorWhite
#define KW_COLOR_TEXT      GColorBlack
#define KW_COLOR_ZART      PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack)
#define KW_COLOR_BALKEN    PBL_IF_COLOR_ELSE(GColorCobaltBlue, GColorBlack)
#define KW_COLOR_BALKEN_BG PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite)

// Breit heisst emery (200 Punkte) und gabbro (260); schmal ist flint mit 144.
// Die Schriftgroessen unterscheiden sich deutlich - was auf emery gut aussieht,
// laeuft auf flint aus dem Bild.
#define KW_BREIT (PBL_DISPLAY_WIDTH >= 180)
