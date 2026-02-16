#include "ADlibc++.hpp"
#include "Config.hpp"
#include "Targets.hpp"

/* xcode++ -e entitlements.plist -I ./ -I ./include main.cc -o xcode_new */

// 顯示幫助信息
void show_help() {
    AD::cout << "iOS Xcode Build Tool v2.0 (Refactored)" << AD::endl;
    AD::cout << "Usage: xcode [options] <source_file>" << AD::endl;
    AD::cout << "Options:" << AD::endl;
    AD::cout << "  -o <name>       Target binary name" << AD::endl;
    AD::cout << "  -cfg <path>     Path to config file (default: xcode.ini)" << AD::endl;
    AD::cout << "  -sdk <path>     Override SDK path" << AD::endl;
    AD::cout << "  -arch <arch>    Override architecture (arm64, etc.)" << AD::endl;
}

int main(int argc, char* argv[]) {
    // 全局計時器
    AD::stopwatch totalTimer("Total Build Process");

    if (argc < 2) {
        show_help();
        return 0;
    }

    Config config;
    std::string sourceFile;
    std::string configPath = "xcode.ini"; // 默認配置路徑

    // 1. 第一遍掃描:尋找配置文件路徑
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-cfg" && i + 1 < argc) {
            configPath = argv[++i];
        }
    }

    // 2. 加載配置
    config.load(configPath);

    // 3. 第二遍掃描：處理命令行覆蓋
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-o" && i + 1 < argc) config.set("TARGET_NAME", argv[++i]);
        else if (arg == "-arch" && i + 1 < argc) config.set("TARGET_ARCH", argv[++i]);
        else if (arg == "-sdk" && i + 1 < argc) config.set("SDK_PATH", argv[++i]);
        else if (arg == "-cfg") { i++; continue; } 
        else if (!AD::str.starts_with(arg, "-")) {
            sourceFile = arg;
        }
    }

    if (sourceFile.empty()) {
        AD::cout << "[Error] No source file specified." << AD::endl;
        return 1;
    }

    if (!AD::fs::readable(sourceFile)) {
        AD::cout << "[Error] Source file not found: " << sourceFile << AD::endl;
        return 1;
    }

    // 4. 工廠模式創建構建器
    Builder* builder = nullptr;
    if (AD::str.ends_with(sourceFile, ".swift")) {
        builder = new SwiftBuilder(config, sourceFile);
    } else {
        // C, C++, ObjC, ObjC++ 都走這裡
        builder = new ClangBuilder(config, sourceFile);
    }

    // 5. 運行
    if (builder) {
        builder->run();
        delete builder;
    }

    return 0;
}