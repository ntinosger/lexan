#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>

// Maximum word length
#define MAX_WORD_LENGTH 50

// Function to process a chunk of the file
void process_chunk(FILE *input, long start, long end, int pipe_fd) {
    char word[MAX_WORD_LENGTH];
    int word_counts[256] = {0}; // Simplified word count storage, e.g., hash words for complexity.

    fseek(input, start, SEEK_SET);

    while (ftell(input) < end && fscanf(input, "%49s", word) == 1) {
        // Normalize word to lowercase and remove punctuation
        for (int i = 0; word[i]; i++) {
            if (isalpha(word[i])) {
                word[i] = tolower(word[i]);
            } else {
                word[i] = '\0';
                break;
            }
        }

        // Increment word count (simplified for unique IDs, should hash and handle conflicts)
        word_counts[word[0] % 256]++;
    }

    // Send results via pipe
    write(pipe_fd, word_counts, sizeof(word_counts));
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <file_path> <start_offset> <end_offset> <pipe_fd>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *file_path = argv[1];
    long start_offset = atol(argv[2]);
    long end_offset = atol(argv[3]);
    int pipe_fd = atoi(argv[4]);

    FILE *input_file = fopen(file_path, "r");
    if (!input_file) {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }

    // Process the assigned chunk
    process_chunk(input_file, start_offset, end_offset, pipe_fd);

    fclose(input_file);
    close(pipe_fd); // Close the pipe after writing
    return 0;
}
