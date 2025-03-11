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

// Struct für die Karteikarten
typedef struct FlashcardNode {
    int id;  // eindeutige Nummer für die Sortierung
    char front[MAX_TEXT_LENGTH];
    char back[MAX_TEXT_LENGTH];
    int repetitions;       // Anzahl der Wiederholungen (hintereinander richtige Antworten)
    double easeFactor;    // E-Faktor (Lernfaktor zur Intervallanpassung)
    int interval;          // aktuelles Intervall
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
 *summary:
 * Entfernt Zeilenumbruch aus einem String
 */
void removeNewline(char *ptrStr) {
    ptrStr[strcspn(ptrStr, "\n")] = '\0';
}

/// Hilfsfunktion für loadFlashcardsToFile, prüft ob eine Datei vorhanden ist
int fileExists(const char *filename) {
    const DWORD attr = GetFileAttributes(filename);
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

/// Hilfsfunktion für loadConfig, um die Existenz des Pfades einer Datei zu prüfen
int directoryExists(const char *path) {
    DWORD attr = GetFileAttributes(path);
    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

/*
 * summary:
 * Liest den Ordnerpfad aus der Konfigurationsdatei. Falls die Datei nicht existiert
 * oder der darin gespeicherte Ordner ungültig ist, wird der Nutzer aufgefordert, einen
 * gültigen Ordnerpfad einzugeben. Der vollständige Pfad (Ordner + FlashcardsMemory.json)
 * wird dann in der Config gespeichert.
 */
void loadConfig(char *path) {
    // versucht, die Konfigurationsdatei zum Lesen zu öffnen.
    FILE *config = fopen(CONFIG_FILE, "r");
    if (config == NULL) {
        // wenn die Config-Datei nicht gefunden wurde, wird der Benutzer zur Eingabe eines Ordnerpfads aufgefordert.
        while (1) {
            printf("Konfigurationsdatei nicht gefunden.\nGeben Sie den ORDNERpfad ein, in dem die Flashcards gespeichert werden sollen:\n> ");
            char userInput[MAX_PATH_LEN];
            if (fgets(userInput, MAX_PATH_LEN, stdin) != NULL) {
                // Entferne das Newline-Zeichen aus der Benutzereingabe.
                userInput[strcspn(userInput, "\n")] = '\0';
            }
            // prüft, ob überhaupt etwas eingegeben wurde.
            if (strlen(userInput) == 0) {
                printf("Kein Ordner angegeben. Abbruch.\n");
                exit(1);
            }
            // stellt sicher, dass der eingegebene Pfad mit einem Trenner endet.
            size_t len = strlen(userInput);
            if (len > 0 && userInput[len - 1] != '\\' && userInput[len - 1] != '/') {
                // fügt einen Backslash hinzu.
                strncat(userInput, "\\", sizeof(userInput) - strlen(userInput) - 1);
            }
            // überprüft, ob das angegebene Verzeichnis existiert.
            if (!directoryExists(userInput)) {
                printf("Der Ordner \"%s\" existiert nicht. Bitte geben Sie einen gültigen Pfad ein.\n", userInput);
                continue;  // Schleife erneut beginnen, falls das Verzeichnis nicht existiert.
            }
            // erstellt den vollständigen Pfad zur JSON-Datei, indem Dateinamen angehängt wird.
            snprintf(path, MAX_PATH_LEN, "%sFlashcardsMemory.json", userInput);
            // schreibt den neuen Pfad in die Konfigurationsdatei.
            config = fopen(CONFIG_FILE, "w");
            if (config) {
                fprintf(config, "%s\n", path);
                fclose(config);
                printf("Konfigurationsdatei erstellt. Speicherort: %s\n", path);
            }
            break;  // verlässt die Schleife, da ein gültiger Pfad gefunden und gespeichert wurde.
        }
    } else {
        // wenn die Config-Datei existiert, lese den gespeicherten Pfad aus.
        if (fgets(path, MAX_PATH_LEN, config) != NULL) {
            path[strcspn(path, "\n")] = '\0';  // entferne das Newline-Zeichen.
        }
        fclose(config);
        // extrahiert den Ordnerpfad (ohne Dateinamen) aus dem vollständigen Pfad.
        char folderPath[MAX_PATH_LEN];
        strncpy(folderPath, path, MAX_PATH_LEN);
        char *lastSlash = strrchr(folderPath, '\\');
        if (!lastSlash)
            lastSlash = strrchr(folderPath, '/');
        if (lastSlash)
            *lastSlash = '\0';  // Setze dort das Ende des Ordnerpfads.
        // überprüft, ob der in der Config gespeicherte Ordner existiert.
        if (!directoryExists(folderPath)) {
            printf("Der in der Config gespeicherte Ordner \"%s\" existiert nicht.\n", folderPath);
            // falls nicht, fordere den Benutzer erneut zur Eingabe eines gültigen Ordnerpfads auf.
            while (1) {
                printf("Bitte geben Sie einen gültigen ORDNERpfad ein:\n> ");
                char userInput[MAX_PATH_LEN];
                if (fgets(userInput, MAX_PATH_LEN, stdin) != NULL) {
                    userInput[strcspn(userInput, "\n")] = '\0';
                }
                if (strlen(userInput) == 0) {
                    printf("Kein Ordner angegeben. Abbruch.\n");
                    exit(1);
                }
                size_t len = strlen(userInput);
                if (len > 0 && userInput[len - 1] != '\\' && userInput[len - 1] != '/') {
                    strncat(userInput, "\\", sizeof(userInput) - strlen(userInput) - 1);
                }
                if (!directoryExists(userInput)) {
                    printf("Der Ordner \"%s\" existiert nicht. Versuchen Sie es erneut.\n", userInput);
                    continue;
                }
                // erstellt den neuen vollständigen Pfad.
                snprintf(path, MAX_PATH_LEN, "%sFlashcardsMemory.json", userInput);
                // schreibt den neuen Pfad in die Konfigurationsdatei.
                config = fopen(CONFIG_FILE, "w");
                if (config) {
                    fprintf(config, "%s\n", path);
                    fclose(config);
                    printf("Konfigurationsdatei aktualisiert. Neuer Speicherort: %s\n", path);
                }
                break;
            }
        }
    }
}

/*
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

/*
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

/*
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

/*
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

/*
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

/*
 * summary:
 * Funktion zum Speichern der Karteikarten in einer JSON-Datei.
 * Der Nutzer gibt dabei nur den Ordnerpfad an, der Dateiname "FlashcardsMemory.json" wird automatisch ergänzt.
 * Bei fehlgeschlagener Öffnung wird der Nutzer zur Eingabe eines neuen Ordners aufgefordert.
 */
void saveFlashcardsToFile() {
    // Dateiname wird initialisiert
    char actualFilename[MAX_PATH_LEN]; // speichert den vollständigen Pfad inkl. Dateiname
    strncpy(actualFilename, flashcardsJSONFile, sizeof(actualFilename));
    actualFilename[sizeof(actualFilename) - 1] = '\0'; // String wird terminiert

    FILE *ptrFile = NULL;
    int firstTry = 1; // damit beim ersten Versuch eine andere Fehlermeldung erscheint
    while (1) {
        if (firstTry) {
            firstTry = 0;
            if (fileExists(actualFilename)) {
                printf("Hinweis: Die Datei \"%s\" existiert bereits und wird ueberschrieben.\n", actualFilename);
            } else {
                printf("Die Datei \"%s\" wurde nicht gefunden. Es wird eine neue Datei erstellt.\n", actualFilename);
            }
        } else {
            printf("Fehler: Die Datei \"%s\" konnte nicht zum Schreiben geoeffnet werden.\n", actualFilename);
        }

        // Datei soll im Schreibmodus "w" geöffnet werden (sie soll ja ueberschrieben werden)
        ptrFile = fopen(actualFilename, "w");
        if (ptrFile != NULL) {
            break;
        }

        // falls die Datei nicht geöffnet werden kann, soll der Nutzer einen neuen Ordnerpfad eingeben.
        printf("Bitte geben Sie einen neuen Speicherordner ein (oder \"cancel\" zum Abbrechen): ");
        char userInput[MAX_PATH_LEN];
        if (fgets(userInput, sizeof(userInput), stdin) == NULL) {
            printf("Keine Eingabe erhalten. Abbruch.\n");
            return;
        }
        userInput[strcspn(userInput, "\n")] = '\0';
        if (strcmp(userInput, "cancel") == 0) {
            printf("Speichern abgebrochen.\n");
            return;
        }
        // stellt sicher, dass der Ordnerpfad mit einem Trenner endet
        size_t len = strlen(userInput);
        if (len > 0 && userInput[len - 1] != '\\' && userInput[len - 1] != '/') {
            strncat(userInput, "\\", sizeof(userInput) - strlen(userInput) - 1);
        }
        // fügt den Dateinamen "FlashcardsMemory.json" an den Ordnerpfad an.
        snprintf(actualFilename, sizeof(actualFilename), "%sFlashcardsMemory.json", userInput);
    }

    // aktualisiert den globalen Dateipfad, sodass dieser auch beim naechsten Laden verwendet wird.
    strncpy(flashcardsJSONFile, actualFilename, sizeof(flashcardsJSONFile));
    flashcardsJSONFile[sizeof(flashcardsJSONFile) - 1] = '\0';

    // schreibt den JSON-Inhalt in die Datei
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
    fclose(ptrFile);
    printf("Karteikarten wurden in \"%s\" gespeichert.\n", flashcardsJSONFile);
}

/*
 * summary:
 * lädt Flashcards aus einer JSON-Datei in eine verkettete Liste. Existierende Karten werden freigegeben,
 * neue Karten zeilenweise geparst und angehängt, und am Ende werden fortlaufende IDs vergeben.
 */
void loadFlashcardsFromFile(const char *filename) {
    // öffnet die Datei zum Lesen.
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
            char *colon = strchr(line, ':');
            if (colon) {
                char *start = strchr(colon, '\"');
                if (start) {
                    start++;  // überspringt das erste Anführungszeichen.
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
            strncpy(newNode->front, frontText, MAX_TEXT_LENGTH);
            newNode->front[MAX_TEXT_LENGTH - 1] = '\0';

            // -------- Parsing des "back" Feldes --------
            if (!fgets(line, sizeof(line), file)) break;
            char backText[MAX_TEXT_LENGTH] = "";
            char *backPtr = strstr(line, "\"back\":");
            if (backPtr) {
                char *colonBack = strchr(backPtr, ':');
                if (colonBack) {
                    char *startBack = strchr(colonBack, '\"');
                    if (startBack) {
                        startBack++;  // Überspringe das erste Anführungszeichen.
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
            strncpy(newNode->back, backText, MAX_TEXT_LENGTH);
            newNode->back[MAX_TEXT_LENGTH - 1] = '\0';

            // -------- Parsing des "repetitions" Feldes --------
            if (!fgets(line, sizeof(line), file)) break;
            int repetitions = 0;
            char *repPtr = strstr(line, "\"repetitions\":");
            if (repPtr) {
                char *colonRep = strchr(repPtr, ':');
                if (colonRep) {
                    repetitions = atoi(colonRep + 1);
                }
            }
            newNode->repetitions = repetitions;

            // -------- Parsing des "easeFactor" Feldes --------
            if (!fgets(line, sizeof(line), file)) break;
            double easeFactor = 2.5;
            // Angepasst: Suche nach "easeFactor" (ohne Unterstrich)
            char *efPtr = strstr(line, "\"easeFactor\":");
            if (efPtr) {
                char *colonEf = strchr(efPtr, ':');
                if (colonEf) {
                    easeFactor = atof(colonEf + 1);
                }
            }
            newNode->easeFactor = easeFactor;

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

            // -------- Parsing des "lastReview" Feldes --------
            if (!fgets(line, sizeof(line), file)) break;
            time_t last_review = time(NULL);
            char *lastPtr = strstr(line, "\"lastReview\":");
            if (lastPtr) {
                char *colonLast = strchr(lastPtr, ':');
                if (colonLast) {
                    last_review = (time_t)atol(colonLast + 1);
                }
            }
            newNode->lastReview = last_review;

            // -------- Parsing des "nextReview" Feldes --------
            if (!fgets(line, sizeof(line), file)) break;
            time_t next_review = time(NULL);
            char *nextPtr = strstr(line, "\"nextReview\":");
            if (nextPtr) {
                char *colonNext = strchr(nextPtr, ':');
                if (colonNext) {
                    next_review = (time_t)atol(colonNext + 1);
                }
            }
            newNode->nextReview = next_review;

            // -------- Hinzufügen des neuen Knotens zur verketteten Liste --------
            if (ptrHead == NULL) {
                ptrHead = newNode;
                ptrTail = newNode;
            } else {
                ptrTail->next = newNode;
                newNode->prev = ptrTail;
                ptrTail = newNode;
            }
        }
    }

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
 * sortiert die Karteikarten in aufsteigender oder absteigender Reihenfolge anhand ihrer ID
 * tauscht die Zeiger (prev und next) der Knoten, statt deren Inhalte zu vertauschen
 * ascending = 1, sortiert aufsteigend; ist ascending = 0, sortiert absteigend
 */
void sortFlashcardsById(int ascending) {
    // prüft, ob die Liste leer ist oder nur ein Element enthält; wenn ja, verlässt die Funktion
    if (ptrHead == NULL || ptrHead->next == NULL)
        return;

    int swapped;
    do {
        swapped = 0;
        FlashcardNode *current = ptrHead;
        while (current->next != NULL) {
            FlashcardNode *nextNode = current->next;
            int condition = ascending ? (current->id > nextNode->id) : (current->id < nextNode->id);
            if (condition) {
                // passt den Zeiger des Vorgängers an
                if (current->prev)
                    current->prev->next = nextNode;
                else
                    ptrHead = nextNode;

                // passt den Zeiger des Nachfolgers von NextNode an
                if (nextNode->next)
                    nextNode->next->prev = current;
                else
                    ptrTail = current;

                // vertauscht die Zeiger von Current und NextNode
                current->next = nextNode->next;
                nextNode->prev = current->prev;
                nextNode->next = current;
                current->prev = nextNode;

                swapped = 1;
                // bleibt an derselben Stelle, um den neuen Nachfolger zu vergleichen
            } else {
                current = current->next;
            }
        }
    } while (swapped);
}

/*
 * summary:
 * aktualisiert Wiederholungsintervall einer Karte mittels SM-2-Algorithmus.
 * Die Intervalle werden in Minuten angegeben:
 *  - bei der ersten erfolgreichen Wiederholung: 10 Minuten,
 *  - bei der zweiten: 30 Minuten,
 *  - danach: multipliziere das vorherige Intervall mit dem E-Faktor.
 */
void updateCardInterval(FlashcardNode *curr, int rating) {
    int rep = curr->repetitions;       // übernimmt die bisherige Anzahl Wiederholungen
    double efactor = curr->easeFactor; // übernimmt den aktuellen E-Faktor
    int interval = curr->interval;     // übernimmt das aktuelle Intervall
    int quality;                       // bestimmt die Bewertung der Antwort

    // setzt die Qualität basierend auf dem Rating:
    // rating == 1 => sehr gutes Ergebnis,
    // rating == 0 => gerade noch gewusst,
    // rating == -1 => nicht gewusst.
    if (rating == 1) {
        quality = 5; // sehr gutes Ergebnis
    } else if (rating == 0) {
        quality = 3; // gerade so gewusst
    } else { // rating == -1
        quality = 2; // nicht gewusst
    }

    // berechnet den neuen E-Faktor
    efactor = efactor + (0.1 - (5 - quality) * (0.08 + (5 - quality) * 0.02));
    if (efactor < 1.3) {
        efactor = 1.3; // setzt den E-Faktor auf das Minimum, falls nötig
    }

    // wenn die Qualität "nicht gewusst" ist, wird die Wiederholung zurückgesetzt,
    // andernfalls wird die Anzahl Wiederholungen erhöht.
    if (quality < 3) {
        rep = 0;  // reset bei schlechter Antwort
    } else {
        rep += 1; // erhöht die Anzahl Wiederholungen
    }

    // bestimmt das neue Intervall basierend auf der Anzahl Wiederholungen:
    // - bei der ersten Wiederholung: 10 Minuten,
    // - bei der zweiten: 30 Minuten,
    // - bei weiteren: multipliziere das bisherige Intervall mit dem E-Faktor.
    if (rep <= 1) {
        interval = 10;   // erste Wiederholung: 10 Minuten
    } else if (rep == 2) {
        interval = 30;   // zweite Wiederholung: 30 Minuten
    } else {
        interval = (int)ceil(interval * efactor);
    }

    time_t now = time(NULL);               // ermittelt die aktuelle Zeit
    double seconds_per_minute = 60;          // definiert die Anzahl Sekunden pro Minute

    // aktualisiert die Zeitstempel:
    // setzt LastReview auf jetzt und berechnet nextReview anhand des neuen Intervalls.
    curr->lastReview = now;
    curr->nextReview = now + (time_t)(interval * seconds_per_minute);

    // speichert die aktualisierten Werte in der Karte.
    curr->repetitions = rep;
    curr->easeFactor = efactor;
    curr->interval = interval;
}

/*
 * summary:
 * sortiert die verkettete Liste der Flashcards anhand des NextReview Zeitstempels (aufsteigend),
 * sodass die am längsten überfälligen Karteikarten zuerst erscheinen.
 */
void bubbleSortByNextReview() {
    // Falls die Liste leer oder nur ein Element vorhanden ist, ist kein Sortieren nötig.
    if (ptrHead == NULL || ptrHead->next == NULL)
        return;

    int swapped;
    FlashcardNode *last = NULL; // Markiert das Ende der bereits sortierten Elemente.

    do {
        swapped = 0;
        FlashcardNode *current = ptrHead;

        // solange current->next noch nicht das "last"-Element erreicht hat
        while (current->next != last) {
            FlashcardNode *nextNode = current->next;
            // wenn current und nextNode in falscher Reihenfolge stehen:
            if (current->nextReview > nextNode->nextReview) {
                // tausch der Knoten in der verketteten Liste

                // verbindet den Vorgänger von current mit nextNode
                if (current->prev)
                    current->prev->next = nextNode;
                else
                    ptrHead = nextNode; // current war Kopf der Liste

                // verbindet den Nachfolger von nextNode mit current
                if (nextNode->next)
                    nextNode->next->prev = current;
                else
                    ptrTail = current; // nextNode war am Ende der Liste

                // vertauscht die Zeiger zwischen current und nextNode
                current->next = nextNode->next;
                nextNode->prev = current->prev;
                nextNode->next = current;
                current->prev = nextNode;

                swapped = 1;
                // Nach einem Tausch soll current nicht sofort ++ werden,
                // da current nun an der neuen Position erneut geprüft werden muss.
                continue;
            }
            // Keine Änderung nötig – gehe zum nächsten Element.
            current = current->next;
        }
        // Das letzte Element, das im Durchlauf geprüft wurde, gilt als sortiert.
        last = current;
    } while (swapped);
}


/*
 * summary:
 * Funktion für das Lernen im Testmodus: Karten werden basierend auf dem next_review Zeitstempel abgefragt.
 * Es werden nur Karten abgefragt, deren next_review <= der aktuellen Zeit ist.
 */
void studyFlashcardsByDueDateTest() {
    if (ptrHead == NULL) {
        // keine Karteikarten zum Lernen vorhanden
        printf("keine Karteikarten zum lernen vorhanden.\n");
        return;
    }
    char dummy[10];
    int count = 1;
    // sortiert Karten anhand des nextReview Zeitstempels
    bubbleSortByNextReview();
    FlashcardNode* current = ptrHead;
    time_t now = time(NULL);
    while (current != NULL) {

        if (difftime(now, current->nextReview) >= 0) {
            // gibt fällige Karte aus
            printf("\n[due Karte %d] Frage: %s\n", count, current->front);
            // wartet auf Enter, zum anzeigen der Antwort
            printf("druecken sie enter, um die Antwort zu sehen...");
            fgets(dummy, sizeof(dummy), stdin);
            printf("antwort: %s\n", current->back);
            // fragt Bewertung ab
            printf("bewerten sie diese Karte (-1=schlecht, 0=schwer, 1=einfach): ");

            if (fgets(dummy, sizeof(dummy), stdin) != NULL) {
                int rating = atoi(dummy);
                // aktualisiert Intervall der Karte
                updateCardInterval(current, rating);
            }
            count++;
        }
        current = current->next;
    }
}

int main() {
    // Beim Programmstart wird die Config geladen.
    // Falls keine Config existiert, fragt das Programm nach dem ORDNERpfad,
    // in dem die JSON-Datei (FlashcardsMemory.json) gespeichert werden soll.
    loadConfig(flashcardsJSONFile);

    // Anschließend werden die Karteikarten aus der JSON-Datei geladen.
    loadFlashcardsFromFile(flashcardsJSONFile);

    // Speichere den aktuell geladenen Pfad erneut in der Konfigurationsdatei.
    FILE *config = fopen(CONFIG_FILE, "w");
    if (config) {
        fprintf(config, "%s\n", flashcardsJSONFile);
        fclose(config);
    }

    int choice;
    char input[10];

    // Endlosschleife für das Hauptmenü der Anwendung.
    while (1) {
        // Ausgabe des Menüs
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
        printf("10. Neuen Config-Pfad eingeben (In diesem Pfad wird dann eine neue 'FlashcardsMemory' gespeichert)\n");
        printf("Ihre Wahl: ");

        // einlesen der Nutzereingabe
        if (fgets(input, sizeof(input), stdin) == NULL)
            continue;
        choice = atoi(input);

        // Switch-Case-Struktur zur Auswahl der Funktionalitäten
        switch (choice) {
            case 1:
                // neue Karteikarte hinzufügen
                addFlashcard();
                break;
            case 2:
                // alle Karteikarten anzeigen
                listFlashcards();
                break;
            case 3:
                // Karteikarten im Standardmodus lernen
                studyFlashcards();
                break;
            case 4:
                // Karteikarten in der JSON-Datei speichern
                saveFlashcardsToFile();
                break;
            case 5:
                // eine Karteikarte anhand ihrer Nummer löschen
                deleteFlashcard();
                break;
            case 6:
                // Karteikarten aufsteigend sortieren (nach ID)
                sortFlashcardsById(1);
                printf("Karteikarten wurden aufsteigend sortiert.\n");
                break;
            case 7:
                // Karteikarten absteigend sortieren (nach ID)
                sortFlashcardsById(0);
                printf("Karteikarten wurden absteigend sortiert.\n");
                break;
            case 8:
                // Karteikarten im Spaced Repetition Modus lernen (nach Timestamp sortiert)
                studyFlashcardsByDueDateTest();
                break;
            case 9:
                // vor dem Beenden werden die Karteikarten gespeichert und freigegeben
                saveFlashcardsToFile();
                freeFlashcards();
                printf("Programm beendet.\n");
                exit(0);
            // Dient nur dem Nutzerkomfort, falls man mehrere Karteikartendecks hat, kann man diese aufrufen (Pro Ordner aber nur eine Datei sonst würde sich der Name doppeln)
            case 10: {
                // neuen Konfigurationspfad eingeben
                printf("Geben Sie den neuen ORDNERpfad ein, in dem die Flashcards gespeichert werden sollen:\n> ");
                char newPath[MAX_PATH_LEN];
                // einlesen des neuen Pfads
                if (fgets(newPath, sizeof(newPath), stdin) == NULL) {
                    printf("Fehlerhafte Eingabe.\n");
                    break;
                }
                // entfernt das Newline-Zeichen vom Ende der Eingabe
                newPath[strcspn(newPath, "\n")] = '\0';
                // Prüfe, ob der Benutzer überhaupt etwas eingegeben hat
                if (strlen(newPath) == 0) {
                    printf("Kein Ordner angegeben. Abbruch der Pfadaenderung.\n");
                    break;
                }
                size_t len = strlen(newPath);
                // Stelle sicher, dass der Pfad mit einem Trenner endet
                if (len > 0 && newPath[len - 1] != '\\' && newPath[len - 1] != '/') {
                    strncat(newPath, "\\", sizeof(newPath) - strlen(newPath) - 1);
                }
                // überprüft, ob das angegebene Verzeichnis existiert
                if (!directoryExists(newPath)) {
                    printf("Der Ordner \"%s\" existiert nicht. Pfadaenderung abgebrochen.\n", newPath);
                    break;
                }
                // aktualisiert den globalen Pfad inklusive Dateinamen "FlashcardsMemory.json"
                snprintf(flashcardsJSONFile, MAX_PATH_LEN, "%sFlashcardsMemory.json", newPath);
                // schreibt den neuen Pfad in die Konfigurationsdatei
                config = fopen(CONFIG_FILE, "w");
                if (config) {
                    fprintf(config, "%s\n", flashcardsJSONFile);
                    fclose(config);
                    printf("Neuer Konfigurationspfad gesetzt: %s\n", flashcardsJSONFile);
                } else {
                    printf("Fehler beim Schreiben der Konfigurationsdatei.\n");
                }
                // lädt Flashcards aus der neuen Datei
                loadFlashcardsFromFile(flashcardsJSONFile);
                break;
            }
            default:
                printf("Ungueltige Eingabe. Bitte erneut versuchen.\n");
        }
    }
}
