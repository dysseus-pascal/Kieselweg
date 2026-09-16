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

Der letzte Punkt ist der wichtigste. Solange die Karten-App nachschiebt, sagt
ein Zeitstempel nichts. Bleibt er aber stehen, **muss** man es sehen — sonst
fährt man nach einer Anweisung, die seit zehn Minuten überholt ist.

Bei einer **neuen** Anweisung summt die Uhr kurz. Nur bei einer neuen: während
der Fahrt schiebt die Karten-App dieselbe Anweisung im Sekundentakt mit
kleinerer Entfernung nach, und ein Summen je Meldung wäre unbrauchbar.

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

## Bauen

```bash
KIESELWEG_SRC=<dieser Ordner> tools/sync_kieselweg.sh
```

Spiegelt nach `~/kieselweg` und baut dort — waf verträgt keine Pfade mit
Leerzeichen. Kein `npm`: diese App braucht kein Clay, ihre Anzeige kommt nicht
von einer Konfigseite.

## Lizenz

[CC0 1.0](LICENSE) — gemeinfrei.
