#include <pebble.h>
#include "weg.h"
#include "kern.h"

#define PERSIST_WEG 1

static Weg s_weg;

const Weg *weg_stand(void) { return &s_weg; }

void weg_lade(void) {
  s_weg.entfernung_m = -1;
  s_weg.fortschritt_m = -1;
  s_weg.fortschritt_max_m = -1;
  if (persist_exists(PERSIST_WEG)) {
    persist_read_data(PERSIST_WEG, &s_weg, sizeof(s_weg));
  }
  // Ein gespeicherter Stand ist beim Oeffnen besser als ein leerer Schirm,
  // aber er ist KEINE gueltige Navigation mehr. Das Alter sagt es; die
  // Anzeige zeigt es an.
}

/**
 * Eine Zeichenkette uebernehmen.
 *
 * Fehlt der Schluessel, bleibt das alte Feld stehen - so kann das Telefon
 * die Entfernung nachschieben, ohne die Anweisung mitzuschicken.
 *
 * STEHT DER SCHLUESSEL ABER DA UND IST LEER, WIRD GELOESCHT. Frueher wurde
 * auch das verworfen, aus Angst vor einer Standmeldung, die versehentlich
 * Leeres schickt und die gueltige Anweisung wegwischt. Diese Angst war
 * begruendet, solange irgendwer beliebige Nachrichten schickte; seit die
 * Zettel Fassung 2 sprechen, ist sie es nicht mehr: aus einer Quelle kann
 * gar kein leerer Text kommen, der wird schon drueben verworfen. Leer
 * bekommt man nur, indem man es hinschreibt.
 *
 * Und man braucht es: "Navigation beendet" mit der Strasse von vorhin
 * darunter ist schlimmer als gar nichts. Genau das stand bisher da.
 */
static bool prv_nimm_text(DictionaryIterator *iter, uint32_t key, char *ziel, size_t len) {
  Tuple *t = dict_find(iter, key);
  if (!t || t->type != TUPLE_CSTRING) return false;
  // Bei laenge 0 ist cstring nicht zu gebrauchen - dann selber eine leere
  // Zeichenkette nehmen, statt ins Leere zu lesen.
  const char *neu = (t->length <= 1) ? "" : t->value->cstring;
  const bool anders = (strncmp(ziel, neu, len) != 0);
  strncpy(ziel, neu, len - 1);
  ziel[len - 1] = '\0';
  return anders;
}

bool weg_uebernimm(DictionaryIterator *iter) {
  // NICHT die Texte vergleichen, sondern ihre Kerne. Google Maps schreibt die
  // Entfernung in den Anweisungstext; er aendert sich damit bei jedem Takt,
  // obwohl der Schritt derselbe bleibt. Siehe kern.h.
  char vorher[KW_ANWEISUNG_LEN];
  kern(s_weg.anweisung, vorher, sizeof(vorher));

  prv_nimm_text(iter, MESSAGE_KEY_ANWEISUNG, s_weg.anweisung, KW_ANWEISUNG_LEN);
  prv_nimm_text(iter, MESSAGE_KEY_ZUSATZ, s_weg.zusatz, KW_ZUSATZ_LEN);

  char nachher[KW_ANWEISUNG_LEN];
  kern(s_weg.anweisung, nachher, sizeof(nachher));
  const bool neu = (strcmp(vorher, nachher) != 0);
  APP_LOG(APP_LOG_LEVEL_INFO, "Kern '%s' -> '%s' : %s",
          vorher, nachher, neu ? "NEUER SCHRITT" : "derselbe");

  Tuple *e = dict_find(iter, MESSAGE_KEY_ENTFERNUNG);
  if (e && e->type == TUPLE_INT) s_weg.entfernung_m = e->value->int32;

  Tuple *a = dict_find(iter, MESSAGE_KEY_ANKUNFT);
  if (a && a->type == TUPLE_INT) s_weg.ankunft = (time_t)a->value->int32;

  Tuple *f = dict_find(iter, MESSAGE_KEY_FORTSCHRITT);
  if (f && f->type == TUPLE_INT) s_weg.fortschritt_m = f->value->int32;
  Tuple *fm = dict_find(iter, MESSAGE_KEY_FORTSCHRITT_MAX);
  if (fm && fm->type == TUPLE_INT) s_weg.fortschritt_max_m = fm->value->int32;

  // DIE BEIDEN SCHLIESSEN EINANDER AUS, und wer zuletzt kommt, gilt.
  //
  // Sonst bleibt ein Wert stehen, den niemand mehr meint: erst zeigt OsmAnd
  // 80 Meter bis zur Abzweigung, dann uebernimmt Google Maps, das nur den
  // Streckenfortschritt kennt - und die Uhr zeigt weiter 80 Meter, fuer immer.
  // Genau so ist es im Emulator passiert. Dass sonst jedes Feld stehen bleibt,
  // wenn es fehlt, ist richtig; hier waere es ein Irrtum mit Folgen.
  if (f || fm) {
    s_weg.entfernung_m = -1;
  } else if (e) {
    s_weg.fortschritt_m = -1;
    s_weg.fortschritt_max_m = -1;
  }

  s_weg.empfangen = time(NULL);
  persist_write_data(PERSIST_WEG, &s_weg, sizeof(s_weg));
  return neu;
}

