#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <getopt.h>
#include <algorithm>

class UserSwitcher {
private:
    std::string target_user;
    std::string target_group;
    bool verbose = false;
    
    // 解析 uid 或 用戶名
    uid_t parse_uid(const std::string& input) {
        // 如果都是數字，直接轉換
        if (std::all_of(input.begin(), input.end(), ::isdigit)) {
            return std::stoi(input);
        }
        
        // 否則查找用戶名
        struct passwd* pw = getpwnam(input.c_str());
        if (pw == nullptr) {
            std::cerr << "Error: User '" << input << "' not found" << std::endl;
            exit(EXIT_FAILURE);
        }
        return pw->pw_uid;
    }
    
    // 解析 gid 或 組名
    gid_t parse_gid(const std::string& input) {
        // 如果都是數字，直接轉換
        if (std::all_of(input.begin(), input.end(), ::isdigit)) {
            return std::stoi(input);
        }
        
        // 否則查找組名
        struct group* gr = getgrnam(input.c_str());
        if (gr == nullptr) {
            std::cerr << "Error: Group '" << input << "' not found" << std::endl;
            exit(EXIT_FAILURE);
        }
        return gr->gr_gid;
    }
    
    // 獲取用戶名
    std::string get_username(uid_t uid) {
        struct passwd* pw = getpwuid(uid);
        if (pw != nullptr) {
            return pw->pw_name;
        }
        return std::to_string(uid);
    }
    
    // 獲取組名
    std::string get_groupname(gid_t gid) {
        struct group* gr = getgrgid(gid);
        if (gr != nullptr) {
            return gr->gr_name;
        }
        return std::to_string(gid);
    }
    
    // 解析組合參數 user:group
    void parse_combined(const std::string& combined) {
        size_t colon_pos = combined.find(':');
        if (colon_pos != std::string::npos) {
            target_user = combined.substr(0, colon_pos);
            target_group = combined.substr(colon_pos + 1);
        } else {
            // 如果沒有冒號，只設置用戶
            target_user = combined;
        }
    }

public:
    void parse_arguments(int argc, char* argv[]) {
        static struct option long_options[] = {
            {"user", required_argument, 0, 'u'},
            {"group", required_argument, 0, 'g'},
            {"user-group", required_argument, 0, 'U'},
            {"verbose", no_argument, 0, 'v'},
            {"help", no_argument, 0, 'h'},
            {0, 0, 0, 0}
        };
        
        int option_index = 0;
        int c;
        
        while ((c = getopt_long(argc, argv, "u:g:U:vh", long_options, &option_index)) != -1) {
            switch (c) {
                case 'u':
                    target_user = optarg;
                    break;
                case 'g':
                    target_group = optarg;
                    break;
                case 'U':  // -U user:group
                    parse_combined(optarg);
                    break;
                case 'v':
                    verbose = true;
                    break;
                case 'h':
                    print_help();
                    exit(EXIT_SUCCESS);
                default:
                    print_help();
                    exit(EXIT_FAILURE);
            }
        }
        
        // 檢查是否有非選項參數 (舊式語法)
        if (optind < argc) {
            parse_combined(argv[optind]);
        }
        
        // 如果沒有指定用戶，使用當前用戶
        if (target_user.empty()) {
            struct passwd* pw = getpwuid(getuid());
            if (pw != nullptr) {
                target_user = pw->pw_name;
            } else {
                target_user = std::to_string(getuid());
            }
        }
        
        // 如果沒有指定組，使用用戶的默認組
        if (target_group.empty() && !target_user.empty()) {
            struct passwd* pw = getpwnam(target_user.c_str());
            if (pw != nullptr) {
                struct group* gr = getgrgid(pw->pw_gid);
                if (gr != nullptr) {
                    target_group = gr->gr_name;
                } else {
                    target_group = std::to_string(pw->pw_gid);
                }
            }
        }
    }
    
