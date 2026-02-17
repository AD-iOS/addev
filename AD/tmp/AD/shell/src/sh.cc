/* shell , author: AD-dev */
#include <iostream>
#include <cstdlib>
#include <string>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <filesystem>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <sys/wait.h>

#define ad_all
#include <AD/ADlibc++.hpp>

#define ps1 "AD:shell:\ntest:>>>:~ "

namespace shell {

struct func {
    std::string name;
    std::vector<std::string> params;
    std::vector<std::string> body;
    
    func() = default;
    func(const std::string& n) : name(n) {}
    
    std::string replace_args(const std::string& cmd, 
                            const std::unordered_map<std::string, std::string>& args) const {
        std::string result = cmd;
        
        for (size_t i = 0; i < params.size(); i++) {
            std::string placeholder = "$" + std::to_string(i + 1);
            size_t pos = 0;
            while ((pos = result.find(placeholder, pos)) != std::string::npos) {
                if (i < args.size()) {
                    auto it = args.begin();
                    std::advance(it, i);
                    result.replace(pos, placeholder.length(), it->second);
                    pos += it->second.length();
                } else {
                    pos += placeholder.length();
                }
            }
        }
        
        for (const auto& [param, value] : args) {
            size_t pos = 0;
            std::string placeholder = "$" + param;
            while ((pos = result.find(placeholder, pos)) != std::string::npos) {
                result.replace(pos, placeholder.length(), value);
                pos += value.length();
            }
        }
        
        return result;
    }
};

class func_manager {
private:
    std::unordered_map<std::string, func> funcs;
    
    bool parse_func(const std::string& line, func& f) {
        size_t pos = line.find("func ");
        if (pos == std::string::npos) return false;
        
        size_t name_start = pos + 5;
        size_t name_end = line.find('(', name_start);
        if (name_end == std::string::npos) return false;
        
        f.name = line.substr(name_start, name_end - name_start);
        
        size_t param_start = name_end + 1;
        size_t param_end = line.find(')', param_start);
        if (param_end == std::string::npos) return false;
        
        std::string params_str = line.substr(param_start, param_end - param_start);
        if (!params_str.empty()) {
            std::stringstream param_ss(params_str);
            std::string param;
            while (std::getline(param_ss, param, ',')) {
                param.erase(0, param.find_first_not_of(" \t"));
                param.erase(param.find_last_not_of(" \t") + 1);
                if (!param.empty()) {
                    f.params.push_back(param);
                }
            }
        }
        
        size_t body_start = line.find('{', param_end);
        if (body_start == std::string::npos) return false;
        
        size_t body_end = line.rfind('}');
        if (body_end == std::string::npos) return false;
        
        std::string body_str = line.substr(body_start + 1, body_end - body_start - 1);
        
        std::stringstream body_ss(body_str);
        std::string cmd;
        while (std::getline(body_ss, cmd, ';')) {
            cmd.erase(0, cmd.find_first_not_of(" \t"));
            cmd.erase(cmd.find_last_not_of(" \t") + 1);
            if (!cmd.empty()) {
                f.body.push_back(cmd);
            }
        }
        
        return true;
    }
    
