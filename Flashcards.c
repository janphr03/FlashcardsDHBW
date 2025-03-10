// PortainerIO
// openmediavault ist das Betriebssystem
// als Stack die Datenbank definieren mit PORT usw
//
// DIE TIMESTAMPS SOLLEN NOCH IN DER JSON GESPEICHERT WERDEN
// MySql installer muss es geben

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tgmath.h>
#include <time.h>
#include <windows.h>

#define CONFIG_FILE "config.txt"
#define MAX_PATH_LEN 256
#define MAX_TEXT_LENGTH 256

// Als Standardname wird "FlashcardsMemory.json" verwendet.
char flashcardsJSONFile[MAX_PATH_LEN] = "./FlashcardsMemory.json";


typedef struct FlashcardNode {
    int id;  // Eindeutige Nummer für die Sortierung
    char front[MAX_TEXT_LENGTH];
    char back[MAX_TEXT_LENGTH];
    int repetitions;       // Anzahl der Wiederholungen (hintereinander richtige Antworten)
    double easeFactor;    // E-Faktor (Lernfaktor zur Intervallanpassung)
    int interval;          // aktuelles Intervall (in Tagen oder, im Testmodus, in Minuten)
    time_t lastReview;     // Zeitpunkt der letzten Abfrage
    time_t nextReview;   // Zeitpunkt, wann die Karte wieder fällig ist
    struct FlashcardNode *prev;
    struct FlashcardNode *next;
} FlashcardNode;

// Globale Zeiger auf Kopf und Ende der Liste sowie globaler Zähler für IDs
FlashcardNode* ptrHead = NULL;
FlashcardNode* ptrTail = NULL;
int next_id = 1; // Nächste zu vergebende ID

/*
 *FERTIG
 *summary:
 * Entfernt Zeilenumbruch aus einem String
 */
void removeNewline(char *ptrStr) {
    ptrStr[strcspn(ptrStr, "\n")] = '\0';
}
/*FERTIG
 * summary:
 * prüft, ob das File vorhanden ist
 */
