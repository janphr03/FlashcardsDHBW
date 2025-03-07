FROM gcc:latest
WORKDIR /usr/src/app
COPY . .
RUN gcc -o flashcards Flashcards.c
CMD ["./flashcards"]
