#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <errno.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64
#define MAX_JOBS 64

typedef enum {
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} JobStatus;

typedef struct {
    int id;
    pid_t pgid;
    char command[MAX_INPUT];
    JobStatus status;
} Job;

Job jobs[MAX_JOBS];
int job_count = 0;

pid_t shell_pgid;
struct termios shell_tmodes;

/* ---------- Job Functions ---------- */

void add_job(pid_t pgid, const char *command, JobStatus status)
{
    if (job_count >= MAX_JOBS) {
        printf("Maximum number of jobs reached.\n");
        return;
    }

    jobs[job_count].id = job_count + 1;
    jobs[job_count].pgid = pgid;

    strncpy(jobs[job_count].command, command, MAX_INPUT - 1);
    jobs[job_count].command[MAX_INPUT - 1] = '\0';

    jobs[job_count].status = status;

    printf("[%d] %d\n",
           jobs[job_count].id,
           jobs[job_count].pgid);

    job_count++;
}

int find_job(int id)
{
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].id == id) {
            return i;
        }
    }

    return -1;
}

void update_jobs()
{
    for (int i = 0; i < job_count; i++) {

        if (jobs[i].status == JOB_DONE) {
            continue;
        }

        int status;

        pid_t result = waitpid(
            -jobs[i].pgid,
            &status,
            WNOHANG | WUNTRACED | WCONTINUED
        );

        if (result == -1) {
            continue;
        }

        if (WIFSTOPPED(status)) {
            jobs[i].status = JOB_STOPPED;
        }
        else if (WIFCONTINUED(status)) {
            jobs[i].status = JOB_RUNNING;
        }
        else if (WIFEXITED(status) || WIFSIGNALED(status)) {
            jobs[i].status = JOB_DONE;
        }
    }
}

void print_jobs()
{
    update_jobs();

    for (int i = 0; i < job_count; i++) {

        if (jobs[i].status == JOB_RUNNING) {
            printf("[%d] Running    %s\n",
                   jobs[i].id,
                   jobs[i].command);
        }
        else if (jobs[i].status == JOB_STOPPED) {
            printf("[%d] Stopped    %s\n",
                   jobs[i].id,
                   jobs[i].command);
        }
        else if (jobs[i].status == JOB_DONE) {
            printf("[%d] Done       %s\n",
                   jobs[i].id,
                   jobs[i].command);
        }
    }
}

/* ---------- Signal Handling ---------- */

void handle_sigint(int sig)
{
    (void)sig;
}

void handle_sigtstp(int sig)
{
    (void)sig;
}

/* ---------- Put Job in Foreground ---------- */

void foreground_job(int job_index)
{
    if (job_index < 0 || job_index >= job_count) {
        printf("No such job.\n");
        return;
    }

    pid_t pgid = jobs[job_index].pgid;

    /* Give terminal to job */

    tcsetpgrp(STDIN_FILENO, pgid);

    /* Continue if stopped */

    if (jobs[job_index].status == JOB_STOPPED) {
        if (kill(-pgid, SIGCONT) < 0) {
            perror("SIGCONT");
        }

        jobs[job_index].status = JOB_RUNNING;
    }

    /* Wait for job */

    int status;

    while (1) {

        pid_t result = waitpid(
            -pgid,
            &status,
            WUNTRACED
        );

        if (result == -1) {

            if (errno == EINTR) {
                continue;
            }

            break;
        }

        if (WIFSTOPPED(status)) {
            jobs[job_index].status = JOB_STOPPED;

            printf("\n[%d] Stopped    %s\n",
                   jobs[job_index].id,
                   jobs[job_index].command);

            break;
        }

        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            jobs[job_index].status = JOB_DONE;
            break;
        }
    }

    /* Take terminal back */

    tcsetpgrp(STDIN_FILENO, shell_pgid);
}

/* ---------- Put Job in Background ---------- */

