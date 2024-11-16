#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#include "hashTable.h"

#define MAX_SPLITTERS 100
#define BUFFER_SIZE 1024

void signal_handler(int sig) {
    if (sig == SIGUSR1) {
        printf("Received SIGUSR1\n");
    } else if (sig == SIGUSR2) {
        printf("Received SIGUSR2\n");
    }
}

int main(int argc, char *argv[])
{
    // int num_splitters = 0, num_builders = 0, top_k = 0;
    // char *input_file = NULL, *exclusion_file = NULL, *output_file = NULL;

    // for (int i = 1; i < argc; i++)
    // {
    //     if (strcmp(argv[i], "-i") == 0 && i + 1 < argc)
    //     {
    //         input_file = argv[i + 1];
    //     }
    //     else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc)
    //     {
    //         num_splitters = atoi(argv[i + 1]);
    //     }
    //     else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc)
    //     {
    //         num_builders = atoi(argv[i + 1]);
    //     }
    //     else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc)
    //     {
    //         top_k = atoi(argv[i + 1]);
    //     }
    //     else if (strcmp(argv[i], "-e") == 0 && i + 1 < argc)
    //     {
    //         exclusion_file = argv[i + 1];
    //     }
    //     else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
    //     {
    //         output_file = argv[i + 1];
    //     }
    // }

    // if (!input_file || !exclusion_file || !output_file || num_splitters <= 0 || num_builders <= 0 || top_k <= 0)
    // {
    //     fprintf(stderr, "Usage: %s -i <input_file> -l <num_splitters> -m <num_builders> -t <top_k> -e <exclusion_file> -o <output_file>\n", argv[0]);
    //     exit(EXIT_FAILURE);
    // }

    // int pipes[MAX_SPLITTERS][2];
    // for (int i = 0; i < num_splitters; i++) {
    //     if (pipe(pipes[i]) == -1) {
    //         perror("pipe");
    //         exit(EXIT_FAILURE);
    //     }
    // }

    // // Fork splitters
    // pid_t pids[MAX_SPLITTERS];
    // FILE *input = fopen(input_file, "r");
    // if (!input) {
    //     perror("Error opening input file");
    //     exit(EXIT_FAILURE);
    // }

    // fseek(input, 0, SEEK_END);
    // long file_size = ftell(input);
    // long chunk_size = file_size / num_splitters;
    // rewind(input);

    // for (int i = 0; i < num_splitters; i++) {
    //     pid_t pid = fork();
    //     if (pid < 0) {
    //         perror("fork");
    //         exit(EXIT_FAILURE);
    //     } else if (pid == 0) {
    //         // Child process
    //         close(pipes[i][0]); // Close reading end in the child

    //         char start_offset[BUFFER_SIZE], end_offset[BUFFER_SIZE], pipe_fd[BUFFER_SIZE];
    //         long start = i * chunk_size;
    //         long end = (i == num_splitters - 1) ? file_size : start + chunk_size;

    //         snprintf(start_offset, BUFFER_SIZE, "%ld", start);
    //         snprintf(end_offset, BUFFER_SIZE, "%ld", end);
    //         snprintf(pipe_fd, BUFFER_SIZE, "%d", pipes[i][1]);

    //         execlp("./splitter", "splitter", input_file, start_offset, end_offset, pipe_fd, NULL);
    //         perror("execlp");
    //         exit(EXIT_FAILURE);
    //     } else {
    //         // Parent process
    //         pids[i] = pid;
    //         close(pipes[i][1]); // Close writing end in the parent
    //     }
    // }

    // fclose(input);

    // // Collect results from splitters
    // int word_counts[256] = {0};
    // for (int i = 0; i < num_splitters; i++) {
    //     int splitter_counts[256] = {0};
    //     read(pipes[i][0], splitter_counts, sizeof(splitter_counts));
    //     close(pipes[i][0]);

    //     // Aggregate word counts
    //     for (int j = 0; j < 256; j++) {
    //         word_counts[j] += splitter_counts[j];
    //     }
    // }

    // // Wait for all splitters to finish
    // for (int i = 0; i < num_splitters; i++) {
    //     waitpid(pids[i], NULL, 0);
    // }

    // // Output aggregated results (for testing)
    // for (int i = 0; i < 256; i++) {
    //     if (word_counts[i] > 0) {
    //         printf("Word starting with '%c': %d occurrences\n", i, word_counts[i]);
    //     }
    // }
    

// Create a hash table
    HashTable* myHashTable = create_hash_table();

    // Insert words into the hash table
    insert_to_hash_table(&myHashTable, "apple");
    insert_to_hash_table(&myHashTable, "banana");
    insert_to_hash_table(&myHashTable, "cherry");
    insert_to_hash_table(&myHashTable, "date");
    insert_to_hash_table(&myHashTable, "elderberry");
    insert_to_hash_table(&myHashTable, "fig");
    insert_to_hash_table(&myHashTable, "grape");
    
    // Print the hash table
    printf("Hash table after insertion:\n");
    print_hash_table(myHashTable);

    // Test search functionality
    printf("\nSearching for 'banana':\n");
    HashNode* node = search_hash_table(myHashTable, "banana");
    if (node) {
        printf("Found '%s' with frequency %d\n", node->word, node->freq);
    } else {
        printf("'banana' not found in the hash table.\n");
    }

    // Insert 'banana' again and check the frequency increase
    insert_to_hash_table(&myHashTable, "banana");
    printf("\nHash table after inserting 'banana' again:\n");
    print_hash_table(myHashTable);

    printf("\nSearching for 'banana again':\n");
    node = search_hash_table(myHashTable, "banana");
    if (node) {
        printf("Found '%s' with frequency %d\n", node->word, node->freq);
    } else {
        printf("'banana' not found in the hash table.\n");
    }

    // Test delete functionality
    printf("\nDeleting 'cherry' from the hash table:\n");
    delete_from_hash_table(myHashTable, "cherry");
    print_hash_table(myHashTable);

    // Try to delete a word that doesn't exist
    printf("\nTrying to delete 'kiwi' (not in the hash table):\n");
    delete_from_hash_table(myHashTable, "kiwi");

    // Print the hash table after deletion
    printf("\nHash table after deletion:\n");
    print_hash_table(myHashTable);

    // Free the hash table memory
    free_hash_table(myHashTable);

    return 0;
}