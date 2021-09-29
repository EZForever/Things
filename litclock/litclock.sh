#!/bin/bash
# litclock_annotated.csv from https://github.com/mmattozzi/LiteraryClockScreenSaver
# litclock_annotated_improved.csv from https://github.com/zenbuffy/LiteraryClock

function litclock {
    local tm=${1:-$(date +%H:%M)}
    local db=${2:-$(dirname ${BASH_SOURCE[0]})"/litclock_annotated_improved.csv"}
    
    local IFS='|'
    local entry=($(grep "^$tm" "$db" | shuf -n 1))
    if [[ -z $entry ]]; then
        echo -e "It is \e[32m$tm\e[0m, a sad minute without its own quote."
        return 1
    fi
    
    local quote=${entry[2]//${entry[1]}/$'\e[32m'${entry[1]}$'\e[0m'}
    local cite="-- \"${entry[3]}\", ${entry[4]}"
    
    printf "%s\n\e[1;30m%80s\e[0m\n" "$quote" "$cite"
    return 0
}

litclock $*
echo

