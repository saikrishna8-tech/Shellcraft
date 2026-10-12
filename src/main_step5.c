#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64

int main() {
    char input[MAX_INPUT];

    while (1) {
        printf("shellcraft$ ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        if (strlen(input) == 0) {
            continue;
        }

        /* exit built-in */
        if (strcmp(input, "exit") == 0) {
            break;
        }

        /* pwd built-in */
        if (strcmp(input, "pwd") == 0) {
            char cwd[MAX_INPUT];

            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            } else {
                perror("pwd");
            }

            continue;
        }

        /* cd built-in */
        if (strncmp(input, "cd ", 3) == 0) {
            char *directory = input + 3;

            if (chdir(directory) != 0) {
                perror("cd");
            }

            continue;
        }

        char *args[MAX_ARGS];
        int argc = 0;

        char *token = strtok(input, " ");

        while (token != NULL && argc < MAX_ARGS - 1) {
            args[argc++] = token;
            token = strtok(NULL, " ");
        }

        args[argc] = NULL;

        /*
         * Redirection variables
         */
        char *output_file = NULL;
        char *input_file = NULL;
        int append = 0;

        /*
         * Check for redirection
         */
        for (int i = 0; i < argc; i++) {

            /* Output: > */
            if (strcmp(args[i], ">") == 0 && i + 1 < argc) {
                output_file = args[i + 1];
                append = 0;
                args[i] = NULL;
                argc = i;
                break;
            }

            /* Output: >> */
            if (strcmp(args[i], ">>") == 0 && i + 1 < argc) {
                output_file = args[i + 1];
                append = 1;
                args[i] = NULL;
                argc = i;
                break;
            }

            /* Input: < */
            if (strcmp(args[i], "<") == 0 && i + 1 < argc) {
                input_file = args[i + 1];
                args[i] = NULL;
                argc = i;
                break;
            }
        }

        args[argc] = NULL;

        /*
         * Create child process
         */
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            continue;
        }

        /*
         * Child process
         */
        if (pid == 0) {

            /*
             * Output redirection
             */
            if (output_file != NULL) {

                int flags = O_WRONLY | O_CREAT;

                if (append) {
                    flags |= O_APPEND;
                } else {
                    flags |= O_TRUNC;
                }

                int fd = open(output_file, flags, 0644);

                if (fd < 0) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }

                if (dup2(fd, STDOUT_FILENO) < 0) {
                    perror("dup2");
                    close(fd);
                    exit(EXIT_FAILURE);
                }

                close(fd);
            }

            /*
             * Input redirection
             */
            if (input_file != NULL) {

                int fd = open(input_file, O_RDONLY);

                if (fd < 0) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }

                if (dup2(fd, STDIN_FILENO) < 0) {
                    perror("dup2");
                    close(fd);
                    exit(EXIT_FAILURE);
                }

                close(fd);
            }

            /*
             * Execute command
             */
            execvp(args[0], args);

            perror("shellcraft");
            exit(EXIT_FAILURE);
        }

        /*
         * Parent process waits
         */
        waitpid(pid, NULL, 0);
    }

    return 0;
}
