#pragma once
#include <pebble.h>

// Was gerade auf dem Schirm steht. Eine Nachricht vom Telefon muss nicht alle
// Felder tragen - fehlt eines, bleibt das alte stehen.
#define KW_ANWEISUNG_LEN 64
#define KW_STRASSE_LEN   48

typedef struct {
  char anweisung[KW_ANWEISUNG_LEN];
  char strasse[KW_STRASSE_LEN];
  int32_t entfernung_m;   // -1 = unbekannt
  time_t ankunft;         // 0 = unbekannt
  time_t empfangen;       // 0 = noch nichts empfangen
} Weg;

// Der eine Stand, den alle lesen.
const Weg *weg_stand(void);

// Beim Start aus dem Speicher holen, damit ein Wiederoeffnen nicht leer ist.
void weg_lade(void);

// Uebernimmt, was in der Nachricht steht, und merkt sich den Stand.
// Rueckgabe true, wenn sich die ANWEISUNG geaendert hat - nur dann lohnt ein
// Summen am Handgelenk.
bool weg_uebernimm(DictionaryIterator *iter);

// Entfernung als Text: unter 1000 m in Metern, darueber in Kilometern mit
// einer Nachkommastelle. Schreibt in puf und gibt puf zurueck.
char *weg_entfernung_text(char *puf, size_t len);

// Wie alt ist der Stand in Sekunden? -1, wenn noch nichts da ist.
int weg_alter_s(void);
