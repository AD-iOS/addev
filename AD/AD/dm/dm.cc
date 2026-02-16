/*    AD-DEV Public General License
 *       Version 1.0.0, December 2025
 *
 *  Copyright (c) 2025 AD-iOS (1107154510@qq.com) All rights reserved.
 *
 *  (Note: AD, AD-dev, and AD-iOS refer to the same person. Email: 1107154510@qq.com)
 *
 *  Hereinafter, the AD-DEV Public General License is referred to as this Agreement or this License. The original source code, executable binaries, and related documentation are collectively referred to as the Software. Use for profit, including sales, leasing, advertising support, etc., is referred to as Commercial Use. Works modified or extended based on the Software are referred to as Derivative Products.
 *
 *  We hereby grant you the following rights, subject to the following conditions, a worldwide, royalty-free, non-exclusive, irrevocable license to:
 *    1. Use, copy, and modify the Software
 *    2. Create derivative works
 *    3. Distribute the Software and derivative works
 *
 *  Conditions:
 *    1. Retain the original author's license and copyright notices
 *    2. The original author assumes no liability
 *    3. Unauthorized commercial use is prohibited
 *    4. All modifications and derivative products based on the Software must use this License and disclose the original source code
 *    5. Disclosure of the original source code is not required when using the Software unmodified (e.g., via APIs, libraries, automatic linkers, etc.)
 *
 *  Anyone may distribute or copy this License file, but may not modify it.
 *  For commercial use, please contact the maintainer of the Software to obtain written and electronic authorization. Note that commercial licensing may involve fees; specific terms shall be negotiated separately.
 *  Violation of this Agreement will automatically terminate the license. Upon termination, all copies of the Software must be destroyed, use of the Software must cease, derivative products must be withdrawn from distribution channels, and distribution of derivative products must stop.
 *
 *  The software is provided "as is", without warranty of any kind, express or implied. In no event shall the authors or copyright holders be liable for any claim, damages, or other liability arising from the use of the software.
 *
 *  If any provision is invalid, it does not affect the validity of the other provisions.
 *  By using, distributing, or modifying the Software, you automatically agree to and intend to comply with this Agreement. If you cannot comply, you must stop using, distributing, or modifying the Software.
 *  This Agreement is governed by and construed in accordance with the laws of the People's Republic of China (without regard to conflict of law principles).
 */
/*
 *
 * dm.cc
 * Created by AD on 28/12/25
 * Copyright (c) 2025 AD All rights reserved.
 * xcode++ -e ../entitlements.plist -I ../ -I ../AD -I ./include dm.cc -o dm
 *
**/

#include "include/ADlibc++.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

/* dm_api_sys */
const string repack_cmd = "dpkg-repack";
const string undeb_cmd = "dpkg-deb -R";
const string make_deb_cmd = "dpkg-deb -b";

struct Options {
    string action;          // "pack", "undeb", "make", "extract"
    string input;           // package name or deb file or folder
    string custom_path;     // custom output path
    string output_name;     // custom output filename
    bool use_custom_name = false;
    bool verbose = false;
};

void show_help() {
    ad::cout << "usage: dm [option] [target]" << ad::endl;
    ad::cout << " options:" << ad::endl;
    ad::cout << "   --pack|-p <package>         # pack installed deb" << ad::endl;
    ad::cout << "   --undeb|-u <deb-file>       # decompression deb" << ad::endl;
    ad::cout << "   --make|-m <folder>          # make deb from folder (folder must have DEBIAN/control)" << ad::endl;
    ad::cout << "   --extract|-x <deb-file>     # extract deb to current directory (alternative to --undeb)" << ad::endl;
    ad::cout << "   -name <path> <name>         # custom output path and filename" << ad::endl;
    ad::cout << "   -o <path>                   # output to specified path" << ad::endl;
    ad::cout << "   -v, --verbose               # verbose output" << ad::endl;
    ad::cout << "   --help|-h                   # show this help" << ad::endl;
    ad::cout << ad::endl;
    ad::cout << "Examples:" << ad::endl;
    ad::cout << "  dm --pack wget               # Pack installed wget package" << ad::endl;
    ad::cout << "  dm --undeb wget.deb          # Extract wget.deb" << ad::endl;
    ad::cout << "  dm --make ./mydeb            # Create deb from ./mydeb folder" << ad::endl;
    ad::cout << "  dm --make ./mydeb -o ./debs  # Create deb to ./debs folder" << ad::endl;
    ad::cout << "  dm --make ./mydeb -name ./debs mypackage.deb # Custom path and name" << ad::endl;
}

