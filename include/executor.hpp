#pragma once

#include "command.hpp"

class Executor {
  public:
    Executor();

    int execute(const Command &cmd);

  private:
    struct Builtin {
        std::string name;
        int (Executor::*function)(const Command &);
    };

    std::vector<Builtin> builtins;

    bool run_builtin(const Command &cmd);

    bool run_external(const Command &cmd);

    int builtin_exit(const Command &cmd);

    int builtin_pwd(const Command &cmd);

    int builtin_cd(const Command &cmd);
};
