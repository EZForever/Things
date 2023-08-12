#!/bin/bash
# litclock_annotated.csv from https://github.com/mmattozzi/LiteraryClockScreenSaver
# litclock_annotated_improved.csv from https://github.com/zenbuffy/LiteraryClock

function litclock {
    local tm=${1:-$(date +%H:%M)}
    local db=${2:-$(dirname ${BASH_SOURCE[0]})/litclock_annotated_improved.csv}
    
    local IFS='|'
    local entry=($(grep "^$tm" "$db" | shuf -n 1))
    if [[ -z $entry ]]; then
        echo -e "It is \e[32m$tm\e[0m, a sad minute without its own quote."
        return 1
    fi
    
    local i
    for i in ${!entry[@]}; do
        local item="${entry[$i]}"
        if [[ $item == '"'*'"' ]]; then
            item=${item:1:${#item}-2}
            item=${item//\"\"/\"}
        fi
        item=${item## }
        item=${item%% }
        entry[$i]=$item
    done
    
    local timestr=${entry[1]}
    local quote=${entry[2]}
    local reference=$'\e[3m'"${entry[3]}"$'\e[23m'
    local author=${entry[4]}
    local cite="-- $reference, $author"
    
    # NOTE: Do not use ${x%%} and ${x#} since they do not respect nocasematch
    shopt -s nocasematch
    local quote_l=${quote/$timestr*}
    quote=$quote_l$'\e[32m'${quote:${#quote_l}:${#timestr}}$'\e[0m'${quote:${#quote_l}+${#timestr}}
    shopt -u nocasematch
    
    # NOTE: That additional 9 is from ANSI sequences in $cite
    printf "%s\n\e[1;30m%*s\e[0m\n" "$quote" $(($COLUMNS + 9)) "$cite"
    return 0
}

litclock $*

