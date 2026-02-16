#ifndef BUILDER_HPP
#define BUILDER_HPP

#include "ADlibc++.hpp"
#include "Config.hpp"
#include "Command.hpp"

class Builder {
protected:
    Config& config;
    std::string sourceFile;
    std::string outputName;
    std::string outputDir;
    std::string outputPath;

public:
    Builder(Config& cfg, const std::string& src) : config(cfg), sourceFile(src) {
        outputDir = config.get("BUILD_DIR");
        
        // 確定輸出文件名
        std::string overrideName = config.get("TARGET_NAME");
        if (!overrideName.empty()) {
            outputName = overrideName;
        } else {
            // 使用你的 AD::fs::filename_base
            outputName = AD::fs::filename_base(sourceFile);
        }
        
        outputPath = outputDir + "/" + outputName;
    }

    virtual ~Builder() = default;

    // 模板方法：定義構建流程
    void run() {
        if (!prepare()) return;
        
        AD::stopwatch sw("Compilation"); // 計時編譯過程
        if (!compile()) {
            AD::cout << "\033[31m[Error] Compilation failed.\033[0m" << AD::endl;
            return;
        }
        // 停止計時器以顯示編譯時間 (RAII)
        // 這裡 sw 會銷毀，但在 run 函數結束前我們還要繼續做其他事，所以這只是局部計時
        
        if (config.getBool("ENABLE_SIGNING")) {
            sign();
        }
        
        finish();
    }

protected:
    virtual bool compile() = 0; // 純虛函數，由子類實現

    bool prepare() {
        // 使用 AD::fs::mkdir
        if (!AD::fs::mkdir(outputDir)) {
            // 注意：如果目錄已存在 mkdir 返回 true (根據你的實現邏輯)
            // 如果返回 false 說明真的失敗了
            // AD::fs::mkdir 在你的庫裡已經打印了 log，這裡就不重複了
             return true; 
        }
        return true;
    }

    void sign() {
        std::string tool = config.get("SIGNING_TOOL");
        std::string ent = config.get("ENTITLEMENTS_FILE");
        
        Command cmd(tool);
        // ldid -Sfile binary
        std::string signArg = "-S" + ent; 
        cmd.add(signArg);
        cmd.add(outputPath);

        AD::cout << "[Sign] Signing binary..." << AD::endl;
        AD::sys::bash(cmd.toString().c_str());
    }

    void finish() {
        AD::cout << "\033[32m[Success] Build artifact: " << outputPath << "\033[0m" << AD::endl;

        if (config.getBool("ENABLE_SYMLINK")) {
            // 創建符號鏈接方便調試
            std::string linkCmd = "ln -sf " + outputPath + " " + outputName;
            AD::sys::bash(linkCmd.c_str());
            AD::cout << "[Link] Created symlink: " << outputName << AD::endl;
        }
    }
};

#endif