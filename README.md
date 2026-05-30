# Shelly
A simple shell written to learn c++.

## Building
```bash
cmake -S . -B build
cmake --build build
```
## Running
```bash
./build/shelly
```

## Features
- Interactive shell prompt (`shelly>`)
- External command execution via `execvp`
- Built-in commands:
  - `cd` with `~` and `~/...` expansion
  - `pwd`
  - `exit [code]`
- Input redirection with `<`
- Output redirection with `>`
- Append redirection with `>>`
- Redirection support for both external commands and built-ins
- Pipelines with `|` between commands
- Single- and double-quoted arguments
