#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>

#include "hashTable.h"
#include "builder.h"


// Process words from the pipe
void process_words_from_pipe(int pipe_fd) {
    // Initialize hash table for storing word frequencies
    HashTable *wordFrequencyTable = create_hash_table();

    // Buffer to store incoming data
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;

    while ((bytesRead = read(pipe_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';

        // Process each word in the buffer
        char *token = strtok(buffer, "\n");
        while (token != NULL) {
            // Add the word frequency in the hash table
            if (strlen(token) > 0) {
                insert_to_hash_table(&wordFrequencyTable, token);
                fprintf(stderr, "Received and processed word: %s\n", token);
            }
            token = strtok(NULL, "\n");
        }
    }

    if (bytesRead == -1) {
        perror("Error reading from pipe");
    }

    // Print the word frequency table
    fprintf(stdout, "Word frequencies:\n");
    print_hash_table(wordFrequencyTable);

    // Free the hash table memory
    free_hash_table(wordFrequencyTable);
}

int main(int argc, char *argv[]) {
    // Validate arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pipe_fd>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Get the pipe file descriptor from the command-line arguments
    pipe_fd = atoi(argv[1]);

    process_words_from_pipe(pipe_fd);

    close(pipe_fd);

    return 0;
}