Options parse_args(const vector<string>& args) {
    Options opts;
    
    for (size_t i = 1; i < args.size(); i++) {
        if (args[i] == "--help" || args[i] == "-h") {
            opts.action = "help";
            return opts;
        }
        else if (args[i] == "--verbose" || args[i] == "-v") {
            opts.verbose = true;
        }
        else if (args[i] == "--pack" || args[i] == "-p") {
            opts.action = "pack";
            if (i + 1 < args.size() && args[i+1][0] != '-') {
                opts.input = args[++i];
            }
        }
        else if (args[i] == "--undeb" || args[i] == "-u") {
            opts.action = "undeb";
            if (i + 1 < args.size() && args[i+1][0] != '-') {
                opts.input = args[++i];
            }
        }
        else if (args[i] == "--make" || args[i] == "-m") {
            opts.action = "make";
            if (i + 1 < args.size() && args[i+1][0] != '-') {
                opts.input = args[++i];
            }
        }
        else if (args[i] == "--extract" || args[i] == "-x") {
            opts.action = "extract";
            if (i + 1 < args.size() && args[i+1][0] != '-') {
                opts.input = args[++i];
            }
        }
        else if (args[i] == "-name") {
            opts.use_custom_name = true;
            if (i + 2 < args.size()) {
                opts.custom_path = args[++i];
                opts.output_name = args[++i];
            }
        }
        else if (args[i] == "-o") {
            if (i + 1 < args.size()) {
                opts.custom_path = args[++i];
                if (opts.output_name.empty()) {
                    
                    if (opts.action == "pack" || opts.action == "make") {
                        opts.output_name = fs::path(opts.input).filename().string() + ".deb";
                    } else if (opts.action == "undeb" || opts.action == "extract") {
                        fs::path p(opts.input);
                        opts.output_name = p.stem().string();
                    }
                }
            }
        }
    }
    
    return opts;
}

bool has_debian_control(const string& folder_path) {
    fs::path control_file = fs::path(folder_path) / "DEBIAN" / "control";
    return fs::exists(control_file) && fs::is_regular_file(control_file);
}

int pack_deb(const Options& opts) {
    if (opts.input.empty()) {
        ad::cerr << "Error: Please specify package name" << ad::endl;
        return 1;
    }
    
    string command = repack_cmd + " " + opts.input;
    
    if (opts.verbose) {
        command += " --verbose";
    }
    
    if (opts.use_custom_name) {
        if (!opts.custom_path.empty()) {
            if (!ad::fs::mkdir(opts.custom_path)) {
                ad::cerr << "Error: Failed to create directory: " << opts.custom_path << ad::endl;
                return 1;
            }
            command += " > " + opts.custom_path + "/";
        }
        command += opts.output_name.empty() ? opts.input + ".deb" : opts.output_name;
    } else if (!opts.custom_path.empty()) {

        if (!ad::fs::mkdir(opts.custom_path)) {
            ad::cerr << "Error: Failed to create directory: " << opts.custom_path << ad::endl;
            return 1;
        }
        command += " > " + opts.custom_path + "/" + opts.input + ".deb";
    }
    
    if (opts.verbose) {
        ad::cout << "Executing: " << command << ad::endl;
    }
    
    ad::cout << "Packing: " << opts.input << ad::endl;
    int result = ad::sys::bash(command.c_str());
    
    if (result == 0) {
        ad::cout << "Successfully packed: " << opts.input << ad::endl;
    } else {
        ad::cerr << "Failed to pack: " << opts.input << ad::endl;
    }
    
    return result;
}

int extract_deb(const Options& opts) {
    if (opts.input.empty()) {
        ad::cerr << "Error: Please specify deb file" << ad::endl;
        return 1;
    }
    
    if (!ad::fs::readable(opts.input)) {
        ad::cerr << "Error: Deb file not found: " << opts.input << ad::endl;
        return 1;
    }
    
    string output_dir;
    if (opts.use_custom_name && !opts.custom_path.empty()) {
        output_dir = opts.custom_path;
        if (!opts.output_name.empty()) {
            output_dir += "/" + opts.output_name;
        }
    } else if (!opts.custom_path.empty()) {

        output_dir = opts.custom_path;
        fs::path p(opts.input);
        output_dir += "/" + p.stem().string();
    } else {

        fs::path p(opts.input);
        output_dir = p.stem().string() + "_extracted";
    }
    
    if (!ad::fs::mkdir(output_dir)) {
        ad::cerr << "Warning: Directory may already exist: " << output_dir << ad::endl;
    }
    
    string command = undeb_cmd + " " + opts.input + " " + output_dir;
    
    if (opts.verbose) {
        command += " --verbose";
        ad::cout << "Executing: " << command << ad::endl;
    }
    
    ad::cout << "Extracting: " << opts.input << " to " << output_dir << ad::endl;
    
    int result = ad::sys::bash(command.c_str());
    
    if (result == 0) {
        ad::cout << "Successfully extracted: " << opts.input << ad::endl;
    } else {
        ad::cerr << "Failed to extract: " << opts.input << ad::endl;
    }
    
    return result;
}

