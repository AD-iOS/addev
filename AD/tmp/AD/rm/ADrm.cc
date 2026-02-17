/*
 *
 * ADrm.cc
 * Created by AD on 9/12/25
 * Copyright (c) 2025 AD All rights reserved.
 *
**/

#include "include/_rm_.hpp"
#include "include/AD_output.hpp"
// #include "AD_null.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <cstring>

using namespace AD;

void show_help() {
    cout << "adrm -r       # 遞歸刪除目錄" << endl;
    cout << "adrm dir|-dir # 遞歸刪除目錄,變體命令" << endl;
    cout << "adrm -f       # 強制刪除" << endl;
    cout << "adrm -rf      # 強制遞歸刪除目錄包括文件" << endl;
    cout << "adrm -safe    # 安全刪除" << endl;
}

void show_version() {
    cout << "adrm v0.1" << endl;
}

void help(char *arg) {
    if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
        show_help();
        exit(0);
    }
}

void version(char *arg) {
    if (strcmp(arg, "-v") == 0 || strcmp(arg, "--version") == 0) {
        show_version();
        exit(0);
    }
}

int rm(int argc, char *argv[]) {
    if (argc < 2) {
        show_help();
        return 1;
    }

    std::string command = argv[1];
    int successCount = 0;

    help(argv[1]);
    version(argv[1]);

    if (argc < 3) {
        std::cerr << "[Error]: No target is specified to be deleted." << std::endl;
        return 1;
    }

    for (int i = 2; i < argc; ++i) {
        fs::path targetPath = argv[i];
        int result = 0;

        help(argv[i]);
        version(argv[i]);

        if (command == "-r" || command == "r") {
            result = _AD_rmdir(targetPath);
        }

        else if (command == "-rf") {
            result = _AD_rmdirf(targetPath);
        }

        else if (command == "rf") {
            result = _AD_rmdirf(targetPath);
        }

        else if (command == "-dir") {
            result = _AD_rmdir(targetPath);
        }

        else if (command == "dir") {
            result = _AD_rmdir(targetPath);
        }

        else if (command == "-safe") {
            result = _AD_rm_safe(targetPath);
        }

        else if (command == "-f" || command == "f") {
            result = _AD_rm(targetPath);
        }

        else {

            targetPath = argv[1];

            i = 1;

            result = _AD_rm_safe(targetPath);

            for (int j = i + 1; j < argc; ++j) {
                targetPath = argv[j];
                result = _AD_rm_safe(targetPath);
                if (result > 0) {
                    successCount++;
                }
            }
            break;
        }

        if (result > 0) {
            successCount++;
        }
    }

    std::cout << "Successfully processed " << successCount << " item(s).\n";
    return (successCount == argc - 2) ? 0 : 1;
}

int main(int argc, char *argv[]) {

    if (argc == 1) {
        show_help();
        return 0;
    }

    help(argv[1]);
    version(argv[1]);

    return rm(argc, argv);
}