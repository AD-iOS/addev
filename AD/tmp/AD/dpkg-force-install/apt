#! /bin/sh -

if [ -f '/etc/profile' ]; then
    . /etc/profile
    jb=""
elif [ -f '/var/jb/etc/profile' ]; then
    . /var/jb/etc/profile
    jb='/var/jb'
else
    echo 'Where the fuck "profile"?' 1>&2
    exit 1
fi

if [ -d "$jb/var/lib/apt-force-install" ]; then
    DOWNLOAD_DIR="$jb/var/lib/apt-force-install"
    if [ -d "$DOWNLOAD_DIR/download" ]; then
        :
    else
        mkdir -p "$DOWNLOAD_DIR/download"
    fi
    # tmp="$TMPDIR"
    :
else
    mkdir -p "$jb/var/lib/apt-force-install"
    DOWNLOAD_DIR="$jb/var/lib/apt-force-install"
    mkdir -p "$DOWNLOAD_DIR/download"
    # tmp="$TMPDIR"
fi

apt_original="$jb/usr/bin/apt.original"
apt() { "$apt_original" "$@"; }
: "${dpkgarch:=iphoneos-arm64}"
dpkgarch="${dpkgarch:-$(dpkg.original --print-architecture)}"

# DOWNLOAD_DIR="/tmp/apt-force-install-$$"
mkdir -p "$DOWNLOAD_DIR"
chmod 755 "$DOWNLOAD_DIR" && chown _apt:wheel "$DOWNLOAD_DIR"
# cd "$DOWNLOAD_DIR" || exit 1

export APT_CONFIG=":"
export DEBIAN_FRONTEND=noninteractive

is_installed() {
    pkg="$1"
    dpkg-query -W -f='${Status}' "$pkg" 2>/dev/null | grep -q "install ok installed"
    return $?
}

version_compare() {
    ver1="$1"
    ver2="$2"
    dpkg --compare-versions "$ver1" ge "$ver2" 2>/dev/null
    return $?
}

# 拿依賴
get_all_deps() {
    pkg="$1"
    all_deps=""
    queue="$pkg"
    processed=""

    while [ -n "$queue" ]; do
        current=$(echo "$queue" | cut -d' ' -f1)
        queue=$(echo "$queue" | cut -s -d' ' -f2-)
        if echo "$processed" | grep -q "$current"; then
            continue
        fi
        processed="$processed $current"
        apt-cache depends "$current" 2>/dev/null | \
        grep -E "^(Depends|PreDepends):" | \
        while IFS= read -r line; do
            dep_full=$(echo "$line" | sed 's/^[^:]*: //')
            dep_name=$(echo "$dep_full" | sed "s/:${dpkgarch}//g" | awk '{print $1}' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
            if echo "$dep_full" | grep -q "("; then
                dep_version=$(echo "$dep_full" | grep -o '([^)]*)' | sed 's/[()]//g')
                dep_operator=$(echo "$dep_version" | awk '{print $1}')
                dep_version=$(echo "$dep_version" | awk '{print $2}')
            else
                dep_version=""
                dep_operator=""
            fi
            if [ -n "$dep_name" ] && [ "$dep_name" != "$current" ]; then
                installed=0
                if is_installed "$dep_name"; then
                    installed=1
                    installed_version=$(dpkg-query -W -f='${Version}' "$dep_name" 2>/dev/null)
                fi
                need_dep=0
                if [ $installed -eq 0 ]; then
                    need_dep=1
                elif [ -n "$dep_version" ]; then
                    case "$dep_operator" in
                        "<<"|"<="|"="|">="|">>"|"<"|">")
                            if ! version_compare "$installed_version" "$dep_version"; then
                                need_dep=1
                            fi
                            ;;
                        *)
                            need_dep=1
                            ;;
                    esac
                fi
                if [ $need_dep -eq 1 ]; then
                    all_deps="$all_deps $dep_name"
                    queue="$queue $dep_name"
                fi
            fi
        done
    done
    echo "$all_deps" | tr ' ' '\n' | sort -u
}

