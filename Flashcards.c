// PortainerIO
// openmediavault ist das Betriebssystem
// als Stack die Datenbank definieren mit PORT usw
//
// DIE TIMESTAMPS SOLLEN NOCH IN DER JSON GESPEICHERT WERDEN
// MySql installer muss es geben
// 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tgmath.h>
#include <time.h>

#ifdef _WIN32
  #include <direct.h>   // Für _mkdir() unter Windows
  #include <windows.h>  // Windows-spezifische Funktionen
#else
  #include <sys/stat.h> // Für mkdir() unter Linux/macOS
  #include <unistd.h>
#endif

#define CONFIG_FILE "config.txt"
#define MAX_PATH_LEN 256
#define MAX_TEXT_LENGTH 256  // Maximale Länge für Vorder- und Rückseite

/*
 * my_mkdir:
 * Erstellt einen Ordner plattformunabhängig.
 * - Unter Windows wird _mkdir() aus <direct.h> aufgerufen.
 * - Unter Linux/macOS wird mkdir() aus <sys/stat.h> verwendet.
 *   Der Modus 0777 setzt Lese-/Schreib-/Ausführrechte für Owner, Gruppe und andere.
 */
int my_mkdir(const char *path)
{
#ifdef _WIN32
    return _mkdir(path);
#else
    return mkdir(path, 0777);
#endif
}

// Standardmäßig wird die JSON-Datei im aktuellen Verzeichnis abgelegt
// Als Standardname wird "FlashcardsMemory.json" verwendet.
char flashcardsJSONFile[MAX_PATH_LEN] = "./FlashcardsMemory.json";

// Maximale Länge für Vorder- und Rückseite
#define MAX_TEXT_LENGTH 256

// Erweiterte Struktur: zusätzliches Feld 'id' für die fortlaufende Nummer
typedef struct FlashcardNode {
    int id;  // Eindeutige Nummer für die Sortierung
    char front[MAX_TEXT_LENGTH];
    char back[MAX_TEXT_LENGTH];
    /* Die Variable spacedRepCount wurde entfernt, da sie nicht mehr benötigt wird */
    int repetitions;       // Anzahl der Wiederholungen (hintereinander richtige Antworten)
    double ease_factor;    // E-Faktor (Lernfaktor zur Intervallanpassung)
    int interval;          // aktuelles Intervall (in Tagen oder, im Testmodus, in Minuten)
    time_t last_review;     // Zeitpunkt der letzten Abfrage
    time_t next_review;   // Zeitpunkt, wann die Karte wieder fällig ist
    struct FlashcardNode *prev;
    struct FlashcardNode *next;
} FlashcardNode;

// Globale Zeiger auf Kopf und Ende der Liste sowie globaler Zähler für IDs
FlashcardNode* ptrHead = NULL;
FlashcardNode* ptr_tail = NULL;
int next_id = 1; // Nächste zu vergebende ID

/*
 *FERTIG
 *summary:
 * Entfernt Zeilenumbruch aus einem String
 */
void removeNewline(char *ptrStr) {
    ptrStr[strcspn(ptrStr, "\n")] = '\0';
}

/*
 *summary:
 * erstellt eine neue Karteikarte, indem zuerst Speicher allokiert wird.
 * anschließend werden die Felder durch Konsoleneingabe definiert.
 */
void addFlashcard() {
    // Speicher für den neuen Node, die Karteikarte allokieren
    FlashcardNode* ptrNewNode = malloc(sizeof(FlashcardNode));
    if (!ptrNewNode) {
        printf("Fehler bei der Speicherzuweisung.\n");
        return;
    }
    ptrNewNode->prev = NULL;
    ptrNewNode->next = NULL;
    ptrNewNode->id = next_id++;

    ptrNewNode->repetitions = 0;
    ptrNewNode->ease_factor = 2.5;
    ptrNewNode->interval = 1; // Testmodus: 1 Minute, sonst 1 Tag
    ptrNewNode->last_review = time(NULL);
    ptrNewNode->next_review = ptrNewNode->last_review; // sofort fällig

    printf("Geben Sie die Vorderseite (Frage) der Karte ein:\n> ");
    if (fgets(ptrNewNode->front, MAX_TEXT_LENGTH, stdin) == NULL) {
        free(ptrNewNode);
        return;
    }
    removeNewline(ptrNewNode->front);

    printf("Geben Sie die Rueckseite der Karte ein:\n> ");
    if (fgets(ptrNewNode->back, MAX_TEXT_LENGTH, stdin) == NULL) {
        free(ptrNewNode);
        return;
    }
    removeNewline(ptrNewNode->back);

    if (ptrHead == NULL) {
        ptrHead = ptrNewNode;
        ptr_tail = ptrNewNode;
    } else {
        ptr_tail->next = ptrNewNode;
        ptrNewNode->prev = ptr_tail;
        ptr_tail = ptrNewNode;
    }
    printf("Karteikarte hinzugefuegt.\n");
}

