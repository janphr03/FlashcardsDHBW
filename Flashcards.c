// PortainerIO
// openmediavault ist das Betriebssystem
// als Stack die Datenbank definieren mit PORT usw
// wie kann ich automatisch die Tabellen dafür bereitstellen?
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
    int spacedRepCount;
    int repetitions;       // Anzahl der Wiederholungen (hintereinander richtige Antworten)
    double ease_factor;    // E-Faktor (Lernfaktor zur Intervallanpassung)
    int interval;          // aktuelles Intervall in Tagen bis zur nächsten Wiederholung
    time_t last_review;    // Zeitpunkt der letzten Abfrage
    time_t next_review;    // Zeitpunkt, wann die Karte wieder fällig ist
    struct FlashcardNode *prev;
    struct FlashcardNode *next;
} FlashcardNode;

// Globale Zeiger auf Kopf und Ende der Liste sowie globaler Zähler für IDs
FlashcardNode* ptr_head = NULL;
FlashcardNode* ptr_tail = NULL;
int next_id = 1; // Nächste zu vergebende ID

/*
 *summary:
 * Entfernt das Newline-Zeichen aus einem String
 */
void removeNewline(char *ptr_str) {
    ptr_str[strcspn(ptr_str, "\n")] = '\0';
}

void addFlashcard() {
    FlashcardNode* ptr_newNode = (FlashcardNode*) malloc(sizeof(FlashcardNode));
    if (!ptr_newNode) {
        printf("Fehler bei der Speicherzuweisung.\n");
        return;
    }
    ptr_newNode->prev = NULL;
    ptr_newNode->next = NULL;
    ptr_newNode->spacedRepCount = 0;
    ptr_newNode->id = next_id++; // Vergibt fortlaufende ID

    printf("Geben Sie die Vorderseite (Frage) der Karte ein:\n> ");
    if (fgets(ptr_newNode->front, MAX_TEXT_LENGTH, stdin) == NULL) {
        free(ptr_newNode);
        return;
    }
    removeNewline(ptr_newNode->front);

    printf("Geben Sie die Rückseite (Antwort) der Karte ein:\n> ");
    if (fgets(ptr_newNode->back, MAX_TEXT_LENGTH, stdin) == NULL) {
        free(ptr_newNode);
        return;
    }
    removeNewline(ptr_newNode->back);

    // Knoten ans Ende der Liste anhängen
    if (ptr_head == NULL) {
        ptr_head = ptr_newNode;
        ptr_tail = ptr_newNode;
    } else {
        ptr_tail->next = ptr_newNode;
        ptr_newNode->prev = ptr_tail;
        ptr_tail = ptr_newNode;
    }
    printf("Karteikarte hinzugefügt.\n");
}

/*
 *summary:
 *Funktion zum Hinzufügen einer neuen Karteikarte
 */

/* Funktion zum Auflisten aller Karteikarten */
void listFlashcards() {
    if (ptr_head == NULL) {
        printf("Keine Karteikarten vorhanden.\n");
        return;
    }
    int count = 1;
    FlashcardNode* current = ptr_head;
    while (current != NULL) {
        printf("%d. (ID: %d)\n  Vorderseite: %s\n  Rueckseite: %s\n  SPA: %d\n",
            count, current->id, current->front, current->back, current->spacedRepCount);
        count++;
        current = current->next;
    }
}

/*
 * summary:
 * Funktion zum Lernen der Karteikarten mit Angabe des "Space Rep Counts"
 * Dieser wird mit einer Konstante multipliziert und je nachdem früher oder später abgefragt
 */
void studyFlashcards() {
    if (ptr_head == NULL) {
        printf("Keine Karteikarten zum Lernen vorhanden.\n");
        return;
    }
    char dummy[10];
    int count = 1;
    FlashcardNode* current = ptr_head;
    while (current != NULL) {
        printf("\nFrage %d: %s\n", count, current->front);
        printf("Druecken Sie Enter, um die Antwort zu sehen...");
        fgets(dummy, sizeof(dummy), stdin);
        printf("Antwort: %s\n", current->back);

        printf("1=einfach 0=schwer -1=nicht gewusst\n");
        if (fgets(dummy, sizeof(dummy), stdin ) != NULL) {
            int factor = atoi(dummy);
            if (factor == -1 || factor == 0 || factor == 1) {
                current->spacedRepCount += factor;
            }
        }
        count++;
        current = current->next;
    }
}

/*
 * summary:
 * updated das Intervall welches bestimmt, wann eine Karte das nächste Mal angefragt wird
 * parameters:
 * FlashcardNode *curr: zeigt auf das aktuelle
 * int rating: Der Wert, den der Benutzer übergeben hat, wie gut er die Karte konnte.
 */
