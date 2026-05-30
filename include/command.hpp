#pragma once

#include <string>
#include <vector>

enum class RedirectType { Input, Output, Append };

struct Redirect {
    RedirectType type;
    std::string filename;
};

class Command {
  public:
    std::vector<std::string> args;
    std::vector<Redirect> redirects;

    explicit Command(const std::string &line);

    bool is_empty() const;

  private:
    static std::vector<std::string> tokenize(const std::string &line);
    void parse_tokens(const std::vector<std::string> &tokens);
};