    void print_help() {
        std::cout << "Usage: switch_user [OPTIONS]\n";
        std::cout << "Switch to another user and group (no password required)\n\n";
        std::cout << "Options:\n";
        std::cout << "  -u, --user USER     Switch to this user (name or UID)\n";
        std::cout << "  -g, --group GROUP   Switch to this group (name or GID)\n";
        std::cout << "  -U, --user-group USER:GROUP  Combined user and group\n";
        std::cout << "  -v, --verbose       Show detailed information\n";
        std::cout << "  -h, --help          Show this help message\n\n";
        std::cout << "Examples:\n";
        std::cout << "  switch_user --user nobody --group nogroup\n";
        std::cout << "  switch_user -u root -g root\n";
        std::cout << "  switch_user -ug nobody:nogroup\n";
        std::cout << "  switch_user -U daemon:daemon\n";
        std::cout << "  switch_user daemon:daemon  (old style)\n";
    }
    
    bool switch_to_user() {
        uid_t uid = parse_uid(target_user);
        gid_t gid = parse_gid(target_group);
        
        if (verbose) {
            std::cout << "Current UID: " << getuid() << " (" << get_username(getuid()) << ")\n";
            std::cout << "Current GID: " << getgid() << " (" << get_groupname(getgid()) << ")\n";
            std::cout << "Target UID: " << uid << " (" << target_user << ")\n";
            std::cout << "Target GID: " << gid << " (" << target_group << ")\n";
        }
        
        // 切換組
        if (setgid(gid) != 0) {
            std::cerr << "Error: Failed to set GID to " << gid 
                      << " (" << strerror(errno) << ")" << std::endl;
            return false;
        }
        
        // 初始化附加組
        struct passwd* pw = getpwuid(uid);
        if (pw != nullptr) {
            if (initgroups(pw->pw_name, gid) != 0) {
                std::cerr << "Warning: Failed to init groups for " << pw->pw_name
                          << " (" << strerror(errno) << ")" << std::endl;
            }
        }
        
        // 切換用戶
        if (setuid(uid) != 0) {
            std::cerr << "Error: Failed to set UID to " << uid 
                      << " (" << strerror(errno) << ")" << std::endl;
            return false;
        }
        
        if (verbose) {
            std::cout << "Successfully switched to user " << target_user 
                      << " and group " << target_group << std::endl;
        }
        
        return true;
    }
    
    void execute_shell() {
        // 獲取用戶的 shell
        struct passwd* pw = getpwnam(target_user.c_str());
        const char* shell_path = "/var/jb/usr/bin/bash";
        
        if (pw != nullptr && pw->pw_shell != nullptr && strlen(pw->pw_shell) > 0) {
            shell_path = pw->pw_shell;
        }
        
        if (verbose) {
            std::cout << "Starting shell: " << shell_path << std::endl;
        }
        
        // 設置環境變數
        setenv("USER", target_user.c_str(), 1);
        setenv("LOGNAME", target_user.c_str(), 1);
        
        if (pw != nullptr) {
            setenv("HOME", pw->pw_dir, 1);
            if (chdir(pw->pw_dir) != 0) {
                // 如果切換目錄失敗，繼續執行
                if (verbose) {
                    std::cerr << "Warning: Cannot change to home directory: " 
                              << strerror(errno) << std::endl;
                }
            }
        }
        
        // 執行 shell
        execl(shell_path, shell_path, nullptr);
        
        // 如果 execl 失敗
        std::cerr << "Error: Failed to execute shell: " << strerror(errno) << std::endl;
    }
};

int main(int argc, char* argv[]) {
    UserSwitcher switcher;
    
    // 解析參數
    switcher.parse_arguments(argc, argv);
    
    // 切換用戶/組
    if (!switcher.switch_to_user()) {
        return EXIT_FAILURE;
    }
    
    // 執行 shell
    switcher.execute_shell();
    
    return EXIT_FAILURE;  // 只有 execl 失敗才會到這裡
}