void update_card_interval(FlashcardNode *curr, int rating) {
    // 1. Aktuelle Werte aus der Karte laden
    int rep = curr->repetitions;
    double efactor = curr->ease_factor;
    int interval = curr->interval;

    // 2. Bewertung (-1,0,1) auf SM-2 Qualitätsfaktor 0-5 abbilden
    int quality;
    if (rating == 1) {
        quality = 5;   // sehr gutes Ergebnis
    } else if (rating == 0) {
        quality = 3;   // gerade noch gewusst
    } else { // rating == -1
        quality = 2;   // nicht gewusst
    }

    // 3. E-Faktor anpassen (Lernfaktor aktualisieren)
    // Formel: EF' = EF_alt + (0.1 - (5 - q) * (0.08 + (5 - q) * 0.02))
    efactor = efactor + (0.1 - (5 - quality) * (0.08 + (5 - quality) * 0.02));
    if (efactor < 1.3) {
        efactor = 1.3;  // Minimum E-Faktor von 1.3 sicherstellen
    }

    // 4. Wiederholungszähler aktualisieren
    if (quality < 3) {
        // Antwort war schlecht -> reset der Wiederholungsfolge
        rep = 0;
    } else {
        // gute Antwort -> Wiederholungszähler erhöhen
        rep += 1;
    }

    // 5. Wiederholungsintervall berechnen (in Tagen)
    if (rep <= 1) {
        interval = 1;           // nach erster (oder Reset-)Lernrunde: 1 Tag
    } else if (rep == 2) {
        interval = 6;           // nach der zweiten richtigen Wiederholung: 6 Tage
    } else {
        // ab der dritten Wiederholung: vorheriges Intervall mit Faktor multiplizieren
        interval = (int)ceil(interval * efactor);
    }

    // 6. Nächstes Wiederholungsdatum (Timestamp) berechnen
    time_t now = time(NULL);
    double seconds_per_day = 24 * 60 * 60;
    curr->last_review = now;
    curr->next_review = now + (time_t)(interval * seconds_per_day);

    // 7. Aktualisierte Werte zurück in die Karte speichern
    curr->repetitions = rep;
    curr->ease_factor = efactor;
    curr->interval = interval;
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

    int temp_spacedRepCount = a->spacedRepCount;
    a->spacedRepCount = b->spacedRepCount;
    b->spacedRepCount = temp_spacedRepCount;

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
 *
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
    FlashcardNode* current = ptr_head;
    while (current != NULL) {
        fprintf(ptr_file, "    {\n");
        fprintf(ptr_file, "      \"front\": \"%s\",\n", current->front);
        fprintf(ptr_file, "      \"back\": \"%s\",\n", current->back);
        fprintf(ptr_file, "      \"spacedRepCount\": %d\n", current->spacedRepCount);
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
    FlashcardNode* current = ptr_head;
    while (current != NULL) {
        FlashcardNode* temp = current;
        current = current->next;
        free(temp);
    }
    ptr_head = NULL;
    ptr_tail = NULL;
}

/* Funktion zum Laden der Karteikarten aus einer JSON-Datei */
void loadFlashcardsFromFile(const char *filename) {
    FILE *file = fopen(filename, "r");
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
            int parsedCount = 0;
            if (fgets(line, sizeof(line), file)) {
                if (strstr(line, "\"spacedRepCount\":") != NULL) {
                    char *colonPtr = strchr(line, ':');
                    if (colonPtr) {
                        colonPtr++;
                        while (*colonPtr == ' ' || *colonPtr == ',')
                            colonPtr++;
                        parsedCount = atoi(colonPtr);
                    }
                }
            }

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
            newNode->spacedRepCount = parsedCount;
            newNode->prev = NULL;
            newNode->next = NULL;

            if (ptr_head == NULL) {
                ptr_head = newNode;
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
    FlashcardNode* current = ptr_head;
    while (current != NULL) {
        current->id = count++;
        current = current->next;
    }
    next_id = count;
    printf("%d Karteikarten geladen.\n", count - 1);
}

/* Funktion zum Löschen einer Karteikarte anhand ihrer Nummer */
void deleteFlashcard() {
    if (ptr_head == NULL) {
        printf("Keine Karteikarten zum Löschen vorhanden.\n");
        return;
    }
    int number;
    char buffer[10];
    printf("Geben Sie die Nummer der zu löschenden Karte ein: ");
    if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        return;
    number = atoi(buffer);
    if (number < 1) {
        printf("Ungültige Nummer.\n");
        return;
    }
    int count = 1;
    FlashcardNode* current = ptr_head;
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
        ptr_head = current->next;  // War head

    if (current->next)
        current->next->prev = current->prev;
    else
        ptr_tail = current->prev;  // War tail

    free(current);
    printf("Karteikarte %d wurde gelöscht.\n", number);
}

/* Funktion zum Neu-„nummerieren“ der Karteikarten */
void renumberFlashcards() {
    printf("Die Karteikarten wurden neu nummeriert.\n");
}


/* Sortiert die Karteikarten anhand der id.
   Bei ascending ≠ 0 erfolgt eine aufsteigende Sortierung, sonst absteigend.
*/
void sortFlashcardsById(int ascending) {
    if (ptr_head == NULL)
        return;
    int swapped;
    do {
        swapped = 0;
        FlashcardNode* current = ptr_head;
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

// Inputs an das Programm sollen über
// argc und argv[] übergeben werden
int main(int argc, char * argv[]) {
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
        printf("3. Karteikarten lernen\n");
        printf("4. Karteikarten speichern\n");
        printf("5. Karteikarte loeschen\n");
        printf("6. Beenden\n");
        printf("7. Karteikarten aufsteigend sortieren\n");
        printf("8. Karteikarten absteigend sortieren\n");
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
                saveFlashcardsToFile(filename);
                renumberFlashcards();
                freeFlashcards();
                printf("Programm beendet.\n");
                exit(0);
            case 7:
                sortFlashcardsById(1);
                printf("Karteikarten wurden aufsteigend sortiert.\n");
                break;
            case 8:
                sortFlashcardsById(0);
                printf("Karteikarten wurden absteigend sortiert.\n");
                break;
            default:
                printf("Ungültige Eingabe. Bitte erneut versuchen.\n");
        }
    }

    return 0;
}
