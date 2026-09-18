#include <pebble.h>
#include "pfeil.h"
#include "theme.h"

// EIN Pfeil, achtmal gedreht. Ein Abbiegepfeil ist nichts anderes als ein
// gerader Pfeil in einem anderen Winkel, und acht Richtungen aus einer Form
// sind acht Formen weniger, die auseinanderlaufen koennen.
static GPoint s_punkte[7];
static GPathInfo s_umriss = { .num_points = 7, .points = s_punkte };
static GPath *s_pfad;

// Halbe Hoehe, halbe Kopfbreite, halbe Schaftbreite, Kopfhoehe.
#define P_H  (KW_BREIT ? 17 : 12)
#define P_K  (KW_BREIT ? 12 :  9)
#define P_S  (KW_BREIT ?  5 :  4)
#define P_KH (KW_BREIT ? 16 : 11)

void pfeil_init(void) {
  if (s_pfad) return;
  const int16_t h = P_H, k = P_K, s = P_S, kh = P_KH;
  const int16_t basis = (int16_t)(-h + kh);   // Unterkante des Kopfes
  s_punkte[0] = GPoint(0, (int16_t)-h);       // Spitze
  s_punkte[1] = GPoint(k, basis);
  s_punkte[2] = GPoint(s, basis);
  s_punkte[3] = GPoint(s, h);
  s_punkte[4] = GPoint((int16_t)-s, h);
  s_punkte[5] = GPoint((int16_t)-s, basis);
  s_punkte[6] = GPoint((int16_t)-k, basis);
  s_pfad = gpath_create(&s_umriss);
}

void pfeil_deinit(void) {
  if (s_pfad) {
    gpath_destroy(s_pfad);
    s_pfad = NULL;
  }
}

int16_t pfeil_breite(void) { return (int16_t)(2 * P_K); }

int16_t pfeil_hoehe(void) { return (int16_t)(2 * P_H); }

// --- Den Text deuten ---

static bool prv_buchstabe(unsigned char c) {
  // Alles jenseits von ASCII gilt als Buchstabe: Umlaute sind in UTF-8 zwei
  // Bytes, und wer sie als Trennzeichen behandelt, zerlegt "für" mitten drin.
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c > 0x7F;
}

