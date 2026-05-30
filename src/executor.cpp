#include "executor.hpp"

#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {
struct SavedFd {
    int target_fd;
    int saved_fd;
};

int redirect_target_fd(RedirectType type) {
    return type == RedirectType::Input ? STDIN_FILENO : STDOUT_FILENO;
}

int open_redirect_file(const Redirect &redirect) {
    switch (redirect.type) {
    case RedirectType::Input:
        return open(redirect.filename.c_str(), O_RDONLY);
    case RedirectType::Output:
        return open(redirect.filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    case RedirectType::Append:
        return open(redirect.filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    }

    return -1;
}

bool apply_redirects(const std::vector<Redirect> &redirects, std::vector<SavedFd> *saved_fds = nullptr) {
    for (const Redirect &redirect : redirects) {
        int target_fd = redirect_target_fd(redirect.type);

        if (saved_fds != nullptr) {
            int saved_fd = dup(target_fd);
            if (saved_fd < 0) {
                perror("dup");
                return false;
            }
            saved_fds->push_back({target_fd, saved_fd});
        }

        int file_fd = open_redirect_file(redirect);
        if (file_fd < 0) {
            perror(redirect.filename.c_str());
            return false;
        }

        if (dup2(file_fd, target_fd) < 0) {
            perror("dup2");
            close(file_fd);
            return false;
        }

        close(file_fd);
    }

    return true;
}

void restore_redirects(std::vector<SavedFd> &saved_fds) {
    for (auto it = saved_fds.rbegin(); it != saved_fds.rend(); ++it) {
        if (dup2(it->saved_fd, it->target_fd) < 0) {
            perror("dup2");
        }
        close(it->saved_fd);
    }
    saved_fds.clear();
}
} // namespace

Executor::Executor()
    : builtins{
          {"exit", &Executor::builtin_exit},
          {"pwd", &Executor::builtin_pwd},
          {"cd", &Executor::builtin_cd},
      } {}

int Executor::execute(const Command &cmd) {
    if (cmd.is_empty()) {
        if (cmd.redirects.empty()) {
            return 0;
        }

        std::vector<SavedFd> saved_fds;
        if (!apply_redirects(cmd.redirects, &saved_fds)) {
            restore_redirects(saved_fds);
            return 1;
        }
        restore_redirects(saved_fds);
        return 0;
    }

    int builtin_status = run_builtin(cmd);

    if (builtin_status != not_builtin) {
        return builtin_status;
    }

    return run_external(cmd);
}

int Executor::run_builtin(const Command &cmd) {
    for (const auto &builtin : builtins) {
        if (cmd.args[0] == builtin.name) {
            std::vector<SavedFd> saved_fds;
            if (!apply_redirects(cmd.redirects, &saved_fds)) {
                restore_redirects(saved_fds);
                return 1;
            }

            int status = (this->*builtin.function)(cmd);
            restore_redirects(saved_fds);
            return status;
        }
    }
    return not_builtin;
}

int Executor::run_external(const Command &cmd) {
    std::vector<char *> argv;

    for (const std::string &arg : cmd.args) {
        argv.push_back(const_cast<char *>(arg.c_str()));
    }

    argv.push_back(nullptr);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    } else if (pid == 0) {
        if (!apply_redirects(cmd.redirects)) {
            std::exit(EXIT_FAILURE);
        }

        execvp(argv[0], argv.data());
        perror("execvp");
        std::exit(EXIT_FAILURE);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }

    return 1;
}

int Executor::builtin_exit(const Command &cmd) {
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

int Executor::builtin_pwd(const Command &cmd) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
        perror("pwd");
        return 1;
    }
    std::cout << cwd << std::endl;
    return 0;
}

int Executor::builtin_cd(const Command &cmd) {
    if (cmd.args.size() > 2) {
        std::cerr << "cd: too many arguments" << std::endl;
        return 1;
    }

    std::string path;

    const char *home = std::getenv("HOME");

    if (cmd.args.size() == 1) {
        if (home == nullptr) {
            std::cerr << "cd: HOME not set" << std::endl;
            return 1;
        }

        path = home;
    } else {
        path = cmd.args[1];

        if (path == "~") {
            if (home == nullptr) {
                std::cerr << "cd: HOME not set" << std::endl;
                return 1;
            }
            path = home;
        } else if (path.starts_with("~/")) {
            if (home == nullptr) {
                std::cerr << "cd: HOME not set" << std::endl;
                return 1;
            }
            path = home + path.substr(1);
        }
    }

    if (chdir(path.c_str()) < 0) {
        perror("cd");
        return 1;
    }

    return 0;
}
