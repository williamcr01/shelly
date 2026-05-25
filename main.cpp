#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

class Command {
  public:
    std::vector<std::string> args;

    explicit Command(const std::string &line) : args(tokenize(line)) {}

    bool is_empty() const {
        return args.empty();
    }

  private:
    std::vector<std::string> tokenize(const std::string &line) {
        std::vector<std::string> tokens;
        std::string token;

        for (char c : line) {
            if (std::isspace(static_cast<unsigned char>(c))) {
                if (!token.empty()) {
                    tokens.push_back(token);
                    token.clear();
                }
            } else {
                token += static_cast<unsigned char>(c);
            }
        }
        if (!token.empty()) {
            tokens.push_back(token);
        }
        return tokens;
    }
};

class Executor {
  public:
    Executor()
        : builtins{
              {"exit", &Executor::builtin_exit},
          } {}

    int execute(const Command &cmd) {
        if (cmd.is_empty()) {
            return 0;
        }

        if (run_builtin(cmd)) {
            return 0;
        }

        return 1;
    }

  private:
    struct Builtin {
        std::string name;
        int (Executor::*function)(const Command &);
    };

    std::vector<Builtin> builtins;

    bool run_builtin(const Command &cmd) {
        for (const auto &builtin : builtins) {
            if (cmd.args[0] == builtin.name) {
                return (this->*builtin.function)(cmd);
            }
        }
        return false;
    }

    int builtin_exit(const Command &cmd) {
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
};

auto main(int argc, char *argv[]) -> int {
    Executor executor;

    while (true) {
        std::cout << "shelly>";

        std::string line;
        if (!std::getline(std::cin, line)) {
            break;
        }

        Command cmd(line);
        executor.execute(cmd);
    }
    return 0;
}
