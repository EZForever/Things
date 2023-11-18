#!/bin/bash

# Check whether we're sourced to make sure that `return` works
if [[ "${BASH_SOURCE[0]}" -ef "$0" ]]; then
    echo 'This script should be sourced, not executed.' 1>&2
    exit 1
fi

# Disable fancy prompt if on a native Linux VT, or by set PS1_OFF manually
if [[ $TERM == "linux" && -n $PS1_OFF ]]; then
    #unset PS1_OFF
    return 0
fi

__PS1_FG_TEXT='\e[37;1m' # Text color
__PS1_FG_TEXT2='\e[38;5;246m' # Secondary text color
__PS1_FG_SEP='\e[38;5;240m' # Separator color

__PS1_BG='\e[48;5;234m' # Common background
__PS1_BG_EOL='\e[48;5;233m' # Shadow after a non-eol command output

__PS1_RESET='\e[0m' # Clear all colors
__PS1_RESET_BG="$__PS1_RESET$__PS1_BG" # Clear all foreground colors
__PS1_REFRESH='\e[K' # Force buffer to terminal

__PS1_ERR_OFF='🔹' # Error indicator
__PS1_ERR_ON='🔸'

#__PS1_SEP='◊' # Separator
#__PS1_SEP='»' # Separator
__PS1_SEP='❯' # Separator

# If last command's output don't end with a newline, add a shadow to rest of this line
function __ps1_eol {
    local pos
    read -s -p $'\e[6n' -d R pos
    [[ "$pos" == *";1" ]] && __PS1_EOL="" || __PS1_EOL="$__PS1_BG_EOL$__PS1_REFRESH\n$__PS1_RESET$__PS1_REFRESH"
}

# Error indicator
function __ps1_errno {
    (($1)) && __PS1_ERRNO="$__PS1_ERR_ON" || __PS1_ERRNO="$__PS1_ERR_OFF"
}

# Shell level indicator
function __ps1_shlvl {
    [[ $SHLVL -gt 1 ]] && __PS1_SHLVL="+$SHLVL " || __PS1_SHLVL=""
}

# 'Construct' MSYSTEM to repersent chroot ($debian_chroot) and virtualenv ($VIRTUAL_ENV)
# DEBIAN:/mnt/newroot (pytorch)
function __ps1_venv {
    local venv="$__PS1_FG_TEXT$MSYSTEM$__PS1_RESET_BG"
    venv+="${debian_chroot:+$__PS1_FG_TEXT2:$debian_chroot$__PS1_RESET_BG}"
    venv+="${VIRTUAL_ENV:+$__PS1_FG_TEXT2 (`basename $VIRTUAL_ENV`)$__PS1_RESET_BG}"
    __PS1_VENV="$venv"
}

# Collapse long paths in the middle
# Before: /usr/include/x86_64-linux-gnu/c++/6.3.0/bits
# After : /usr/include/x86…gnu/c++/6.3.0/bits
function __ps1_pwd {
    if [[ "$PWD" == "/" ]]; then
        echo -n "/"
        return
    fi

    local homepwd='\w'
    local homepwd="${homepwd@P}"

    local IFS='/'
    local elems=(${homepwd})
    local len=${#elems[@]}

    local i
    for ((i=0; i<len-1; i++)); do
        local elem=${elems[$i]}
        if [[ ${#elem} -gt 7 ]]; then
            elems[$i]=${elem:0:3}…${elem:0-3}
        fi
    done

    local IFS='/'
    __PS1_PWD="$__PS1_FG_TEXT${elems[*]}$__PS1_RESET_BG"
}

# Collapse paths to only first two and last element for window title
# Before: /usr/include/x86_64-linux-gnu/c++/6.3.0/bits
# After : /usr/include/…/bits
function __ps1_pwd2 {
    local homepwd='\w'
    local homepwd="${homepwd@P}"

    local IFS='/'
    local elems=(${homepwd})
    local len=${#elems[@]}
    
    if [[ $len -le 4 ]]; then
        __PS1_PWD2="$homepwd"
    else
        __PS1_PWD2="${elems[0]}/${elems[1]}/${elems[2]}/…/${elems[0-1]}"
    fi
}

function __ps1 {
    local errno=$?
    
    __ps1_eol #; echo -ne "$__PS1_EOL"
    __ps1_errno $errno
    __ps1_shlvl
    __ps1_venv
    __ps1_pwd
    __ps1_pwd2
    
    return $errno
}

function __ps1_init {
    # Using a MinGW concept
    if [[ -z $MSYSTEM ]]; then
        MSYSTEM=${WSL_DISTRO_NAME:-$(lsb_release -is)}
        MSYSTEM=${MSYSTEM@U}
    fi

    # Cramming all processing in here so not to spawn subshells
    # Also, EOL thing has to be dealt before any text can be displayed
    PROMPT_COMMAND="__ps1 ${PROMPT_COMMAND:+; $PROMPT_COMMAND}"
    
    # Disable venv from changing the prompt
    # Otherwise __PS1_EOL will apply too late and requires hacking in __ps1 with \r
    VIRTUAL_ENV_DISABLE_PROMPT=1
    
    # ---
    
    # Window title
    # NOTE: Not needed on actual linux env with bashrc, but needed on MinGW
    #PS1='\[\e]0;\u@\h: \w\a\]'
    PS1='\[\e]0;$MSYSTEM: $__PS1_PWD2\a\]'
    
    # EOL indicator
    PS1+='\[${__PS1_EOL@E}\]'
    
    # Make sure the prompt start from the beginning (for masking out the venv prefix)
    #PS1+='\r'
    
    # Set common background
    PS1+="\[$__PS1_RESET_BG\]"
    
    # Error indicator
    PS1+='${__PS1_ERRNO@E}'

    # Shell level indicator
    PS1+='${__PS1_SHLVL@E}'

    # Separator
    # Current error indicator is a full-width character, so a space is removed for better appearence
    PS1+="\[$__PS1_FG_SEP\]$__PS1_SEP\[$__PS1_RESET_BG\] "
    
    # Environment
    PS1+='${__PS1_VENV@E}'
    
    # Separator
    PS1+=" \[$__PS1_FG_SEP\]$__PS1_SEP\[$__PS1_RESET_BG\] "
    
    # pwd
    PS1+='${__PS1_PWD@E}'
    
    # Newline and clear colors
    #PS1+="$__PS1_REFRESH\n$__PS1_RESET$__PS1_REFRESH"
    
    PS1+=' '
    
    # Just newline
    PS1+="$__PS1_RESET\n"
    
    # prompt
    PS1+='\$ '
}

__ps1_init

