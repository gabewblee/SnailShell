# SnailShell

## Description

SnailShell is a simplified shell program written in C that enables users to execute commands interactively or through scripts.

## Features

Currently, it supports the following features:

1. Input Redirection
2. Output Redirection
3. Pipelining
4. Environment Variables
5. Script Execution
6. Command Execution

## Installation

1. Clone the repository:
```
git clone https://github.com/gabewblee/SnailShell.git
```
2. Compile the program:
```
make
```
3. (Optional) To remove the generated files from the build process:
```
make clean
```

## Usage

1. To find help, run:
```
./SnailShell -h
```
Or alternatively,
```
./SnailShell --help
```
2. **Interactive Mode:** Runs the shell interactively.
```
./SnailShell
```
3. **Script Mode:** Runs a pre-defined script.
```
./SnailShell -i [Filename]
```
Or alternatively,
```
./SnailShell --init-file=[Filename]
```
