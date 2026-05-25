#include "command.hpp"

#include <cctype>

Command::Command(const std::string &line) : args(tokenize(line)) {}

bool Command::is_empty() const {
    return args.empty();
}

std::vector<std::string> Command::tokenize(const std::string &line) {
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
