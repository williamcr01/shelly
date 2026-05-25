#include "executor.hpp"

#include <cstdlib>
#include <iostream>

Executor::Executor()
    : builtins{
          {"exit", &Executor::builtin_exit},
      } {}

int Executor::execute(const Command &cmd) {
    if (cmd.is_empty()) {
        return 0;
    }

    if (run_command(cmd)) {
        return 0;
    }

    return 1;
}

bool Executor::run_builtin(const Command &cmd) {
    for (const auto &builtin : builtins) {
        if (cmd.args[0] == builtin.name) {
            return (this->*builtin.function)(cmd);
        }
    }
    return false;
}

bool Executor::run_command(const Command &cmd) {
    return run_builtin(cmd);
}

int Executor::builtin_exit(const Command &cmd) {
    int code = 0;

    if (cmd.args.size() > 1) {
        try {
            code = std::stoi(cmd.args[1]);
        } catch (const std::invalid_argument &) {
            std::cerr << "Invalid exit code: " << cmd.args[1] << std::endl;
            return -1;
        }
    }

    std::exit(code);
}
