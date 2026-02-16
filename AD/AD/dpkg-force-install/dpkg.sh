#! /bin/sh

# set -x

AUTOPATCHES=${AUTOPATCHES:-1}

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

if [ -d "$jb/var/lib/dpkg-force-install" ]; then
    TMPDIR="$jb/var/lib/dpkg-force-install"
    tmp="$TMPDIR"
    :
else
    mkdir -p "$jb/var/lib/dpkg-force-install"
    TMPDIR="$jb/var/lib/dpkg-force-install"
    tmp="$TMPDIR"
fi

dpkg_original="${jb}/usr/bin/dpkg.original"

if [ ! -f "$dpkg_original" ]; then
    if [ -f "${jb}/usr/bin/dpkg" ] && [ ! -L "${jb}/usr/bin/dpkg" ]; then
        mv "${jb}/usr/bin/dpkg" "$dpkg_original"
    else
        echo "[Error]: where the fuck original dpkg?" >&2
        exit 1
    fi
fi

dpkgarch=$(dpkg.original --print-architecture)
force_install="--force-architecture"

I_N_T() {
    if ! install_name_tool "$@" ; then
        ldid -s "${!#}"
        install_name_tool "$@"
        return $?
    fi
    return 0
}

lsrpath() {
    otool -l "$@" |
    awk '
        /^[^ ]/ {f = 0}
        $2 == "LC_RPATH" && $1 == "cmd" {f = 1}
        f && gsub(/^ *path | \(offset [0-9]+\)$/, "") == 2
    ' | sort | uniq
}

get_target_arch() {
    echo "$dpkgarch"
}

handle_symlink_conflicts() {
    local pkg_name="$1"
    local deb_root="$2"

    echo "Checking for potential symlink conflicts in package: $pkg_name"

    find "$deb_root" -type l | while read symlink; do
        rel_path="${symlink#$deb_root/}"
        link_target=$(readlink "$symlink")
        normalized_target=$(realpath -m "$link_target" 2>/dev/null || echo "$link_target")
        source_dir=$(dirname "$rel_path")
        source_base=$(basename "$rel_path")
        target_dir=$(dirname "$normalized_target")
        target_base=$(basename "$normalized_target")
        if [ "$source_base" = "$target_base" ]; then
            case "$target_dir" in
                */"$source_dir")
                    echo "  Found merged directory symlink: $rel_path -> $link_target"
                    target_subdir="$deb_root/opt/local/$pkg_name/$(dirname "$rel_path")"
                    mkdir -p "$target_subdir"
                    mv "$symlink" "$deb_root/opt/local/$pkg_name/$rel_path"
                    echo "    Moved to: /opt/local/$pkg_name/$rel_path (preemptive)"
                    ;;
                *)
                    if [ -e "$jb/$rel_path" ] || [ -L "$jb/$rel_path" ]; then
                        echo "  Found conflicting symlink: $rel_path"
                        target_subdir="$deb_root/opt/local/$pkg_name/$(dirname "$rel_path")"
                        mkdir -p "$target_subdir"
                        
                        mv "$symlink" "$deb_root/opt/local/$pkg_name/$rel_path"
                        echo "    Moved to: /opt/local/$pkg_name/$rel_path"
                    fi
                    ;;
            esac
        fi
    done
}

