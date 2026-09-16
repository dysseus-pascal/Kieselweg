// Diese App braucht keine Telefonseite: was sie anzeigt, schickt ihr
// Kiesel-Helper ueber die klassische PebbleKit-Schnittstelle. Die Datei steht
// hier, weil der Bau eine Einstiegsdatei erwartet - und weil eine leere Seite
// ehrlicher ist als eine, die so tut, als gaebe es hier etwas zu tun.
Pebble.addEventListener('ready', function () {
  console.log('Kieselweg bereit - die Anzeige kommt von Kiesel-Helper.');
});
