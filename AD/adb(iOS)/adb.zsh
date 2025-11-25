#!/bin/zsh
# Apple-iPhone Debug Bridge (adb-like CLI)
# 开发者测试版本(Version-Developer-beta) 不代表最终版本
# 伪adb但是命令行版本
# 用法: adb install path/to/app.ipa
#       adb reboot [recovery|sbreload]
#       adb install-deb path/to/package.deb
#       adb version
#       adb help
# ————————————————————————————————————————————————————————

jailbreak_jb="/var/jb" #无根目录
#
Version="adb v2.6-39" #版本号
#
Developer_Team="AD iOS·StarCreate Team(iOS·STC | SCT)" # 开发者及其团队
#
version_Beta="beta 2.6-48"
#
Developer_version="Developer-version v2.6-59"
#
adb_Name="Apple-iPhone Debug Bridge" #全称
#
temp_dir="/tmp/ipa_extract_$$" #临时目录
#
Storagespace=$(df -k / | awk 'NR==2 {printf "%.0fGB\n", ($2 * 1024) / 1000000000}') #存储空间显示命令
#
Real_storagespace=$(df -k / | awk 'NR==2 {printf "%.0fGB\n", $2 / 1048576}') #真实存储空间显示命令
# —————————————————————————————————————————————————————————
# 检查越狱环境 #这里是访问jb文件夹是否存在后续添加越狱商店的检测但是目前够用
check_jailbreak() {
    [ -d "$jailbreak_jb" ] || { 
        echo "Error: Device not jailbroken or jailbreak directory not found!" >&2
        exit 1
    }
}

# 显示帮助信息 #优化显示详细从中英各一行优化成一行两个同时不会有信息丢失的情况
show_help() {
    echo "Apple-iPhone Debug Bridge (adb-like CLI) $Version"
    echo ""
    echo "Use/用法:"
    echo "adb install <ipa_file>"
    echo " Installation of IPA applications to equipment|安装IPA应用到设备"
    echo "adb install-deb <deb_file>"
    echo " Installing the DEB package to the device|安装DEB包到设备"
    echo "adb reboot"
    echo " reboot|重启设备"
    echo "adb sbreload"
    echo " reopen SpringBoard|重启SpringBoard"
    echo "adb shell [user]"
    echo " Enter the device shell (default user: mobile)"
    echo " 进入设备shell (默认用户: mobile)"
    echo "adb devices"
    echo " List device information|列出设备信息"
    echo "adb --version"
    echo " Displaying version information|显示版本信息"
    echo "adb --help"
    echo " Display help information|显示帮助信息"
    echo "adb Update-log"
    echo " Update log|更新日志"
    echo "adb sh-[shell]"
    echo " Example use:adb sh-bash.At present, only zsh and bash are supported."
    echo " 使用示例:adb sh-bash.目前仅支持zsh、bash"
    echo "adb --Developer-log"
    echo " Check the developer log|查看开发者日志"
    echo "adb uninstall"
    echo " 卸载无根越狱的应用|Uninstall the rootless jailbreak application"
    echo "adb F-uninstall"
    echo " 无提示并且强制卸载无根越狱应用|Force uninstall rootless jailbreak apps."
    echo "adb logcat-mobile"
    echo "adb logcat-System"
    echo " 查看系统或者mobile的日志文件|Check the log files of the system or mobile"
    echo "adb ssh"
    echo " 使用ssh链接其他设备|Use ssh to link to other devices"
    echo ""
    echo "Example/示例:"
    echo "adb install ~/Downloads/App.ipa"
    echo " Install IPA|安装IPA"
    echo "adb install-deb /path/to/tweak.deb"
    echo " Install deb|安装deb"
    echo "adb shell root"
    echo " Enter the shell with the root user|以root用户进入shell"
    echo "adb reboot"
    echo " Restart|重启"
}

# 显示版本信息
show_version() {
    zsh --version #依赖于zsh所以zsh的版本号信息也要虽然不是规范之一
    echo "$Developer_Team" #开发者及其团队
    echo "Version Info:"
    echo "$Version|$version_Beta|$Developer_version" #开发者、测试版
    # echo "Version:$Version" #我们的版本号 开发者、测试版不需要正式版才需要
    echo "$adb_Name (aarch64-apple-darwin)" #我们自己的信息
}

