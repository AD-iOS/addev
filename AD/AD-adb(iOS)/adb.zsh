#!/bin/zsh

#
#
# adb.zsh
# Created by AD on 13/10/25
# Copyright (c) 2025 AD All rights reserved.
#
#

jailbreak_jb="/var/jb"

Version="adb v2.7"

version_Beta="beta 2.7-11"

Developer_version="Developer-version v2.7-16"

adb_Name="Apple-iPhone Debug Bridge"

temp_dir="/tmp/ipa_extract_$$"

Storagespace=$(df -k / | awk 'NR==2 {printf "%.0fGB\n", ($2 * 1024) / 1000000000}')

Real_storagespace=$(df -k / | awk 'NR==2 {printf "%.0fGB\n", $2 / 1048576}')

check_jailbreak() {
    [ -d "$jailbreak_jb" ] || { 
        echo "Error: Device not jailbroken or jailbreak directory not found!" >&2
        exit 1
    }
}

# 显示帮助信息
show_help() {
    echo "Apple-iPhone Debug Bridge (adb-like CLI) $Version"
    echo ""
    echo "Use:"
    echo "adb install <ipa_file>"
    echo " Installation of IPA applications to equipment"
    echo "adb install-deb <deb_file>"
    echo " Installing the DEB package to the device"
    echo "adb reboot"
    echo " Restart the user space"
    echo "adb sbreload"
    echo " reopen SpringBoard"
    echo "adb shell [user]"
    echo " Enter the device shell (default user: mobile)"
    echo "adb devices"
    echo " List device information"
    echo "adb --version"
    echo " Displaying version information"
    echo "adb --help"
    echo " Display help information"
    echo "adb sh-[shell]"
    echo " Example use:adb sh-bash.At present, only zsh and bash are supported."
    echo "adb uninstall"
    echo " Uninstall the rootless jailbreak application"
    echo "adb f-uninstall"
    echo " Force uninstall rootless jailbreak apps."
    echo "adb logcat-mobile"
    echo "adb logcat-System"
    echo " Check the log files of the system or mobile"
    echo "adb ssh"
    echo " Use ssh to link to other devices"
    echo ""
    echo "Example:"
    echo "adb install ~/Downloads/App.ipa"
    echo " Install IPA"
    echo "adb install-deb /path/to/tweak.deb"
    echo " Install deb"
    echo "adb shell root"
    echo " Enter the shell with the root user"
    echo "adb reboot"
    echo " Restart the user space"
}

# 显示版本信息
show_version() {
    zsh --version
    echo "Version Info:"
    echo "$Version|$version_Beta|$Developer_version"
    # echo "Version:$Version"
    echo "$adb_Name (aarch64-apple-darwin)"
}

# Installation of IPA files
install_ipa() {
    local ipa_path="$1"
    
    # Check if the file exists
    if [ ! -f "$ipa_path" ]; then
        echo "Error: IPA file '$ipa_path' not found!" >&2
        exit 1
    fi
    
    # Checking file extensions
    if [[ "$ipa_path" != *.ipa ]]; then
        echo "Error: '$ipa_path' is not an IPA file!" >&2
        exit 1
    fi
    
    echo "Installing $ipa_path..."
    
    # Create a temporary decompression directory
    mkdir -p "$temp_dir"
    
    # Extract IPA
    echo "Extracting IPA..."
    if ! unzip -q "$ipa_path" -d "$temp_dir"; then
        echo "Error: Failed to extract IPA file!" >&2
        rm -rf "$temp_dir"
        exit 1
    fi
    
    # Find the .app folder
    local app_folder=$(find "$temp_dir/Payload" -name "*.app" -type d 2>/dev/null | head -n 1)
    
    if [ -z "$app_folder" ]; then
        echo "Error: No .app folder found in IPA!" >&2
        rm -rf "$temp_dir"
        exit 1
    fi
    
    # Get application name
    local app_name=$(basename "$app_folder")
    
    # 检查是否已存在同名应用
    if [ -d "$jailbreak_jb/Applications/$app_name" ]; then
        echo "Removing existing application: $app_name"
        rm -rf "$jailbreak_jb/Applications/$app_name"
    fi
    
    # 移动应用到Applications目录
    echo "Installing application: $app_name"
    mv "$app_folder" "$jailbreak_jb/Applications/"
    
    # 设置权限
    chmod -R 755 "$jailbreak_jb/Applications/$app_name"
    
    # 清理临时文件
    rm -rf "$temp_dir"
    
    echo "Successfully installed $app_name"
    echo "Restarting SpringBoard in 3 seconds..."
    sleep 3
    killall -HUP SpringBoard
}