    int run_cmd(const std::string& cmd_line) {
        std::stringstream ss(cmd_line);
        std::string cmd;
        std::vector<std::string> args;
        
        ss >> cmd;
        std::string arg;
        while (ss >> std::quoted(arg)) {
            args.push_back(arg);
        }
        
        if (cmd == "echo") {
            for (size_t i = 0; i < args.size(); i++) {
                if (i > 0) AD::cout << " ";
                AD::cout << args[i];
            }
            AD::cout << AD::endl;
            return 0;
        }
        else if (cmd == "cd") {
            if (args.empty()) {
                AD::cerr << "cd: need path" << AD::endl;
                return 1;
            }
            if (chdir(args[0].c_str()) != 0) {
                AD::cerr << "cd: " << strerror(errno) << AD::endl;
                return 1;
            }
            return 0;
        }
        else if (cmd == "pwd") {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd))) {
                AD::cout << cwd << AD::endl;
                return 0;
            }
            return 1;
        }
        else if (cmd == "func") {
            AD::cerr << "cant define func in func" << AD::endl;
            return 1;
        }
        
        auto it = funcs.find(cmd);
        if (it != funcs.end()) {
            const func& f = it->second;
            if (args.size() != f.params.size()) {
                AD::cerr << cmd << ": need " << f.params.size() 
                         << " args, got " << args.size() << AD::endl;
                return 1;
            }
            
            std::unordered_map<std::string, std::string> arg_map;
            for (size_t i = 0; i < f.params.size(); i++) {
                arg_map[f.params[i]] = args[i];
                arg_map[std::to_string(i + 1)] = args[i];
            }
            
            int result = 0;
            for (const auto& body_cmd : f.body) {
                std::string expanded_cmd = f.replace_args(body_cmd, arg_map);
                result = run_cmd(expanded_cmd);
                if (result != 0) break;
            }
            return result;
        }
        
        pid_t pid = fork();
        if (pid == -1) {
            AD::cerr << "fork: " << strerror(errno) << AD::endl;
            return 1;
        }
        
        if (pid == 0) {
            std::vector<char*> argv;
            argv.push_back(const_cast<char*>(cmd.c_str()));
            for (const auto& arg : args) {
                argv.push_back(const_cast<char*>(arg.c_str()));
            }
            argv.push_back(nullptr);
            
            execvp(argv[0], argv.data());
            
            AD::cerr << cmd << ": not found" << AD::endl;
            _exit(127);
        }
        else {
            int status;
            waitpid(pid, &status, 0);
            
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            } else {
                return 1;
            }
        }
    }

public:
    bool define(const std::string& def) {
        func f;
        if (!parse_func(def, f)) {
            AD::cerr << "bad func" << AD::endl;
            return false;
        }
        
        funcs[f.name] = f;
        AD::cout << "func " << f.name << "() defined" << AD::endl;
        return true;
    }
    
    int call(const std::string& name, const std::vector<std::string>& args) {
        auto it = funcs.find(name);
        if (it == funcs.end()) {
            AD::cerr << name << ": no such func" << AD::endl;
            return 1;
        }
        
        const func& f = it->second;
        
        if (args.size() != f.params.size()) {
            AD::cerr << name << ": need " << f.params.size() 
                     << " args, got " << args.size() << AD::endl;
            return 1;
        }
        
        std::unordered_map<std::string, std::string> arg_map;
        for (size_t i = 0; i < f.params.size(); i++) {
            arg_map[f.params[i]] = args[i];
            arg_map[std::to_string(i + 1)] = args[i];
        }
        
        int result = 0;
        for (const auto& cmd : f.body) {
            std::string expanded_cmd = f.replace_args(cmd, arg_map);
            result = run_cmd(expanded_cmd);
            if (result != 0) break;
        }
        
        return result;
    }
    
    bool has(const std::string& name) const {
        return funcs.find(name) != funcs.end();
    }
    
    void list() const {
        if (funcs.empty()) {
            AD::cout << "no funcs" << AD::endl;
            return;
        }
        
        for (const auto& [name, f] : funcs) {
            AD::cout << "func " << name << "(";
            for (size_t i = 0; i < f.params.size(); i++) {
                if (i > 0) AD::cout << ", ";
                AD::cout << f.params[i];
            }
            AD::cout << ")" << AD::endl;
        }
    }
    
    bool remove(const std::string& name) {
        return funcs.erase(name) > 0;
    }
    
    int run(const std::string& cmd_line) {
        return run_cmd(cmd_line);
    }
};

func_manager fm;

std::unordered_map<std::string, std::function<int(const std::vector<std::string>&)>> builtins;

void add_builtin(const std::string& name, std::function<int(const std::vector<std::string>&)> f) {
    builtins[name] = f;
}

