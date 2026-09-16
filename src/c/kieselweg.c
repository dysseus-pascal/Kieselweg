#include <pebble.h>
#include "nav_window.h"
#include "weg.h"

// Ein Woerterbuch mit zwei Zeichenketten und zwei Zahlen. 256 Byte lassen
// Luft, ohne dass jemand nachrechnen muss.
#define INBOX_SIZE  256
#define OUTBOX_SIZE 64

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

  // Nur bei einer NEUEN Anweisung summen. Waehrend der Fahrt schiebt die
  // Karten-App im Sekundentakt dieselbe Anweisung mit kleinerer Entfernung
  // nach - ein Summen je Meldung waere unbrauchbar.
  if (neue_anweisung) vibes_short_pulse();
}

static void prv_abgewiesen(AppMessageResult grund, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "Nachricht abgewiesen: %d", (int)grund);
}

static void prv_init(void) {
  weg_lade();
  app_message_register_inbox_received(prv_empfangen);
  app_message_register_inbox_dropped(prv_abgewiesen);
  app_message_open(INBOX_SIZE, OUTBOX_SIZE);
  nav_window_push();
}

static void prv_deinit(void) {
  nav_window_destroy();
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