convert_deb() {
    # set -x
    deb="$1"
    target_arch="$2"

    echo "Converting $deb to $target_arch ..."
    echo "AutoPatches: $([ "$AUTOPATCHES" = "1" ] && echo "enabled" || echo "disabled")"
    
    local TMPDIR="$tmp/convert/convert-$$" # 004
    mkdir -p "$TMPDIR/old" "$TMPDIR/new"
    dpkg-deb -R "$deb" "$TMPDIR/old"
    DEBIAN_DIR="$TMPDIR/old/DEBIAN"
    if [ ! -d "$DEBIAN_DIR" ]; then
        echo "Error: No DEBIAN directory found"
        rm -rf "$TMPDIR"
        return 1
    fi

    DEB_PACKAGE=$(grep '^Package:' "$DEBIAN_DIR/control" | cut -f2 -d ' ' | tr -d '\n\r')
    DEB_VERSION=$(grep '^Version:' "$DEBIAN_DIR/control" | cut -f2 -d ' ' | tr -d '\n\r')
    DEB_ARCH=$(grep '^Architecture:' "$DEBIAN_DIR/control" | cut -f2 -d ' ' | tr -d '\n\r')

    echo "Package: $DEB_PACKAGE"
    echo "Original Architecture: $DEB_ARCH"
    echo "Target Architecture: $target_arch"

    cp -r "$DEBIAN_DIR" "$TMPDIR/new/"

    case "$DEB_ARCH:$target_arch" in
        # rootful -> rootless
        "iphoneos-arm:iphoneos-arm64")
            echo "Converting rootful to rootless..."
            find "$TMPDIR/old" -mindepth 1 -maxdepth 1 -not -name "DEBIAN" | while read item; do
                mkdir -p "$TMPDIR/new/var/jb"
                cp -a "$item" "$TMPDIR/new/var/jb/"
            done
            ;;
        # rootful -> roothide
        "iphoneos-arm:iphoneos-arm64e")
            echo "Converting rootful to roothide..."
            find "$TMPDIR/old" -mindepth 1 -maxdepth 1 -not -name "DEBIAN" | while read item; do
                cp -a "$item" "$TMPDIR/new/"
            done
            ;;
        # rootless -> rootful
        "iphoneos-arm64:iphoneos-arm")
            echo "Converting rootless to rootful..."
            if [ -d "$TMPDIR/old/var/jb" ]; then
                cp -a "$TMPDIR/old/var/jb"/* "$TMPDIR/new/"
            fi

            if [ -d "$TMPDIR/old/var" ]; then
                find "$TMPDIR/old/var" -mindepth 1 -maxdepth 1 -not -name "jb" | while read item; do
                    mkdir -p "$TMPDIR/new/var"
                    cp -a "$item" "$TMPDIR/new/var/"
                done
            fi

            find "$TMPDIR/old" -mindepth 1 -maxdepth 1 -not -name "DEBIAN" -not -name "var" | while read item; do
                cp -a "$item" "$TMPDIR/new/"
            done
            ;;

        # rootless -> roothide
        "iphoneos-arm64:iphoneos-arm64e")
            echo "Converting rootless to roothide..."
            if [ -d "$TMPDIR/old/var/jb" ]; then
                cp -a "$TMPDIR/old/var/jb"/* "$TMPDIR/new/"
            fi

            if [ -d "$TMPDIR/old/var" ]; then
                find "$TMPDIR/old/var" -mindepth 1 -maxdepth 1 -not -name "jb" | while read item; do
                    mkdir -p "$TMPDIR/new/rootfs/var"
                    cp -a "$item" "$TMPDIR/new/rootfs/var/"
                done
            fi

            find "$TMPDIR/old" -mindepth 1 -maxdepth 1 -not -name "DEBIAN" -not -name "var" | while read item; do
                mkdir -p "$TMPDIR/new/rootfs"
                cp -a "$item" "$TMPDIR/new/rootfs/"
            done
            ;;
        # roothide -> rootful
        "iphoneos-arm64e:iphoneos-arm")
            echo "Converting roothide to rootful..."
            find "$TMPDIR/old" -mindepth 1 -maxdepth 1 -not -name "DEBIAN" -not -name "rootfs" | while read item; do
                cp -a "$item" "$TMPDIR/new/"
            done

            if [ -d "$TMPDIR/old/rootfs" ]; then
                cp -a "$TMPDIR/old/rootfs"/* "$TMPDIR/new/"
            fi
            ;;
        # roothide -> rootless
        "iphoneos-arm64e:iphoneos-arm64")
            echo "Converting roothide to rootless..."
            find "$TMPDIR/old" -mindepth 1 -maxdepth 1 -not -name "DEBIAN" -not -name "rootfs" | while read item; do
                mkdir -p "$TMPDIR/new/var/jb"
                cp -a "$item" "$TMPDIR/new/var/jb/"
            done

            if [ -d "$TMPDIR/old/rootfs" ]; then
                cp -a "$TMPDIR/old/rootfs"/* "$TMPDIR/new/"
            fi
            ;;
        # 相同架構
        "$DEB_ARCH:$target_arch")
            echo "Same architecture, copying directly..."
            find "$TMPDIR/old" -mindepth 1 -maxdepth 1 -not -name "DEBIAN" | while read item; do
                cp -a "$item" "$TMPDIR/new/"
            done
            ;;
        *)
            echo "Unsupported conversion: $DEB_ARCH -> $target_arch"
            rm -rf "$TMPDIR"
            return 1
            ;;
    esac
    # deal with Mach-O file
    find "$TMPDIR/new" -type f -not -path "*/DEBIAN/*" | while read file; do
        if file -b "$file" 2>/dev/null | grep -q "Mach-O"; then
            echo "Patching: $file"
            case "$target_arch" in
                "iphoneos-arm64"|"iphoneos-arm64e")
                    # deal with rpath
                    lsrpath "$file" 2>/dev/null | while read line; do
                        case "$line" in
                            /var/jb/*)
                                newpath=$(echo "$line" | sed 's|/var/jb/|@loader_path/.jbroot/|')
                                I_N_T -rpath "$line" "$newpath" "$file" 2>/dev/null
                                ;;
                        esac
                    done
                    # deal with library path
                    otool -L "$file" 2>/dev/null | tail -n +2 | cut -d' ' -f1 | tr -d "[:blank:]" | while read line; do
                        case "$line" in
                            /var/jb/*)
                                newlib=$(echo "$line" | sed 's|/var/jb/|@loader_path/.jbroot/|')
                                I_N_T -change "$line" "$newlib" "$file" 2>/dev/null
                                ;;
                            /usr/lib/*|/Library/Frameworks/*)
                                if [ "$target_arch" = "iphoneos-arm64e" ]; then
                                    I_N_T -change "$line" "@rpath/${line#/usr/lib/}" "$file" 2>/dev/null
                                fi
                                ;;
                        esac
                    done
                    # add rpath (roothide)
                    if [ "$target_arch" = "iphoneos-arm64e" ]; then
                        I_N_T -add_rpath "/usr/lib" "$file" 2>/dev/null
                        I_N_T -add_rpath "@loader_path/.jbroot/usr/lib" "$file" 2>/dev/null
                        I_N_T -add_rpath "/Library/Frameworks" "$file" 2>/dev/null
                        I_N_T -add_rpath "@loader_path/.jbroot/Library/Frameworks" "$file" 2>/dev/null
                    fi
                    ;;
            esac

            file_dir=$(dirname "$file")
            current_dir="$file_dir"
            # find .jbroot, until / or /var/jb
            while [ "$current_dir" != "$jb/" ] && [ "$current_dir" != "$TMPDIR/new" ]; do
                parent_dir=$(dirname "$current_dir")
                # check up path is it .jbroot
                if [ -e "$parent_dir/.jbroot" ]; then
                    # calculate relative path
                    rel_path="../"
                    depth=$(echo "$current_dir" | tr '/' '\n' | grep -c "^" || true)
                    parent_depth=$(echo "$parent_dir" | tr '/' '\n' | grep -c "^" || true)
                    levels=$((depth - parent_depth))
                    rel_path=""
                    for i in $(seq 1 $levels); do
                        rel_path="${rel_path}../"
                    done
                    ln -sf "${rel_path}.jbroot" "$current_dir/.jbroot" 2>/dev/null
                    echo "  Created .jbroot symlink in $current_dir"
                    break
                fi
                current_dir="$parent_dir"
            done
            # if AutoPatches = 1, make link
            if [ "$AUTOPATCHES" = "1" ]; then
                # judge whether the document is executable
                if echo "$(file -b "$file")" | grep -q "Mach-O"; then
                    ln -sf /usr/lib/DynamicPatches/AutoPatches.dylib "$file.roothidepatch"
                fi
            fi
            ldid -S "$file" 2>/dev/null
        fi
    done

    sed -i 's/'$DEB_ARCH'/'$target_arch'/g' "$TMPDIR/new/DEBIAN/control"
    handle_symlink_conflicts "$DEB_PACKAGE" "$TMPDIR/new"
    # dpkg-deb -b
    echo "Repacking deb..."
    if [ -d "$TMPDIR/new/DEBIAN" ] && [ -f "$TMPDIR/new/DEBIAN/control" ]; then
        # backup
        mv "$deb" "${deb}.backup" 2>/dev/null
        dpkg-deb -Zzstd -b "$TMPDIR/new" "$deb"
        if [ $? -eq 0 ]; then
            echo "Conversion complete: $deb"
            rm -f "${deb}.backup" 2>/dev/null
        else
            echo "Repack failed, restoring backup"
            mv "${deb}.backup" "$deb" 2>/dev/null
        fi
    else
        echo "Error: Invalid package structure"
    fi

    rm -rf "$TMPDIR" # 005
}

show_usage() {
    local show_help="\
usage: dpkg-force-install [options] [deb_file]
options:
  -i, --install <deb>          Install deb (auto-convert to current environment)
  --to-rootful <deb>           Convert to rootful (iphoneos-arm)
  --to-rootless <deb>          Convert to rootless (iphoneos-arm64)
  --to-roothide <deb>          Convert to roothide (iphoneos-arm64e)
  --to-autoPatches <0/1>       acquiesced 1
  -h, --help                   Show this help and dpkg help

"
    printf "$show_help"
    $dpkg_original --help
}

force_opts=""
while [ $# -gt 0 ]; do
    case "$1" in
        --force-*)
            force_opts="$force_opts $1"
            shift
            ;;
        *)
            break
            ;;
    esac
done

case "$1" in
    -i|--install)
        shift
        target_arch=$(get_target_arch)
        for deb in "$@"; do
            if echo "$deb" | grep -q "\.deb$" && [ -f "$deb" ]; then
                convert_deb "$deb" "$target_arch"
                $dpkg_original $force_install $force_opts -i "$deb"
            else
                echo "Skipping non-deb file: $deb"
            fi
        done
        ;;
    --to-rootful)
        shift
        for deb in "$@"; do
            if echo "$deb" | grep -q "\.deb$" && [ -f "$deb" ]; then
                convert_deb "$deb" "iphoneos-arm"
            fi
        done
        ;;
    --to-rootless)
        shift
        for deb in "$@"; do
            if echo "$deb" | grep -q "\.deb$" && [ -f "$deb" ]; then
                convert_deb "$deb" "iphoneos-arm64"
            fi
        done
        ;;
    --to-roothide)
        shift
        for deb in "$@"; do
            if echo "$deb" | grep -q "\.deb$" && [ -f "$deb" ]; then
                convert_deb "$deb" "iphoneos-arm64e"
            fi
        done
        ;;
    --help) show_usage ;;
    --to-autoPatches) AUTOPATCHES=$2 ;;
    *)
        $dpkg_original "$@"
        ;;
esac