# 安装IPA文件 #Installation of IPA files
install_ipa() {
    local ipa_path="$1"
    
    # 检查文件是否存在 #Check if the file exists
    if [ ! -f "$ipa_path" ]; then
        echo "Error: IPA file '$ipa_path' not found!" >&2
        exit 1
    fi
    
    # 检查文件扩展名 #Checking file extensions
    if [[ "$ipa_path" != *.ipa ]]; then
        echo "Error: '$ipa_path' is not an IPA file!" >&2
        exit 1
    fi
    
    echo "Installing $ipa_path..."
    
    # 创建临时解压目录 #Create a temporary decompression directory
    mkdir -p "$temp_dir"
    
    # 解压IPA #Extract IPA
    echo "Extracting IPA..."
    if ! unzip -q "$ipa_path" -d "$temp_dir"; then
        echo "Error: Failed to extract IPA file!" >&2
        rm -rf "$temp_dir"
        exit 1
    fi
    
    # 查找.app文件夹 #Find the .app folder
    local app_folder=$(find "$temp_dir/Payload" -name "*.app" -type d 2>/dev/null | head -n 1)
    
    if [ -z "$app_folder" ]; then
        echo "Error: No .app folder found in IPA!" >&2
        rm -rf "$temp_dir"
        exit 1
    fi
    
    # 获取应用名称 #Get application name
    local app_name=$(basename "$app_folder")
    
    # 检查是否已存在同名应用
    if [ -d "$jailbreak_jb/Applications/$app_name" ]; then
        echo "Removing existing application: $app_name"
        rm -rf "$jailbreak_jb/Applications/$app_name"
    fi
    
    # 移动应用到Applications目录 #Move the application to the Applications directory
    echo "Installing application: $app_name"
    mv "$app_folder" "$jailbreak_jb/Applications/"
    
    # 设置权限 #Setting Up Permissions
    chmod -R 755 "$jailbreak_jb/Applications/$app_name"
    
    # 清理临时文件 #Cleaning up temporary files
    rm -rf "$temp_dir"
    
    echo "Successfully installed $app_name"
    echo "Restarting SpringBoard in 3 seconds..."
    sleep 3
    sbreload
}