int echo(const std::vector<std::string>& args) {
    for (size_t i = 0; i < args.size(); i++) {
        if (i > 0) AD::cout << " ";
        AD::cout << args[i];
    }
    AD::cout << AD::endl;
    return 0;
}

int cd(const std::vector<std::string>& args) {
    if (args.empty()) {
        const char* home = getenv("HOME");
        if (home) {
            if (chdir(home) != 0) {
                AD::cerr << "cd: " << strerror(errno) << AD::endl;
                return 1;
            }
        }
        return 0;
    }
    
    if (chdir(args[0].c_str()) != 0) {
        AD::cerr << "cd: " << strerror(errno) << AD::endl;
        return 1;
    }
    return 0;
}

int pwd(const std::vector<std::string>& args) {
    (void)args;
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd))) {
        AD::cout << cwd << AD::endl;
        return 0;
    }
    return 1;
}

int help(const std::vector<std::string>& args) {
    (void)args;
    AD::cout << "commands:" << AD::endl;
    AD::cout << "  echo [args]" << AD::endl;
    AD::cout << "  cd [dir]" << AD::endl;
    AD::cout << "  pwd" << AD::endl;
    AD::cout << "  func name() { cmds; }" << AD::endl;
    AD::cout << "  funcs" << AD::endl;
    AD::cout << "  help" << AD::endl;
    AD::cout << "  exit" << AD::endl;
    AD::cout << "func ex:" << AD::endl;
    AD::cout << "  func hello() { echo hello; }" << AD::endl;
    AD::cout << "  func greet(name) { echo \"hi $name\"; }" << AD::endl;
    AD::cout << "  func add(a, b) { echo $(($a + $b)); }" << AD::endl;
    
    fm.list();
    return 0;
}

int funcs(const std::vector<std::string>& args) {
    (void)args;
    fm.list();
    return 0;
}

int exit_cmd(const std::vector<std::string>& args) {
    if (args.empty()) {
        return 0;
    }
    try {
        return std::stoi(args[0]);
    } catch (...) {
        return 0;
    }
}

void setup() {
    add_builtin("echo", echo);
    add_builtin("cd", cd);
    add_builtin("pwd", pwd);
    add_builtin("help", help);
    add_builtin("funcs", funcs);
    add_builtin("exit", exit_cmd);
}

int run(const std::string& cmd_line) {
    std::stringstream ss(cmd_line);
    std::string cmd;
    std::vector<std::string> args;
    
    ss >> cmd;
    
    if (cmd == "func") {
        std::string def;
        std::string rest;
        while (std::getline(ss, rest)) {
            def += " " + rest;
        }
        def = "func " + def;
        
        return fm.define(def) ? 0 : 1;
    }
    
    std::string arg;
    while (ss >> std::quoted(arg)) {
        args.push_back(arg);
    }
    
    auto it = builtins.find(cmd);
    if (it != builtins.end()) {
        return it->second(args);
    }
    
    if (fm.has(cmd)) {
        return fm.call(cmd, args);
    }
    
    return fm.run(cmd_line);
}

} // namespace shell

int main() {
    using namespace shell;
    
    setup();
    
    // AD::cout << "shell v1" << AD::endl;
    
    bool running = true;
    
    while (running) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd))) {
            AD::cout << ps1 << " $ ";
        } else {
            AD::cout << "$ ";
        }
        AD::cout.flush();
        
        std::string line;
        if (!std::getline(std::cin, line)) {
            break;
        }
        
        if (line.empty()) {
            continue;
        }
        
        if (line.find("exit") == 0) {
            std::stringstream ss(line);
            std::string cmd;
            std::vector<std::string> args;
            ss >> cmd;
            std::string arg;
            while (ss >> arg) args.push_back(arg);
            
            int code = exit_cmd(args);
            running = false;
            return code;
        }
        
        int result = run(line);
        (void)result;
    }
    
    return 0;
}