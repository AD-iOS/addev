/*
 *
 * adcreate.cc
 * Created by AD on 9/12/25
 * Copyright (c) 2025 AD All rights reserved.
 *
 */
/*
 * Add admk by AD on 25/1/26
 * by AD on 25/1/26 fix `main()` function use the bug of `admk()` function
 * fix `admk()` function by AD on 25/1/26
**/

#include "include/_file.hpp"
#include "include/AD_output.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <cstring>

using namespace AD;

void adcreate_show_help() {
    cout << "usage: adcreate [option]" << endl;
    cout << "adcreate touch     # 創建文件" << endl;
    cout << "adcreate mkdir     # 創建文檔夾" << endl;
    cout << "adcreate --help    # 顯示幫助信息" << endl;
    cout << "adcreate --version # 顯示版本號" << endl;
}

void admk_show_help() {
    cout << "usage: admk [option]" << endl
         << "  admk file <name>  # make file" << endl
         << "  admk dir <name>   # make dir" << endl
         << "  admk --version    # show version" << endl
         << "  admk --help       # show help" << endl;
}

void show_version() {
    cout << "adcreate v0.2" << endl;
}

bool is_help_option(const char* arg) {
    return strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0;
}

bool is_version_option(const char* arg) {
    return strcmp(arg, "-v") == 0 || strcmp(arg, "--version") == 0;
}

int admk(int argc, char *argv[]) {
    if (argc < 2) {
        admk_show_help();
        return 1;
    }

    if (is_help_option(argv[1])) {
        admk_show_help();
        return 0;
    }
    
    if (is_version_option(argv[1])) {
        show_version();
        return 0;
    }

    std::string command = argv[1];
    int successCount = 0;

    if (argc < 3) {
        cerr << "[Error]: No name is specified." << endl;
        return 1;
    }

    for (int i = 2; i < argc; ++i) {
        fs::path targetPath = argv[i];
        int result = 0;

        // 檢查幫助和版本選項
        if (is_help_option(argv[i])) {
            admk_show_help();
            continue;
        }
        
        if (is_version_option(argv[i])) {
            show_version();
            continue;
        }

        if (command == "--file" || command == "file") {
            result = _AD_touch(targetPath);
        }
        else if (command == "dir" || command == "--dir") {
            result = _AD_mkdir(targetPath);
        }

        if (result > 0) {
            successCount++;
        }
    }

    cout << "Successful creation " << successCount << " item(s).\n";
    return (successCount == argc - 2) ? 0 : 1;
}

int adcreate(int argc, char *argv[]) {
    if (argc < 2) {
        adcreate_show_help();
        return 1;
    }

    if (is_help_option(argv[1])) {
        adcreate_show_help();
        return 0;
    }
    
    if (is_version_option(argv[1])) {
        show_version();
        return 0;
    }

    std::string command = argv[1];
    int successCount = 0;

    if (argc < 3) {
        cerr << "[Error]: No name is specified." << endl;
        return 1;
    }

    for (int i = 2; i < argc; ++i) {
        fs::path targetPath = argv[i];
        int result = 0;
        if (is_help_option(argv[i])) {
            adcreate_show_help();
            continue;
        }
        
        if (is_version_option(argv[i])) {
            show_version();
            continue;
        }

        if (command == "-touch" || command == "touch") {
            result = _AD_touch(targetPath);
        }
        else if (command == "-mkdir" || command == "mkdir") {
            result = _AD_mkdir(targetPath);
        }

        if (result > 0) {
            successCount++;
        }
    }

    cout << "Successful creation " << successCount << " item(s).\n";
    return (successCount == argc - 2) ? 0 : 1;
}

int main(int argc, char *argv[]) {
    if (argc < 1) return 1;

    if (strstr(argv[0], "admk") != nullptr) {
        return admk(argc, argv);
    }
    else {
        return adcreate(argc, argv);
    }
}