# less /var/log/system.log 标记
# /var/mobile/Library/Logs/CrashReporter 标记
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
    echo "↑/↓/←/→(上/下/左/右一行)|Up/down/left/right line"
    echo "按'q'退出|Press 'q' to exit"
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
        read -r "serial_number?请输入序号|Please enter the serial number: "
        
        # 验证输入是否为数字且在有效范围内
        if [[ "$serial_number" =~ ^[0-?]+$ ]] && \
           [[ $serial_number -ge 1 ]] && \
           [[ $serial_number -le ${#files[@]} ]]; then
            break
        else
            echo "无效的序号，请输入 1 到 ${#files[@]} 之间的数字"
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
        echo "文件未找到或无效!"
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
            sudo -i #I'm using sudo here because su is not available. Why not su? Because su's password is Apple's randomized password, and it's very difficult for a normal user to get it, so it's better to use sudo.
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

# shell bash #来自被注释的adb_shell函数因为adb_shell函数的bug过多已经无力修复所以拆成adb_shell_bash
adb_shell_bash() {
# 被删除一部分因为bug我们无力修复如果您会修复请到adb_shell_zsh函数中查看完整实现方法(包括被注释部分)
           echo "Entering bash with sudo..."
           sudo bash
           # echo "Error: Unknown shell '$shell'. Use 'zsh' or 'bash'." >&2
            # exit 1
}

# shell zsh #来自被注释的adb_shell函数因为adb_shell函数的bug过多已经无力修复所以拆成adb_shell_zsh
adb_shell_zsh() {
    # local shell_zsh="${1:-zsh}"

    # case "$shell_zsh" in
       # "zsh")
           echo "Entering zsh with sudo..."
           sudo zsh
           # ;;
       # *)
            # echo "Error: Unknown shell '$shell'. Use 'zsh' or 'bash'." >&2
            # exit 1
            # ;;
    # esac
}

# shell 模式 #bug多直接注释这个函数了就用那两个被拆成两个的了
# adb_shell() {
    # local shell="${1:-bash}"

    # case "$shell" in
        # "zsh")
            # echo "Entering zsh with sudo..."
            # exec sudo zsh
            # ;;
        # "bash")
            # echo "Entering bash with sudo..."
            # exec sudo bash
            # ;;
        # *)
            # echo "Error: Unknown shell '$shell'. Use 'zsh' or 'bash'." >&2
            # exit 1
            # ;;
    # esac
# }

# 显示设备信息 I don't know which one of the gods provided it works pretty well
adb_devices() {
    echo "设备名称/Equipment Name: $(uname -n)"
    echo "内核版本/kernel version: $(uname -r)"
    echo "内部机型名称/Internal model name: $(uname -m)"
    echo "内核信息/Kernel Information: $(uname -s)"
    echo "手机存储空间是/Cell phone storage space is: $Storagespace"
    echo "真实存储空间是/The real storage space is: $Real_storagespace"
    echo "越狱状态/jailbreak status: $(if [ -d "$jailbreak_jb" ]; then echo "已越狱/jailbroken"; else echo "可能未越狱/Probably not jailbroken."; fi)"
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
            sleep 1 #延迟一秒钟执行
            sudo reboot #必须sudo #從ldrestart重啟用戶空間改為reboot真重啟,不然ldrestart疑似無法啟動所以改成這樣子
            ;;
        "sbreload")
            echo "Restarting SpringBoard..."
            sleep 1 #延迟一秒钟执行
            sudo sbreload
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

# 开发者日志
adb_Developer_log() {
    echo "Because some devices of the ldrestart command may not exist or we can't use it due to insufficient permissions, we can't use it, so we use the reboot command to restart it, but note that rootless jailbreak restart will be jailbreak."
    echo "Because we can t fix the 'adb sh', that is, the 'adb_shell' function method, we split the 'adb_shell' function into two functions for separate calls and use. The current main function and the 'help' function have changed the usage method."
    echo "How did we repair it? Because the problem of the 'adb_shell' function is more painful, it is divided into two separate functions, 'adb_shell_bash' and 'adb_shell_zsh' functions, and add these two function calls to the 'main' function"
    echo "Maybe it s not very good here. It s recommended to check the source code directly."
    echo ""
    echo "因为ldrestart(假重启通过重启用户空间引发内核恐慌来重启)命令部分设备可能未存在或者我们的权限不足的因为导致无法使用,所以改用reboot(真重启)命令来重启,但是注意无根越狱重启会掉越狱."
    echo "因为'adb sh'即'adb_shell'函数方法我们无法修复所以我们把'adb_shell'函数拆分成两个函数来单独调用和使用目前main函数和'help'函数已经更改使用方法"
    echo "我们是怎么修复的?因为'adb_shell'函数的问题比较折磨人所以拆分成两个单独函数分别是'adb_shell_bash'和'adb_shell_zsh'函数并且在'main'函数中添加这两个函数调用"
    echo "可能这里讲得不是很好建议直接查看源代码"
}

# 更新日志 #后续吧如果很长会选择放进指定目录里面
adb_Update_log() {
    echo "$Version"
    echo "US_en:"
    echo "Fix the bug that failed to restart the command"
    echo "'The developer log has been opened. Enter Developer-log to view the developer log.'"
    echo "Because adb sh cannot be repaired, we have changed a method to adb sh-[shell] to start the corresponding shell (developers, please check the developer log update log, there will be no repair method, etc.)"
    echo ""
    echo "New functions have been added to optimize some code logic."
    echo ""
    echo "Optimize the display of help information, change from the original command, English and Chinese lines to command lines, English and Chinese will not lose information or most of the information will not be lost."
    echo ""
    echo "Optimized some functions"
    echo ""
    echo "English information has been added to most of the functions that only display in Chinese (most of the Chinese display is maintained/updated/written by Chinese)"
    echo "If the developer sees the Chinese annotation of our internal code, don‘t be careful because it is written by Chinese. We are now adding English prompts to some annotation information, but in general, it is easier to understand directly reading the code."
    echo ""
    echo ""
    echo "zh_cn: "
    echo "修复重启命令失败的bug"
    echo "'开发者日志已经开放输入Developer-log即可查看开发者日志'"
    echo "能力因为无法修复'adb sh'目前我们更换了一种方法adb sh-[shell]这样子来启动对应的shell(开发者请查看开发者日志因为更新日志不会有修复方法等等)"
    echo "添加了新功能优化了部分代码逻辑"
    echo "优化help帮助信息的显示,从原来的命令、英文、中文各一行改为命令一行、英文和中文同行并且不会丢失信息或者大部分信息都不会丢失"
    echo "优化了部分函数"
    echo "给大部分只有中文显示的功能添加了英文信息(中文显示的大部分是中国人维护/更新/写的)"
    echo "若是开发者看到我们内部代码的中文注释不用当心因为那是中国人写的我们现在在给部分注释信息添加英文提示不过一般情况下直接看代码更容易看懂"
}

# 主函数
main() {
    local command="$1"
    local argument="$2"
    
    # 检查越狱环境（除了help和version命令）
    if [[ "$command" != "help" && "$command" != "version" && "$command" != "devices" ]]; then
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
        "--version") #改用一种约定俗成的方式从version改为--version
            show_version
            ;;
        "--help"|"") #约定俗成的行为从help改为--help
            show_help
            ;;
        "Update-log") #更新日志
            adb_Update_log
            ;;
        "sh-bash")
            adb_shell_bash
            ;;
        "sh-zsh")
            adb_shell_zsh
            ;;
        "--Developer-log")
            adb_Developer_log
            ;;
        "uninstall")
            adb_uninstall_jb_ipa
            ;;
        "F-uninstall")
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
            echo "Use 'adb help' for usage information" >&2
            exit 1
            ;;
    esac
}

# 执行主函数
main "$@"