// PortainerIO
// openmediavault ist das Betriebssystem
// als Stack die Datenbank definieren mit PORT usw

// DIE TIMESTAMPS SOLLEN NOCH IN DER JSON GESPEICHERT WERDEN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tgmath.h>
#include <time.h>
#ifdef _WIN32
  #include <windows.h>
#endif

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
    time_t last_review;    // Zeitpunkt der letzten Abfrage
    time_t next_review;    // Zeitpunkt, wann die Karte wieder fällig ist
    struct FlashcardNode *prev;
    struct FlashcardNode *next;
} FlashcardNode;

// Globale Zeiger auf Kopf und Ende der Liste sowie globaler Zähler für IDs
FlashcardNode* ptrHead = NULL;
FlashcardNode* ptr_tail = NULL;
int next_id = 1; // Nächste zu vergebende ID

/*FERTIG
 *summary:
 * Entfernt das Newline-Zeichen aus einem String
 */
void removeNewline(char *ptrStr) {
    ptrStr[strcspn(ptrStr, "\n")] = '\0';
}

/*FERTIG
 *summary:
 * erstellt eine neue Karteikarte, indem zuerst Speicher allokiert wird.
 * anschließend werden die Felder durch Konsoleneingabe definiert.
*/
void addFlashcard() {
    // Speicher für den neuen Node, die Karteikarte allokieren
    FlashcardNode* ptrNewNode = malloc(sizeof(FlashcardNode));

    // Fehlermeldung, falls kein Speicher allokiert werden konnte
    if (!ptrNewNode) {
        printf("Fehler bei der Speicherzuweisung.\n");
        return;
    }
    // prev und next zeigen auf NULL weil das Element noch keine Nachbarn hat
    ptrNewNode->prev = NULL;
    ptrNewNode->next = NULL;
    ptrNewNode->id = next_id++; // Vergibt fortlaufende ID

    // Initialisierung der SM-2 Felder für den SpacedRep Algorithmus
    ptrNewNode->repetitions = 0;
    ptrNewNode->ease_factor = 2.5;
    ptrNewNode->interval = 1; // Anfangswert (bei TEST_MODE entspricht 1 Minute, ansonsten 1 Tag)
    ptrNewNode->last_review = time(NULL);
    ptrNewNode->next_review = ptrNewNode->last_review; // sofort fällig

    // Die Vorderseite der Karte wird befüllt
    printf("Geben Sie die Vorderseite (Frage) der Karte ein:\n> ");
    if (fgets(ptrNewNode->front, MAX_TEXT_LENGTH, stdin) == NULL) {
        free(ptrNewNode);
        return;
    }
    removeNewline(ptrNewNode->front);

    printf("Geben Sie die Rückseite der Karte ein:\n> ");
    if (fgets(ptrNewNode->back, MAX_TEXT_LENGTH, stdin) == NULL) {
        free(ptrNewNode);
        return;
    }
    removeNewline(ptrNewNode->back);

    // Knoten ans Ende der Liste anhängen
    if (ptrHead == NULL) {
        ptrHead = ptrNewNode;
        ptr_tail = ptrNewNode;
    } else {
        ptr_tail->next = ptrNewNode;
        ptrNewNode->prev = ptr_tail;
        ptr_tail = ptrNewNode;
    }
    printf("Karteikarte hinzugefügt.\n");
}

/* FERTIG
 *summary:
 * Diese Funktion listet alle vorhandenen Karteikarten in der verketteten Liste aus
 * Dabei zeigt sie auch den Zeitpunkt an, wann die Karteikarte das nächste Mal gelernt wird
 */
void listFlashcards() {
    // Es wird geprüft ob die Liste überhaupt Elemente enthält
    if (ptrHead == NULL) {
        printf("Keine Karteikarten vorhanden.\n");
        return;
    }
    int count = 1; // count nummeriert die Karten durch
    FlashcardNode* current = ptrHead;
    char timeBuffer[100]; // Zwischenspeicher für das Datum/Zeit-Format

    // geht durch die ganze Liste bis ein Nullpointer kommt
    while (current != NULL) {

        // wandelt die Funktion localtime in einen Struct mit den Infos um
        struct tm *tmInfo = localtime(&current->next_review);
        strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", tmInfo);

        // Ausgabe aller informationen über die Karte
        printf("%d. (ID: %d)\n  Vorderseite: %s\n  Rueckseite: %s\n  Nächste Wiederholung: %s\n",
            count, current->id, current->front, current->back, timeBuffer);
        count++;
        current = current->next;
    }
}


/*
 * summary:
 * Funktion zum Lernen der Karteikarten ohne Spaced Rep Funktionalität einfach nur zum durchgehen
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

        // In dieser Funktion wird der spacedRepCount nicht verwendet,
        // da der SM-2 Algorithmus die Wiederholungen über "repetitions" verwaltet.
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

    // Der spacedRepCount wird nicht mehr verwendet und wurde entfernt

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
 * Überprüft, ob eine Datei existiert
 */
