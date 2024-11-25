#ifndef LEXAN_H
#define LEXAN_H

// Global constants
#define MAX_SPLITTERS 100
#define BUFFER_SIZE 1024

// Function prototypes
pid_t* fork_builders(int num_builders);

pid_t *fork_splitters(int num_splitters, int num_builders, int splitter_pipes[num_splitters][2], char *input_file, char *exclude_file, int num_lines);

#endif // LEXAN_H