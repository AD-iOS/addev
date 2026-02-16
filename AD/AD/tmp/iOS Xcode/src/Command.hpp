#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <vector>
#include <string>
#include <sstream>

class Command {
private:
    std::string tool;
    std::vector<std::string> args;

public:
    explicit Command(const std::string& toolName) : tool(toolName) {}

    // 添加普通參數
    Command& add(const std::string& arg) {
        if (!arg.empty()) args.push_back(arg);
        return *this;
    }

    // 添加標誌和值，如 -o output
    Command& addFlag(const std::string& flag, const std::string& value) {
        if (!value.empty()) {
            args.push_back(flag);
            args.push_back(value);
        }
        return *this;
    }

    // 添加前綴式參數，如 -I/path/to/header (無空格)
    Command& addPrefix(const std::string& prefix, const std::string& value) {
        if (!value.empty()) {
            args.push_back(prefix + value);
        }
        return *this;
    }

    // 獲取最終命令字符串
    std::string toString() const {
        std::stringstream ss;
        ss << tool;
        for (const auto& arg : args) {
            ss << " " << arg;
        }
        return ss.str();
    }
};

#endif