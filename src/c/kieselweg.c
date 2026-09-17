#include <pebble.h>
#include "nav_window.h"
#include "weg.h"
#include "pfeil.h"

// Ein Woerterbuch mit zwei Zeichenketten und zwei Zahlen. 256 Byte lassen
// Luft, ohne dass jemand nachrechnen muss.
#define INBOX_SIZE  256
#define OUTBOX_SIZE 64

// Kuerzester Abstand zwischen zwei Summen.
//
// Das Netz unter dem Kern-Vergleich: der erkennt eine Entfernung im Text und
// laesst sie beim Vergleich weg - eine Quelle, die ihren Text anders umbaut,
// kann ihn trotzdem ueberlisten. Diesen Riegel kann sie nicht. Dreissig
// Sekunden sind laenger als jede Folge echter Abbiegungen und kuerzer als
// jede Strecke, auf der man eine verpassen wuerde.
#define KW_SUMM_PAUSE_S 30

static time_t s_zuletzt_gesummt;

/**
 * Was vom Telefon kommt.
 *
 * Geschickt wird es von Kiesel-Helper ueber die klassische
 * PebbleKit-Schnittstelle (com.getpebble.action.app.SEND). Diese App hat
 * deshalb ABSICHTLICH keinen companionApp-Eintrag in der package.json: ein
 * Paketname dort waehlt PebbleKit2, und das bindet sich an einen Dienst,
 * statt zu senden.
 */
static void prv_empfangen(DictionaryIterator *iter, void *context) {
  const bool neue_anweisung = weg_uebernimm(iter);
  nav_window_auffrischen();

  // Nur bei einem NEUEN Schritt summen, und hoechstens alle dreissig Sekunden.
  //
  // "Neu" heisst: der Kern der Anweisung hat sich geaendert, nicht ihr Text -
  // sonst summt die Uhr alle paar hundert Meter Autofahrt, weil Google Maps
  // die schrumpfende Entfernung in den Text schreibt. Am Steuer nachgewiesen,
  // nicht im Emulator: dort hatte ich denselben Text zweimal geschickt, in
  // Wirklichkeit kommt er nie zweimal gleich.
  if (neue_anweisung) {
    const time_t jetzt = time(NULL);
    if (s_zuletzt_gesummt == 0 || jetzt - s_zuletzt_gesummt >= KW_SUMM_PAUSE_S) {
      vibes_short_pulse();
      s_zuletzt_gesummt = jetzt;
    } else {
      APP_LOG(APP_LOG_LEVEL_INFO, "Summen unterdrueckt: erst %d s her",
              (int)(jetzt - s_zuletzt_gesummt));
    }
  }
}

static void prv_abgewiesen(AppMessageResult grund, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "Nachricht abgewiesen: %d", (int)grund);
}

static void prv_init(void) {
  weg_lade();
  pfeil_init();
  app_message_register_inbox_received(prv_empfangen);
  app_message_register_inbox_dropped(prv_abgewiesen);
  app_message_open(INBOX_SIZE, OUTBOX_SIZE);
  nav_window_push();
}

static void prv_deinit(void) {
  nav_window_destroy();
  pfeil_deinit();
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
