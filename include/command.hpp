#pragma once

#include <string>
#include <vector>

enum class RedirectType { Input, Output, Append };

struct Redirect {
    RedirectType type;
    std::string filename;
};

struct SimpleCommand {
    std::vector<std::string> args;
    std::vector<Redirect> redirects;

    bool is_empty() const;
};

class Command {
  public:
    // Kept as convenient aliases for the single-command case.
    std::vector<std::string> args;
    std::vector<Redirect> redirects;
    std::vector<SimpleCommand> stages;

    explicit Command(const std::string &line);

    bool is_empty() const;
    bool is_pipeline() const;

  private:
    static std::vector<std::string> tokenize(const std::string &line);
    static SimpleCommand parse_stage(const std::vector<std::string> &tokens,
                                     std::size_t begin,
                                     std::size_t end);
    void parse_tokens(const std::vector<std::string> &tokens);
};
