#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "ADlibc++.hpp"
#include <map>
#include <string>

class Config {
private:
    std::map<std::string, std::string> settings;

public:
    Config() {
        // 1. 設置默認值 (防止配置文件缺失時崩潰)
        settings["SDK_PATH"] = "/var/jb/theos/sdks/iPhoneOS.sdk";
        settings["TARGET_ARCH"] = "arm64";
        settings["MIN_OS_VERSION"] = "14.0";
        settings["SWIFT_COMPILER"] = "swiftc";
        settings["OBJC_COMPILER"] = "clang";
        settings["SIGNING_TOOL"] = "ldid";
        settings["ENABLE_SIGNING"] = "yes";
        settings["ENABLE_SYMLINK"] = "yes";
        settings["BUILD_DIR"] = "build";
        settings["OPTIMIZATION_LEVEL"] = "-O";
    }

    // 加載配置文件
    void load(const std::string& path) {
        if (!AD::fs::readable(path)) {
            AD::cout << "[Config] Info: Config file not found at " << path << ", using defaults." << AD::endl;
            return;
        }

        auto lines = AD::fs::read_lines(path);
        for (const auto& line : lines) {
            std::string clean = AD::str.trim(line);
            // 跳過空行和註釋
            if (clean.empty() || clean[0] == '#' || clean[0] == ';') continue;

            auto parts = AD::str.split(clean, '=');
            if (parts.size() >= 2) {
                std::string key = AD::str.trim(parts[0]);
                std::string value = AD::str.trim(parts[1]);
                settings[key] = value;
            }
        }
        AD::cout << "[Config] Loaded configuration from " << path << AD::endl;
    }

    // 用於命令行參數覆蓋配置
    void set(const std::string& key, const std::string& value) {
        if (!value.empty()) settings[key] = value;
    }

    std::string get(const std::string& key) const {
        if (settings.find(key) != settings.end()) {
            return settings.at(key);
        }
        return "";
    }

    bool getBool(const std::string& key) const {
        std::string val = get(key);
        return (val == "yes" || val == "true" || val == "1" || val == "on");
    }

    // 獲取列表類型的配置 (假設用空格分隔)
    std::vector<std::string> getList(const std::string& key) const {
        return AD::str.split(get(key), ' ');
    }
};

#endif