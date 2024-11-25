#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#include "hashTable.h"
#include "lexan.h"
#include "globals.h"

int **builder_pipes;

pid_t *fork_builders(int num_builders) {
    // Allocate memory for storing PIDs of the builder processes
    pid_t *builder_pids = malloc(num_builders * sizeof(pid_t));
    if (!builder_pids) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    // Declare builder pipes array
    int builder_pipes[num_builders][2];

    printf("NUM BUILDERS: %d\n", num_builders);

    for (int i = 0; i < num_builders; i++) {
        // Create a pipe for each builder
        if (pipe(builder_pipes[i]) == -1) {
            perror("pipe failed");
            exit(EXIT_FAILURE);
        }

        // Fork a new process for each builder
        pid_t pid = fork();
        if (pid == 0) {
            // Child process (builder)
            close(builder_pipes[i][0]); // Close read end of the pipe in the builder process

            // Prepare arguments for execlp
            char pipe_fd_str[10];
            snprintf(pipe_fd_str, sizeof(pipe_fd_str), "%d", builder_pipes[i][1]);

            // Execute the builder program with the write-end of the pipe as argument
            execlp("./builder", "builder", pipe_fd_str, NULL);

            // If execlp fails
            perror("execlp failed");
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("fork failed for builder");
            exit(EXIT_FAILURE);
        }

        // Parent process (main)
        builder_pids[i] = pid; // Store the PID of the builder process
        close(builder_pipes[i][1]); // Parent closes the write end of the pipe
    }

    // Return the array of builder process PIDs
    return builder_pids;
}

pid_t* fork_splitters(int num_splitters, int num_builders, int splitter_pipes[num_splitters][2], 
                    char *input_file, char *exclusion_file, int num_lines) {
    int lines_per_splitter = num_lines / num_splitters;
    pid_t pids[num_splitters];

    for (int i = 0; i < num_splitters; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process (splitter)
            close(splitter_pipes[i][0]); // Close read end of the splitter pipe

            int start_line = i * lines_per_splitter;
            int end_line = (i == num_splitters - 1) ? num_lines : start_line + lines_per_splitter;

            char start_line_str[10], end_line_str[10], num_builders_str[10], pipe_fd_str[10];
            snprintf(start_line_str, sizeof(start_line_str), "%d", start_line);
            snprintf(end_line_str, sizeof(end_line_str), "%d", end_line);
            snprintf(num_builders_str, sizeof(num_builders_str), "%d", num_builders);
            snprintf(pipe_fd_str, sizeof(pipe_fd_str), "%d", splitter_pipes[i][1]);

            int builder_pipe_fds[num_builders];
            for (int j = 0; j < num_builders; j++) {
                if (write(splitter_pipes[i][1], &builder_pipes[j][1], sizeof(int)) == -1) {
                    perror("Failed to write builder pipe descriptor to splitter pipe");
                    exit(EXIT_FAILURE);
                }
            }
            
            execlp("./splitter", "splitter", input_file, exclusion_file, start_line_str, end_line_str, num_builders_str, pipe_fd_str, NULL);
            perror("execlp failed");
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
        pids[i] = pid; // Store the PID of the splitter process
        close(splitter_pipes[i][1]); // Parent closes the write end of the pipe
    }
    return pids;
}

int main(int argc, char *argv[]) {
    int num_splitters = 0, num_builders = 0, top_k = 0;
    char *input_file = NULL, *exclusion_file = NULL, *output_file = NULL;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            input_file = argv[i + 1];
        } else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
            num_splitters = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            num_builders = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            top_k = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-e") == 0 && i + 1 < argc) {
            exclusion_file = argv[i + 1];
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[i + 1];
        }
    }

    if (!input_file || !exclusion_file || !output_file || num_splitters <= 0 || num_builders <= 0 || top_k <= 0) {
        fprintf(stderr, "Usage: %s -i <input_file> -l <num_splitters> -m <num_builders> -t <top_k> -e <exclusion_file> -o <output_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Allocate and initialize pipes
    int splitter_pipes[num_splitters][2];
    builder_pipes = malloc(num_builders * sizeof(int *));
    if (!builder_pipes) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_builders; i++) {
        builder_pipes[i] = malloc(2 * sizeof(int));
        if (!builder_pipes[i] || pipe(builder_pipes[i]) == -1) {
            perror("pipe or malloc failed");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < num_splitters; i++) {
        if (pipe(splitter_pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    // Fork builders
    pid_t *builder_pids = fork_builders(num_builders);

    // Calculate number of lines in input file
    FILE *input = fopen(input_file, "r");
    if (!input) {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }

    int num_lines = 0;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), input)) {
        num_lines++;
    }
    fclose(input);

    // Fork splitters
    pid_t* splitter_pids = fork_splitters(num_splitters, num_builders, splitter_pipes, input_file, exclusion_file, num_lines);

    for (int i = 0; i < num_splitters; i++) {
        waitpid(splitter_pids[i], NULL, 0);  // Wait for each splitter to finish
    }
    fprintf(stderr, "All splitters have finished processing.\n");

    // Wait for all builders to finish
    for (int i = 0; i < num_builders; i++) {
        if (waitpid(builder_pids[i], NULL, 0) == -1) {
            perror("Error waiting for builder");
        }
    }
    printf("All builders have finished processing.\n");

    // Free allocated memory
    free(builder_pids);
    for (int i = 0; i < num_builders; i++) {
        free(builder_pipes[i]);
    }
    free(builder_pipes);

    return 0;
}