int make_deb(const Options& opts) {
    if (opts.input.empty()) {
        ad::cerr << "Error: Please specify folder path" << ad::endl;
        return 1;
    }
    
    if (!fs::exists(opts.input) || !fs::is_directory(opts.input)) {
        ad::cerr << "Error: Folder not found or not a directory: " << opts.input << ad::endl;
        return 1;
    }
    
    if (!has_debian_control(opts.input)) {
        ad::cerr << "Error: Folder must contain DEBIAN/control file" << ad::endl;
        ad::cerr << "Expected: " << fs::path(opts.input) / "DEBIAN" / "control" << ad::endl;
        return 1;
    }
    
    string deb_name;
    if (opts.use_custom_name && !opts.output_name.empty()) {
        deb_name = opts.output_name;
        if (fs::path(deb_name).extension() != ".deb") {
            deb_name += ".deb";
        }
    } else {
        fs::path p(opts.input);
        deb_name = p.filename().string() + ".deb";
    }
    
    string output_path;
    if (opts.use_custom_name && !opts.custom_path.empty()) {
        if (!ad::fs::mkdir(opts.custom_path)) {
            ad::cerr << "Error: Failed to create directory: " << opts.custom_path << ad::endl;
            return 1;
        }
        output_path = opts.custom_path + "/" + deb_name;
    } else if (!opts.custom_path.empty()) {
        if (!ad::fs::mkdir(opts.custom_path)) {
            ad::cerr << "Error: Failed to create directory: " << opts.custom_path << ad::endl;
            return 1;
        }
        output_path = opts.custom_path + "/" + deb_name;
    } else {
        output_path = deb_name;
    }
    
    string command = make_deb_cmd + " " + opts.input + " " + output_path;
    
    if (opts.verbose) {
        command += " --verbose";
        ad::cout << "Executing: " << command << ad::endl;
    }
    
    ad::cout << "Creating deb from folder: " << opts.input << ad::endl;
    ad::cout << "Output: " << output_path << ad::endl;
    
    int result = ad::sys::bash(command.c_str());
    
    if (result == 0) {
        ad::cout << "Successfully created deb: " << output_path << ad::endl;
    } else {
        ad::cerr << "Failed to create deb" << ad::endl;
    }
    
    return result;
}

int extract_to_current(const Options& opts) {
    if (opts.input.empty()) {
        ad::cerr << "Error: Please specify deb file" << ad::endl;
        return 1;
    }
    
    if (!ad::fs::readable(opts.input)) {
        ad::cerr << "Error: Deb file not found: " << opts.input << ad::endl;
        return 1;
    }
    
    fs::path p(opts.input);
    string output_dir = p.stem().string();
    
    if (fs::exists(output_dir)) {
        ad::cout << "Directory " << output_dir << " already exists. Overwrite? (y/N): ";
        string answer;
        getline(cin, answer);
        if (answer != "y" && answer != "Y") {
            ad::cout << "Aborted." << ad::endl;
            return 0;
        }
        ad::fs::rmdirf(output_dir);
    }
    
    string command = undeb_cmd + " " + opts.input + " " + output_dir;
    
    if (opts.verbose) {
        command += " --verbose";
        ad::cout << "Executing: " << command << ad::endl;
    }
    
    ad::cout << "Extracting: " << opts.input << " to ./" << output_dir << "/" << ad::endl;
    
    int result = ad::sys::bash(command.c_str());
    
    if (result == 0) {
        ad::cout << "Successfully extracted to ./" << output_dir << "/" << ad::endl;
    } else {
        ad::cerr << "Failed to extract: " << opts.input << ad::endl;
    }
    
    return result;
}

int main(int argc, char* argv[]) {
    /* ==== debug ==== */
    ad::stopwatch sw("Total execution time: ");
    
    vector<string> args;
    for (int i = 0; i < argc; i++) {
        args.push_back(argv[i]);
    }
    
    if (argc < 2) {
        show_help();
        return 0;
    }
    
    Options opts = parse_args(args);
    
    if (opts.action == "help" || opts.action.empty()) {
        show_help();
        return 0;
    }
    else if (opts.action == "pack") {
        return pack_deb(opts);
    }
    else if (opts.action == "undeb") {
        return extract_deb(opts);
    }
    else if (opts.action == "make") {
        return make_deb(opts);
    }
    else if (opts.action == "extract") {
        return extract_to_current(opts);
    }
    else {
        ad::cerr << "Error: Unknown action" << ad::endl;
        show_help();
        return 1;
    }
    
    return 0;
}