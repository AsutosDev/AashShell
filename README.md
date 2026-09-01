# AashShell

A small Unix-like command-line shell written in C++ for macOS.

AashShell was built from scratch to understand how shells work internally, including process creation, command execution, pipes, redirection, and terminal input handling.

## Features

* Execute external commands
* Built-in commands:

  * `cd`
  * `pwd`
  * `echo`
  * `export`
  * `unset`
  * `help`
  * `exit`
* Single and double quote parsing
* Environment variable expansion
* Input redirection: `<`
* Output redirection: `>`
* Append redirection: `>>`
* Pipelines: `|`
* Conditional operators:

  * `&&`
  * `||`
* Command history
* Up/down arrow history navigation
* Left/right cursor movement
* Backspace editing
* Basic command error handling

## Build

Compile AashShell with a C++ compiler:

```bash
g++ main.cpp -o aash
```

## Run

```bash
./aash
```

You should see:

```text
Welcome to AashShell!
aash$
```

## Examples

```bash
ls
pwd
cd ..
echo hello
echo hello > file.txt
echo world >> file.txt
cat < file.txt
ls | wc -l
false || echo success
true && echo success
```

## Project Structure

```text
AashShell/
├── main.cpp
├── .gitignore
└── README.md
```

## Version

**v1.0.0**

AashShell v1.0.0 is the first stable release.

## Limitations

AashShell is an educational project and does not aim to replace a full-featured shell such as `bash` or `zsh`.

Some advanced shell features are not currently supported.