void background_job(int job_index)
{
    if (job_index < 0 || job_index >= job_count) {
        printf("No such job.\n");
        return;
    }

    if (jobs[job_index].status == JOB_DONE) {
        printf("Job already completed.\n");
        return;
    }

    if (jobs[job_index].status == JOB_STOPPED) {

        if (kill(-jobs[job_index].pgid, SIGCONT) < 0) {
            perror("SIGCONT");
            return;
        }

        jobs[job_index].status = JOB_RUNNING;
    }

    printf("[%d] Running    %s &\n",
           jobs[job_index].id,
           jobs[job_index].command);
}

/* ---------- Execute Command ---------- */

void execute_command(
    char **args,
    int input_fd,
    int output_fd
)
{
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

/* ---------- Main ---------- */

int main()
{
    char input[MAX_INPUT];

    /* Shell process group */

    shell_pgid = getpid();

    if (setpgid(shell_pgid, shell_pgid) < 0 &&
        errno != EACCES) {
        perror("setpgid");
    }

    tcsetpgrp(STDIN_FILENO, shell_pgid);

    tcgetattr(STDIN_FILENO, &shell_tmodes);

    /* Ignore terminal signals in shell */

    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    while (1) {

        update_jobs();

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

        /* ---------- exit ---------- */

        if (strcmp(input, "exit") == 0) {
            break;
        }

        /* ---------- jobs ---------- */

        if (strcmp(input, "jobs") == 0) {
            print_jobs();
            continue;
        }

        /* ---------- pwd ---------- */

        if (strcmp(input, "pwd") == 0) {

            char cwd[MAX_INPUT];

            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            }
            else {
                perror("pwd");
            }

            continue;
        }

        /* ---------- cd ---------- */

        if (strncmp(input, "cd ", 3) == 0) {

            char *directory = input + 3;

            if (chdir(directory) != 0) {
                perror("cd");
            }

            continue;
        }

        /* ---------- fg ---------- */

        if (strncmp(input, "fg %", 3) == 0) {

            int job_id = atoi(input + 3);

            int index = find_job(job_id);

            foreground_job(index);

            continue;
        }

        /* ---------- bg ---------- */

        if (strncmp(input, "bg %", 3) == 0) {

            int job_id = atoi(input + 3);

            int index = find_job(job_id);

            background_job(index);

            continue;
        }

        /* ---------- Background ---------- */

        int background = 0;

        int input_length = strlen(input);

        if (input_length > 0 &&
            input[input_length - 1] == '&') {

            background = 1;

            input[input_length - 1] = '\0';

            while (strlen(input) > 0 &&
                   input[strlen(input) - 1] == ' ') {

                input[strlen(input) - 1] = '\0';
            }
        }

        /* Save command */

        char original_command[MAX_INPUT];

        strncpy(
            original_command,
            input,
            MAX_INPUT - 1
        );

        original_command[MAX_INPUT - 1] = '\0';

        /* ---------- Pipe ---------- */

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

            /* Left arguments */

            char *left_args[MAX_ARGS];
            int left_argc = 0;

            char *token = strtok(left_command, " ");

            while (token != NULL &&
                   left_argc < MAX_ARGS - 1) {

                left_args[left_argc++] = token;

                token = strtok(NULL, " ");
            }

            left_args[left_argc] = NULL;

            /* Right arguments */

            char *right_args[MAX_ARGS];
            int right_argc = 0;

            token = strtok(right_command, " ");

            while (token != NULL &&
                   right_argc < MAX_ARGS - 1) {

                right_args[right_argc++] = token;

                token = strtok(NULL, " ");
            }

            right_args[right_argc] = NULL;

            /* Create pipe */

            int pipe_fd[2];

            if (pipe(pipe_fd) == -1) {
                perror("pipe");
                continue;
            }

            /* First child */

            pid_t pid1 = fork();

            if (pid1 < 0) {

                perror("fork");

                close(pipe_fd[0]);
                close(pipe_fd[1]);

                continue;
            }

            if (pid1 == 0) {

                signal(SIGINT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
                signal(SIGQUIT, SIG_DFL);

                setpgid(0, 0);

                close(pipe_fd[0]);

                execute_command(
                    left_args,
                    STDIN_FILENO,
                    pipe_fd[1]
                );
            }

            /* Create process group */

            setpgid(pid1, pid1);

            /* Second child */

            pid_t pid2 = fork();

            if (pid2 < 0) {

                perror("fork");

                close(pipe_fd[0]);
                close(pipe_fd[1]);

                kill(pid1, SIGTERM);

                waitpid(pid1, NULL, 0);

                continue;
            }

            if (pid2 == 0) {

                signal(SIGINT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
                signal(SIGQUIT, SIG_DFL);

                setpgid(0, pid1);

                close(pipe_fd[1]);

                execute_command(
                    right_args,
                    pipe_fd[0],
                    STDOUT_FILENO
                );
            }

            setpgid(pid2, pid1);

            close(pipe_fd[0]);
            close(pipe_fd[1]);

            if (background) {

                add_job(
                    pid1,
                    original_command,
                    JOB_RUNNING
                );

            }
            else {

                tcsetpgrp(STDIN_FILENO, pid1);

                int status;

                waitpid(
                    pid1,
                    &status,
                    WUNTRACED
                );

                waitpid(
                    pid2,
                    &status,
                    WUNTRACED
                );

                tcsetpgrp(
                    STDIN_FILENO,
                    shell_pgid
                );
            }

            continue;
        }

        /* ---------- Normal Command ---------- */

        char *args[MAX_ARGS];

        int argc = 0;

        char *token = strtok(input, " ");

        while (token != NULL &&
               argc < MAX_ARGS - 1) {

            args[argc++] = token;

            token = strtok(NULL, " ");
        }

        args[argc] = NULL;

        /* ---------- Redirection ---------- */

        char *output_file = NULL;
        char *input_file = NULL;

        int append = 0;

        for (int i = 0; i < argc; i++) {

            if (strcmp(args[i], ">") == 0 &&
                i + 1 < argc) {

                output_file = args[i + 1];

                append = 0;

                args[i] = NULL;
                argc = i;

                break;
            }

            if (strcmp(args[i], ">>") == 0 &&
                i + 1 < argc) {

                output_file = args[i + 1];

                append = 1;

                args[i] = NULL;
                argc = i;

                break;
            }

            if (strcmp(args[i], "<") == 0 &&
                i + 1 < argc) {

                input_file = args[i + 1];

                args[i] = NULL;
                argc = i;

                break;
            }
        }

        args[argc] = NULL;

        /* ---------- Create Child ---------- */

        pid_t pid = fork();

        if (pid < 0) {

            perror("fork");
            continue;
        }

        /* ---------- Child ---------- */

        if (pid == 0) {

            /* Restore signals */

            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);

            /* Create process group */

            setpgid(0, 0);

            /* Output redirection */

            if (output_file != NULL) {

                int flags = O_WRONLY | O_CREAT;

                if (append) {
                    flags |= O_APPEND;
                }
                else {
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

            /* Input redirection */

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

            execvp(args[0], args);

            perror("shellcraft");

            exit(EXIT_FAILURE);
        }

        /* ---------- Parent ---------- */

        setpgid(pid, pid);

        if (background) {

            add_job(
                pid,
                original_command,
                JOB_RUNNING
            );

        }
        else {

            /* Give terminal to foreground process */

            tcsetpgrp(
                STDIN_FILENO,
                pid
            );

            int status;

            while (1) {

                pid_t result = waitpid(
                    pid,
                    &status,
                    WUNTRACED
                );

                if (result == -1) {

                    if (errno == EINTR) {
                        continue;
                    }

                    break;
                }

                if (WIFSTOPPED(status)) {

                    add_job(
                        pid,
                        original_command,
                        JOB_STOPPED
                    );

                    printf(
                        "\n[%d] Stopped    %s\n",
                        job_count,
                        original_command
                    );

                    break;
                }

                if (WIFEXITED(status) ||
                    WIFSIGNALED(status)) {

                    break;
                }
            }

            /* Take terminal back */

            tcsetpgrp(
                STDIN_FILENO,
                shell_pgid
            );
        }
    }

    return 0;
}
