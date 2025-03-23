Amleitung und README
=====================================

Übersicht
---------
Die Flashcards App ist eine Konsolenanwendung in C, mit der du Karteikarten erstellen, verwalten und lernen kannst. Die Karteikarten werden in einer doppelt verketteten Liste gespeichert und in einer JSON-Datei (FlashcardsMemory.json) abgelegt. Die Anwendung nutzt den SM-2-Algorithmus im Spaced Repetition Modus, um Wiederholungsintervalle dynamisch anzupassen.

Features
--------
- **Karteikarten verwalten:**
  - Neue Karteikarten hinzufügen
  - Bestehende Karteikarten löschen
  - Alle Karteikarten auflisten
  - Sortierung der Karteikarten (aufsteigend oder absteigend nach ID)

- **Lernmodi:**
  - Standard-Lernmodus: Durchgehen aller Karteikarten
  - Spaced Repetition Modus: Abfrage fälliger Karteikarten basierend auf Zeitstempeln

- **Datenspeicher:**
  - Speicherung und Laden der Karteikarten als JSON-Datei (FlashcardsMemory.json)
  - Eine Konfigurationsdatei (config.txt) speichert den Ordnerpfad, in dem die JSON-Datei abgelegt wird

- **Konfigurationsverwaltung:**
  - Möglichkeit, während der Laufzeit einen neuen Konfigurationspfad einzugeben (Option 10 im Hauptmenü). Der neue Pfad wird gespeichert und Flashcards werden aus der neuen Datei geladen.

- **Fehlerbehandlung:**
  - Überprüfung, ob Dateien und Verzeichnisse existieren
  - Automatisches Anfügen des Dateinamens an den Ordnerpfad
  - Benutzeraufforderung zur erneuten Eingabe eines gültigen Pfades, falls erforderlich

Systemvoraussetzungen
---------------------
- **Betriebssystem:** Windows (die Anwendung nutzt `windows.h` zur Überprüfung von Dateiattributen)
- **Compiler:** Jeder C-Compiler (z. B. GCC, MinGW oder der integrierte Compiler in IDEs wie CLion)

Ausführung
----------

   - Beim ersten Programmstart wird die Konfigurationsdatei (config.txt) nicht gefunden. Du wirst dann aufgefordert, einen Ordnerpfad einzugeben, in dem die JSON-Datei (FlashcardsMemory.json) gespeichert werden soll.
   - Beispiel:
     ```
     Konfigurationsdatei nicht gefunden.
     Geben Sie den ORDNERpfad ein, in dem die Flashcards gespeichert werden sollen:
     > C:\Users\DeinBenutzer\Documents
     ```
   - Danach wird die Konfigurationsdatei erstellt und FlashcardsMemory.json in dem angegebenen Ordner gespeichert.

Bedienungsanleitung (Hauptmenü)
-------------------------------
Nach dem Start erscheint ein Menü mit folgenden Optionen:

1. **Neue Karteikarte hinzufügen:**  
   Erlaubt dir, eine neue Karteikarte zu erstellen, indem du Vorder- und Rückseite der Karte eingibst.

2. **Alle Karteikarten anzeigen:**  
   Listet alle vorhandenen Karteikarten mit Details wie ID, Vorder- und Rückseite sowie dem Zeitpunkt der nächsten Wiederholung auf.

3. **Karteikarten lernen (Standardmodus):**  
   Zeigt die Karteikarten nacheinander an. Du drückst Enter, um die Antwort zu sehen.

4. **Karteikarten speichern:**  
   Speichert alle Karteikarten in der JSON-Datei am aktuellen Speicherort.

5. **Karteikarte löschen:**  
   Ermöglicht das Löschen einer Karte anhand ihrer Nummer.

6. **Karteikarten aufsteigend sortieren (nach ID):**  
   Sortiert die Liste der Karteikarten aufsteigend nach ihrer ID.

7. **Karteikarten absteigend sortieren (nach ID):**  
   Sortiert die Karteikarten absteigend nach ihrer ID.

8. **Karteikarten lernen (Spaced Rep: Karten nach Timestamp):**  
   Im Testmodus werden nur die Karteikarten angezeigt, deren Wiederholungszeitpunkt erreicht oder überschritten ist. Nach der Anzeige kannst du die Karte bewerten (1 = einfach, 0 = schwer, -1 = schlecht). Das Intervall wird dann automatisch angepasst.

9. **Beenden:**  
   Vor dem Verlassen werden die Karteikarten gespeichert, und der Speicher wird freigegeben.

10. **Neuen Config-Pfad eingeben:**  
    Mit dieser Option kannst du während der Laufzeit einen neuen Ordnerpfad eingeben, in dem künftig die JSON-Datei (FlashcardsMemory.json) gespeichert wird. Der neue Pfad wird in der Konfigurationsdatei (config.txt) aktualisiert und Flashcards werden aus der neuen Datei geladen.




DME_Flashcards(2).txt…]()
