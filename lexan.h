#ifndef LEXAN_H
#define LEXAN_H

// Global constants
#define MAX_SPLITTERS 100
#define BUFFER_SIZE 1024

int num_splitters, num_builders, splitters_done, builders_ready;
pid_t *splitter_pids, *builder_pids;

// Function prototypes
pid_t* fork_builders(int builder_pipes[num_builders][2]);

pid_t *fork_splitters(int splitter_pipes[num_splitters][2], int builder_pipes[num_builders][2], char *input_file, char *exclude_file, int num_lines);

#endif // LEXAN_H