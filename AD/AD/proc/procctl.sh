#! /bin/sh -
if [ -f '/etc/profile' ]; then
    . /etc/profile
elif [ -f '/var/jb/etc/profile' ]; then
    . /var/jb/etc/profile
    jb="/var/jb";
else
    echo 'where the fuck "profile"?' 1>&2
fi

proc_dir="$jb/proc"
log_file="$jb/proc/log/monitor.log"

mkdir -p "$proc_dir"
mkdir -p "$jb/proc/log"
touch "$log_file"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" >> "$log_file"
    echo "$1"
}

cleanup_dead_processes() {
    log "開始清理已終止的進程目錄..."
    
    alive_pids=$(ps -A -o pid | tail -n +2)
    
    for pid_dir in "$proc_dir"/*/ ; do
        if [ -d "$pid_dir" ]; then
            pid=$(basename "$pid_dir")
            
            if ! echo "$alive_pids" | grep -q "^$pid$"; then
                log "清理進程 PID $pid 的目錄..."
                rm -rf "$pid_dir"
            fi
        fi
    done
}

collect_process_info() {
    local pid=$1
    local pid_dir="$proc_dir/$pid"
    
    mkdir -p "$pid_dir"
    
    ps -p "$pid" -o pid,ppid,pgid,sid,tty,user,pcpu,pmem,vsz,rss,stat,start,time,comm,command > "$pid_dir/basic_info.txt" 2>/dev/null
    
    sudo ps eww -p "$pid" 2>/dev/null | tail -n 1 > "$pid_dir/environment.txt" 2>/dev/null
    
    lsof -p "$pid" > "$pid_dir/open_files.txt" 2>/dev/null
    
    sudo vmmap "$pid" 2>/dev/null > "$pid_dir/memory_map.txt" || sudo pmap "$pid" 2>/dev/null > "$pid_dir/memory_map.txt"
    
    top -l 1 -pid "$pid" -stats pid,command,cpu,mem,threads > "$pid_dir/status.txt" 2>/dev/null
    
    ps -M -p "$pid" > "$pid_dir/threads.txt" 2>/dev/null
    
    lsof -p "$pid" -i > "$pid_dir/network_connections.txt" 2>/dev/null
    
    pstree -p "$pid" > "$pid_dir/process_tree.txt" 2>/dev/null 2>&1 || echo "無法獲取進程樹" > "$pid_dir/process_tree.txt"
    
    cat > "$pid_dir/real_time_info.json" << EOF
{
    "timestamp": "$(date '+%Y-%m-%d %H:%M:%S')",
    "pid": "$pid",
    "status": "$(ps -p "$pid" -o stat= 2>/dev/null | xargs)",
    "cpu_usage": "$(ps -p "$pid" -o %cpu= 2>/dev/null | xargs)%",
    "memory_usage": "$(ps -p "$pid" -o %mem= 2>/dev/null | xargs)%",
    "resident_memory": "$(ps -p "$pid" -o rss= 2>/dev/null | xargs) KB",
    "virtual_memory": "$(ps -p "$pid" -o vsz= 2>/dev/null | xargs) KB"
}
EOF
}

monitor_processes() {
    log "開始監控進程..."
    
    pids=$(ps -A -o pid | tail -n +2)
    
    for pid in $pids; do
        pid_dir="$proc_dir/$pid"
        
        if [ ! -d "$pid_dir" ]; then
            log "發現新進程 PID: $pid，創建監控目錄..."
            collect_process_info "$pid"
            
            echo "創建時間: $(date '+%Y-%m-%d %H:%M:%S')" > "$pid_dir/creation_time.txt"
            echo "進程命令: $(ps -p "$pid" -o comm= 2>/dev/null | xargs)" >> "$pid_dir/creation_time.txt"
        else
            collect_process_info "$pid"
        fi
    done
}

# 主循環
main() {
    log "進程監控系統啟動，監控目錄: $proc_dir"
    
    while true; do
        cleanup_dead_processes

        mkdir -p "$proc_dir"
        mkdir -p "$jb/proc/log"
        touch "$log_file"
        
        monitor_processes
        
        sleep 5
    done
}

trap 'log "監控系統停止"; exit 0' SIGINT SIGTERM

main
