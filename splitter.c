#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>

#include "splitter.h"
#include "lexan.h"

// Function to load excluded words into a hash table for better search
void load_exclusion_to_hash_table(FILE *excludeFile, HashTable *excludeHashTable) {
    char line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), excludeFile)) {
        // Remove newline characters
        line[strcspn(line, "\n")] = '\0';

        // Convert the word to lowercase
        for (int i = 0; line[i] != '\0'; i++) {
            line[i] = tolower(line[i]);
        }

        // Insert the word into the hash table
        insert_to_hash_table(&excludeHashTable, line);
    }
}

// Function to process a chunk of the file
void process_chunk(FILE *input, HashTable *hashTable, long start, long end, int num_builders, int buidler_pipes[num_builders][2]) {
    
    char line[MAX_LINE_LENGTH];
    long current_line = 0;

    // Read and process lines from start to end
    while (current_line < end && fgets(line, sizeof(line), input) != NULL) {
        if (current_line >= start) {    
            // Remove newline characters
            line[strcspn(line, "\n")] = '\0';

            char *token = strtok(line, " \t");

            while (token != NULL) {
                // Remove punctuation and convert to lowercase
                char clean_word[MAX_WORD_LENGTH] = "";
                int idx = 0;
                for (int i = 0; token[i] != '\0'; i++) {
                    if (isalpha(token[i])) { // Keep only alphabetic characters
                        clean_word[idx] = tolower(token[i]);
                        idx++;
                    }
                }
                clean_word[idx] = '\0'; // Null-terminate the cleaned word

                if (strlen(clean_word) > 1) {
                    if (!search_hash_table(hashTable, clean_word)) {
                        int builder_index = hashing_builders(clean_word, num_builders); // Map word to builder
                        fprintf(stderr, "Writing to builder pipe %d: %s\n", buidler_pipes[builder_index][1], clean_word);
                        write(buidler_pipes[builder_index][1], clean_word, strlen(clean_word) + 1); // Send word to the builder

                    }
                }

                // Get the next word
                token = strtok(NULL, " \t");
            }
        }
        current_line++;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 7) {
        fprintf(stderr, "Usage: %s <input_file_path> <exclude_file_path> <start_offset> <end_offset> <num_of_builders> <pipe_fd> [builder_fds...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Extract arguments
    char *input_file_path = argv[1];
    char *exclude_file_path = argv[2];
    long start_offset = atol(argv[3]);
    long end_offset = atol(argv[4]);
    int num_builders = atoi(argv[5]);
    int pipe_fd = atoi(argv[6]);  

    int builder_pipes[num_builders][2];
    for (int i = 0; i < num_builders; i++) {
        int pipe_fds[2];
        if (read(pipe_fd, pipe_fds, sizeof(pipe_fds)) != sizeof(pipe_fds)) {
            perror("read from splitter pipe failed");
            exit(EXIT_FAILURE);
        }
        builder_pipes[i][0] = pipe_fds[0]; // Read FD
        builder_pipes[i][1] = pipe_fds[1]; // Write FD
    }

    // Open the input file
    FILE *inputFile = fopen(input_file_path, "r");
    if (!inputFile) {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }

    // Open the exclusion file
    FILE *excludeFile = fopen(exclude_file_path, "r");
    if (!excludeFile) {
        perror("Error opening exclude file");
        fclose(inputFile);
        exit(EXIT_FAILURE);
    }

    // Initialize the exclusion hash table
    HashTable *excludeHashTable = create_hash_table();
    load_exclusion_to_hash_table(excludeFile, excludeHashTable);
    fclose(excludeFile); // Close exclusion file after loading

    // Process the assigned chunk
    process_chunk(inputFile, excludeHashTable, start_offset, end_offset, num_builders, builder_pipes);

    fclose(inputFile);
    close(pipe_fd); // Close the splitter's pipe after reading

    free_hash_table(excludeHashTable); // Clean up hash table
    return 0;
}