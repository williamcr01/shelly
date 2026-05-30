#include "command.hpp"
#include "executor.hpp"

#include <exception>
#include <iostream>
#include <string>

auto main() -> int {
    Executor executor;

    while (true) {
        std::cout << "shelly> ";

        std::string line;
        if (!std::getline(std::cin, line)) {
            break;
        }

        try {
            Command cmd(line);
            executor.execute(cmd);
        } catch (const std::exception &error) {
            std::cerr << error.what() << std::endl;
        }
    }
    return 0;
}
