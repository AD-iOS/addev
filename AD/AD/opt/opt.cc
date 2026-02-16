// opt.cc
// c++ -std=c++17 -o opt opt.cc -I./ -I../AD/ -lc++ -lc && ldid -Sentitlements.plist -M -Hsha256 opt
#include "../AD/ADlibc++.hpp"
#include <vector>
#include <string>
#include <cstring>

namespace fs = std::filesystem;

// 定義顏色常量
const char* RED = "\033[0;31m";
const char* GREEN = "\033[32m";
const char* YELLOW = "\033[1;33m";
const char* BLUE = "\033[0;34m";
const char* NC = "\033[0m";
const char* BRIGHT_YELLOW = "\033[93m";
const char* BRIGHT_BLUE = "\033[94m";

class OptTool {
private:
    std::string base_path;
    
    bool detect_base_path() {
        if (fs::exists("/var/jb/opt")) {
            base_path = "/var/jb/opt";
            return true;
        } else if (fs::exists("/opt")) {
            base_path = "/opt";
            return true;
        }
        return false;
    }
    
    void print_error(const std::string& msg) {
        AD::cout << RED << "[Error]" << NC << " " << YELLOW << msg << NC << AD::endl;
    }
    
    void print_success(const std::string& msg) {
        AD::cout << GREEN << "[Yes]" << NC << " " << msg << AD::endl;
    }
    
    void print_info(const std::string& msg) {
        AD::cout << BLUE << msg << NC << AD::endl;
    }
    
    void print_warning(const std::string& msg) {
        AD::cout << BRIGHT_YELLOW << msg << NC << AD::endl;
    }

public:
    OptTool() {
        if (!detect_base_path()) {
            print_warning("未找到標準 opt 目錄，將使用當前目錄");
            base_path = ".";
        }
    }
    
    void show_help() {
        if (fs::exists("help.txt")) {
            AD::cout << AD::fs::read_all("help.txt") << AD::endl;
        } else {
            std::string help_content = 
                "Optional:\n"
                "  rootless:\n"
                "    opt pack xxx.pkg /var/jb/opt/xxx\n"
                "    opt pack xxx.pkg /var/jb/opt/xxx -o /path/to/...\n"
                "    opt unpkg xxx.pkg\n"
                "    opt unpkg xxx.pkg -o /path/to/...\n"
                "  roothide|rootful:\n"
                "    opt pack xxx.pkg /opt/xxx\n"
                "    opt pack xxx.pkg /opt/xxx -o /path/to/...\n"
                "    opt unpkg xxx.pkg\n"
                "    opt unpkg xxx.pkg -o /path/to/...\n";
            AD::cout << help_content << AD::endl;
        }
    }
    
    void show_version() {
        AD::cout << BRIGHT_YELLOW << " v0.2 (C++ Rewrite)" << NC << AD::endl;
    }
    
    bool pack(const std::string& pkg_name, const std::string& source_path, const std::string& output_dir = "") {
        print_info("打包中...");
        
        // 檢查源路徑是否存在
        if (!fs::exists(source_path)) {
            print_error("源路徑不存在: " + source_path);
            return false;
        }
        
        // 構建完整命令
        std::string cmd = "xar -cvf ";
        
        // 確定輸出路徑
        std::string final_pkg_path = pkg_name;
        if (!output_dir.empty()) {
            if (!fs::exists(output_dir)) {
                AD::fs::mkdir(output_dir);
            }
            final_pkg_path = output_dir + "/" + pkg_name;
        }
        
        cmd += final_pkg_path + " " + source_path;
        
        print_info("執行命令: " + cmd);
        
        // 使用系統調用
        int result = ad_bash_system(cmd);
        
        if (result == 0) {
            print_success("打包完成: " + final_pkg_path);
            return true;
        } else {
            print_error("打包失敗");
            return false;
        }
    }
    
    bool unpack(const std::string& pkg_name, const std::string& output_dir = "") {
        print_info("解包中...");
        
        // 檢查 pkg 文件是否存在
        if (!fs::exists(pkg_name)) {
            print_error("包文件不存在: " + pkg_name);
            return false;
        }
        
        // 構建命令
        std::string cmd = "xar -xvf " + pkg_name;
        
        // 如果有輸出目錄，創建並解壓到該目錄
        if (!output_dir.empty()) {
            if (!fs::exists(output_dir)) {
                AD::fs::mkdir(output_dir);
            }
            cmd += " -C " + output_dir;
        }
        
        print_info("執行命令: " + cmd);
        
        int result = ad_bash_system(cmd);
        
        if (result == 0) {
            print_success("解包完成");
            return true;
        } else {
            print_error("解包失敗");
            return false;
        }
    }
    
    void list_contents() {
        print_info("opt 目錄內容 (" + base_path + "):");
        
        try {
            for (const auto& entry : fs::directory_iterator(base_path)) {
                std::string type = fs::is_directory(entry.path()) ? "[DIR]" : "[FILE]";
                AD::cout << "  " << type << " " << entry.path().filename().string() << AD::endl;
            }
        } catch (const std::exception& e) {
            print_error("無法讀取目錄: " + std::string(e.what()));
        }
    }
    
    bool validate_pkg_name(const std::string& pkg_name) {
        if (pkg_name.empty()) {
            print_error("包名不能為空");
            return false;
        }
        
        // 檢查是否以 .pkg 結尾
        if (!AD::str.ends_with(pkg_name, ".pkg")) {
            print_warning("包名建議以 .pkg 結尾");
        }
        
        return true;
    }
    
    void run(int argc, char* argv[]) {
        if (argc < 2) {
            show_help();
            return;
        }
        
        std::string command = argv[1];
        
        if (command == "pack") {
            if (argc < 4) {
                print_error("用法: opt pack <包名> <源路徑> [-o 輸出目錄]");
                return;
            }
            
            std::string pkg_name = argv[2];
            std::string source_path = argv[3];
            std::string output_dir = "";
            
            // 解析可選參數
            for (int i = 4; i < argc; i++) {
                if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
                    output_dir = argv[i + 1];
                    i++; // 跳過下一個參數
                }
            }
            
            if (!validate_pkg_name(pkg_name)) {
                return;
            }
            
            pack(pkg_name, source_path, output_dir);
            
        } else if (command == "unpkg") {
            if (argc < 3) {
                print_error("用法: opt unpkg <包名> [-o 輸出目錄]");
                return;
            }
            
            std::string pkg_name = argv[2];
            std::string output_dir = "";
            
            // 解析可選參數
            for (int i = 3; i < argc; i++) {
                if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
                    output_dir = argv[i + 1];
                    i++; // 跳過下一個參數
                }
            }
            
            if (!validate_pkg_name(pkg_name)) {
                return;
            }
            
            unpack(pkg_name, output_dir);
            
        } else if (command == "list" || command == "ls") {
            list_contents();
            
        } else if (command == "-h" || command == "--help") {
            show_help();
            
        } else if (command == "-v" || command == "--version") {
            show_version();
            
        } else {
            print_error("未知命令: " + command);
            AD::cout << "使用 'opt -h' 查看幫助" << AD::endl;
        }
    }
};

int main(int argc, char* argv[]) {
    AD::stopwatch sw("opt tool", false);
    
    try {
        OptTool tool;
        tool.run(argc, argv);
        
        double elapsed = sw.elapsed_ms();
        AD::cout << AD::endl << "執行時間: " << elapsed << " ms" << AD::endl;
        
        return 0;
    } catch (const std::exception& e) {
        AD::cerr << RED << "[致命錯誤]" << NC << " " << e.what() << AD::endl;
        return 1;
    }
}