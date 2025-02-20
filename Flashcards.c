// PortainerIO
//openmediavault ist das Betriebssystem
// als Stack

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #ifdef _WIN32
  #include <windows.h>
// #endif

// max Länge für Vorderseite und Rückseite definiert
#define MAX_TEXT_LENGTH 256

// Definition eines Knotens der doppelt verketteten Liste
typedef struct FlashcardNode {
    char front[MAX_TEXT_LENGTH];
    char back[MAX_TEXT_LENGTH];
    int spacedRepCount;
    struct FlashcardNode *prev;
    struct FlashcardNode *next;
} FlashcardNode;

// Globale Zeiger auf den Kopf und das Ende der Liste
FlashcardNode* ptr_head = NULL;
FlashcardNode* ptr_tail = NULL;

/* Entfernt das Newline-Zeichen aus einem String */
void removeNewline(char *ptr_str) {
    ptr_str[strcspn(ptr_str, "\n")] = '\0';
}

/* Funktion zum Hinzufügen einer neuen Karteikarte (am Ende der Liste) */
void addFlashcard() {
    FlashcardNode* ptr_newNode = (FlashcardNode*) malloc(sizeof(FlashcardNode));
    if (!ptr_newNode) {
        printf("Fehler bei der Speicherzuweisung.\n");
        return;
    }
    ptr_newNode->prev = NULL;
    ptr_newNode->next = NULL;
    ptr_newNode->spacedRepCount = 0;

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

    // Knoten am Ende der Liste anhängen
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

/* Funktion zum Auflisten aller Karteikarten */
void listFlashcards() {
    if (ptr_head == NULL) {
        printf("Keine Karteikarten vorhanden.\n");
        return;
    }
    int count = 1;
    FlashcardNode* current = ptr_head;
    while (current != NULL) {
        printf("%d.\n  Vorderseite: %s\n  Rueckseite: %s\n SPA: %d", count, current->front, current->back, current->spacedRepCount);
        count++;
        current = current->next;
    }
}

/* Funktion zum Lernen der Karteikarten */
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
            int factor = atoi(dummy);  // Wandelt den Eingabestring in einen int um
            if (factor == -1 || factor == 0 || factor == 1) {
                current->spacedRepCount += factor;
            }
        }
        count++;
        current = current->next;
    }
}

/* Überprüft, ob eine Datei existiert.
   Gibt 1 zurück, falls ja, sonst 0. */
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
 * Funktion zum Speichern der Karteikarten in einer JSON-Datei
 *
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

        // "front" wird als erstes ausgegeben
        fprintf(ptr_file, "      \"front\": \"%s\",\n", current->front);

        // "back" wird als zweites ausgegeben; hier folgt ein Komma, um den nächsten Eintrag zu trennen
        fprintf(ptr_file, "      \"back\": \"%s\",\n", current->back);

        // "spacedRepCount" wird als drittes ausgegeben, hier ohne Anführungszeichen (als Zahl)
        fprintf(ptr_file, "      \"spacedRepCount\": %d\n", current->spacedRepCount);
        // Falls dies der letzte Knoten ist, kein Komma hinter der schließenden Klammer schreiben
        if (current->next == NULL)
            fprintf(ptr_file, "    }\n");
        else
            fprintf(ptr_file, "    },\n");
        current = current->next;
    }
    // JSON-Abschluss schreiben
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

/*
  Funktion zum Laden der Karteikarten aus einer JSON-Datei.
  Die Implementierung durchsucht Zeilen nach den Schlüsseln "front" und "back"
  und erstellt für jeden Eintrag einen neuen Knoten in der verketteten Liste.
  Es wird vorausgesetzt, dass die Datei exakt im Format von saveFlashcardsToFile() vorliegt.
*/
void loadFlashcardsFromFile(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Datei \"%s\" nicht gefunden – starte mit leeren Karteikarten.\n", filename);
        return;
    }

    // Vor dem Laden bestehende Karteikarten freigeben
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
            int parsedCount = 0; // Standardwert, falls nichts gefunden wird
            if (fgets(line, sizeof(line), file)) {
                if (strstr(line, "\"spacedRepCount\":") != NULL) {
                    // Hier gehen wir davon aus, dass der Wert als Zahl gespeichert wurde,
                    // also ohne Anführungszeichen (wie in unserem neuen saveFlashcardsToFile)
                    char *colonPtr = strchr(line, ':');
                    if (colonPtr) {
                        // Falls noch ein Komma oder Leerzeichen vorhanden sind, überspringen wir diese:
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
            strncpy(newNode->front, frontText, MAX_TEXT_LENGTH);
            newNode->front[MAX_TEXT_LENGTH - 1] = '\0';
            strncpy(newNode->back, backText, MAX_TEXT_LENGTH);
            newNode->back[MAX_TEXT_LENGTH - 1] = '\0';
            newNode->spacedRepCount = parsedCount;
            newNode->prev = NULL;
            newNode->next = NULL;

            // An die Liste anhängen
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

    // Zähle die geladenen Karteikarten
    int count = 0;
    FlashcardNode* current = ptr_head;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    printf("%d Karteikarten geladen.\n", count);
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

/* Funktion zum Neu-„nummerieren“ der Karteikarten.
   Da die Nummerierung in einer verketteten Liste beim Durchlaufen ermittelt wird,
   dient diese Funktion lediglich als Bestätigung. */
void renumberFlashcards() {
    printf("Die Karteikarten wurden neu nummeriert.\n");
}

int main() {
    /* Beispielhafter Pfad – passe diesen Pfad an deine Gegebenheiten an */
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
                // Vor dem Beenden speichern und alle Ressourcen freigeben
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
