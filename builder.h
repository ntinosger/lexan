#ifndef BUILDER_H
#define BUILDER_H

#define MAX_WORD_LENGTH 100
#define BUFFER_SIZE 1024

pid_t root_pid;
int pipe_fd;

void process_words_from_pipe(int pipe_fd);

#endif // BUILDER_H