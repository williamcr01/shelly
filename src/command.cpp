#include "command.hpp"

#include <cctype>
#include <stdexcept>

Command::Command(const std::string &line) : args(tokenize(line)) {}

bool Command::is_empty() const {
    return args.empty();
}

std::vector<std::string> Command::tokenize(const std::string &line) {
    std::vector<std::string> tokens;
    std::string token;

    bool in_single_quote = false;
    bool in_double_quote = false;

    for (char c : line) {
        if (c == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
        } else if (c == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
        } else if (std::isspace(static_cast<unsigned char>(c)) && !in_single_quote &&
                   !in_double_quote) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }

    if (in_single_quote || in_double_quote) {
        throw std::runtime_error("unclosed quote");
    }

    if (!token.empty()) {
        tokens.push_back(token);
    }

    return tokens;
}