apt_form_install() {
    # set -x # debug
    local_pkgs=""
    remote_pkgs=""

    for pkg in "$@"; do
        if ( [ -f "$pkg" ] || [ -f "./$pkg" ] ) && (echo "$pkg" | grep -q "\.deb$"); then
            if [ ! -f "$pkg" ] && [ -f "./$pkg" ]; then
                pkg="./$pkg"
            fi
            local_pkgs="$local_pkgs $pkg"
        else
            if [[ "$pkg" =~ \.deb$ ]]; then # 001
                pkg="./$pkg"
                local_pkgs="$local_pkgs $pkg"
            else
                remote_pkgs="$remote_pkgs $pkg"
            fi
        fi
    done

    if [ -n "$local_pkgs" ]; then
        local tmp="apt-force-install-local-tmp-$$"
        echo "Processing local packages: $local_pkgs"

        all_deps=""
        for deb in $local_pkgs; do
            echo "Analyzing: $(basename "$deb")"
            deps=$(dpkg-deb -f "$deb" Depends | tr ',' '\n' | sed 's/|.*//g' | sed 's/([^)]*)//g' | sed 's/[[:space:]]//g' | grep -v "^$")
            pre_deps=$(dpkg-deb -f "$deb" Pre-Depends | tr ',' '\n' | sed 's/|.*//g' | sed 's/([^)]*)//g' | sed 's/[[:space:]]//g' | grep -v "^$")
            all_deps="$all_deps $deps $pre_deps"
        done

        all_deps=$(echo "$all_deps" | tr ' ' '\n' | sort -u | grep -v "^$")

        need_download=""
        for dep in $all_deps; do
            if ! is_installed "$dep"; then
                need_download="$need_download $dep"
            fi
        done

        if [ -n "$need_download" ]; then
            echo "Downloading dependencies: $(echo $need_download | wc -w) packages"
            mkdir -p "$tmp"
            cd "$tmp" || exit 1
            
            for dep in $need_download; do
                echo "Downloading: $dep"
                if apt download \
                    --allow-unauthenticated \
                    -o APT::Architecture=$dpkgarch \
                    -o quiet=2 \
                    -o APT::Get::AllowUnauthenticated=true \
                    -o Acquire::AllowInsecureRepositories=true \
                    "$dep:$dpkgarch" 2>/dev/null; then
                    echo "  ✓ Downloaded $dep"
                else
                    echo "  ✗ Failed to download $dep"
                fi
            done
        fi

        if [ -d "$tmp" ] && [ -n "$(ls $tmp/*.deb 2>/dev/null)" ]; then
            echo "Installing dependencies..."
            for dep_deb in $tmp/*.deb; do
                if [ -f "$dep_deb" ]; then
                    echo "Installing: $(basename "$dep_deb")"
                    if ! dpkg -i "$dep_deb" 2>/dev/null; then
                        dpkg --force-depends --force-confmiss --force-overwrite -i "$dep_deb"
                    fi
                    if [ $? -eq 0 ]; then
                        echo "  ✓ Installed"
                    else
                        echo "  ✗ Failed to install"
                    fi
                fi
            done
        fi
        # cd .. && pwd -P # 003
        echo "Installing local packages..."
        for deb in $local_pkgs; do
            # echo "-----------------------------------------"
            echo "Installing: $(basename "$deb")"
            dpkg-deb -I "$deb" | grep -E "Package|Version|Architecture|Depends" | sed 's/^/  /'
            
            if ! dpkg -i "$deb" 2>/dev/null; then
                echo "Retrying with force..."
                dpkg --force-depends --force-confmiss --force-overwrite -i "$deb"
            fi
            
            if [ $? -eq 0 ]; then
                echo "✓ Installed successfully"
            else
                echo "✗ Failed to install"
                return 1
            fi
        done

        echo "Fixing dependencies..."
        if apt-get install -f -y --allow-unauthenticated -o quiet=2 >/dev/null 2>&1; then
            echo "fix complete"
        else
            echo "error: apt-get failed, return $?"
        fi

        rm -rf "$tmp"

        if [ -z "$remote_pkgs" ]; then
            # set +x
            return 0
        fi
    fi

    if [ -n "$remote_pkgs" ]; then
        DOWNLOAD_DIR="$DOWNLOAD_DIR/download/download-$$"
        mkdir -p "$DOWNLOAD_DIR"
        set -- $remote_pkgs
        echo "Resolving dependencies for: $@"
        all_deps=""
        for pkg in "$@"; do
            deps=$(get_all_deps "$pkg")
            all_deps="$all_deps $deps"
        done
        need_download=""
        for pkg in "$@" $all_deps; do
            if ! is_installed "$pkg"; then
                need_download="$need_download $pkg"
            fi
        done
        need_download=$(echo "$need_download" | tr ' ' '\n' | sort -u)
        echo "Packages to download: $(echo $need_download | wc -w) packages"
        rm -f "$DOWNLOAD_DIR"/*.deb
        if [ -n "$need_download" ]; then
            # echo "Downloading: $need_download"
            echo "Downloading: "
            for pkg in $need_download; do
                if [ -n "$pkg" ]; then
                    cd "$DOWNLOAD_DIR" || continue
                    apt download \
                        --allow-unauthenticated \
                        -o APT::Architecture=$dpkgarch \
                        -o quiet=2 \
                        -o APT::Get::AllowUnauthenticated=true \
                        -o Acquire::AllowInsecureRepositories=true \
                        "$pkg:$dpkgarch" 2>&1 | \
                        grep -v "^WARNING:" | \
                        grep -v "^W: " | \
                        grep -v "^【" | \
                        grep -v "^Ign" | \
                        grep -v "^Hit" | \
                        grep -v "^Get" | \
                        grep -v "does not have a stable" | \
                        grep -v "無法驗證" | \
                        grep -v "忽略了驗證" | \
                        grep -v "Warning" | \
                        grep -v "warn" | \
                        grep -v "^$" || true
                fi
            done
        fi
        cd "$DOWNLOAD_DIR" || exit 1
        debs=$(ls *.deb 2>/dev/null)
        if [ -n "$debs" ]; then
            echo "Installing with dpkg..."
            install_order=""
            for dep in $all_deps; do
                is_target=0
                for target in "$@"; do
                    if [ "$dep" = "$target" ]; then
                        is_target=1
                        break
                    fi
                done
                if [ $is_target -eq 0 ]; then
                    install_order="$install_order $dep"
                fi
            done
            install_order="$install_order $@"
            unique_order=""
            for pkg in $install_order; do
                if ! echo "$unique_order" | grep -q "$pkg"; then
                    unique_order="$unique_order $pkg"
                fi
            done
            if [ -n "$unique_order" ]; then
                echo "Installation order: $unique_order"
                for pkg in $unique_order; do
                    deb_file=$(ls "${pkg}_"*.deb 2>/dev/null | head -1)
                    if [ -n "$deb_file" ]; then
                        # echo "-----------------------------------------"
                        echo "Installing: $deb_file"
                        echo "Package: $(dpkg-deb -f "$deb_file" Package) | Version: $(dpkg-deb -f "$deb_file" Version) | Architecture: $(dpkg-deb -f "$deb_file" Architecture)"
                        if ! dpkg -i "$deb_file" 2>/dev/null; then
                            echo "Retrying with force..."
                            dpkg --force-depends --force-confmiss --force-overwrite -i "$deb_file" 2>/dev/null
                        fi
                        if [ $? -eq 0 ]; then
                            echo "✓ Installed successfully"
                        else
                            echo "✗ Failed to install"
                        fi
                        echo ""
                    fi
                done
                echo "Fixing dependencies..."
                apt-get install -f -y --allow-unauthenticated -o quiet=2 >/dev/null 2>&1
                echo "fix complete"
            fi
        else
            if [ -z "$local_pkgs" ]; then
                echo "No .deb files downloaded"
                exit 1
            fi
        fi
        rm -rf "$DOWNLOAD_DIR"
    fi
}

apt_form_download() {
    pkgs="$@"
    all_deps=""
    local DOWNLOAD_DIR="$DOWNLOAD_DIR/download"
    for pkg in $pkgs; do
        deps=$(get_all_deps "$pkg")
        all_deps="$all_deps $deps"
    done
    all_pkgs=$(echo "$pkgs $all_deps" | tr ' ' '\n' | sort -u)
    echo "Downloading: $(echo $all_pkgs | wc -w) packages"
    mkdir -p "$DOWNLOAD_DIR"
    cd "$DOWNLOAD_DIR" || exit 1
    for pkg in $all_pkgs; do
        if [ -n "$pkg" ]; then
            # echo "Downloading: $pkg:iphoneos-arm64"
            echo "Downloading: "
            apt download \
                --allow-unauthenticated \
                -o APT::Architecture=$dpkgarch \
                -o quiet=2 \
                -o APT::Get::AllowUnauthenticated=true \
                -o Acquire::AllowInsecureRepositories=true \
                "$pkg:$dpkgarch" 2>&1 | \
                grep -v "^WARNING:" | \
                grep -v "^W: " | \
                grep -v "^【" | \
                grep -v "^Ign" | \
                grep -v "^Hit" | \
                grep -v "^Get" | \
                grep -v "does not have a stable" | \
                grep -v "無法驗證" | \
                grep -v "忽略了驗證" | \
                grep -v "Warning" | \
                grep -v "warn" | \
                grep -v "^$" || true
        fi
    done
    echo "Downloaded to: $DOWNLOAD_DIR"
    ls -l *.deb
}

_apt_form_show_help="\
usage: apt-force-install [option] [command]
option:
  install               install deb, acquiesced rootless
  install-to-rootful    designate install rootful deb
  install-to-rootless   designate install rootless
  install-to-roothide   designate install roothide deb
  download              download deb, acquiesced rootless
  download-to-rootful   designate download rootful deb
  download-to-rootless  designate download rootless deb
  download-to-roothide  designate download roothide deb

"


case $1 in
    install) shift; apt_form_install "$@" ;;
    download) shift; apt_form_download "$@" ;;

    install-to-rootful|i-to-rootful) shift; dpkgarch="iphoneos-arm" apt_form_install "$@" ;;
    install-to-rootless|i-to-rootless) shift; dpkgarch="iphoneos-arm64" apt_form_install "$@" ;;
    install-to-roothide|i-to-roothide) shift; dpkgarch="iphoneos-arm64e" apt_form_install "$@" ;;

    download-to-rootful|d-to-rootful) shift; dpkgarch="iphoneos-arm" apt_form_download "$@" ;;
    download-to-rootless|d-to-rootless) shift; dpkgarch="iphoneos-arm64" apt_form_download "$@" ;;
    download-to-roothide|d-to-roothide) shift; dpkgarch="iphoneos-arm64e" apt_form_download "$@" ;;
    --help|help) printf "$_apt_form_show_help"; apt --help ;;
    *) unset APT_CONFIG; apt "$@" ;;
esac