/**
 * Welche Zahl gross dasteht.
 *
 * Die Entfernung zur naechsten Abzweigung, wenn die Quelle sie kennt - das
 * ist die nuetzlichere. Kennt sie nur den Fortschritt auf der Gesamtstrecke,
 * wie Google Maps, bleibt der Rest bis zum Ziel. Beides in derselben Zeile
 * anzuzeigen waere falsch; welches gerade gilt, sagt weg_bezug().
 */
static int32_t prv_grosse_zahl(void) {
  if (s_weg.entfernung_m >= 0) return s_weg.entfernung_m;
  if (s_weg.fortschritt_max_m > 0 && s_weg.fortschritt_m >= 0) {
    const int32_t rest = s_weg.fortschritt_max_m - s_weg.fortschritt_m;
    return rest > 0 ? rest : 0;
  }
  return -1;
}

bool weg_hat_zahl(void) { return prv_grosse_zahl() >= 0; }

const char *weg_bezug(void) {
  if (s_weg.entfernung_m >= 0) return "";
  if (s_weg.fortschritt_max_m > 0) return "bis zum Ziel";
  return "";
}

int weg_balken_prozent(void) {
  // Naechste Abzweigung: der Balken laeuft unter 300 m voll. Das ist die
  // Spanne, in der ein Blick aufs Handgelenk noch etwas aendert.
  if (s_weg.entfernung_m >= 0) {
    if (s_weg.entfernung_m >= 300) return -1;
    return (int)((300 - s_weg.entfernung_m) * 100 / 300);
  }
  // Gesamtstrecke: der Balken ist die gefahrene Strecke.
  if (s_weg.fortschritt_max_m > 0 && s_weg.fortschritt_m >= 0) {
    const int p = (int)((int64_t)s_weg.fortschritt_m * 100 / s_weg.fortschritt_max_m);
    return p < 0 ? 0 : (p > 100 ? 100 : p);
  }
  return -1;
}

char *weg_entfernung_text(char *puf, size_t len) {
  const int32_t m = prv_grosse_zahl();
  if (m < 0) {
    snprintf(puf, len, "--");
  } else if (m < 1000) {
    // Unter einem Kilometer zaehlen Meter, und zwar ganze. Eine Zahl mit
    // Nachkommastelle waere hier nur Unruhe im Blickfeld.
    snprintf(puf, len, "%d m", (int)m);
  } else {
    // Darueber in Kilometern mit einer Stelle. Pebble kann kein %f, also von
    // Hand: 1234 m -> "1,2 km". KOMMA und nicht Punkt: die Anweisung daneben
    // kommt aus einer deutschen Karten-App und schreibt "1,2 km" - zwei
    // Schreibweisen derselben Zahl auf demselben Schirm sind ein Fehler.
    snprintf(puf, len, "%d,%d km", (int)(m / 1000), (int)((m % 1000) / 100));
  }
  return puf;
}

/**
 * Ab wann ein Stand keine Navigation mehr ist.
 *
 * Zehn Minuten sind grosszuegig: ein Tunnel, eine Pause an der Ampel, ein
 * Telefon, das kurz den Empfang verliert - all das soll die Anzeige nicht
 * loeschen. Ein Stand von gestern dagegen ist keine Anweisung mehr, sondern
 * eine Erinnerung, und er darf sich nicht als Anweisung ausgeben.
 */
#define KW_VERALTET_S 600

bool weg_veraltet(void) {
  const int a = weg_alter_s();
  return a < 0 || a > KW_VERALTET_S;
}

char *weg_alter_text(char *puf, size_t len) {
  const int a = weg_alter_s();
  if (a < 0) {
    puf[0] = 0;   // leere Zeichenkette - snprintf mit "" ist ein Fehler
  } else if (a < 60) {
    snprintf(puf, len, "vor %d s", a);
  } else if (a < 3600) {
    snprintf(puf, len, "vor %d min", a / 60);
  } else if (a < 86400) {
    // Ab einer Stunde in Stunden. "vor 1440 min" ist zwar richtig, aber
    // niemand rechnet das im Vorbeischauen um.
    snprintf(puf, len, "vor %d Std", a / 3600);
  } else {
    const int tage = a / 86400;
    snprintf(puf, len, tage == 1 ? "gestern" : "vor %d Tagen", tage);
  }
  return puf;
}

int weg_alter_s(void) {
  if (s_weg.empfangen == 0) return -1;
  const int alter = (int)(time(NULL) - s_weg.empfangen);
  return alter < 0 ? 0 : alter;
}