# 查看系统日志
adb_System_log() {
    local System_logs="/var/log/system.log"
    ls "$System_logs"
    if [ $? -ne 0 ]; then
        echo "System log file not found!"
        exit 1
    fi
    read -r "Continue" Continue_1
    echo "=== System log ==="
    echo "↑/↓/←/→(Up/down/left/right line"
    echo "Press 'q' to exit"
    less "/var/log/system.log"
}

# 查看mobile日志
adb_mobile_log() {
    local mobile_log_log="${1:-}"
    local mobile_logs="/var/mobile/Library/Logs/CrashReporter"
    
    # 检查目录是否存在
    if [[ ! -d "$mobile_logs" ]]; then
        echo "Mobile logs directory not found!"
        return 1
    fi
    
    # 获取文件列表到数组
    local files=("$mobile_logs"/*(N))
    if [[ ${#files[@]} -eq 0 ]]; then
        echo "No log files found in directory!"
        return 1
    fi
    
    # 列出文件并编号
    echo "Available log files:"
    for i in {1..${#files[@]}}; do
        echo "$i. ${files[$i]##*/}"
    done
    
    # Zsh 兼容的 read 命令
    while true; do
        read -r "serial_number? Please enter the serial number: "
        
        # 验证输入是否为数字且在有效范围内
        if [[ "$serial_number" =~ ^[0-?]+$ ]] && \
           [[ $serial_number -ge 1 ]] && \
           [[ $serial_number -le ${#files[@]} ]]; then
            break
        else
            echo "Invalid serial number, please enter a number between 1 and ${#files[@]}"
        fi
    done
    
    local selected_file="${files[$serial_number]}"
    
    if [[ -n "$selected_file" ]] && [[ -f "$selected_file" ]]; then
        echo "=== mobile log: ${selected_file##*/} ==="
        echo "↑/↓/←/→ (Up/down/left/right move a line)"
        echo "q to quit, / to search"
        read -r "continue_10?Press Enter to continue..."
        less "$selected_file"
    else
        echo "File not found or invalid!"
    fi
}

# Uninstall rootless jailbreak applications
adb_uninstall_jb_ipa() {
    # 定义受保护的应用白名单
    local protected_apps=(
        "Cydia.app"
        "sileo.app"
        "Zebra.app"
        "chromatic.app"
    )
    
    echo "Installed rootless apps:"
    ls "$jailbreak_jb/Applications"
    
    read -r "Uninstall application name: " uninstall_ipa
    
    # 检查是否在保护列表中
    for app in "${protected_apps[@]}"; do
        if [[ "$uninstall_ipa" == "$app" ]]; then
            echo "Error: Uninstalling '$app' is not allowed!"
            exit 1
        fi
    done
    
    # 检查应用是否存在
    local app_path="$jailbreak_jb/Applications/$uninstall_ipa"
    if [[ ! -d "$app_path" ]]; then
        echo "Error: Application '$uninstall_ipa' does not exist!"
        exit 1
    fi
    
    # 确认操作
    read -r "REPLY?Confirm uninstall '$uninstall_ipa'? (y/N) "
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        sudo rm -rf "$app_path"
        echo "Uninstall completed: $uninstall_ipa"
    else
        echo "Uninstall cancelled"
    fi
}

# 强制卸载版本（无确认提示）
adb_uninstall_jb_ipa_f() {
    # 定义受保护的应用白名单
    local protected_apps=(
        "Cydia.app"
        "sileo.app"
        "Zebra.app"
        "chromatic.app"
    )
    
    echo "Installed rootless apps:"
    ls "$jailbreak_jb/Applications"
    
    read -r "Uninstall application name: " uninstall_ipa
    
    # 检查是否在保护列表中
    for app in "${protected_apps[@]}"; do
        if [[ "$uninstall_ipa" == "$app" ]]; then
            echo "Error: Uninstalling '$app' is not allowed!"
            exit 1
        fi
    done
    
    # 检查应用是否存在
    local app_path="$jailbreak_jb/Applications/$uninstall_ipa"
    if [[ ! -d "$app_path" ]]; then
        echo "Error: Application '$uninstall_ipa' does not exist!"
        exit 1
    fi
    
    # 直接执行卸载
    sudo rm -rf "$app_path"
    echo "Uninstall completed: $uninstall_ipa"
}

# Realize the shell function
adb_sh() {
    local user="${1:-mobile}"
    
    case "$user" in
        "root")
            echo "Starting shell as root..."
            sudo -i
            ;;
        "mobile")
            echo "Starting shell as mobile..."
            sudo -u mobile -s #这里用sudo进入mobile
            ;;
        *)
            echo "Error: Unknown user '$user'. Use 'root' or 'mobile'." >&2
            exit 1
            ;;
    esac
}

adb_shell_bash() {

           echo "Entering bash with sudo..."
           sudo bash
           
}

adb_shell_zsh() {

           echo "Entering zsh with sudo..."
           sudo zsh
}

