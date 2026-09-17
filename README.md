# Kieselweg

Navigationsanzeige für die Pebble. Sie rechnet nichts und weiss nichts — sie
zeigt, was [Kiesel-Helper](https://github.com/dysseus-pascal/Kiesel-Helper) ihr
vom Telefon schickt.

*Kieselweg, weil Kiesel-Helper den Weg schickt.*

## Was sie zeigt

| | |
|---|---|
| **Entfernung** | gross in LECO, unter 1 km in Metern, darüber in Kilometern mit einer Stelle |
| **Balken** | erscheint unter 300 m und läuft leer — die Spanne, in der ein Blick aufs Handgelenk noch etwas ändert |
| **Anweisung** | »Rechts abbiegen auf«, umgebrochen |
| **Strasse** | darunter, leiser |
| **»vor N min«** | nur wenn seit über zwei Minuten nichts mehr kam |

**Ein Stand älter als zehn Minuten ist keine Navigation mehr.** Dann steht
»Keine Navigation« da, und darunter klein, was zuletzt kam und wann. Die Uhr
lädt beim Öffnen zwar den gespeicherten Stand — aber eine Anzeige, die Altes
wie Neues aussehen lässt, ist schlimmer als eine leere.

Der letzte Punkt ist der wichtigste. Solange die Karten-App nachschiebt, sagt
ein Zeitstempel nichts. Bleibt er aber stehen, **muss** man es sehen — sonst
fährt man nach einer Anweisung, die seit zehn Minuten überholt ist.

Bei einem **neuen Schritt** summt die Uhr kurz — und »neu« heisst: der *Kern*
der Anweisung hat sich geändert, nicht ihr Text.

Das ist der Unterschied, an dem die erste Fassung gescheitert ist. Google Maps
schreibt die Entfernung **in** den Text: »In 250 m rechts abbiegen«, dann
»In 200 m …«, dann »In 80 m …«. Derselbe Schritt, jedes Mal ein anderer Text —
und die Uhr summte alle paar hundert Meter Autofahrt. Am Steuer aufgefallen,
nicht im Emulator: dort hatte ich denselben Text zweimal geschickt, in
Wirklichkeit kommt er nie zweimal gleich.

Vor dem Vergleich werden deshalb Zahlen **mit Längeneinheit** ausgeblendet:

| Text | Kern |
|---|---|
| `In 250 m rechts abbiegen` | `In # rechts abbiegen` |
| `In 1,2 km rechts abbiegen` | `In # rechts abbiegen` |
| `In 0.2 mi turn right` | `In # turn right` |
| `Ausfahrt 3 nehmen` | `Ausfahrt 3 nehmen` |

Die letzte Zeile ist der Grund für die Einschränkung auf Einheiten: »Ausfahrt 3«
und »Ausfahrt 4« sind wirklich zwei Schritte, und wer sie zusammenwirft,
verschweigt einen.

Darunter liegt ein Netz: **höchstens einmal alle 30 Sekunden**. Eine Quelle, die
ihren Text anders umbaut, kann den Kern-Vergleich überlisten — diesen Riegel
nicht. Der Preis ist ehrlich zu nennen: folgt ein echter zweiter Schritt
innerhalb von 30 Sekunden (»rechts, dann sofort links«), bleibt sein Summen
aus. Auf dem Schirm steht er trotzdem.

## Was sie empfängt

Vier Felder, alle einzeln und alle freiwillig. Fehlt eines, bleibt das alte
stehen — so kann das Telefon nur die Entfernung nachschieben, ohne die
Anweisung mitzuschicken.

| Feld | Nummer | Art | Bedeutung |
|---|---|---|---|
| `ANWEISUNG` | 10000 | Text | »Rechts abbiegen auf« |
| `ENTFERNUNG` | 10001 | Zahl | Meter bis zur Abzweigung |
| `STRASSE` | 10002 | Text | »Bahnhofstrasse« |
| `ANKUNFT` | 10003 | Zahl | Ankunftszeit, Sekunden seit 1970 |

Die Nummern ergeben sich aus der Reihenfolge der `messageKeys` in der
`package.json`, beginnend bei 10000. **Wer dort eine Zeile dazwischenschiebt,
verschiebt alle folgenden** — und der Zettel auf dem Telefon schickt danach
still die falschen Werte ins falsche Feld.

Geschickt wird über die klassische PebbleKit-Schnittstelle
(`com.getpebble.action.app.SEND`). Diese App hat deshalb **absichtlich keinen
`companionApp`-Eintrag** in ihrer `package.json`: ein Paketname dort wählt
PebbleKit2, und das bindet sich an einen Dienst, statt zu senden.

## Ohne Telefon ausprobieren

```bash
pebble install --emulator emery
pebble send-app-message --emulator emery --app-uuid <uuid> \
  --string 10000="Rechts abbiegen auf" 10002="Bahnhofstrasse" --int 10001=80
```

Zwei Fallen im Werkzeug, in beide bin ich hineingetreten:

* **Es will Nummern, keine Namen.** Mit `--app-uuid` löst es die Namen aus der
  `package.json` nicht auf.
* **`--string` darf nur einmal vorkommen.** Steht es zweimal, überschreibt die
  zweite Angabe die erste, und die Nachricht kommt lautlos unvollständig an —
  genau so fehlte hier eine Stunde lang die Anweisung, während Entfernung und
  Strasse ankamen. Alle Zeichenketten gehören in **eine** Gruppe.

## Aufs Handgelenk

Das fertige Paket liegt als [kieselweg.pbw](kieselweg.pbw) im Repo — alle drei
Uhren in einer Datei (emery, flint, gabbro).

```bash
pebble install --phone kieselweg.pbw
```

Oder die Datei aufs Telefon schieben und in der Pebble-App öffnen.

## Bauen

```bash
KIESELWEG_SRC=<dieser Ordner> tools/sync_kieselweg.sh
```

Spiegelt nach `~/kieselweg` und baut dort — waf verträgt keine Pfade mit
Leerzeichen. Kein `npm`: diese App braucht kein Clay, ihre Anzeige kommt nicht
von einer Konfigseite.

## Lizenz

[CC0 1.0](LICENSE) — gemeinfrei.
