#include "command.hpp"
#include "executor.hpp"

#include <csignal>
#include <exception>
#include <iostream>
#include <string>

auto main() -> int {
    std::signal(SIGINT, SIG_IGN);

    Executor executor;

    while (true) {
        std::cout << "shelly> ";

        std::string line;
        if (!std::getline(std::cin, line)) {
            break;
        }

        try {
            executor.add_history(line);
            Command cmd(line);
            executor.execute(cmd);
        } catch (const std::exception &error) {
            std::cerr << error.what() << std::endl;
        }
    }
    return 0;
}
