/*
 * adsu.cc
 * AD Super User Tool
 * Powered by ADlibc++
 */

#include "include/ADlibc++.hpp"
#include <getopt.h>
#include <filesystem>
#include <pwd.h>
#include <unistd.h>
#include <vector>

using namespace AD;

class UserSwitcher {
private:
    std::string target_user;
    std::string target_group;
    bool verbose = false;

    void parse_combined(const std::string& combined) {
        size_t colon_pos = combined.find(':');
        if (colon_pos != std::string::npos) {
            target_user = combined.substr(0, colon_pos);
            target_group = combined.substr(colon_pos + 1);
        } else {
            target_user = combined;
        }
    }

    std::string resolve_shell(const char* preferred_shell) {
        std::vector<std::string> candidates;

        if (preferred_shell && strlen(preferred_shell) > 0) {
            candidates.push_back(preferred_shell);
        }

        candidates.push_back("/var/jb/bin/bash");
        candidates.push_back("/var/jb/usr/bin/bash");
        candidates.push_back("/var/jb/bin/zsh");
        candidates.push_back("/var/jb/usr/bin/zsh");

        candidates.push_back("/bin/bash");
        candidates.push_back("/usr/bin/bash");
        candidates.push_back("/bin/zsh");
        candidates.push_back("/bin/sh");

        for (const auto& path : candidates) {
            if (std::filesystem::exists(path)) {
                return path;
            }
        }
        
        return "/bin/sh"; 
    }

public:
    void parse_args(int argc, char* argv[]) {
        static struct option long_opts[] = {
            {"user", required_argument, 0, 'u'},
            {"group", required_argument, 0, 'g'},
            {"verbose", no_argument, 0, 'v'},
            {"help", no_argument, 0, 'h'},
            {0, 0, 0, 0}
        };

        int c;
        while ((c = getopt_long(argc, argv, "u:g:vh", long_opts, nullptr)) != -1) {
            switch (c) {
                case 'u': target_user = optarg; break;
                case 'g': target_group = optarg; break;
                case 'v': verbose = true; break;
                case 'h': 
                    AD::cout << "Usage: adsu [user] | [-u user] [-g group]\n" << AD::endl;
                    exit(0);
                default: exit(1);
            }
        }

        if (optind < argc && target_user.empty()) {
            parse_combined(argv[optind]);
        }

        if (target_user.empty()) {
            target_user = "root";
        }
    }

    void execute() {
        struct passwd* pw = getpwnam(target_user.c_str());
        if (!pw) {
            AD::cerr << "Error: User '" << target_user << "' not found." << AD::endl;
            exit(EXIT_FAILURE);
        }

        if (!user::change(target_user, target_group, verbose)) {
            AD::cerr << "Error: Failed to switch to user " << target_user << AD::endl;
            exit(EXIT_FAILURE);
        }

        std::string shell_path = resolve_shell(pw->pw_shell);
        
        if (verbose) {
            AD::cout << "[INFO] Shell resolved: " << shell_path << AD::endl;
            AD::cout << "[INFO] Home directory: " << pw->pw_dir << AD::endl;
        }

        setenv("USER", target_user.c_str(), 1);
        setenv("LOGNAME", target_user.c_str(), 1);
        setenv("HOME", pw->pw_dir, 1);
        setenv("SHELL", shell_path.c_str(), 1);

        if (chdir(pw->pw_dir) != 0) {
            if (verbose) AD::cerr << "[WARN] Could not chdir to home." << AD::endl;
        }

        execl(shell_path.c_str(), 
              shell_path.c_str(), // argv[0]
              (char*)NULL);

        AD::cerr << "Fatal: execl failed: " << strerror(errno) << AD::endl;
        exit(EXIT_FAILURE);
    }
};

int main(int argc, char* argv[]) {
    UserSwitcher app;
    app.parse_args(argc, argv);
    app.execute();
    return 0;
}
