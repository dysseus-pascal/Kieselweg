#include <pebble.h>
#include "nav_window.h"
#include "theme.h"
#include "weg.h"

// Ab hier gilt der Stand als kalt: die Navigation ist entweder beendet oder
// das Telefon ist weg. Zwei Minuten sind grosszuegig - Google Maps schiebt
// waehrend der Fahrt im Sekundentakt nach, OsmAnd seltener.
#define KW_KALT_S 120

static Window *s_window;
static Layer *s_leinwand;

// Masse. Eine Navigationsanzeige liest man im Vorbeischauen, deshalb ist die
// Entfernung so gross wie moeglich und alles andere so klein wie noetig.
// Auf der runden Uhr verengt sich der Schirm nach oben und unten. Mit dem
// Rand der eckigen wurde die Strasse links angeschnitten - im Emulator
// gesehen. Dort deshalb ein Vielfaches davon, und mittig statt linksbuendig:
// eine linksbuendige Zeile beginnt auf einem Kreis auf jeder Hoehe woanders.
#define RAND       PBL_IF_ROUND_ELSE(26, (KW_BREIT ? 8 : 5))
#define AUSR       PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft)
#define ZAHL_H     (KW_BREIT ? 56 : 42)
#define ANW_H      (KW_BREIT ? 30 : 22)
#define STR_H      (KW_BREIT ? 22 : 18)
#define BALKEN_H   (KW_BREIT ? 6 : 4)

static char s_zahl[16];

static GFont prv_font_zahl(void) {
  return fonts_get_system_font(KW_BREIT ? FONT_KEY_LECO_42_NUMBERS
                                        : FONT_KEY_LECO_32_BOLD_NUMBERS);
}

/**
 * Die Entfernung ganz gross.
 *
 * LECO ist die Ziffernschrift der Pebble-Timeline und hat KEINE Buchstaben -
 * "250 m" wuerde als "250" mit einer Luecke erscheinen. Deshalb zwei Stuecke:
 * die Zahl in LECO, die Einheit daneben in GOTHIC.
 */
