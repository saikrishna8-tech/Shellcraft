#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64

void execute_command(char **args, int input_fd, int output_fd) {

    if (input_fd != STDIN_FILENO) {
        dup2(input_fd, STDIN_FILENO);
        close(input_fd);
    }

    if (output_fd != STDOUT_FILENO) {
        dup2(output_fd, STDOUT_FILENO);
        close(output_fd);
    }

    execvp(args[0], args);

    perror("shellcraft");
    exit(EXIT_FAILURE);
}

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

        /* exit */
        if (strcmp(input, "exit") == 0) {
            break;
        }

        /* pwd */
        if (strcmp(input, "pwd") == 0) {

            char cwd[MAX_INPUT];

            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            } else {
                perror("pwd");
            }

            continue;
        }

        /* cd */
        if (strncmp(input, "cd ", 3) == 0) {

            char *directory = input + 3;

            if (chdir(directory) != 0) {
                perror("cd");
            }

            continue;
        }

        /*
         * Check for background execution
         */
        int background = 0;

        int input_length = strlen(input);

        if (input_length > 0 && input[input_length - 1] == '&') {

            background = 1;

            input[input_length - 1] = '\0';

            while (input[strlen(input) - 1] == ' ') {
                input[strlen(input) - 1] = '\0';
            }
        }

        /*
         * Check for pipe
         */

        char *pipe_position = strchr(input, '|');

        if (pipe_position != NULL) {

            *pipe_position = '\0';

            char *left_command = input;
            char *right_command = pipe_position + 1;

            while (*left_command == ' ') {
                left_command++;
            }

            while (*right_command == ' ') {
                right_command++;
            }

            /*
             * Left command
             */

            char *left_args[MAX_ARGS];
            int left_argc = 0;

            char *token = strtok(left_command, " ");

            while (token != NULL && left_argc < MAX_ARGS - 1) {

                left_args[left_argc++] = token;

                token = strtok(NULL, " ");
            }

            left_args[left_argc] = NULL;

            /*
             * Right command
             */

            char *right_args[MAX_ARGS];
            int right_argc = 0;

            token = strtok(right_command, " ");

            while (token != NULL && right_argc < MAX_ARGS - 1) {

                right_args[right_argc++] = token;

                token = strtok(NULL, " ");
            }

            right_args[right_argc] = NULL;

            /*
             * Create pipe
             */

            int pipe_fd[2];

            if (pipe(pipe_fd) == -1) {
                perror("pipe");
                continue;
            }

            /*
             * First child
             */

            pid_t pid1 = fork();

            if (pid1 < 0) {

                perror("fork");

                close(pipe_fd[0]);
                close(pipe_fd[1]);

                continue;
            }

            if (pid1 == 0) {

                close(pipe_fd[0]);

                execute_command(
                    left_args,
                    STDIN_FILENO,
                    pipe_fd[1]
                );
            }

            /*
             * Second child
             */

            pid_t pid2 = fork();

            if (pid2 < 0) {

                perror("fork");

                close(pipe_fd[0]);
                close(pipe_fd[1]);

                waitpid(pid1, NULL, 0);

                continue;
            }

            if (pid2 == 0) {

                close(pipe_fd[1]);

                execute_command(
                    right_args,
                    pipe_fd[0],
                    STDOUT_FILENO
                );
            }

            close(pipe_fd[0]);
            close(pipe_fd[1]);

            /*
             * Background pipeline
             */

            if (!background) {

                waitpid(pid1, NULL, 0);
                waitpid(pid2, NULL, 0);

            } else {

                printf("[Background] PIDs: %d %d\n",
                       pid1, pid2);
            }

            continue;
        }

        /*
         * Normal command
         */

        char *args[MAX_ARGS];
        int argc = 0;

        char *token = strtok(input, " ");

        while (token != NULL && argc < MAX_ARGS - 1) {

            args[argc++] = token;

            token = strtok(NULL, " ");
        }

        args[argc] = NULL;

        /*
         * Redirection
         */

        char *output_file = NULL;
        char *input_file = NULL;

        int append = 0;

        for (int i = 0; i < argc; i++) {

            /* > */

            if (strcmp(args[i], ">") == 0 && i + 1 < argc) {

                output_file = args[i + 1];

                append = 0;

                args[i] = NULL;

                argc = i;

                break;
            }

            /* >> */

            if (strcmp(args[i], ">>") == 0 && i + 1 < argc) {

                output_file = args[i + 1];

                append = 1;

                args[i] = NULL;

                argc = i;

                break;
            }

            /* < */

            if (strcmp(args[i], "<") == 0 && i + 1 < argc) {

                input_file = args[i + 1];

                args[i] = NULL;

                argc = i;

                break;
            }
        }

        args[argc] = NULL;

        /*
         * Create child
         */

        pid_t pid = fork();

        if (pid < 0) {

            perror("fork");

            continue;
        }

        /*
         * Child
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

                int fd = open(
                    output_file,
                    flags,
                    0644
                );

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

                int fd = open(
                    input_file,
                    O_RDONLY
                );

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
         * Parent
         */

        if (!background) {

            waitpid(pid, NULL, 0);

        } else {

            printf("[Background] PID: %d\n", pid);
        }
    }

    return 0;
}