/*
 *summary:
 * Diese Funktion listet alle vorhandenen Karteikarten in der verketteten Liste aus
 * Dabei zeigt sie auch den Zeitpunkt an, wann die Karteikarte das nächste Mal gelernt wird
 */
void listFlashcards() {
    if (ptrHead == NULL) {
        printf("Keine Karteikarten vorhanden.\n");
        return;
    }
    int count = 1;
    FlashcardNode* current = ptrHead;
    char timeBuffer[100];
    while (current != NULL) {
        struct tm *tmInfo = localtime(&current->next_review);
        strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", tmInfo);
        printf("%d. (ID: %d)\n  Vorderseite: %s\n  Rueckseite: %s\n  Naechste Wiederholung: %s\n",
               count, current->id, current->front, current->back, timeBuffer);
        count++;
        current = current->next;
    }
}

/*
 * summary:
 * Funktion zum Lernen der Karteikarten ohne Spaced Rep Funktionalität einfach nur zum Durchgehen
 */
void studyFlashcards() {
    if (ptrHead == NULL) {
        printf("Keine Karteikarten zum Lernen vorhanden.\n");
        return;
    }
    char dummy[10];
    int count = 1;
    FlashcardNode* current = ptrHead;
    while (current != NULL) {
        printf("\nFrage %d: %s\n", count, current->front);
        printf("Druecken Sie Enter, um die Antwort zu sehen...");
        fgets(dummy, sizeof(dummy), stdin);
        printf("Antwort: %s\n", current->back);
        count++;
        current = current->next;
    }
}

/*
 * summary:
 * Vertauscht alle inhaltlichen Felder von zwei FlashcardNode-Knoten,
 * ohne die Verkettung (prev/next) zu verändern.
 */
void swapNodeContents(FlashcardNode* a, FlashcardNode* b) {
    int temp_id = a->id;
    a->id = b->id;
    b->id = temp_id;

    char temp_front[MAX_TEXT_LENGTH];
    char temp_back[MAX_TEXT_LENGTH];
    strcpy(temp_front, a->front);
    strcpy(temp_back, a->back);
    strcpy(a->front, b->front);
    strcpy(a->back, b->back);
    strcpy(b->front, temp_front);
    strcpy(b->back, temp_back);

    int temp_repetitions = a->repetitions;
    a->repetitions = b->repetitions;
    b->repetitions = temp_repetitions;

    double temp_ease_factor = a->ease_factor;
    a->ease_factor = b->ease_factor;
    b->ease_factor = temp_ease_factor;

    int temp_interval = a->interval;
    a->interval = b->interval;
    b->interval = temp_interval;

    time_t temp_last_review = a->last_review;
    a->last_review = b->last_review;
    b->last_review = temp_last_review;

    time_t temp_next_review = a->next_review;
    a->next_review = b->next_review;
    b->next_review = temp_next_review;
}

/*
 * summary:
 * Überprueft, ob eine Datei existiert
 *
 */
int fileExists(const char *filename) {
#ifdef _WIN32
    const DWORD attr = GetFileAttributes(filename);
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
#else
    FILE *file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
#endif
}

/*
 * summary:
 * Funktion zum Speichern der Karteikarten in einer JSON-Datei
 */