int fileExists(const char *filename) {
#ifdef _WIN32
    DWORD attr = GetFileAttributes(filename);
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
void saveFlashcardsToFile(const char *ptr_filename) {
    if (fileExists(ptr_filename)) {
        printf("Die Datei \"%s\" existiert bereits und wird überschrieben.\n", ptr_filename);
    } else {
        printf("Die Datei \"%s\" wurde nicht gefunden. Es wird eine neue Datei angelegt.\n", ptr_filename);
    }

    FILE *ptr_file = fopen(ptr_filename, "w");
    if (!ptr_file) {
        printf("Fehler: Konnte die Datei \"%s\" nicht zum Schreiben öffnen.\n", ptr_filename);
        return;
    }

    // JSON-Header schreiben
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
    printf("Karteikarten wurden in \"%s\" gespeichert.\n", ptr_filename);
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
void loadFlashcardsFromFile(const char *filename) {
    FILE *file = fopen(filename, "r");
    //Keine Datei gefunden
    if (!file) {
        printf("Datei \"%s\" nicht gefunden – starte mit leeren Karteikarten.\n", filename);
        return;
    }

    // Bestehende Karteikarten freigeben
    freeFlashcards();

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        // Suche nach "front":
        char *frontPtr = strstr(line, "\"front\":");
        if (frontPtr) {
            char frontText[MAX_TEXT_LENGTH] = "";
            char *colon = strchr(frontPtr, ':');
            if (colon) {
                char *start = strchr(colon, '\"');
                if (start) {
                    start++;  // Überspringe das erste Anführungszeichen
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

            // Als nächstes sollte die Zeile mit "back" kommen:
            char backText[MAX_TEXT_LENGTH] = "";
            if (fgets(line, sizeof(line), file)) {
                char *backPtr = strstr(line, "\"back\":");
                if (backPtr) {
                    char *colonBack = strchr(backPtr, ':');
                    if (colonBack) {
                        char *startBack = strchr(colonBack, '\"');
                        if (startBack) {
                            startBack++;  // Überspringe das erste Anführungszeichen
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
            }

            // Als nächstes sollte die Zeile mit "spacedRepCount" kommen:
            /* Da spacedRepCount entfernt wurde, wird dieser Abschnitt übersprungen */
            int parsedCount = 0;

            // Neuen Knoten erstellen:
            FlashcardNode* newNode = (FlashcardNode*) malloc(sizeof(FlashcardNode));
            if (!newNode) {
                printf("Fehler bei der Speicherzuweisung.\n");
                fclose(file);
                return;
            }
            // IDs werden neu vergeben
            newNode->id = 0;
            strncpy(newNode->front, frontText, MAX_TEXT_LENGTH);
            newNode->front[MAX_TEXT_LENGTH - 1] = '\0';
            strncpy(newNode->back, backText, MAX_TEXT_LENGTH);
            newNode->back[MAX_TEXT_LENGTH - 1] = '\0';
            /* spacedRepCount entfällt */
            newNode->prev = NULL;
            newNode->next = NULL;

            // Für SM-2 Felder initialisieren (können beim Laden überschrieben werden)
            newNode->repetitions = 0;
            newNode->ease_factor = 2.5;
            newNode->interval = 1;
            newNode->last_review = time(NULL);
            newNode->next_review = newNode->last_review;

            if (ptrHead == NULL) {
                ptrHead = newNode;
                ptr_tail = newNode;
            } else {
                ptr_tail->next = newNode;
                newNode->prev = ptr_tail;
                ptr_tail = newNode;
            }
        }
    }
    fclose(file);

    // IDs neu vergeben (basierend auf der aktuellen Reihenfolge)
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
        printf("Keine Karteikarten zum Löschen vorhanden.\n");
        return;
    }
    int number;
    char buffer[10];
    printf("Gib die Nummer der zu löschenden Karte ein: ");
    if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        return;
    number = atoi(buffer);
    if (number < 1) {
        printf("Ungültige Nummer.\n");
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
    // Knoten aus der Liste entfernen:
    if (current->prev)
        current->prev->next = current->next;
    else
        ptrHead = current->next;  // War head

    if (current->next)
        current->next->prev = current->prev;
    else
        ptr_tail = current->prev;  // War tail

    free(current);
    printf("Karteikarte %d wurde gelöscht.\n", number);
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


/* ======================= Neuer Code für SM-2 Testmodus und Timestamp-basiertes Lernen ======================= */

/*
 * summary:
 * Aktualisiert das Intervall einer Karte mittels SM-2 Algorithmus im Testmodus.
 * Hier werden Intervalle in Minuten (statt Tagen) berechnet:
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
 * Sortiert die verkettete Liste der Flashcards mittels Bubble Sort anhand des next_review Timestamps (aufsteigend).
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
    // Zuerst die Liste anhand von next_review sortieren
    bubbleSortByNextReview();
    FlashcardNode* current = ptrHead;
    time_t now = time(NULL);
    while (current != NULL) {
        // Nur Karten, die fällig sind (next_review in der Vergangenheit)
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

/* ======================= Ende des neuen Codes ======================= */

int main() {
    /* Speicherort der JSON Datei */
    const char *filename = "C:\\Users\\herrmannja\\Downloads\\flashcards.json";

    // Versuche, bereits existierende Karteikarten zu laden
    loadFlashcardsFromFile(filename);

    int choice;
    char input[10];

    while (1) {
        printf("\n--- Karteikarten App Menu ---\n");
        printf("1. Neue Karteikarte hinzufügen\n");
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
                saveFlashcardsToFile(filename);
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
                saveFlashcardsToFile(filename);
                renumberFlashcards();
                freeFlashcards();
                printf("Programm beendet.\n");
                exit(0);
            default:
                printf("Ungültige Eingabe. Bitte erneut versuchen.\n");
        }
    }

    return 0;
}

