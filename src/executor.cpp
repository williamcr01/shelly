#include "executor.hpp"

#include <cstdlib>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

Executor::Executor()
    : builtins{
          {"exit", &Executor::builtin_exit},
      } {}

int Executor::execute(const Command &cmd) {
    if (cmd.is_empty()) {
        return 0;
    }

    if (run_builtin(cmd)) {
        return 0;
    }

    return run_external(cmd);
}

bool Executor::run_builtin(const Command &cmd) {
    for (const auto &builtin : builtins) {
        if (cmd.args[0] == builtin.name) {
            return (this->*builtin.function)(cmd);
        }
    }
    return false;
}

bool Executor::run_external(const Command &cmd) {
    std::vector<char *> argv;

    for (const std::string &arg : cmd.args) {
        argv.push_back(const_cast<char *>(arg.c_str()));
    }

    argv.push_back(nullptr);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return false;
    } else if (pid == 0) {
        execvp(argv[0], argv.data());
        perror("execvp");
        std::exit(EXIT_FAILURE);
    } else {
        int status;
        if (waitpid(pid, &status, 0) < 0) {
            perror("waitpid");
            return 1;
        }
    }

    return 0;
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