void saveFlashcardsToFile() {
    char actualFilename[256];
    strncpy(actualFilename, flashcardsJSONFile, sizeof(actualFilename));
    actualFilename[sizeof(actualFilename) - 1] = '\0';

    FILE *ptr_file = NULL;
    int firstTry = 1;
    while (1) {
        if (firstTry) {
            firstTry = 0;
            if (fileExists(actualFilename)) {
                printf("Die Datei \"%s\" existiert bereits und wird ueberschrieben.\n", actualFilename);
            } else {
                printf("Die Datei \"%s\" wurde nicht gefunden.\n", actualFilename);
            }
        } else {
            printf("Fehler: Konnte die Datei \"%s\" nicht zum Schreiben oeffnen.\n", actualFilename);
        }
        ptr_file = fopen(actualFilename, "w");
        if (ptr_file != NULL) {
            break;
        }
        printf("Bitte geben Sie einen neuen Dateipfad(inkl Namen der Datei | im Container z.B. :""/usr/src/app/FlashcardsMemory.json"") ein (oder \"cancel\" zum Abbrechen): ");
        if (fgets(actualFilename, sizeof(actualFilename), stdin) == NULL) {
            printf("Keine Eingabe erhalten. Abbruch.\n");
            return;
        }
        actualFilename[strcspn(actualFilename, "\n")] = '\0';
        if (strcmp(actualFilename, "cancel") == 0) {
            printf("Speichern abgebrochen.\n");
            return;
        }
    }
    // Aktualisiere den globalen Dateipfad, sodass dieser auch beim nächsten Laden verwendet wird.
    strncpy(flashcardsJSONFile, actualFilename, sizeof(flashcardsJSONFile));
    flashcardsJSONFile[sizeof(flashcardsJSONFile) - 1] = '\0';

    // Schreibe den JSON-Inhalt in die Datei
    fprintf(ptr_file, "{\n  \"cards\": [\n");
    FlashcardNode* current = ptrHead;
    while (current != NULL) {
        fprintf(ptr_file, "    {\n");
        fprintf(ptr_file, "      \"front\": \"%s\",\n", current->front);
        fprintf(ptr_file, "      \"back\": \"%s\",\n", current->back);
        fprintf(ptr_file, "      \"repetitions\": %d,\n", current->repetitions);
        fprintf(ptr_file, "      \"ease_factor\": %.2f,\n", current->ease_factor);
        fprintf(ptr_file, "      \"interval\": %d,\n", current->interval);
        fprintf(ptr_file, "      \"last_review\": %ld,\n", current->last_review);
        fprintf(ptr_file, "      \"next_review\": %ld\n", current->next_review);
        if (current->next == NULL)
            fprintf(ptr_file, "    }\n");
        else
            fprintf(ptr_file, "    },\n");
        current = current->next;
    }
    fprintf(ptr_file, "  ]\n}\n");
    fclose(ptr_file);
    printf("Karteikarten wurden in \"%s\" gespeichert.\n", flashcardsJSONFile);
}

/* Gibt alle Karteikarten frei und setzt head und tail zurück */
void freeFlashcards() {
    FlashcardNode* current = ptrHead;
    while (current != NULL) {
        FlashcardNode* temp = current;
        current = current->next;
        free(temp);
    }
    ptrHead = NULL;
    ptr_tail = NULL;
}

/* Funktion zum Laden der Karteikarten aus einer JSON-Datei */
/*
 * Lädt Karteikarten aus einer JSON-Datei und fügt sie in die verkettete Liste ein.
 * Dabei werden neben "front" und "back" auch zusätzliche Felder wie "repetitions",
 * "ease_factor", "interval", "last_review" und "next_review" eingelesen.
 */
