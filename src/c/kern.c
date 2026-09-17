#include <pebble.h>
#include "kern.h"

// Laengeneinheiten, laengste zuerst - sonst schluckt "m" den Anfang von
// "meter" und der Rest bleibt als Buchstabensalat stehen.
static const char *const s_einheiten[] = {
  "kilometers", "kilometer", "meters", "meter", "miles", "mile",
  "yards", "yard", "feet", "foot", "km", "mi", "ft", "yd", "m",
};

static bool prv_buchstabe(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static char prv_klein(char c) {
  return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

/**
 * Faengt der Text mit einer Einheit an? Rueckgabe ist ihre Laenge, sonst 0.
 *
 * Nach der Einheit muss ein Nicht-Buchstabe folgen. Ohne diese Bedingung
 * passte "m" auf den Anfang von "mitte", und aus "In 3 Minuten" wuerde ein
 * Kern, der mit jeder Minute anders aussieht.
 */
static size_t prv_einheit(const char *t) {
  for (size_t i = 0; i < ARRAY_LENGTH(s_einheiten); i++) {
    const char *e = s_einheiten[i];
    size_t n = 0;
    while (e[n] && prv_klein(t[n]) == e[n]) n++;
    if (e[n] == '\0' && !prv_buchstabe(t[n])) return n;
  }
  return 0;
}

void kern(const char *text, char *aus, size_t len) {
  size_t j = 0;
  size_t i = 0;
  while (text[i] && j + 1 < len) {
    if (text[i] < '0' || text[i] > '9') {
      aus[j++] = text[i++];
      continue;
    }

    // Eine Zahl. Komma und Punkt gehoeren dazu: "1,2 km" ist eine Zahl.
    size_t ende = i;
    while (text[ende] &&
           ((text[ende] >= '0' && text[ende] <= '9') ||
            text[ende] == ',' || text[ende] == '.')) {
      ende++;
    }

    // Leerzeichen dazwischen ueberspringen, auch das geschuetzte (UTF-8
    // C2 A0) - Karten-Apps setzen es gern zwischen Zahl und Einheit.
    size_t u = ende;
    while (text[u] == ' ' ||
           ((unsigned char)text[u] == 0xC2 && (unsigned char)text[u + 1] == 0xA0)) {
      u += (text[u] == ' ') ? 1 : 2;
    }

    const size_t n = prv_einheit(text + u);
    if (n > 0) {
      aus[j++] = '#';
      i = u + n;
      continue;
    }

    // Keine Einheit dahinter: die Ziffern bleiben stehen.
    while (i < ende && j + 1 < len) aus[j++] = text[i++];
  }
  aus[j] = '\0';
}
