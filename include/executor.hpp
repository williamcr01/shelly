#pragma once

#include "command.hpp"

class Executor {
  public:
    Executor();

    int execute(const Command &cmd);
    void add_history(const std::string &line);

  private:
    struct Builtin {
        std::string name;
        int (Executor::*function)(const SimpleCommand &);
    };

    std::vector<Builtin> builtins;
    std::vector<std::string> history;

    const int not_builtin = -1;

    int run_builtin(const SimpleCommand &cmd);
    int execute_builtin(const SimpleCommand &cmd);

    int run_external(const SimpleCommand &cmd);
    int run_pipeline(const Command &cmd);

    int builtin_exit(const SimpleCommand &cmd);

    int builtin_pwd(const SimpleCommand &cmd);

    int builtin_cd(const SimpleCommand &cmd);
    int builtin_echo(const SimpleCommand &cmd);
    int builtin_export(const SimpleCommand &cmd);
    int builtin_unset(const SimpleCommand &cmd);
    int builtin_env(const SimpleCommand &cmd);
    int builtin_history(const SimpleCommand &cmd);
};
