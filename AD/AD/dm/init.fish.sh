#!/usr/bin/fish

set files dm dm.cc

set includes incline AD

set _ldid (which ldid)
set sdk "/var/theos/sdks/iPhoneOS17.5.sdk"

if test -x (which clang 2>/dev/null); and test -x (which clang++ 2>/dev/null)
    set cxx (which clang++)
    set cc (which clang)
else if test -x (which gcc 2>/dev/null); and test -x (which g++ 2>/dev/null)
    set cxx (which g++)
    set cc (which gcc)
else
    echo "Error: No C/C++ compiler found"
    exit 1
end

function show_help
    printf '%s\n' \
        "usage: ./init [option]" \
        "  option:" \
        "   --help|-h # Show help" \
        "   --cmake|-c|-cmake # Generate CMakeLists.txt" \
        "   --init|-i|-init # Initialize project" \
        "   -e|--ens|--entitlements|-ens # Generate entitlements.plist" \
        "   --build|-b # Build project" \
        "   --clean|-clean # Clean build files" \
        "   --init|-init|-i" \
        "     --cmake|-cmake|-c" \
        "     -e|--ens|--entitlements|-ens"
end

function init
    if test -d ../AD
        mkdir -p src
        ln -sf ../AD include
        ln -sf ../AD .
        if test (id -u) = 0
            chown -h mobile:mobile ./AD ./include
        end
    else if test -d ~/ADLibrary
        mkdir -p src
        ln -sf ~/ADLibrary AD
        ln -sf ~/ADLibrary include
        if test (id -u) = 0
            chown -h mobile:mobile ./AD ./include
        end
    else if test -d /var/jb/opt/AD/include
        mkdir -p src
        ln -sf /var/jb/opt/AD .
        ln -sf /var/jb/opt/AD include
        if test (id -u) = 0
            chown -h mobile:mobile ./AD ./include
        end
    else
        echo "Error: Library not found"
        exit 1
    end
    echo "Initialization complete"
end

function entitlements
    printf '%s\n' \
        '<?xml version="1.0" encoding="UTF-8"?>' \
        '<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">' \
        '<plist version="1.0">' \
        '<dict>' \
        '    <key>com.apple.security.application-groups</key>' \
        '    <true/>' \
        '    <key>com.apple.security.background-task</key>' \
        '    <true/>' \
        '    <key>com.apple.security.network.client</key>' \
        '    <true/>' \
        '    <key>com.apple.security.get-task-allow</key>' \
        '    <true/>' \
        '    <key>com.apple.private.skip-library-validation</key>' \
        '    <true/>' \
        '    <key>com.apple.private.security.no-sandbox</key>' \
        '    <true/>' \
        '    <key>com.apple.private.allow-explicit-app-level-api</key>' \
        '    <true/>' \
        '    <key>com.apple.private.tcc.allow</key>' \
        '    <true/>' \
        '    <key>com.apple.private.security.storage.AppData</key>' \
        '    <true/>' \
        '    <key>com.apple.private.filesystems.userfs</key>' \
        '    <true/>' \
        '    <key>com.apple.private.kernel.extensions</key>' \
        '    <true/>' \
        '    <key>com.apple.private.network.socket-delegate</key>' \
        '    <true/>' \
        '    <key>com.apple.private.cs.debugger</key>' \
        '    <true/>' \
        '</dict>' \
        '</plist>' > entitlements.plist
    echo "entitlements.plist generated"
end

function cmakelists
    printf '%s\n' \
        'cmake_minimum_required(VERSION 3.13)' \
        'project(dm)' \
        '' \
        "set(CMAKE_C_COMPILER \"$cc\")" \
        "set(CMAKE_CXX_COMPILER \"$cxx\")" \
        '' \
        'enable_language(OBJC)' \
        "set(CMAKE_OBJC_COMPILER \"$cc\")" \
        '' \
        "set(CMAKE_OSX_SYSROOT \"$sdk\")" \
        '' \
        "add_executable($files)" \
        "target_include_directories($files[1] PRIVATE $includes)" \
        '' \
        'set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)' > CMakeLists.txt
    echo "CMakeLists.txt generated"
end

function build
    init
    cmakelists
    entitlements
    rm -rf build && mkdir build
    cd build && cmake ..
    mv ../entitlements.plist ./
    make
    $_ldid -Sentitlements.plist bin/$files[1]
    echo "Build complete"
    echo "Binary: build/bin/$files[1]"
end

function clean
    rm -rf src
    rm -rf include
    rm -rf AD
    rm -rf build
    rm -rf CMakeLists.txt
    rm -rf entitlements.plist 2>/dev/null
    echo "Clean complete"
end

function main
    switch $argv[1]
        case --init -i -init
            init
            switch $argv[2]
                case -c --cmake -cmake
                    cmakelists
                case -e --entitlements -ens --ens
                    entitlements
            end

            switch $argv[3]
                case -e --entitlements -ens --ens
                    entitlements
            end

        case -c --cmake -cmake
            cmakelists
        case -e --entitlements -ens --ens
            entitlements
        case --help -h ""
            show_help
        case -b --build
            build
        case --clean -clean
            clean
        case '*'
            echo "[Error]: Unknown command: $argv[1]"
            show_help
            return 1
    end
end

main $argv