static char prv_klein(char c) {
  return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

/**
 * Kommt `nadel` als eigenes Wort vor?
 *
 * Die Wortgrenze ist der Punkt. Ohne sie faende "rechts" sich auch in
 * "Rechtsweg" wieder, und die Uhr schickte einen ins Abbiegen, weil die
 * Strasse so heisst.
 */
static bool prv_wort(const char *heu, const char *nadel) {
  const size_t n = strlen(nadel);
  for (size_t i = 0; heu[i]; i++) {
    size_t j = 0;
    while (j < n && heu[i + j] && prv_klein(heu[i + j]) == nadel[j]) j++;
    if (j != n) continue;
    const bool davor = (i == 0) || !prv_buchstabe((unsigned char)heu[i - 1]);
    const bool danach = !prv_buchstabe((unsigned char)heu[i + n]);
    if (davor && danach) return true;
  }
  return false;
}

Richtung pfeil_richtung(const char *anweisung) {
  if (!anweisung || !anweisung[0]) return RICHTUNG_UNBEKANNT;

  // DIE REIHENFOLGE IST DIE REGEL. "scharf rechts" enthaelt "rechts"; wer
  // zuerst auf das kurze Wort prueft, findet nie das lange.
  if (prv_wort(anweisung, "wenden") || prv_wort(anweisung, "u-turn") ||
      prv_wort(anweisung, "umkehren")) {
    return RICHTUNG_WENDEN;
  }
  if (prv_wort(anweisung, "ziel") || prv_wort(anweisung, "destination") ||
      prv_wort(anweisung, "arrive") || prv_wort(anweisung, "angekommen")) {
    return RICHTUNG_ZIEL;
  }
  if (prv_wort(anweisung, "scharf")) {
    if (prv_wort(anweisung, "rechts")) return RICHTUNG_SCHARF_RECHTS;
    if (prv_wort(anweisung, "links")) return RICHTUNG_SCHARF_LINKS;
  }
  if (prv_wort(anweisung, "sharp")) {
    if (prv_wort(anweisung, "right")) return RICHTUNG_SCHARF_RECHTS;
    if (prv_wort(anweisung, "left")) return RICHTUNG_SCHARF_LINKS;
  }
  if (prv_wort(anweisung, "leicht") || prv_wort(anweisung, "slight") ||
      prv_wort(anweisung, "halten") || prv_wort(anweisung, "keep") ||
      prv_wort(anweisung, "bear")) {
    if (prv_wort(anweisung, "rechts") || prv_wort(anweisung, "right")) {
      return RICHTUNG_LEICHT_RECHTS;
    }
    if (prv_wort(anweisung, "links") || prv_wort(anweisung, "left")) {
      return RICHTUNG_LEICHT_LINKS;
    }
  }
  if (prv_wort(anweisung, "rechts") || prv_wort(anweisung, "right")) {
    return RICHTUNG_RECHTS;
  }
  if (prv_wort(anweisung, "links") || prv_wort(anweisung, "left")) {
    return RICHTUNG_LINKS;
  }
  // "Head toward Bahnhofstrasse", "geradeaus weiter", "continue on"
  if (prv_wort(anweisung, "geradeaus") || prv_wort(anweisung, "straight") ||
      prv_wort(anweisung, "weiter") || prv_wort(anweisung, "continue") ||
      prv_wort(anweisung, "head") || prv_wort(anweisung, "folgen") ||
      prv_wort(anweisung, "follow")) {
    return RICHTUNG_GERADE;
  }
  return RICHTUNG_UNBEKANNT;
}

// --- Zeichnen ---

static int32_t prv_winkel(Richtung r) {
  switch (r) {
    case RICHTUNG_GERADE:         return 0;
    case RICHTUNG_LEICHT_RECHTS:  return TRIG_MAX_ANGLE / 8;
    case RICHTUNG_RECHTS:         return TRIG_MAX_ANGLE / 4;
    case RICHTUNG_SCHARF_RECHTS:  return TRIG_MAX_ANGLE * 3 / 8;
    case RICHTUNG_WENDEN:         return TRIG_MAX_ANGLE / 2;
    case RICHTUNG_SCHARF_LINKS:   return TRIG_MAX_ANGLE * 5 / 8;
    case RICHTUNG_LINKS:          return TRIG_MAX_ANGLE * 3 / 4;
    case RICHTUNG_LEICHT_LINKS:   return TRIG_MAX_ANGLE * 7 / 8;
    default:                      return 0;
  }
}

void pfeil_zeichne(GContext *ctx, GPoint mitte, Richtung r) {
  if (r == RICHTUNG_UNBEKANNT) return;

  graphics_context_set_fill_color(ctx, KW_COLOR_TEXT);

  if (r == RICHTUNG_ZIEL) {
    // Kein Pfeil, sondern ein Ring: am Ziel gibt es keine Richtung mehr.
    const int16_t aussen = P_K;
    graphics_fill_circle(ctx, mitte, aussen);
    graphics_context_set_fill_color(ctx, KW_COLOR_BG);
    graphics_fill_circle(ctx, mitte, (int16_t)(aussen - (KW_BREIT ? 4 : 3)));
    graphics_context_set_fill_color(ctx, KW_COLOR_TEXT);
    graphics_fill_circle(ctx, mitte, (int16_t)(KW_BREIT ? 4 : 3));
    return;
  }

  if (!s_pfad) pfeil_init();
  gpath_rotate_to(s_pfad, prv_winkel(r));
  gpath_move_to(s_pfad, mitte);
  gpath_draw_filled(ctx, s_pfad);
}
