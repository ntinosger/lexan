#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#include "hashTable.h"
#include "lexan.h"


pid_t *fork_builders(int builder_pipes[num_builders][2]) {
    // Allocate memory for the PIDs of the builder processes
    pid_t *builder_pids = malloc(num_builders * sizeof(pid_t));
    if (!builder_pids) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_builders; i++) {
        // Fork a new process for each builder
        pid_t pid = fork();
        if (pid == 0) {
            // Child process (builder)
            close(builder_pipes[i][1]); // Close write end of the pipe in the builder process

            // Prepare arguments for execlp
            char pipe_fd_str[10];
            snprintf(pipe_fd_str, sizeof(pipe_fd_str), "%d", builder_pipes[i][0]);

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
    }

    // Return the array of builder PIDs
    return builder_pids;
}

pid_t* fork_splitters(int splitter_pipes[num_splitters][2], int builder_pipes[num_builders][2], char *input_file, char *exclusion_file, int num_lines) {
    int lines_per_splitter = num_lines / num_splitters;

    pid_t *pids = malloc(num_splitters * sizeof(pid_t));
    if (!pids) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_splitters; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process (splitter)
            close(splitter_pipes[i][1]); // Close write end of the splitter pipe

            // Compute the line range for this splitter
            int start_line = i * lines_per_splitter;
            int end_line = (i == num_splitters - 1) ? num_lines : start_line + lines_per_splitter;

            // Prepare arguments for the splitter
            char start_line_str[10], end_line_str[10], num_builders_str[10], splitter_pipe_fd_str[10];
            snprintf(start_line_str, sizeof(start_line_str), "%d", start_line);
            snprintf(end_line_str, sizeof(end_line_str), "%d", end_line);
            snprintf(num_builders_str, sizeof(num_builders_str), "%d", num_builders);
            snprintf(splitter_pipe_fd_str, sizeof(splitter_pipe_fd_str), "%d", splitter_pipes[i][0]);

            // Build the argument list for execlp
            char *args[8];
            args[0] = "splitter";
            args[1] = input_file;
            args[2] = exclusion_file;
            args[3] = start_line_str;
            args[4] = end_line_str;
            args[5] = num_builders_str;
            args[6] = splitter_pipe_fd_str;
            args[7] = NULL;

            // Execute the splitter program
            execvp("./splitter", args);

            perror("execvp failed for splitter");
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        } else {

            pids[i] = pid; // Store the PID of the splitter process

            // Write the builder PIDs in the splitter pipes
            for (int j = 0; j < num_builders; j++) {
                int pipe_fds[2] = {builder_pipes[j][0], builder_pipes[j][1]};
                if (write(splitter_pipes[i][1], pipe_fds, sizeof(pipe_fds)) == -1) {
                    perror("write to splitter failed");
                    exit(EXIT_FAILURE);
                }
            }
            close(splitter_pipes[i][1]);
        }

        // Parent process (root)
        close(splitter_pipes[i][1]); // Parent closes the write end of the splitter pipe
    }

    return pids;
}

int main(int argc, char *argv[]) {
    splitters_done = 0;
    builders_ready = 0;
    num_splitters = 0;
    num_builders = 0;  
    int top_k = 0;
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
    int builder_pipes[num_builders][2];

    for (int i = 0; i < num_builders; i++) {
        if (pipe(builder_pipes[i]) == -1) {
            perror("builder pipe");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < num_splitters; i++) {
        if (pipe(splitter_pipes[i]) == -1) {
            perror("splitter pipe");
            exit(EXIT_FAILURE);
        }
    }

    // Fork builders
    builder_pids = fork_builders(builder_pipes);    

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
    splitter_pids = fork_splitters(splitter_pipes, builder_pipes, input_file, exclusion_file, num_lines);

    // Wait for all splitters to finish
    for (int i = 0; i < num_splitters; i++) {
        waitpid(splitter_pids[i], NULL, 0);
    }
    // printf("All splitters have finished processing.\n");   

    // Wait for all builders to finish
    for (int i = 0; i < num_builders; i++) {
        waitpid(builder_pids[i], NULL, 0);
    }
    // printf("All builders have finished processing.\n");

    return 0;
}