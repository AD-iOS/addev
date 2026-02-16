#!/bin/bash

files=(
    "opt"
    "opt.cc"
)

includes=(
    "incline"
    "AD"
)

_ldid=$(which ldid)
sdk="/var/theos/sdks/iPhoneOS17.5.sdk"

if [ -x "$(which clang 2>/dev/null)" ] && [ -x "$(which clang++ 2>/dev/null)" ]; then
    cxx="$(which clang++)"
    cc="$(which clang)"
elif [ -x "$(which gcc 2>/dev/null)" ] && [ -x "$(which g++ 2>/dev/null)" ]; then
    cxx="$(which g++)"
    cc="$(which gcc)"
else
    echo "不存在可用的 C/C++ 编译器"
    exit 1
fi

function show_help {
    cat << 'EOF'
usage: ./init [option]
  option:
   --help|-h # 本幫助信息
   --cmake|-c|-cmake # 生成 CMakeLists.txt
   --init|-i|-init # 初始化
   -e|--ens|--entitlements|-ens # 生成 entitlements.plist
   --build|-b # 構建
   --clean|-clean # 清理
   --fish|-fish # 啟用 fish 版本
   --init|-init|-i
     --cmake|-cmake|-c
     -e|--ens|--entitlements|-ens

EOF
}

function init {
    if [ -d ../AD ]; then
        mkdir -p src
        ln -sf ../AD include
        ln -sf ../AD .
        [ "$(id -u)" = "0" ] && chown -h mobile:mobile ./AD ./include
    elif [ -d ~/ADLibrary ]; then
        mkdir -p src
        ln -sf ~/ADLibrary AD
        ln -sf ~/ADLibrary include
        [ "$(id -u)" = "0" ] && chown -h mobile:mobile ./AD ./include
    elif [ -d /var/jb/opt/AD/include ]; then
        mkdir -p src
        ln -sf /var/jb/opt/AD .
        ln -sf /var/jb/opt/AD include
        [ "$(id -u)" = "0" ] && chown -h mobile:mobile ./AD ./include
    else
        echo "库不存在"
        exit 1
    fi
    echo "初始化完成"
}

function entitlements {
    cat > entitlements.plist << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>com.apple.security.application-groups</key>
    <true/>
    <key>com.apple.security.background-task</key>
    <true/>
    <key>com.apple.security.network.client</key>
    <true/>
    <key>com.apple.security.get-task-allow</key>
    <true/>
    <key>com.apple.private.skip-library-validation</key>
    <true/>
    <key>com.apple.private.security.no-sandbox</key>
    <true/>
    <key>com.apple.private.allow-explicit-app-level-api</key>
    <true/>
    <key>com.apple.private.tcc.allow</key>
    <true/>
    <key>com.apple.private.security.storage.AppData</key>
    <true/>
    <key>com.apple.private.filesystems.userfs</key>
    <true/>
    <key>com.apple.private.kernel.extensions</key>
    <true/>
    <key>com.apple.private.network.socket-delegate</key>
    <true/>
    <key>com.apple.private.cs.debugger</key>
    <true/>
</dict>
</plist>
EOF
    echo "entitlements.plist 已生成完成"
}

function cmakelists {
    cat > CMakeLists.txt << EOF
cmake_minimum_required(VERSION 3.13)
project(dm)

set(CMAKE_C_COMPILER "$cc")
set(CMAKE_CXX_COMPILER "$cxx")

enable_language(OBJC)
set(CMAKE_OBJC_COMPILER "$cc")

set(CMAKE_OSX_SYSROOT "$sdk")

add_executable(${files[@]})
target_include_directories(${files[0]} PRIVATE ${includes[@]})

set(EXECUTABLE_OUTPUT_PATH \${CMAKE_BINARY_DIR}/bin)
EOF
    echo "CMakeLists.txt 已生成"
}

function build {
    init
    cmakelists
    entitlements
    rm -rf build && mkdir build
    cd build && cmake ..
    mv ../entitlements.plist ./
    make
    ldid -Sentitlements.plist bin/${files[0]}
    echo "完成"
    echo "位於 build/bin/ 中,名為 ${files[0]}"
}

function clean {
    rm -rf src
    rm -rf include
    rm -rf AD
    rm -rf build
    rm -rf CMakeLists.txt
    rm -rf entitlements.plist 2>/dev/null
    echo "完成"
}

function main {
    case "$1" in
        "--init"|"-i"|"-init")
            if [[ "$1" == "--init" || "$1" == "-i" || "$1" == "-init" ]]; then
                init
            fi
            if [[ "$2" == "-c" || "$2" == "--cmake" || "$2" == "-cmake" ]]; then
                cmakelists
            # fi
            elif [[ "$2" == "-e" || "$2" == "--entitlements" || "$2" == "-ens" || "$2" == "--ens" ]]; then
                entitlements
            fi
            if [[ "$3" == "-e" || "$3" == "--entitlements" || "$3" == "-ens" || "$3" == "--ens" ]]; then
                entitlements
            fi
            ;;
        "--cmake"|"-c"|"-cmake")
            cmakelists
            ;;
        "--help"|"-h"|"")
            show_help
            ;;
        "-e"|"--ens"|"--entitlements"|"-ens")
            entitlements
            ;;
        "-b"|"--build")
            build
            ;;
        "--clean"|"-clean")
            clean
            ;;
        "-fish"|"--fish")
            ./init.fish.sh "$2" "$3"
            ;;
        *)
            echo "[Error]: 未知命令 $1"
            show_help
            return 1
            ;;
    esac
}

main "$@"