#!/bin/bash
# litclock_annotated.csv from https://github.com/mmattozzi/LiteraryClockScreenSaver
# litclock_annotated_improved.csv from https://github.com/zenbuffy/LiteraryClock

currln=$(grep "^$(date +%H:%M)" litclock_annotated_improved.csv | shuf -n 1)
if [[ -z $currln ]]; then
    echo -e "It is \e[32m$(date +%H:%M)\e[0m, a sad minute without its own quote."
    exit 1
fi
hilight=$(echo "$currln" | awk -F '|' '{ print $2 }')
hilighte=$(echo -e "\e[32m$hilight\e[0m")
quote=$(echo "$currln" | awk -F '|' '{ print $3 }' | sed "s/$hilight/_/g" | sed "s/_/$hilighte/g")
cite=$(echo "$currln" | awk -F '|' '{ print $4 }')
author=$(echo "$currln" | awk -F '|' '{ print $5 }')

echo $quote
echo
echo -- $cite, $author
exit 0
