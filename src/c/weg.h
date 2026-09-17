#pragma once
#include <pebble.h>

// Was gerade auf dem Schirm steht. Eine Nachricht vom Telefon muss nicht alle
// Felder tragen - fehlt eines, bleibt das alte stehen.
#define KW_ANWEISUNG_LEN 64
#define KW_ZUSATZ_LEN   48

typedef struct {
  char anweisung[KW_ANWEISUNG_LEN];
  char zusatz[KW_ZUSATZ_LEN];
  int32_t entfernung_m;      // -1 = unbekannt
  time_t ankunft;            // 0 = unbekannt
  // Fortschritt auf der GESAMTSTRECKE, in Metern. Google Maps schickt genau
  // das und keine Entfernung zur naechsten Abzweigung - sein "Live Update"
  // traegt android.progress von android.progressMax. Beides zusammen ergibt,
  // was noch vor einem liegt; die Uhr rechnet es aus, nicht der Zettel.
  int32_t fortschritt_m;     // -1 = unbekannt
  int32_t fortschritt_max_m; // -1 = unbekannt
  time_t empfangen;          // 0 = noch nichts empfangen
} Weg;

// Der eine Stand, den alle lesen.
const Weg *weg_stand(void);

// Beim Start aus dem Speicher holen, damit ein Wiederoeffnen nicht leer ist.
void weg_lade(void);

// Uebernimmt, was in der Nachricht steht, und merkt sich den Stand.
// Rueckgabe true, wenn sich die ANWEISUNG geaendert hat - nur dann lohnt ein
// Summen am Handgelenk.
bool weg_uebernimm(DictionaryIterator *iter);

// Die Zahl, die gross auf dem Schirm steht, als Text: entweder die Entfernung
// zur naechsten Abzweigung oder - wenn die Quelle die nicht kennt - der Rest
// der Gesamtstrecke. Unter 1000 m in Metern, darueber in Kilometern mit einer
// Nachkommastelle. Schreibt in puf und gibt puf zurueck.
char *weg_entfernung_text(char *puf, size_t len);

// Was die grosse Zahl bedeutet - die Uhr schreibt es klein daneben, damit
// niemand den Rest der Strecke fuer den Abstand zur Abzweigung haelt.
const char *weg_bezug(void);

// Der Balken: 0..100, oder -1 wenn keiner zu zeichnen ist.
int weg_balken_prozent(void);

// Gibt es ueberhaupt eine Zahl zu zeigen? Nach dem Ende einer Navigation
// nicht mehr - dann steht nur die Anweisung da.
bool weg_hat_zahl(void);

// Wie alt ist der Stand in Sekunden? -1, wenn noch nichts da ist.
int weg_alter_s(void);

// Ist der Stand so alt, dass er keine Navigation mehr ist? Ein gespeicherter
// Stand von gestern ist keine Anweisung, sondern eine Erinnerung.
bool weg_veraltet(void);

// Das Alter in Worten: "vor 12 min", "vor 3 Std", "vor 2 Tagen".
char *weg_alter_text(char *puf, size_t len);
