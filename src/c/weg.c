#include <pebble.h>
#include "weg.h"

#define PERSIST_WEG 1

static Weg s_weg;

const Weg *weg_stand(void) { return &s_weg; }

void weg_lade(void) {
  s_weg.entfernung_m = -1;
  if (persist_exists(PERSIST_WEG)) {
    persist_read_data(PERSIST_WEG, &s_weg, sizeof(s_weg));
  }
  // Ein gespeicherter Stand ist beim Oeffnen besser als ein leerer Schirm,
  // aber er ist KEINE gueltige Navigation mehr. Das Alter sagt es; die
  // Anzeige zeigt es an.
}

/**
 * Eine Zeichenkette uebernehmen - aber nur, wenn sie wirklich dasteht.
 *
 * dict_find gibt auch fuer ein leeres Feld einen Eintrag zurueck. Wer das
 * nicht prueft, loescht mit der naechsten Standmeldung die Anweisung, die
 * noch gilt.
 */
static bool prv_nimm_text(DictionaryIterator *iter, uint32_t key, char *ziel, size_t len) {
  Tuple *t = dict_find(iter, key);
  if (!t || t->type != TUPLE_CSTRING || t->length <= 1) return false;
  const bool anders = (strncmp(ziel, t->value->cstring, len) != 0);
  strncpy(ziel, t->value->cstring, len - 1);
  ziel[len - 1] = '\0';
  return anders;
}

bool weg_uebernimm(DictionaryIterator *iter) {
  const bool neu = prv_nimm_text(iter, MESSAGE_KEY_ANWEISUNG,
                                s_weg.anweisung, KW_ANWEISUNG_LEN);
  prv_nimm_text(iter, MESSAGE_KEY_STRASSE, s_weg.strasse, KW_STRASSE_LEN);

  Tuple *e = dict_find(iter, MESSAGE_KEY_ENTFERNUNG);
  if (e && e->type == TUPLE_INT) s_weg.entfernung_m = e->value->int32;

  Tuple *a = dict_find(iter, MESSAGE_KEY_ANKUNFT);
  if (a && a->type == TUPLE_INT) s_weg.ankunft = (time_t)a->value->int32;

  s_weg.empfangen = time(NULL);
  persist_write_data(PERSIST_WEG, &s_weg, sizeof(s_weg));
  return neu;
}

char *weg_entfernung_text(char *puf, size_t len) {
  const int32_t m = s_weg.entfernung_m;
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

int weg_alter_s(void) {
  if (s_weg.empfangen == 0) return -1;
  const int alter = (int)(time(NULL) - s_weg.empfangen);
  return alter < 0 ? 0 : alter;
}