# I don't know which one of the gods provided it works pretty well
adb_devices() {
    echo "Equipment Name: $(uname -n)"
    echo "kernel version: $(uname -r)"
    echo "Internal model name: $(uname -m)"
    echo "Kernel Information: $(uname -s)"
    echo "Cell phone storage space is: $Storagespace"
    echo "The real storage space is: $Real_storagespace"
    echo "jailbreak status: $(if [ -d "$jailbreak_jb" ]; then echo "jailbroken"; else echo "Probably not jailbroken."; fi)"
}

# 安装DEB文件
install_deb() {
    local deb_path="$1"
    
    # 检查文件是否存在
    if [ ! -f "$deb_path" ]; then
        echo "Error: DEB file '$deb_path' not found!" >&2
        exit 1
    fi
    
    # 检查文件扩展名
    if [[ "$deb_path" != *.deb ]]; then
        echo "Error: '$deb_path' is not a DEB file!" >&2
        exit 1
    fi
    
    echo "Installing $deb_path..."
    sudo dpkg -i "$deb_path"
    sudo apt-get install -f -y
    echo "Successfully installed DEB package"
}

# 重启设备
reboot_device() {
    local mode="${1:-normal}"
    
    case "$mode" in
        "normal")
            echo "Restart device..."
            sleep 1 # 延迟一秒钟执行
            sudo launchctl reboot userspace # 改為重啟用戶空間因為找到了重啟用戶空間的命令
            ;;
        "sbreload")
            echo "Restarting SpringBoard..."
            sleep 1 # 延迟一秒钟执行
            sudo killall -HUP SpringBoard
            ;;
        *)
            echo "Error: Unknown reboot mode '$mode'" >&2
            exit 1
            ;;
    esac
}

# ADB-SSH
adb_ssh() {
    echo "Please ensure both devices have the necessary SSH plugins installed, such as OpenSSH and its configuration tools."

    local ip=""
    
    while true; do
        read -r "ip?Target device IP address: "
        
        if [[ -z "$ip" ]]; then
            echo "IP address is required."
            continue
        fi
        
        # IP格式验证
        if [[ "$ip" =~ ^([0-9]{1,3}\.){3}[0-9]{1,3}$ ]]; then
            local valid=true
            local IFS=.
            local segments=(${=ip})  # Zsh 数组语法
            for segment in $segments; do
                if (( segment < 0 || segment > 255 )); then
                    valid=false
                    break
                fi
            done
            if [[ $valid == true ]]; then
                break
            fi
        fi
        echo "Invalid IP address format. Please enter a valid IP (e.g., 192.168.1.1)"
    done
    
    echo "Logging in as mobile user @$ip..."
    
    # 设置自定义提示符
    ssh -o ConnectTimeout=10 -o ServerAliveInterval=60 -t mobile@"$ip" "
        export PS1='[ADB-SSH] mobile@$ip:~ '
        exec \$SHELL
    "
    
    if [ $? -ne 0 ]; then
        echo "SSH connection failed, please check:"
        echo "1. Whether the target device has turned on the SSH service"
        echo "2. Is the IP address correct?"
        echo "3. Is the network connection normal?"
        echo "4. Whether the necessary SSH plug-ins have been installed"
        return 1
    fi
}

# 主函数
main() {
    local command="$1"
    local argument="$2"
    
    # 检查越狱环境（除了help和version命令）
    if [[ "$command" != "help" && "$command" != "version" && "$command" != "h" && "$command" != "devices" ]]; then
        check_jailbreak
    fi
    
    case "$command" in
        "install")
            install_ipa "$argument"
            ;;
        "install-deb")
            install_deb "$argument"
            ;;
        "reboot")
            reboot_device "${argument:-normal}"
            ;;
        "sbreload")
            reboot_device "sbreload"
            ;;
        "shell")
            adb_sh "$argument"
            ;;
        "devices")
            adb_devices
            ;;
        "--version") # 改用一种约定俗成的方式从version改为--version
            show_version
            ;;
        "-v")
            show_version
            ;;
        "--help") # 约定俗成的行为从help改为--help
            show_help
            ;;
        "-h")
            show_help
            ;;
        "sh-bash")
            adb_shell_bash
            ;;
        "sh-zsh")
            adb_shell_zsh
            ;;
        "uninstall")
            adb_uninstall_jb_ipa
            ;;
        "f-uninstall")
            adb_uninstall_jb_ipa_f
            ;;
        "logcat-System")
            adb_System_log
            ;;
        "logcat-mobile")
            adb_mobile_log "$argument"
            ;;
        "ssh")
            adb_ssh
            ;;
        *)
            echo "Error: Unknown command '$command'" >&2
            echo "Use 'adb -h' or 'adb --help' for usage information" >&2
            exit 1
            ;;
    esac
}

# 执行主函数
main "$@"
