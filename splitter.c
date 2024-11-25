#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>

#include "hashTable.h"
#include "lexan.h"

// Maximum word length
#define MAX_LINE_LENGTH 1024
#define MAX_WORD_LENGTH 50


// Access builder pipes and num_builders
extern int **builder_pipes;
extern int num_builders;

void load_exclusion_to_hash_table(FILE *excludeFile, HashTable *excludeHashTable) {
    char line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), excludeFile)) {
        // Remove trailing newline characters
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
void process_chunk(FILE *input, HashTable *hashTable, long start, long end, int pipe_fd) {
    fprintf(stderr, "in process %ld, %ld, %d\n",  start, end, pipe_fd);
    
    char line[MAX_LINE_LENGTH];
    long current_line = 0;

    // Read and process lines from start to end
    while (current_line < end && fgets(line, sizeof(line), input) != NULL) {
        if (current_line >= start) {    
            // Remove trailing newline characters
            line[strcspn(line, "\n")] = '\0';

            char *token = strtok(line, " \t"); // Delimiters: space and tab
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
                        int builder_index = hash_code(clean_word); // Map word to builder
                        fprintf(stderr, "Cleaned word: %s with builder index: %d\n", clean_word, builder_index);
                        write(builder_pipes[builder_index][1], clean_word, strlen(clean_word) + 1); // Send word to the builder


                    } else {
                        fprintf(stderr, "Word exluded from file: %s\n", clean_word);
                    }
                    // Send the word to the pipe (or handle it as needed)
                    // Example: write(pipe_fd, clean_word, strlen(clean_word) + 1);
                } else {
                    fprintf(stderr, "Word not a word: %s\n", token);
                }

                // Get the next word
                token = strtok(NULL, " \t");
            }
            // Send the line or results to the parent process via pipe
            // write(pipe_fd, line, strlen(line) + 1); // Include null terminator
        }
        current_line++;
    }
    // Send results via pipe
    // write(pipe_fd, word_counts, sizeof(word_counts));
}

int main(int argc, char *argv[]) {
    fprintf(stderr, "splitter started\n");
    if (argc != 7) {
        fprintf(stderr, "Usage: %s <input_file_path> <exclude_file_path> <start_offset> <end_offset> <num_of_builders> <pipe_fd>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "You have entered %d arguments:\n", argc);

    for (int i = 0; i < argc; i++) {
        fprintf(stderr, "%s\n", argv[i]);
    }

    char *input_file_path = argv[1];
    char *exclude_file_path = argv[2];
    long start_offset = atol(argv[3]);
    long end_offset = atol(argv[4]);
    int num_of_builders = atoi(argv[5]);
    int pipe_fd = atoi(argv[6]);

    fprintf(stderr, "%s, %ld, %ld, %d, %d\n", input_file_path, start_offset, end_offset, num_of_builders, pipe_fd);

    // for (int i = 0; i < num_of_builders; i++)
    // {
    //     fprintf(stderr ,"BUILDERS_PIPES %d\n", builder_pipes[0][1]);
    // }
    

    FILE *inputFile = fopen(input_file_path, "r");
    if (!inputFile) {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }

    // Open exclusion file
    FILE *excludeFile = fopen(exclude_file_path, "r");
    if (!excludeFile) {
        perror("Error opening exclude file");
        fclose(excludeFile);
        exit(EXIT_FAILURE);
    }

    // Initialize the exclusion hash table
    HashTable *excludeHashTable = create_hash_table();
    load_exclusion_to_hash_table(excludeFile, excludeHashTable);
    fclose(excludeFile); // Close exclusion file after loading
    // print_hash_table(excludeHashTable);
    // HashNode* node = search_hash_table(excludeHashTable, "is");
    // fprintf(stderr, "node: %s\n \n", node->word);

    // Process the assigned chunk
    process_chunk(inputFile, excludeHashTable, start_offset, end_offset, pipe_fd);

    fclose(inputFile);
    close(pipe_fd); // Close the pipe after writing

    free_hash_table(excludeHashTable);
    return 0;
}
