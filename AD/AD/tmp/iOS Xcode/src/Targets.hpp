#ifndef TARGETS_HPP
#define TARGETS_HPP

#include "Builder.hpp"

// ==========================================
// Swift Builder
// ==========================================
class SwiftBuilder : public Builder {
public:
    using Builder::Builder;

    bool compile() override {
        Command cmd(config.get("SWIFT_COMPILER"));

        // 1. 輸入與輸出
        cmd.add(sourceFile);
        cmd.addFlag("-o", outputPath);

        // 2. SDK 與 目標架構
        cmd.addFlag("-sdk", config.get("SDK_PATH"));
        
        // Swift 需要 target triple: e.g. arm64-apple-ios14.0
        std::string triple = config.get("TARGET_ARCH") + "-apple-ios" + config.get("MIN_OS_VERSION");
        cmd.addFlag("-target", triple);

        // 3. 模塊名稱
        cmd.addFlag("-module-name", outputName);

        // 4. 路徑搜索 (從 Config 讀取列表)
        for (const auto& path : config.getList("INCLUDE_PATHS")) cmd.addPrefix("-I", path);
        for (const auto& path : config.getList("FRAMEWORK_PATHS")) cmd.addPrefix("-F", path);
        for (const auto& path : config.getList("LIBRARY_PATHS")) cmd.addPrefix("-L", path);
        
        // 5. 鏈接庫
        for (const auto& lib : config.getList("LINK_LIBRARIES")) cmd.addPrefix("-l", lib);

        // 6. 優化選項
        cmd.add(config.get("OPTIMIZATION_LEVEL"));
        
        // 7. 抑制警告 (你提到的痛點)
        if (config.getBool("SUPPRESS_WARNINGS")) {
            cmd.add("-suppress-warnings"); 
        }

        AD::cout << "[Swift] Compiling " << outputName << " (" << config.get("TARGET_ARCH") << ")..." << AD::endl;
        
        // 使用 AD::sys::bash 執行
        int res = AD::sys::bash(cmd.toString().c_str());
        return (res == 0);
    }
};

// ==========================================
// Clang Builder (C/C++/ObjC/ObjC++)
// ==========================================
class ClangBuilder : public Builder {
public:
    using Builder::Builder;

protected:
    bool compile() override {
        // 自動檢測是否需要 C++ 編譯器
        bool isCpp = AD::str.ends_with(sourceFile, ".cpp") || 
                     AD::str.ends_with(sourceFile, ".cc") || 
                     AD::str.ends_with(sourceFile, ".mm");
        
        std::string compiler = isCpp ? "clang++" : config.get("OBJC_COMPILER");

        Command cmd(compiler);

        // 1. 基礎參數
        cmd.add(sourceFile);
        cmd.addFlag("-o", outputPath);
        cmd.addFlag("-isysroot", config.get("SDK_PATH"));
        cmd.addFlag("-arch", config.get("TARGET_ARCH"));
        
        // 2. 版本控制 (Clang 語法: -miphoneos-version-min=14.0)
        cmd.addFlag("-miphoneos-version-min=", config.get("MIN_OS_VERSION"));

        // 3. ARC 支持
        if (config.getBool("ENABLE_ARC")) {
            cmd.add("-fobjc-arc");
        }

        // 4. 標準 (C++17 等)
        if (isCpp) {
            std::string stdVer = config.get("CPP_STD");
            if (!stdVer.empty()) cmd.addFlag("-std=", stdVer);
        }

        // 5. 路徑與庫
        for (const auto& path : config.getList("INCLUDE_PATHS")) cmd.addPrefix("-I", path);
        for (const auto& path : config.getList("FRAMEWORK_PATHS")) cmd.addPrefix("-F", path);
        
        // 6. 優化
        cmd.add(config.get("OPTIMIZATION_LEVEL"));

        AD::cout << "[" << compiler << "] Compiling " << outputName << "..." << AD::endl;
        
        int res = AD::sys::bash(cmd.toString().c_str());
        return (res == 0);
    }
};

#endif
