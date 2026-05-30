#include "command.hpp"

#include <cctype>
#include <stdexcept>

Command::Command(const std::string &line) {
    std::vector<std::string> tokens = tokenize(line);
    parse_tokens(tokens);
}

bool SimpleCommand::is_empty() const {
    return args.empty() && redirects.empty();
}

bool Command::is_empty() const {
    return stages.empty();
}

bool Command::is_pipeline() const {
    return stages.size() > 1;
}

std::vector<std::string> Command::tokenize(const std::string &line) {
    std::vector<std::string> tokens;
    std::string token;

    bool in_single_quote = false;
    bool in_double_quote = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        char c = line[i];

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
        } else if ((c == '<' || c == '>' || c == '|') && !in_single_quote && !in_double_quote) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }

            if (c == '>' && i + 1 < line.size() && line[i + 1] == '>') {
                tokens.push_back(">>");
                ++i;
            } else {
                tokens.emplace_back(1, c);
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

SimpleCommand Command::parse_stage(const std::vector<std::string> &tokens,
                                   std::size_t begin,
                                   std::size_t end) {
    SimpleCommand stage;

    for (std::size_t i = begin; i < end; ++i) {
        const std::string &token = tokens[i];
        if (token == "<" || token == ">" || token == ">>") {
            if (i + 1 >= end || tokens[i + 1] == "|") {
                throw std::runtime_error("missing filename for redirection");
            }

            RedirectType type;

            if (token == "<") {
                type = RedirectType::Input;
            } else if (token == ">") {
                type = RedirectType::Output;
            } else {
                type = RedirectType::Append;
            }

            stage.redirects.push_back({type, tokens[i + 1]});
            i++;
        } else {
            stage.args.push_back(token);
        }
    }

    return stage;
}

void Command::parse_tokens(const std::vector<std::string> &tokens) {
    if (tokens.empty()) {
        return;
    }

    std::size_t stage_begin = 0;
    for (std::size_t i = 0; i <= tokens.size(); ++i) {
        if (i == tokens.size() || tokens[i] == "|") {
            SimpleCommand stage = parse_stage(tokens, stage_begin, i);
            if (stage.is_empty()) {
                throw std::runtime_error("syntax error near unexpected token `|'");
            }
            stages.push_back(stage);
            stage_begin = i + 1;
        }
    }

    if (stages.size() == 1) {
        args = stages[0].args;
        redirects = stages[0].redirects;
    }
}
