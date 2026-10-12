# SHELLCRAFT
## A Job-Control Shell

### 1. Abstract

Shellcraft is a command-line shell developed using the C programming
language for Linux systems. It demonstrates essential operating system
concepts, including process creation, program execution, inter-process
communication, file redirection, and job control.

The shell accepts user commands, executes external programs, and provides
basic facilities for managing foreground and background processes.

### 2. Introduction

A shell is a program that provides an interface between the user and the
operating system. It accepts commands, interprets them, and executes
the requested operations.

Shellcraft is designed to demonstrate how a simple Unix-like shell can
be implemented using Linux system calls and C library functions.

### 3. Objectives

- Develop an interactive command-line shell.
- Execute external Linux commands.
- Implement basic built-in commands.
- Support input and output redirection.
- Implement communication between commands using pipes.
- Support background process execution.
- Demonstrate basic foreground and background job management.
- Understand process management and signal handling.

### 4. Technologies Used

- Programming language: C
- Operating system: Ubuntu/Linux
- Compiler: GCC
- Build tool: GNU Make

### 5. System Calls and Functions

#### fork()

Creates a child process so that a command can run separately from the shell.

#### execvp()

Replaces the child process with the requested external program.

#### waitpid()

Allows the shell to wait for child processes and observe their status.

#### pipe()

Creates a communication channel between processes.

#### dup2()

Redirects standard input or standard output to another file descriptor.

#### open()

Opens files for reading or writing.

#### chdir()

Changes the current working directory.

#### kill()

Sends a signal to a process or process group.

### 6. Features

- Execute external commands.
- Display the current directory using `pwd`.
- Change directories using `cd`.
- Redirect output using `>`.
- Append output using `>>`.
- Redirect input using `<`.
- Connect two commands using a single pipe.
- Run commands in the background using `&`.
- List jobs using `jobs`.
- Bring a job to the foreground using `fg`.
- Resume a stopped job in the background using `bg`.
- Handle foreground process interruption and stopping signals.

Note: Feature availability and reliability depend on the current
implementation. Advanced Bash functionality is not included.

### 7. Architecture

The shell follows this general execution flow:

1. Display the shell prompt.
2. Read a command from the user.
3. Check whether the command is a built-in command.
4. Identify supported operators such as pipes, redirection, and `&`.
5. Create child processes when required.
6. Execute external commands.
7. Wait for foreground commands or track background jobs.
8. Return to the prompt.

### 8. Compilation and Execution

Navigate to the project directory:

    cd ~/shellcraft

Compile the project:

    make

Run the shell:

    make run

Exit the shell:

    exit

### 9. Limitations

- Command parsing is simplified.
- Only a single pipe between two commands is supported.
- Advanced shell scripting and command expansion are not implemented.
- Complex quoting and escaping are not fully supported.
- Job-control functionality requires further validation before being
  considered fully reliable.

### 10. Conclusion

Shellcraft demonstrates the fundamental mechanisms used by a Unix-like
shell to execute commands and manage processes.

The project provides practical experience with process creation,
program execution, pipes, file descriptors, signals, and job management.
