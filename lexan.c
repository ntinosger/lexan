#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#include "hashTable.h"
#include "lexan.h"
#include "globals.h"

void fork_builders(int num_builders) {
    for (int i = 0; i < num_builders; i++) {
        if (pipe(builder_pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();
        if (pid == 0) {
            // Child process (builder)
            close(builder_pipes[i][1]); // Close write end
            dup2(builder_pipes[i][0], STDIN_FILENO); // Redirect stdin to read end
            close(builder_pipes[i][0]); // Close read end after dup2

            execlp("./builder", "builder", NULL); // Replace with actual builder command
            perror("execlp failed");
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("fork failed for builder");
            exit(EXIT_FAILURE);
        }
        close(builder_pipes[i][0]); // Parent closes read end
    }
}

int main(int argc, char *argv[])
{
    int num_splitters = 0, num_builders = 0, top_k = 0;
    char *input_file = NULL, *exclusion_file = NULL, *output_file = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc)
        {
            input_file = argv[i + 1];
        }
        else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc)
        {
            num_splitters = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc)
        {
            num_builders = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc)
        {
            top_k = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-e") == 0 && i + 1 < argc)
        {
            exclusion_file = argv[i + 1];
        }
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
        {
            output_file = argv[i + 1];
        }
    }

    if (!input_file || !exclusion_file || !output_file || num_splitters <= 0 || num_builders <= 0 || top_k <= 0)
    {
        fprintf(stderr, "Usage: %s -i <input_file> -l <num_splitters> -m <num_builders> -t <top_k> -e <exclusion_file> -o <output_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int splitter_pipes[num_splitters][2];
    for (int i = 0; i < num_splitters; i++) {
        if (pipe(splitter_pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    // Allocate memory for the builder pipes
    builder_pipes = malloc(num_builders * sizeof(int *));
    if (!builder_pipes) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_builders; i++) {
        builder_pipes[i] = malloc(2 * sizeof(int)); // Allocate space for each pipe
        if (!builder_pipes[i]) {
            perror("malloc failed");
            exit(EXIT_FAILURE);
        }

        if (pipe(builder_pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < num_builders; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Builder process
            close(builder_pipes[i][1]); // Close write end in builder

            char pipe_fd_str[10];
            snprintf(pipe_fd_str, sizeof(pipe_fd_str), "%d", builder_pipes[i][0]);

            // Redirect stdin to the read end of the pipe
            dup2(builder_pipes[i][0], STDIN_FILENO);
            close(builder_pipes[i][0]);

            // Execute builder program
            execlp("./builder", "builder", pipe_fd_str, NULL);
            perror("execlp failed");
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }

        // Parent process: close read end in parent
        close(builder_pipes[i][0]);
    }

    for (int i = 0; i < num_builders; i++) {
        fprintf(stderr, "Builder pipe %d: read end = %d, write end = %d\n", i, builder_pipes[i][0], builder_pipes[i][1]);
    }

    printf("Splitters: %d\n", num_splitters);

    // Fork splitters
    pid_t pids[num_splitters];
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
    rewind(input);

    int lines_per_splitter = num_lines / num_splitters;

    // Δημιουργία splitters
    for (int i = 0; i < num_splitters; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            close(splitter_pipes[i][0]); // Close read end of pipe

            int start_line = i * lines_per_splitter;
            int end_line = (i == num_splitters - 1) ? num_lines : start_line + lines_per_splitter;

            char start_line_str[10], end_line_str[10], num_builders_str[10], pipe_fd_str[10];
            snprintf(start_line_str, sizeof(start_line_str), "%d", start_line);
            snprintf(end_line_str, sizeof(end_line_str), "%d", end_line);
            snprintf(num_builders_str, sizeof(num_builders_str), "%d", num_builders);
            snprintf(pipe_fd_str, sizeof(pipe_fd_str), "%d", splitter_pipes[i][1]);
            printf("Child process started\n"); // Debugging line
            printf("args: %s %s %s %s\n", input_file, start_line_str, end_line_str, num_builders_str);

            dup2(splitter_pipes[i][1], STDOUT_FILENO); // Redirect stdout to pipe
            close(splitter_pipes[i][1]); // Close write end after dup2

            execlp("./splitter", "splitter", input_file, exclusion_file, start_line_str, end_line_str, num_builders_str, pipe_fd_str, NULL);
            perror("execlp failed");
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
        close(splitter_pipes[i][1]); // Κλείσιμο εγγραφής από pipe στο γονέα
    }

    fclose(input);

    // Wait for all splitters to finish
    for (int i = 0; i < num_splitters; i++) {
        printf("Waiting pid: %d\n", pids[i]);
        waitpid(pids[i], NULL, 0); // Wait for each child process
        // waitpid(pids[i]);
    }
    printf("All splitters have finished processing.\n");

    // Wait for all builders to finish
    for (int i = 0; i < num_builders; i++) {
        wait(NULL); // Wait for each builder process
    }
    printf("All builders have finished processing.\n");

    // You can now process the results from the splitters (this part is up to you)

    for (int i = 0; i < num_builders; i++) {
        free(builder_pipes[i]); // Free each pipe array
    }
    free(builder_pipes); // Free the array of pointers

    return 0;
}