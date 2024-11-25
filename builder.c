#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "hashTable.h"
#include "builder.h"

// Buffer size for reading from the pipe
#define BUFFER_SIZE 1024

void process_words_from_pipe(int pipe_fd) {
    // Initialize a hash table for storing word frequencies
    HashTable *wordFrequencyTable = create_hash_table();

    // Buffer to hold incoming data from the pipe
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;

    while ((bytesRead = read(pipe_fd, buffer, sizeof(buffer) - 1)) > 0) {
        // Null-terminate the buffer for safe string operations
        buffer[bytesRead] = '\0';

        // Process each word in the buffer
        char *token = strtok(buffer, "\n"); // Words are separated by newline
        while (token != NULL) {
            // Increment the word frequency in the hash table
            if (strlen(token) > 0) {
                insert_to_hash_table(&wordFrequencyTable, token);
                fprintf(stderr, "Received and processed word: %s\n", token);
            }
            token = strtok(NULL, "\n");
        }
    }

    if (bytesRead == -1) {
        perror("Error reading from pipeee");
    }

    // Print the word frequency table
    // fprintf(stdout, "Word frequencies:\n");
    // print_hash_table(wordFrequencyTable);

    // Free the hash table memory
    free_hash_table(wordFrequencyTable);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pipe_fd>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Get the pipe file descriptor from the command-line argument
    int pipe_fd = atoi(argv[1]);

    fprintf(stderr, "pipe in builder: %d\n", pipe_fd);

    // Process words from the pipe
    process_words_from_pipe(pipe_fd);

    // Close the pipe file descriptor
    close(pipe_fd);

    return 0;
}