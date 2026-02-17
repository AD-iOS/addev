#ifndef AD_STD_AUX_SH
#define AD_STD_AUX_SH

alias 'source'='.'
__SH_MODULES=""

_extract_macro_name() {
    file="$1"
    macro_name=$(awk '
        BEGIN { found = 0 }
        
        /^[[:space:]]*#[[:space:]]*ifndef[[:space:]]+/ {
            if (match($0, /#[[:space:]]*ifndef[[:space:]]+([[:alpha:]_-][[:alnum:]_-]*)/, arr)) {
                macro = arr[1]
                found = 1
                next
            }
        }
        
        found && /^[[:space:]]*#[[:space:]]*define[[:space:]]+/ {
            if (match($0, /#[[:space:]]*define[[:space:]]+([[:alpha:]_-][[:alnum:]_-]*)/, arr)) {
                if (arr[1] == macro) {
                    print macro
                    exit 0
                } else {
                    found = 0
                }
            }
        }

        found && !/^[[:space:]]*$/ && !/^[[:space:]]*#/ && !/^[[:space:]]*\/\// && !/^[[:space:]]*\/\*/ {
            found = 0
        }
        
        END { exit 1 }
    ' "$file" 2>/dev/null)
    
    if [ -n "$macro_name" ]; then
        echo "$macro_name"
        return 0
    fi
    return 1
}

include() {
    for file in "$@"; do
        if [ ! -f "$file" ]; then
            echo "[Error] : $file It doesn't exist" >&2
            return 1
        fi

        macro_name=$(_extract_macro_name "$file")
        
        if [ -n "$macro_name" ]; then
            case " $__SH_MODULES " in
                *" $macro_name "*)
                    continue
                    ;;
            esac
            __SH_MODULES="$__SH_MODULES $macro_name "
        fi

        . "$file" || {
            echo "[Error] Failed to load: $file" >&2
            return 1
        }
    done
}

__SH_MODULES=""

#endif