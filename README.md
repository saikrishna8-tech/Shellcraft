# Shellcraft — A Job-Control Shell

## Project Overview

Shellcraft is a simple command-line shell implemented in the C programming
language for Linux systems.

It demonstrates fundamental operating system concepts such as process
creation, program execution, inter-process communication, file redirection,
and job control.

## Features

- Execute external Linux commands
- Change the current directory using `cd`
- Display the current directory using `pwd`
- Redirect output using `>`
- Append output using `>>`
- Redirect input using `<`
- Connect commands using a single pipe (`|`)
- Run commands in the background using `&`
- List background jobs using `jobs`
- Bring jobs to the foreground using `fg`
- Resume stopped jobs in the background using `bg`
- Handle foreground process interruption with Ctrl+C
- Stop foreground processes with Ctrl+Z

## Technologies Used

- Language: C
- Operating System: Linux / Ubuntu
- Compiler: GCC
- System calls and functions: `fork()`, `execvp()`, `waitpid()`,
  `pipe()`, `dup2()`, `open()`, `chdir()`, and `kill()`

## Project Structure

```text
shellcraft/
├── Makefile
├── README.md
├── docs/
├── tests/
└── src/
    └── main.c
