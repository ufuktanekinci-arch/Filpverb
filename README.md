# FlipVerb – Rückwärts-Hall, Tremolo & Echo-Würfe für FL Studio

Ein einfaches, modernes Effekt-Plugin (VST3) mit drei Dingen:

1. **Rückwärts-Hall** – der Hall baut sich *vor* dem Ton auf und endet genau auf dem Ton.
2. **Tremolo** – lässt den Rückwärts-Hall im Tempo pulsieren.
3. **Echo-Wurf** – Echo (z. B. Rap-Ad-Libs) nur an den Stellen, die du auswählst.

Unten gibt es 16 Felder pro Takt: Was leuchtet, dort wirkt der Effekt. Klicken oder mit gedrückter Maus drüberziehen.

---

## So kommst du an die fertige Plugin-Datei (ohne Programmieren)

Das Plugin muss einmal „gebaut" werden. GitHub macht das kostenlos für dich:

1. Kostenloses Konto auf <https://github.com> anlegen.
2. Oben rechts **+ → New repository**, Name `FlipVerb`, auf **Create repository** klicken.
3. **uploading an existing file** wählen und den **gesamten Inhalt** dieses Ordners hineinziehen
   (wichtig: auch der versteckte Ordner `.github` muss mit – sonst startet der Bau nicht).
   Dann unten **Commit changes**.
4. Reiter **Actions** öffnen. Der Bau startet von selbst (ca. 10–15 Minuten). Falls nicht:
   links **FlipVerb bauen → Run workflow**.
5. Wenn der Haken grün ist: auf den Durchlauf klicken, unten bei **Artifacts**
   **FlipVerb-Windows-VST3** herunterladen und entpacken. Darin liegt der Ordner `FlipVerb.vst3`.

*(Alternative für Fortgeschrittene: Visual Studio 2022 + CMake installieren, dann
`cmake -B build` und `cmake --build build --config Release` ausführen.)*

## Installation in FL Studio

1. Den Ordner `FlipVerb.vst3` nach `C:\Program Files\Common Files\VST3\` kopieren.
2. FL Studio → **Options → Manage plugins → Find installed plugins**.
3. Im Mixer auf einen freien Effekt-Slot klicken → **FlipVerb** wählen.

## So benutzt du es

| Bereich | Was es macht |
|---|---|
| **Rückwärts-Hall** (an/aus) | Hall, der sich vor dem Ton aufbaut |
| Länge des Aufbaus | 1 Schlag, 2 Schläge oder 1 Takt (passt sich dem Tempo an) |
| Menge / Klang | Lautstärke und Helligkeit des Halls |
| **Tremolo** | Lässt den Rückwärts-Hall pulsieren (1/4, 1/8, 1/16, 1/8 Triole), Tiefe = Stärke |
| **Echo-Wurf** | Echo mit Abstand, Menge, Wiederholungen, optional Ping-Pong |
| **Wo soll es wirken?** | 16 Felder = ein Takt (oder zwei, siehe „Ein Feld ="). Nur in leuchtenden Feldern wirkt der Effekt |

**Tipp:** Wähle die Felder dort, wo dein Wort oder Ton im Takt liegt. Der Rückwärts-Hall wird dann aus
genau diesem Ton erzeugt und baut sich davor auf; der Echo-Wurf wiederholt genau diese Stelle.
Voreinstellung: Hall auf Feld 1 und 9, Echo auf Feld 8 und 16.

**Wichtig – Verzögerung (Latenz):** Damit sich der Hall *vor* dem Ton aufbauen kann, muss der Ton
um die gewählte Länge verzögert werden. Das meldet das Plugin an FL Studio, und FL gleicht es automatisch
aus (Mixer-Spur: Latency compensation nicht ausschalten). Beim Ändern der Länge kann es kurz knacken.

## Technik

- C++ mit [JUCE](https://juce.com) 8, Format VST3, Stereo.
- Der Rückwärts-Hall ist eine Faltung mit einem zeitlich umgedrehten Hall-Schweif (Echtzeit, tempo-synchron).
- Beim Bauen lädt CMake JUCE automatisch von GitHub.
- Ordner `test/`: Offline-Test (`-DFLIPVERB_BUILD_TESTS=ON`), der Timing und Pegel prüft.

## Lizenz-Hinweis

JUCE steht unter AGPLv3 bzw. einer kommerziellen Lizenz. Für den privaten Gebrauch ist das kein Problem.
Wenn du das Plugin verkaufen oder ohne Quellcode weitergeben willst, brauchst du eine JUCE-Lizenz.
