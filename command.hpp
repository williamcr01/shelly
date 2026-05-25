#pragma once

#include <string>
#include <vector>

class Command {
  public:
    std::vector<std::string> args;

    explicit Command(const std::string &line);

    bool is_empty() const;

  private:
    std::vector<std::string> tokenize(const std::string &line);
};
