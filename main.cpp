#include "command.hpp"
#include "executor.hpp"

#include <iostream>
#include <string>

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