void loadFlashcardsFromFile(const char *filename) {
    // Öffne die Datei zum Lesen.
    FILE *file = fopen(filename, "r");
    if (!file) {
        // Falls die Datei nicht existiert, gebe eine Meldung aus und beende die Funktion.
        printf("Datei \"%s\" nicht gefunden, starte mit leeren Karteikarten.\n", filename);
        return;
    }

    // Vor dem Laden alle vorhandenen Karteikarten freigeben.
    freeFlashcards();

    char line[512];  // Buffer zum Einlesen einer Zeile aus der Datei.

    // Durchlaufe die Datei Zeile für Zeile.
    while (fgets(line, sizeof(line), file)) {
        // Falls die Zeile den String "\"front\":" enthält, handelt es sich um den Start eines neuen Karteikarteneintrags.
        if (strstr(line, "\"front\":") != NULL) {
            // Allokiere Speicher für einen neuen FlashcardNode.
            FlashcardNode* newNode = malloc(sizeof(FlashcardNode));
            if (!newNode) {
                printf("Fehler bei der Speicherzuweisung.\n");
                fclose(file);
                return;
            }
            // Initialisiere die Zeiger des neuen Knotens.
            newNode->prev = NULL;
            newNode->next = NULL;

            // --- Parsing des "front" Feldes ---
            char frontText[MAX_TEXT_LENGTH] = "";
            // Finde den Doppelpunkt, der den Schlüssel vom Wert trennt.
            char *colon = strchr(line, ':');
            if (colon) {
                // Finde das erste Anführungszeichen nach dem Doppelpunkt.
                char *start = strchr(colon, '\"');
                if (start) {
                    start++;  // Überspringe das erste Anführungszeichen.
                    // Finde das nächste Anführungszeichen, das das Ende des Wertes markiert.
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
            // Speichere den eingelesenen Text in newNode->front.
            strncpy(newNode->front, frontText, MAX_TEXT_LENGTH);
            newNode->front[MAX_TEXT_LENGTH - 1] = '\0';

            // --- Parsing des "back" Feldes ---
            // Lese die nächste Zeile, in der der "back" Wert erwartet wird.
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

            // --- Parsing des "repetitions" Feldes ---
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

            // --- Parsing des "ease_factor" Feldes ---
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
            newNode->ease_factor = ease_factor;

            // --- Parsing des "interval" Feldes ---
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

            // --- Parsing des "last_review" Feldes ---
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
            newNode->last_review = last_review;

            // --- Parsing des "next_review" Feldes ---
            if (!fgets(line, sizeof(line), file)) break;
            time_t next_review = time(NULL);  // Standardwert, falls Parsing fehlschlägt.
            char *nextPtr = strstr(line, "\"next_review\":");
            if (nextPtr) {
                char *colonNext = strchr(nextPtr, ':');
                if (colonNext) {
                    next_review = (time_t)atol(colonNext + 1);
                }
            }
            newNode->next_review = next_review;

            // --- Hinzufügen des neuen Knotens zur verketteten Liste ---
            if (ptrHead == NULL) {
                // Falls die Liste leer ist, wird newNode der erste Knoten.
                ptrHead = newNode;
                ptr_tail = newNode;
            } else {
                // Andernfalls wird newNode am Ende der Liste angehängt.
                ptr_tail->next = newNode;
                newNode->prev = ptr_tail;
                ptr_tail = newNode;
            }
        }
    }
    // Schließe die Datei, da das Einlesen abgeschlossen ist.
    fclose(file);

    // Vergibt fortlaufend neue IDs an die Knoten in der Liste.
    int count = 1;
    FlashcardNode* current = ptrHead;
    while (current != NULL) {
        current->id = count++;
        current = current->next;
    }
    next_id = count;
    printf("%d Karteikarten geladen.\n", count - 1);
}


/* Funktion zum Löschen einer Karteikarte anhand ihrer Nummer */
void deleteFlashcard() {
    if (ptrHead == NULL) {
        printf("Keine Karteikarten zum Loeschen vorhanden.\n");
        return;
    }
    int number;
    char buffer[10];
    printf("Gib die Nummer der zu loeschenden Karte ein: ");
    if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        return;
    number = atoi(buffer);
    if (number < 1) {
        printf("Ungueltige Nummer.\n");
        return;
    }
    int count = 1;
    FlashcardNode* current = ptrHead;
    while (current != NULL && count < number) {
        current = current->next;
        count++;
    }
    if (current == NULL) {
        printf("Keine Karte mit dieser Nummer gefunden.\n");
        return;
    }
    if (current->prev)
        current->prev->next = current->next;
    else
        ptrHead = current->next;
    if (current->next)
        current->next->prev = current->prev;
    else
        ptr_tail = current->prev;
    free(current);
    printf("Karteikarte %d wurde geloescht.\n", number);
}

/* Funktion zum Neu-"nummerieren" der Karteikarten */
void renumberFlashcards() {
    printf("Die Karteikarten wurden neu nummeriert.\n");
}

/* Sortiert die Karteikarten anhand der id.
   Bei ascending ≠ 0 erfolgt eine aufsteigende Sortierung, sonst absteigend. */
void sortFlashcardsById(int ascending) {
    if (ptrHead == NULL)
        return;
    int swapped;
    do {
        swapped = 0;
        FlashcardNode* current = ptrHead;
        while (current->next != NULL) {
            int condition = ascending ? (current->id > current->next->id) : (current->id < current->next->id);
            if (condition) {
                swapNodeContents(current, current->next);
                swapped = 1;
            }
            current = current->next;
        }
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
    double efactor = curr->ease_factor;
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
    curr->last_review = now;
    curr->next_review = now + (time_t)(interval * seconds_per_minute);
    curr->repetitions = rep;
    curr->ease_factor = efactor;
    curr->interval = interval;
}

/*
 * summary:
 * Sortiert die verkettete Liste der Flashcards mittels Bubble Sort anhand des next_review Zeitstempels (aufsteigend).
 */
void bubbleSortByNextReview() {
    if (ptrHead == NULL)
        return;
    int swapped;
    do {
        swapped = 0;
        FlashcardNode* current = ptrHead;
        while (current->next != NULL) {
            if (current->next_review > current->next->next_review) {
                swapNodeContents(current, current->next);
                swapped = 1;
            }
            current = current->next;
        }
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
        // Versuche, das angegebene Verzeichnis zu erstellen (falls nicht vorhanden)
        if (my_mkdir(userInput) != 0) {
            /* Hier könnte man errno prüfen;
             * wenn der Fehler nicht "Directory exists" ist, könnte man eine Warnung ausgeben.
             * Für diese Implementierung ignorieren wir einen Fehler, falls das Verzeichnis bereits existiert.
             */
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
        if (difftime(now, current->next_review) >= 0) {
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
                renumberFlashcards();
                freeFlashcards();
                printf("Programm beendet.\n");
                exit(0);
            default:
                printf("Ungueltige Eingabe. Bitte erneut versuchen.\n");
        }
    }
}
