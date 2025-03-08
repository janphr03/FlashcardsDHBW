FROM gcc:latest

WORKDIR /usr/src/app



# Wichtig: Hier -lm anhängen, damit die Mathe-Bibliothek gelinkt wird.
RUN gcc -o flashcards Flashcards.c -lm

CMD ["./flashcards"]
