#pragma once
#include <pebble.h>

/**
 * Die Richtung, in die es gleich geht.
 *
 * WOHER SIE KOMMT: aus dem Anweisungstext. Google Maps schickt kein Feld
 * dafuer - die Richtung steht in Worten da ("rechts abbiegen", "turn left").
 * Ein Zettel koennte sie nicht herausrechnen; er reicht Werte weiter, mehr
 * nicht. Also deutet die Uhr den Text, und das ist die richtige Stelle: hier
 * steht Code, dort ein Zettel.
 *
 * Das Symbol in der Benachrichtigung waere der genauere Weg - Maps legt dort
 * ein fertiges Abbiegesymbol hinein. Ein Bild passt aber weder in das
 * Woerterbuch, das ein Zettel fuellen kann, noch durch die Senke "senden".
 * Der Text ist der Weg, der mit dem vorhandenen Werkzeug funktioniert.
 */
typedef enum {
  RICHTUNG_UNBEKANNT = 0,
  RICHTUNG_GERADE,
  RICHTUNG_LEICHT_RECHTS,
  RICHTUNG_RECHTS,
  RICHTUNG_SCHARF_RECHTS,
  RICHTUNG_WENDEN,
  RICHTUNG_SCHARF_LINKS,
  RICHTUNG_LINKS,
  RICHTUNG_LEICHT_LINKS,
  RICHTUNG_ZIEL,
} Richtung;

/** Den Pfad einmal bauen. Muss vor dem ersten Zeichnen laufen. */
void pfeil_init(void);
void pfeil_deinit(void);

/** Die Richtung aus dem Anweisungstext lesen. Deutsch und Englisch. */
Richtung pfeil_richtung(const char *anweisung);

/** Wie breit der Pfeil auf dieser Uhr ist - fuer die Aufteilung der Zeile. */
int16_t pfeil_breite(void);

/** Wie hoch er ist - fuer die Anzeige ohne Zahl, wo er ueber dem Text steht. */
int16_t pfeil_hoehe(void);

/** Zeichnet den Pfeil mittig um `mitte`. UNBEKANNT zeichnet nichts. */
void pfeil_zeichne(GContext *ctx, GPoint mitte, Richtung r);