int fileExists(const char *filename) {
    const DWORD attr = GetFileAttributes(filename);
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

/*FERTIG
 *summary:
 * erstellt eine neue Karteikarte, indem zuerst Speicher allokiert wird.
 * anschließend werden die Felder durch Konsoleneingabe definiert.
 */
void addFlashcard() {
    // Speicher für den neuen Node, die Karteikarte allokieren
    FlashcardNode* ptrNewNode = malloc(sizeof(FlashcardNode));

    // Falls kein Speicher mehr für einen neuen Knoten allokiert werden kann, bricht die Methode ab
    if (!ptrNewNode) {
        printf("Fehler bei der Speicherzuweisung.\n");
        return;
    }

    // prev & next auf NULL weil neues Element noch nicht in der Liste ist
    ptrNewNode->prev = NULL;
    ptrNewNode->next = NULL;
    ptrNewNode->id = next_id++; //id ist 1 höher als die vorherige

    // Die Bestandteile des SM2 Algorithmus
    ptrNewNode->repetitions = 0; // Karte wurde 0x wiederholt
    ptrNewNode->easeFactor = 2.5; //Standartwert für den Schwierigkeitsgrad
    ptrNewNode->interval = 1; // Testmodus: 1 Minute, sonst 1 Tag
    ptrNewNode->lastReview = time(NULL); // Der Zeitstempel wann die nächste Wiederholung fällig ist
    ptrNewNode->nextReview = ptrNewNode->lastReview; // sofort fällig


    printf("Geben Sie die Vorderseite (Frage) der Karte ein:\n> ");
    // falls fgets NULL zurückgibt, wird der Speicher wieder freigegeben
    if (fgets(ptrNewNode->front, MAX_TEXT_LENGTH, stdin) == NULL) {
        free(ptrNewNode);
        return;
    }
    removeNewline(ptrNewNode->front);

    // Das Selbe für die Rückseite
    printf("Geben Sie die Rueckseite der Karte ein:\n> ");
    if (fgets(ptrNewNode->back, MAX_TEXT_LENGTH, stdin) == NULL) {
        free(ptrNewNode);
        return;
    }
    removeNewline(ptrNewNode->back);

    // Wenn die Liste noch leer ist
    if (ptrHead == NULL) {
        ptrHead = ptrNewNode;
        ptrTail = ptrNewNode;
    }
    // Die Liste enthält schon Elemente
    else {
        ptrTail->next = ptrNewNode;
        ptrNewNode->prev = ptrTail;
        ptrTail = ptrNewNode;
    }
    printf("Karteikarte hinzugefuegt.\n");
}

/*FERTIG
 * summary:
 * Funktion zum Löschen einer Karteikarte anhand ihrer Nummer
 */
void deleteFlashcard() {
    // prüfen ob Liste leer ist
    if (ptrHead == NULL) {
        printf("Keine Karteikarten zum Loeschen vorhanden.\n");
        return;
    }

    int number; // speichert den Input als int Wert
    char input[10];
    printf("Gib die Nummer der zu loeschenden Karte ein: ");
    // liest die Eingabe ein, wenn Eingabe = NULL dann abbruch
    if (fgets(input, sizeof(input), stdin) == NULL)
        return;
    number = atoi(input);

    // ungültige Eingabe abfangen
    if (number < 1) {
        printf("Ungueltige Nummer.\n");
        return;
    }
    int count = 1;
    FlashcardNode* current = ptrHead;

    // Liste durchlaufen bis Karte die gelöscht werden soll gefunden ist
    while (current != NULL && count < number) {
        current = current->next;
        count++;
    }
    // Eingabe ist größer als die Liste
    if (current == NULL) {
        printf("Keine Karte mit dieser Nummer gefunden.\n");
        return;
    }

    // Element aus der Liste entfernen und die verbleibenden miteinander verbinden
    // Der next Zeiger des vorherigen Elementes wird nun mit dem Element nach dem gelöschten Element Verbunden
    if (current->prev)
        current->prev->next = current->next;
    else
        ptrHead = current->next;

    // Der prev Zeiger des nachfolgenden Elements wird mit dem Element vor dem gelöschten Element verbunden
    if (current->next)
        current->next->prev = current->prev;
    else
        ptrTail = current->prev;
    free(current);
    printf("Karteikarte %d wurde geloescht.\n", number);
}

/*FERTIG
 *summary:
 * Diese Funktion listet alle vorhandenen Karteikarten in der verketteten Liste aus
 * Dabei zeigt sie auch den Zeitpunkt an, wann die Karteikarte das nächste Mal gelernt wird
 */
void listFlashcards() {
    // Überprüfung ob es eine Liste gibt
    if (ptrHead == NULL) {
        printf("Keine Karteikarten vorhanden.\n");
        return;
    }

    int count = 1;
    FlashcardNode* current = ptrHead; // startet am ersten Element
    char timeStamp[100];


    while (current != NULL) {
        struct tm *tmInfo = localtime(&current->nextReview); // Umwandlung des Zeitstempels in eine Struct Darstellung
        strftime(timeStamp, sizeof(timeStamp), "%Y-%m-%d %H:%M:%S", tmInfo); // wandelt die Daten aus Struct in einen Datums-String um

        printf("%d. (ID: %d)\n  Vorderseite: %s\n  Rueckseite: %s\n  Naechste Wiederholung: %s\n",
               count, current->id, current->front, current->back, timeStamp);
        // zur nächsten Karte wechseln
        count++;
        current = current->next;
    }
}

/*FERTIG
 * summary:
 * Funktion zum Lernen der Karteikarten ohne Spaced Rep Funktionalität einfach nur zum Durchgehen
 */
void studyFlashcards() {
    // prüfen ob es Karten gibt
    if (ptrHead == NULL) {
        printf("Keine Karteikarten zum Lernen vorhanden.\n");
        return;
    }

    char dummy[10]; // soll die Enter Eingabe speichern
    int count = 1;
    FlashcardNode* current = ptrHead;

    // geht alle Karten beginnend an dem Head der Liste durch
    while (current != NULL) {
        printf("\nFrage %d: %s\n", count, current->front);
        printf("Druecken Sie Enter, um die Antwort zu sehen...");
        fgets(dummy, sizeof(dummy), stdin);
        printf("Antwort: %s\n", current->back);
        count++;
        current = current->next;
    }
}


/*FERTIG
 * summary:
 * Gibt alle Karteikarten frei und setzt head und tail zurück
 * wird aufgerufen beim beenden der Anwendung (Nr.9) oder
 */
void freeFlashcards() {
    FlashcardNode* current = ptrHead;
    // solange es noch Elemente in der Liste gibt wird diese durchgelaufen
    while (current != NULL) {
        FlashcardNode* temp = current;
        current = current->next;
        free(temp);
    }
    ptrHead = NULL;
    ptrTail = NULL;
}

/*FERTIG
 * summary:
 * Funktion zum Speichern der Karteikarten in einer JSON-Datei.
 * Der Nutzer gibt dabei nur den Ordnerpfad an, der Dateiname "FlashcardsMemory.json" wird automatisch ergänzt.
 */
void saveFlashcardsToFile() {
    // Dateinamen wird initialisiert
    char actualFilename[256]; // speichert den vollständigen Pfad inkl. Dateiname
    strncpy(actualFilename, flashcardsJSONFile, sizeof(actualFilename));
    actualFilename[sizeof(actualFilename) - 1] = '\0'; // String wird terminiert

    FILE *ptrFile = NULL;
    int firstTry = 1; // damit beim ersten Versuch eine andere Fehlermeldung erscheint
    while (1) {

        // prüft, ob die Datei existiert beim 1. Versuch
        if (firstTry) {
            firstTry = 0;

            // Datei existiert und wird überschrieben
            if (fileExists(actualFilename)) {
                printf("Hinweis: Die Datei \"%s\" existiert bereits und wird ueberschrieben.\n", actualFilename);
            }
            // Datei nicht gefunden
            else {
                printf("Die Datei \"%s\" wurde nicht gefunden. Es wird eine neue Datei erstellt.\n", actualFilename);
            }
        }
        else {
            printf("Fehler: Die Datei \"%s\" konnte nicht zum Schreiben geöffnet werden.\n", actualFilename);
        }

        // Datei soll im Schreibmodus "w" geöffnet werden (sie soll ja überschrieben werden)
        ptrFile = fopen(actualFilename, "w");
        // Abbruch, falls Datei erfolgreich geöffnet wurde
        if (ptrFile != NULL) {
            break;
        }

        // Benutzer soll den Ordner angeben, in dem die Datei gespeichert werden soll.
        // Der Dateiname "FlashcardsMemory.json" wird automatisch ergänzt.
        printf("Bitte geben Sie einen neuen Speicherordner ein (oder \"cancel\" zum Abbrechen): ");
        if (fgets(actualFilename, sizeof(actualFilename), stdin) == NULL) {
            printf("Keine Eingabe erhalten. Abbruch.\n");
            return;
        }
        actualFilename[strcspn(actualFilename, "\n")] = '\0';
        if (strcmp(actualFilename, "cancel") == 0) {
            printf("Speichern abgebrochen.\n");
            return;
        }

        // Ergänze den Dateinamen "FlashcardsMemory.json" zum eingegebenen Ordnerpfad des Nutzers
        size_t len = strlen(actualFilename);
        if (len > 0 && actualFilename[len - 1] != '\\')
            snprintf(actualFilename, sizeof(actualFilename), "%s\\FlashcardsMemory.json", actualFilename);
        else
            snprintf(actualFilename, sizeof(actualFilename), "%sFlashcardsMemory.json", actualFilename);
        if (len > 0 && actualFilename[len - 1] != '/')
            snprintf(actualFilename, sizeof(actualFilename), "%s/FlashcardsMemory.json", actualFilename);
        else
            snprintf(actualFilename, sizeof(actualFilename), "%sFlashcardsMemory.json", actualFilename);
    }

    // Aktualisiere den globalen Dateipfad, sodass dieser auch beim nächsten Laden verwendet wird.
    strncpy(flashcardsJSONFile, actualFilename, sizeof(flashcardsJSONFile));
    flashcardsJSONFile[sizeof(flashcardsJSONFile) - 1] = '\0';

    // Schreibe den JSON-Inhalt in die Datei
    fprintf(ptrFile, "{\n  \"cards\": [\n");
    FlashcardNode* current = ptrHead;
    while (current != NULL) {
        fprintf(ptrFile, "    {\n");
        fprintf(ptrFile, "      \"front\": \"%s\",\n", current->front);
        fprintf(ptrFile, "      \"back\": \"%s\",\n", current->back);
        fprintf(ptrFile, "      \"repetitions\": %d,\n", current->repetitions);
        fprintf(ptrFile, "      \"easeFactor\": %.2f,\n", current->easeFactor);
        fprintf(ptrFile, "      \"interval\": %d,\n", current->interval);
        fprintf(ptrFile, "      \"lastReview\": %ld,\n", current->lastReview);
        fprintf(ptrFile, "      \"nextReview\": %ld\n", current->nextReview);
        if (current->next == NULL)
            fprintf(ptrFile, "    }\n");
        else
            fprintf(ptrFile, "    },\n");
        current = current->next;
    }
    fprintf(ptrFile, "  ]\n}\n");
    fclose(ptrFile); //
    printf("Karteikarten wurden in \"%s\" gespeichert.\n", flashcardsJSONFile);
}


/*
 * summary:
 * Lädt Karteikarten aus einer JSON-Datei und fügt sie in die verkettete Liste ein.
 */
void loadFlashcardsFromFile(const char *filename) {
    // öffne die Datei zum Lesen.
    FILE *file = fopen(filename, "r");
    if (!file) {
        // falls die Datei nicht existiert, gebe eine Meldung aus und beende die Funktion.
        printf("Datei \"%s\" nicht gefunden, starte mit leeren Karteikarten.\n", filename);
        return;
    }

    // vor dem Laden alle vorhandenen Karteikarten freigeben.
    freeFlashcards();

    char line[512];  // Buffer zum Einlesen einer Zeile aus der Datei.

    // durchläuft die Datei Zeile für Zeile.
    while (fgets(line, sizeof(line), file)) {
        // falls die Zeile den String "\"front\":" enthält, handelt es sich um den Start eines neuen Karteikarteneintrags.
        if (strstr(line, "\"front\":") != NULL) {
            // allokiert Speicher für einen neuen FlashcardNode.
            FlashcardNode* newNode = malloc(sizeof(FlashcardNode));
            if (!newNode) {
                printf("Fehler bei der Speicherzuweisung.\n");
                fclose(file);
                return;
            }
            // initialisiert die Zeiger des neuen Knotens.
            newNode->prev = NULL;
            newNode->next = NULL;

            // -------- Parsing des "front" Feldes --------
            char frontText[MAX_TEXT_LENGTH] = "";
            // findet den Doppelpunkt, der den Schlüssel vom Wert trennt.
            char *colon = strchr(line, ':');
            if (colon) {
                // findet das erste Anführungszeichen nach dem Doppelpunkt.
                char *start = strchr(colon, '\"');
                if (start) {
                    start++;  // überspringt das erste Anführungszeichen.
                    // findet das nächste Anführungszeichen, das das Ende des Wertes markiert.
                    char *end = strchr(start, '\"');
                    if (end) {
                        size_t len = end - start;
                        if (len >= MAX_TEXT_LENGTH)
                            len = MAX_TEXT_LENGTH - 1;
                        strncpy(frontText, start, len);
                        frontText[len] = '\0';
                    }
                }
            }
            // speichert den eingelesenen Text in newNode->front.
            strncpy(newNode->front, frontText, MAX_TEXT_LENGTH);
            newNode->front[MAX_TEXT_LENGTH - 1] = '\0';

            // -------- Parsing des "back" Feldes --------
            // liest die nächste Zeile, in der der "back" Wert erwartet wird.
            if (!fgets(line, sizeof(line), file)) break;
            char backText[MAX_TEXT_LENGTH] = "";
            // Suche in der Zeile nach dem Schlüssel "back".
            char *backPtr = strstr(line, "\"back\":");
            if (backPtr) {
                char *colonBack = strchr(backPtr, ':');
                if (colonBack) {
                    // Finde das erste Anführungszeichen, das den Wert einleitet.
                    char *startBack = strchr(colonBack, '\"');
                    if (startBack) {
                        startBack++;  // Überspringe das erste Anführungszeichen.
                        // Finde das schließende Anführungszeichen.
                        char *endBack = strchr(startBack, '\"');
                        if (endBack) {
                            size_t len = endBack - startBack;
                            if (len >= MAX_TEXT_LENGTH)
                                len = MAX_TEXT_LENGTH - 1;
                            strncpy(backText, startBack, len);
                            backText[len] = '\0';
                        }
                    }
                }
            }
            // Speichere den eingelesenen Text in newNode->back.
            strncpy(newNode->back, backText, MAX_TEXT_LENGTH);
            newNode->back[MAX_TEXT_LENGTH - 1] = '\0';

            // -------- Parsing des "repetitions" Feldes --------
            // Lese die nächste Zeile, in der der Wert für "repetitions" erwartet wird.
            if (!fgets(line, sizeof(line), file)) break;
            int repetitions = 0;
            char *repPtr = strstr(line, "\"repetitions\":");
            if (repPtr) {
                char *colonRep = strchr(repPtr, ':');
                if (colonRep) {
                    // Umwandeln des Strings in einen Integer-Wert.
                    repetitions = atoi(colonRep + 1);
                }
            }
            newNode->repetitions = repetitions;

            // -------- Parsing des "ease_factor" Feldes --------
            if (!fgets(line, sizeof(line), file)) break;
            double ease_factor = 2.5;
            char *efPtr = strstr(line, "\"ease_factor\":");
            if (efPtr) {
                char *colonEf = strchr(efPtr, ':');
                if (colonEf) {
                    // Umwandeln des Strings in einen Double-Wert.
                    ease_factor = atof(colonEf + 1);
                }
            }
            newNode->easeFactor = ease_factor;

            // -------- Parsing des "interval" Feldes --------
            if (!fgets(line, sizeof(line), file)) break;
            int interval = 1;
            char *intervalPtr = strstr(line, "\"interval\":");
            if (intervalPtr) {
                char *colonInterval = strchr(intervalPtr, ':');
                if (colonInterval) {
                    interval = atoi(colonInterval + 1);
                }
            }
            newNode->interval = interval;

            // -------- Parsing des "last_review" Feldes --------
            if (!fgets(line, sizeof(line), file)) break;
            time_t last_review = time(NULL);  // Standardwert, falls Parsing fehlschlägt.
            char *lastPtr = strstr(line, "\"last_review\":");
            if (lastPtr) {
                char *colonLast = strchr(lastPtr, ':');
                if (colonLast) {
                    // Umwandeln des Strings in einen long-Wert, der dann als time_t genutzt wird.
                    last_review = (time_t)atol(colonLast + 1);
                }
            }
            newNode->lastReview = last_review;

            // -------- Parsing des "next_review" Feldes --------
            if (!fgets(line, sizeof(line), file)) break;
            time_t next_review = time(NULL);  // Standardwert, falls Parsing fehlschlägt.
            char *nextPtr = strstr(line, "\"next_review\":");
            if (nextPtr) {
                char *colonNext = strchr(nextPtr, ':');
                if (colonNext) {
                    next_review = (time_t)atol(colonNext + 1);
                }
            }
            newNode->nextReview = next_review;

            // -------- hinzufügen des neuen Knotens zur verketteten Liste --------
            if (ptrHead == NULL) {
                // falls die Liste leer ist, wird newNode der erste Knoten.
                ptrHead = newNode;
                ptrTail = newNode;
            } else {
                // andernfalls wird newNode am Ende der Liste angehängt.
                ptrTail->next = newNode;
                newNode->prev = ptrTail;
                ptrTail = newNode;
            }
        }
    }


    // schließt die Datei, da das Einlesen abgeschlossen ist.
    fclose(file);

    // vergibt fortlaufend neue IDs an die Knoten in der Liste.
    int count = 1;
    FlashcardNode* current = ptrHead;
    while (current != NULL) {
        current->id = count++;
        current = current->next;
    }
    next_id = count;
    printf("%d Karteikarten geladen.\n", count - 1);
}


/*
 * summary:
 * Sortiert die Karteikarten in aufsteigender oder absteigender Reihenfolge nach ihrer ID.
 * Das Sortieren wird durch das Tauschen der **Zeiger** (`prev` und `next`) umgesetzt,
 * nicht durch das Vertauschen der Inhalte der Knoten.
 *
 * @param ascending Falls 1, wird aufsteigend sortiert. Falls 0, wird absteigend sortiert.
 */
void sortFlashcardsById(int ascending) {
    // Falls die Liste leer ist oder nur ein Element enthält, gibt es nichts zu sortieren
    if (ptrHead == NULL || ptrHead->next == NULL)
        return;

    int swapped;
    FlashcardNode *last = NULL; // Begrenzung für die unsortierten Elemente

    do {
        swapped = 0;
        FlashcardNode *current = ptrHead;

        while (current->next != last) {
            FlashcardNode *nextNode = current->next;

            // Prüfen, ob die Reihenfolge falsch ist
            int condition = ascending ? (current->id > nextNode->id) : (current->id < nextNode->id);
            if (condition) {
                // **Tausche die Zeiger der Knoten, ohne deren Inhalte zu ändern**

                // Zeiger des vorherigen Elements auf das neue "erste" Element setzen
                if (current->prev)
                    current->prev->next = nextNode;
                else
                    ptrHead = nextNode; // Falls `current` das erste Element ist, wird `ptrHead` aktualisiert

                // Zeiger des nächsten Elements auf das neue "hintere" Element setzen
                if (nextNode->next)
                    nextNode->next->prev = current;
                else
                    ptrTail = current; // Falls `nextNode` das letzte Element war, wird `ptrTail` aktualisiert

                // Die Verkettung zwischen `current` und `nextNode` umdrehen
                current->next = nextNode->next;
                nextNode->prev = current->prev;

                nextNode->next = current;
                current->prev = nextNode;

                // Da ein Tausch stattgefunden hat, markieren wir, dass noch ein Durchlauf nötig ist
                swapped = 1;
            }
            current = current->next;
        }
        last = current; // Das letzte Element ist jetzt sortiert, wir ignorieren es im nächsten Durchlauf
    } while (swapped);
}


/*
 * summary:
 * Aktualisiert das Intervall einer Karte mittels SM-2 Algorithmus im Testmodus.
 * Intervalle in Minuten (bei echtem Einsatz würde man in Tagen arbeiten, hier einfacher zu testen):
 *  - Bei der ersten erfolgreichen Wiederholung: 10 Minuten
 *  - Bei der zweiten: 30 Minuten
 *  - Danach wird das vorherige Intervall mit dem E-Faktor multipliziert.
 */
void update_card_interval_test(FlashcardNode *curr, int rating) {
    int rep = curr->repetitions;
    double efactor = curr->easeFactor;
    int interval = curr->interval;
    int quality;
    if (rating == 1) {
        quality = 5;   // sehr gutes Ergebnis
    } else if (rating == 0) {
        quality = 3;   // gerade noch gewusst
    } else { // rating == -1
        quality = 2;   // nicht gewusst
    }
    efactor = efactor + (0.1 - (5 - quality) * (0.08 + (5 - quality) * 0.02));
    if (efactor < 1.3) {
        efactor = 1.3;
    }
    if (quality < 3) {
        rep = 0;  // Reset bei schlechter Antwort
    } else {
        rep += 1;
    }
    if (rep <= 1) {
        interval = 10;   // erste Wiederholung: 10 Minuten
    } else if (rep == 2) {
        interval = 30;   // zweite Wiederholung: 30 Minuten
    } else {
        interval = (int)ceil(interval * efactor);
    }
    time_t now = time(NULL);
    double seconds_per_minute = 60;
    curr->lastReview = now;
    curr->nextReview = now + (time_t)(interval * seconds_per_minute);
    curr->repetitions = rep;
    curr->easeFactor = efactor;
    curr->interval = interval;
}

/*
 * summary:
 * Sortiert die verkettete Liste der Flashcards mittels Bubble Sort anhand des next_review Zeitstempels (aufsteigend).
 */
/*
 * summary:
 * Sortiert die verkettete Liste der Flashcards anhand des `nextReview` Zeitstempels (aufsteigend),
 * sodass die fälligsten Karteikarten zuerst erscheinen.
 */
void bubbleSortByNextReview() {
    if (ptrHead == NULL || ptrHead->next == NULL)
        return;

    int swapped;
    FlashcardNode *last = NULL; // Begrenzung für bereits sortierte Elemente

    do {
        swapped = 0;
        FlashcardNode *current = ptrHead;

        while (current->next != last) {
            FlashcardNode *nextNode = current->next;

            // Prüfen, ob die Reihenfolge falsch ist (größere Timestamps sollen später kommen)
            if (current->nextReview > nextNode->nextReview) {
                // **Tausche die Zeiger der Knoten, ohne deren Inhalte zu ändern**

                // Zeiger des vorherigen Elements auf das neue "erste" Element setzen
                if (current->prev)
                    current->prev->next = nextNode;
                else
                    ptrHead = nextNode; // Falls `current` das erste Element war

                // Zeiger des nächsten Elements auf das neue "hintere" Element setzen
                if (nextNode->next)
                    nextNode->next->prev = current;
                else
                    ptrTail = current; // Falls `nextNode` das letzte Element war

                // Die Verkettung zwischen `current` und `nextNode` umdrehen
                current->next = nextNode->next;
                nextNode->prev = current->prev;

                nextNode->next = current;
                current->prev = nextNode;

                // Da ein Tausch stattgefunden hat, markieren wir, dass noch ein Durchlauf nötig ist
                swapped = 1;
            }
            current = current->next;
        }
        last = current; // Das letzte Element ist jetzt sortiert, wir ignorieren es im nächsten Durchlauf
    } while (swapped);
}


// Liest den Ordnerpfad aus der Konfigurationsdatei.
// Falls die Datei nicht existiert oder leer ist, wird der Nutzer dazu aufgefordert,
// einen ORDNERpfad anzugeben, in dem die JSON-Datei (FlashcardsMemory.json) abgelegt werden soll.
void loadConfig(char *path) {
    FILE *config = fopen(CONFIG_FILE, "r");
    if (config == NULL) {
        printf("Konfigurationsdatei nicht gefunden.\nGeben Sie den ORDNERpfad ein, in dem die Flashcards gespeichert werden sollen:\n> ");
        char userInput[MAX_PATH_LEN];
        if (fgets(userInput, MAX_PATH_LEN, stdin) != NULL) {
            userInput[strcspn(userInput, "\n")] = '\0';
        }
        if (strlen(userInput) == 0) {
            printf("Kein Ordner angegeben. Abbruch.\n");
            exit(1);
        }

        // Prüfen, ob userInput mit '/' oder '\' endet
        size_t len = strlen(userInput);
#ifdef _WIN32
        if (len > 0 && userInput[len-1] != '\\')
            snprintf(path, MAX_PATH_LEN, "%s\\FlashcardsMemory.json", userInput);
        else
            snprintf(path, MAX_PATH_LEN, "%sFlashcardsMemory.json", userInput);
#else
        if (len > 0 && userInput[len-1] != '/')
            snprintf(path, MAX_PATH_LEN, "%s/FlashcardsMemory.json", userInput);
        else
            snprintf(path, MAX_PATH_LEN, "%sFlashcardsMemory.json", userInput);
#endif
        // Speichere den vollständigen Dateipfad in der Konfigurationsdatei
        config = fopen(CONFIG_FILE, "w");
        if (config) {
            fprintf(config, "%s\n", path);
            fclose(config);
        }
    } else {
        if (fgets(path, MAX_PATH_LEN, config) != NULL) {
            path[strcspn(path, "\n")] = '\0';
        }
        fclose(config);
    }
}

/*
 * summary:
 * Funktion für das Lernen im Testmodus: Karten werden basierend auf dem next_review Zeitstempel abgefragt.
 * Es werden nur Karten abgefragt, deren next_review <= aktuelle Zeit ist.
 */
void studyFlashcardsByDueDateTest() {
    if (ptrHead == NULL) {
        printf("Keine Karteikarten zum Lernen vorhanden.\n");
        return;
    }
    char dummy[10];
    int count = 1;
    bubbleSortByNextReview();
    FlashcardNode* current = ptrHead;
    time_t now = time(NULL);
    while (current != NULL) {
        if (difftime(now, current->nextReview) >= 0) {
            printf("\n[Due Karte %d] Frage: %s\n", count, current->front);
            printf("Druecken Sie Enter, um die Antwort zu sehen...");
            fgets(dummy, sizeof(dummy), stdin);
            printf("Antwort: %s\n", current->back);
            printf("Bewerten Sie diese Karte (-1=schlecht, 0=schwer, 1=einfach): ");
            if (fgets(dummy, sizeof(dummy), stdin) != NULL) {
                int rating = atoi(dummy);
                update_card_interval_test(current, rating);
            }
            count++;
        }
        current = current->next;
    }
}

int main() {
    // Beim Programmstart wird die Config geladen. Falls keine Config existiert,
    // fragt das Programm nach dem ORDNERpfad, in dem die JSON-Datei (FlashcardsMemory.json) gespeichert werden soll.
    loadConfig(flashcardsJSONFile);

    // Anschließend werden die Karteikarten aus der JSON-Datei geladen.
    loadFlashcardsFromFile(flashcardsJSONFile);

    // Speichere den aktuell geladenen Pfad noch einmal in der Config.
    FILE *config = fopen(CONFIG_FILE, "w");
    if (config) {
        fprintf(config, "%s\n", flashcardsJSONFile);
        fclose(config);
    }

    int choice;
    char input[10];
    while (1) {
        printf("\n--- Karteikarten App Menu ---\n");
        printf("1. Neue Karteikarte hinzufuegen\n");
        printf("2. Alle Karteikarten anzeigen\n");
        printf("3. Karteikarten lernen (Standardmodus)\n");
        printf("4. Karteikarten speichern\n");
        printf("5. Karteikarte loeschen\n");
        printf("6. Karteikarten aufsteigend sortieren (nach ID)\n");
        printf("7. Karteikarten absteigend sortieren (nach ID)\n");
        printf("8. Karteikarten lernen (Spaced Rep: Karten nach Timestamp)\n");
        printf("9. Beenden\n");
        printf("Ihre Wahl: ");

        if (fgets(input, sizeof(input), stdin) == NULL)
            continue;
        choice = atoi(input);

        switch (choice) {
            case 1:
                addFlashcard();
                break;
            case 2:
                listFlashcards();
                break;
            case 3:
                studyFlashcards();
                break;
            case 4:
                saveFlashcardsToFile();
                break;
            case 5:
                deleteFlashcard();
                break;
            case 6:
                sortFlashcardsById(1);
                printf("Karteikarten wurden aufsteigend sortiert.\n");
                break;
            case 7:
                sortFlashcardsById(0);
                printf("Karteikarten wurden absteigend sortiert.\n");
                break;
            case 8:
                studyFlashcardsByDueDateTest();
                break;
            case 9:
                saveFlashcardsToFile();
                freeFlashcards();
                printf("Programm beendet.\n");
                exit(0);
            default:
                printf("Ungueltige Eingabe. Bitte erneut versuchen.\n");
        }
    }
}
