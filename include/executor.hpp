#pragma once

#include "command.hpp"

class Executor {
  public:
    Executor();

    int execute(const Command &cmd);

  private:
    struct Builtin {
        std::string name;
        int (Executor::*function)(const SimpleCommand &);
    };

    std::vector<Builtin> builtins;

    const int not_builtin = -1;

    int run_builtin(const SimpleCommand &cmd);
    int execute_builtin(const SimpleCommand &cmd);

    int run_external(const SimpleCommand &cmd);
    int run_pipeline(const Command &cmd);

    int builtin_exit(const SimpleCommand &cmd);

    int builtin_pwd(const SimpleCommand &cmd);

    int builtin_cd(const SimpleCommand &cmd);
};