static void prv_zeichne_entfernung(GContext *ctx, GRect b, int16_t y) {
  weg_entfernung_text(s_zahl, sizeof(s_zahl));

  // Zahl und Einheit trennen: alles bis zum ersten Leerzeichen ist die Zahl.
  char zahl[12] = {0};
  const char *einheit = "";
  const char *leer = strchr(s_zahl, ' ');
  if (leer) {
    const size_t n = (size_t)(leer - s_zahl);
    strncpy(zahl, s_zahl, n < sizeof(zahl) - 1 ? n : sizeof(zahl) - 1);
    einheit = leer + 1;
  } else {
    strncpy(zahl, s_zahl, sizeof(zahl) - 1);
  }

  const GFont fz = prv_font_zahl();
  const GFont fe = fonts_get_system_font(KW_BREIT ? FONT_KEY_GOTHIC_24_BOLD
                                                  : FONT_KEY_GOTHIC_18_BOLD);
  const GSize sz = graphics_text_layout_get_content_size(
      zahl, fz, GRect(0, 0, b.size.w, ZAHL_H),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
  const GSize se = graphics_text_layout_get_content_size(
      einheit, fe, GRect(0, 0, b.size.w, ZAHL_H),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);

  // Beide zusammen mittig setzen, damit die Zeile nicht wandert, wenn aus
  // "980 m" ein "1.2 km" wird.
  const int16_t breite = sz.w + (einheit[0] ? se.w + 4 : 0);
  int16_t x = (b.size.w - breite) / 2;
  if (x < RAND) x = RAND;

  graphics_context_set_text_color(ctx, KW_COLOR_TEXT);
  graphics_draw_text(ctx, zahl, fz, GRect(x, y, sz.w + 4, ZAHL_H),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  if (einheit[0]) {
    // Die Einheit sitzt auf der Grundlinie der Ziffern, nicht auf ihrer
    // Oberkante - sonst schwebt sie.
    graphics_draw_text(ctx, einheit, fe,
                       GRect(x + sz.w + 4, y + ZAHL_H - se.h - (KW_BREIT ? 10 : 7),
                             se.w + 4, se.h + 4),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  }

  // WORAUF SICH DIE ZAHL BEZIEHT. Steht nur da, wenn sie NICHT die Entfernung
  // zur naechsten Abzweigung ist - sonst waere es eine Selbstverstaendlichkeit
  // im Blickfeld. Ohne den Zusatz haelt man aber den Rest der Gesamtstrecke
  // fuer den Abstand zur Abzweigung, und das ist ein Irrtum mit Folgen.
  const char *bezug = weg_bezug();
  if (bezug[0]) {
    graphics_context_set_text_color(ctx, KW_COLOR_ZART);
    graphics_draw_text(ctx, bezug,
                       fonts_get_system_font(KW_BREIT ? FONT_KEY_GOTHIC_14
                                                      : FONT_KEY_GOTHIC_14),
                       GRect(RAND, y + ZAHL_H - (KW_BREIT ? 4 : 2), b.size.w - 2 * RAND, 18),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }
}

static void prv_zeichne(Layer *layer, GContext *ctx) {
  const GRect b = layer_get_bounds(layer);
  const Weg *w = weg_stand();
  const int alter = weg_alter_s();

  graphics_context_set_fill_color(ctx, KW_COLOR_BG);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  graphics_context_set_text_color(ctx, KW_COLOR_TEXT);

  const int16_t breite = b.size.w - 2 * RAND;

  // --- Noch nie etwas empfangen ---
  if (alter < 0) {
    const GFont f = fonts_get_system_font(KW_BREIT ? FONT_KEY_GOTHIC_24_BOLD
                                                   : FONT_KEY_GOTHIC_18_BOLD);
    const GFont fz = fonts_get_system_font(KW_BREIT ? FONT_KEY_GOTHIC_18
                                                    : FONT_KEY_GOTHIC_14);
    graphics_draw_text(ctx, "Wartet auf\ndas Telefon", f,
                       GRect(RAND, b.size.h / 3, breite, ANW_H * 3),
                       GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    graphics_context_set_text_color(ctx, KW_COLOR_ZART);
    graphics_draw_text(ctx, "Kiesel-Helper schickt,\nwas die Karten-App meldet.", fz,
                       GRect(RAND, b.size.h / 3 + ANW_H * 2, breite, STR_H * 3),
                       GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    return;
  }

  int16_t y = PBL_IF_ROUND_ELSE(34, 10);

  prv_zeichne_entfernung(ctx, b, y);
  // Der Bezug unter der Zahl braucht eigenen Platz - ohne diese Zeile laege
  // der Balken darauf.
  y += ZAHL_H + (KW_BREIT ? 2 : 0) + (weg_bezug()[0] ? (KW_BREIT ? 16 : 14) : 0);

  // --- Balken ---
  // Was er zeigt, haengt davon ab, was die Quelle hergibt: bei einer
  // Entfernung zur Abzweigung laeuft er unter 300 m voll, bei blossem
  // Streckenfortschritt zeigt er den zurueckgelegten Teil. weg_balken_prozent
  // entscheidet das, damit hier keine zweite Fassung derselben Regel steht.
  const int proz = weg_balken_prozent();
  if (proz >= 0) {
    const int16_t voll = (int16_t)((int32_t)proz * breite / 100);
    graphics_context_set_fill_color(ctx, KW_COLOR_BALKEN_BG);
    graphics_fill_rect(ctx, GRect(RAND, y, breite, BALKEN_H), 0, GCornerNone);
    graphics_context_set_fill_color(ctx, KW_COLOR_BALKEN);
    graphics_fill_rect(ctx, GRect(RAND, y, voll, BALKEN_H), 0, GCornerNone);
  }
  y += BALKEN_H + (KW_BREIT ? 8 : 5);

  // --- Die Anweisung ---
  graphics_context_set_text_color(ctx, KW_COLOR_TEXT);
  const GFont fa = fonts_get_system_font(KW_BREIT ? FONT_KEY_GOTHIC_28_BOLD
                                                  : FONT_KEY_GOTHIC_18_BOLD);
  graphics_draw_text(ctx, w->anweisung[0] ? w->anweisung : "—", fa,
                     GRect(RAND, y, breite, ANW_H * 3),
                     GTextOverflowModeWordWrap, AUSR, NULL);
  y += ANW_H * 2;

  // --- Strasse, wenn eine kam ---
  if (w->zusatz[0]) {
    graphics_context_set_text_color(ctx, KW_COLOR_ZART);
    graphics_draw_text(ctx, w->zusatz,
                       fonts_get_system_font(KW_BREIT ? FONT_KEY_GOTHIC_24
                                                      : FONT_KEY_GOTHIC_14),
                       GRect(RAND, y, breite, STR_H * 2),
                       GTextOverflowModeTrailingEllipsis, AUSR, NULL);
  }

  // --- Fuss: nur wenn der Stand kalt ist ---
  // Solange nachgeschoben wird, sagt ein Zeitstempel nichts. Bleibt er aber
  // stehen, MUSS man es sehen - sonst faehrt man nach einer Anweisung, die
  // seit zehn Minuten ueberholt ist.
  if (alter > KW_KALT_S) {
    char fuss[32];
    snprintf(fuss, sizeof(fuss), "vor %d min", alter / 60);
    graphics_context_set_text_color(ctx, KW_COLOR_ZART);
    graphics_draw_text(ctx, fuss,
                       fonts_get_system_font(KW_BREIT ? FONT_KEY_GOTHIC_18
                                                      : FONT_KEY_GOTHIC_14),
                       GRect(RAND, b.size.h - PBL_IF_ROUND_ELSE(38, 22), breite, 20),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
  }
}

void nav_window_auffrischen(void) {
  if (s_leinwand) layer_mark_dirty(s_leinwand);
}

static void prv_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_leinwand = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_leinwand, prv_zeichne);
  layer_add_child(root, s_leinwand);
}

static void prv_unload(Window *window) {
  layer_destroy(s_leinwand);
  s_leinwand = NULL;
}

void nav_window_push(void) {
  s_window = window_create();
  window_set_background_color(s_window, KW_COLOR_BG);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = prv_load,
    .unload = prv_unload,
  });
  window_stack_push(s_window, true);
}

void nav_window_destroy(void) {
  if (s_window) {
    window_destroy(s_window);
    s_window = NULL;
  }
}
