#include "executor.hpp"

#include <csignal>
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
          {"echo", &Executor::builtin_echo},
          {"export", &Executor::builtin_export},
          {"unset", &Executor::builtin_unset},
          {"env", &Executor::builtin_env},
          {"history", &Executor::builtin_history},
      } {}

void Executor::add_history(const std::string &line) {
    if (!line.empty()) {
        history.push_back(line);
    }
}

int Executor::execute(const Command &cmd) {
    if (cmd.is_empty()) {
        return 0;
    }

    if (cmd.is_pipeline()) {
        return run_pipeline(cmd);
    }

    const SimpleCommand &stage = cmd.stages[0];

    if (stage.args.empty()) {
        std::vector<SavedFd> saved_fds;
        if (!apply_redirects(stage.redirects, &saved_fds)) {
            restore_redirects(saved_fds);
            return 1;
        }
        restore_redirects(saved_fds);
        return 0;
    }

    int builtin_status = run_builtin(stage);

    if (builtin_status != not_builtin) {
        return builtin_status;
    }

    return run_external(stage);
}

int Executor::execute_builtin(const SimpleCommand &cmd) {
    for (const auto &builtin : builtins) {
        if (!cmd.args.empty() && cmd.args[0] == builtin.name) {
            return (this->*builtin.function)(cmd);
        }
    }
    return not_builtin;
}

int Executor::run_builtin(const SimpleCommand &cmd) {
    bool is_builtin = false;
    for (const auto &builtin : builtins) {
        if (!cmd.args.empty() && cmd.args[0] == builtin.name) {
            is_builtin = true;
            break;
        }
    }

    if (!is_builtin) {
        return not_builtin;
    }

    std::vector<SavedFd> saved_fds;
    if (!apply_redirects(cmd.redirects, &saved_fds)) {
        restore_redirects(saved_fds);
        return 1;
    }

    int status = execute_builtin(cmd);
    restore_redirects(saved_fds);
    return status;
}

int Executor::run_external(const SimpleCommand &cmd) {
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
        std::signal(SIGINT, SIG_DFL);

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

int Executor::run_pipeline(const Command &cmd) {
    std::vector<pid_t> pids;
    int previous_read_fd = -1;

    for (std::size_t i = 0; i < cmd.stages.size(); ++i) {
        int pipe_fds[2] = {-1, -1};
        bool has_next = i + 1 < cmd.stages.size();

        if (has_next && pipe(pipe_fds) < 0) {
            perror("pipe");
            if (previous_read_fd >= 0) {
                close(previous_read_fd);
            }
            return 1;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            if (previous_read_fd >= 0) {
                close(previous_read_fd);
            }
            if (pipe_fds[0] >= 0) {
                close(pipe_fds[0]);
            }
            if (pipe_fds[1] >= 0) {
                close(pipe_fds[1]);
            }
            return 1;
        }

        if (pid == 0) {
            std::signal(SIGINT, SIG_DFL);

            if (previous_read_fd >= 0) {
                if (dup2(previous_read_fd, STDIN_FILENO) < 0) {
                    perror("dup2");
                    std::exit(EXIT_FAILURE);
                }
                close(previous_read_fd);
            }

            if (has_next) {
                close(pipe_fds[0]);
                if (dup2(pipe_fds[1], STDOUT_FILENO) < 0) {
                    perror("dup2");
                    std::exit(EXIT_FAILURE);
                }
                close(pipe_fds[1]);
            }

            const SimpleCommand &stage = cmd.stages[i];
            if (!apply_redirects(stage.redirects)) {
                std::exit(EXIT_FAILURE);
            }

            if (stage.args.empty()) {
                std::exit(EXIT_SUCCESS);
            }

            int builtin_status = execute_builtin(stage);
            if (builtin_status != not_builtin) {
                std::exit(builtin_status);
            }

            std::vector<char *> argv;
            for (const std::string &arg : stage.args) {
                argv.push_back(const_cast<char *>(arg.c_str()));
            }
            argv.push_back(nullptr);

            execvp(argv[0], argv.data());
            perror("execvp");
            std::exit(EXIT_FAILURE);
        }

        pids.push_back(pid);

        if (previous_read_fd >= 0) {
            close(previous_read_fd);
        }
        if (has_next) {
            close(pipe_fds[1]);
            previous_read_fd = pipe_fds[0];
        } else {
            previous_read_fd = -1;
        }
    }

    int last_status = 0;
    for (pid_t pid : pids) {
        int status = 0;
        if (waitpid(pid, &status, 0) < 0) {
            perror("waitpid");
            last_status = 1;
            continue;
        }

        if (pid == pids.back()) {
            if (WIFEXITED(status)) {
                last_status = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                last_status = 128 + WTERMSIG(status);
            } else {
                last_status = 1;
            }
        }
    }

    return last_status;
}

int Executor::builtin_exit(const SimpleCommand &cmd) {
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

int Executor::builtin_pwd(const SimpleCommand &cmd) {
    (void)cmd;

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
        perror("pwd");
        return 1;
    }
    std::cout << cwd << std::endl;
    return 0;
}

int Executor::builtin_cd(const SimpleCommand &cmd) {
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

int Executor::builtin_echo(const SimpleCommand &cmd) {
    for (std::size_t i = 1; i < cmd.args.size(); ++i) {
        if (i > 1) {
            std::cout << ' ';
        }
        std::cout << cmd.args[i];
    }
    std::cout << std::endl;
    return 0;
}

int Executor::builtin_export(const SimpleCommand &cmd) {
    if (cmd.args.size() == 1) {
        return builtin_env(cmd);
    }

    for (std::size_t i = 1; i < cmd.args.size(); ++i) {
        std::string name;
        std::string value;
        std::size_t equals = cmd.args[i].find('=');

        if (equals != std::string::npos) {
            name = cmd.args[i].substr(0, equals);
            value = cmd.args[i].substr(equals + 1);
        } else if (i + 1 < cmd.args.size()) {
            name = cmd.args[i];
            value = cmd.args[++i];
        } else {
            name = cmd.args[i];
            value.clear();
        }

        if (name.empty() || name.find('=') != std::string::npos) {
            std::cerr << "export: invalid name: " << name << std::endl;
            return 1;
        }

        if (setenv(name.c_str(), value.c_str(), 1) < 0) {
            perror("export");
            return 1;
        }
    }

    return 0;
}

int Executor::builtin_unset(const SimpleCommand &cmd) {
    for (std::size_t i = 1; i < cmd.args.size(); ++i) {
        if (unsetenv(cmd.args[i].c_str()) < 0) {
            perror("unset");
            return 1;
        }
    }
    return 0;
}

extern char **environ;

int Executor::builtin_env(const SimpleCommand &cmd) {
    (void)cmd;

    for (char **entry = environ; *entry != nullptr; ++entry) {
        std::cout << *entry << std::endl;
    }
    return 0;
}

int Executor::builtin_history(const SimpleCommand &cmd) {
    (void)cmd;

    for (std::size_t i = 0; i < history.size(); ++i) {
        std::cout << i + 1 << "  " << history[i] << std::endl;
    }
    return 0;
}
