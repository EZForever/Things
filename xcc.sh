#!/bin/bash
# usage: xcc [lineno = 1] <script> [args...]
# use $__ for script path in shebang lines

shopt -s extglob

# assume lineno exists if the 1st argument is a valid positive number ...
if [ $1 -gt 0 ] 2>/dev/null; then
	lineno=$1
	shift
fi

# ... and default to 1 if otherwise
let ${lineno:=1}

# script path and sanity check
script="$1"
shift
if [ ! -f "$script" ]; then
	echo "$0: not found: script $script" 1>&2
	exit 1
fi

# read the selected shebang line
for (( i = 0; i < $lineno; i++ )); do
	read line
done < $script

# remove possible shebang markers
case "${line:0:2}" in
'#!')
	real_shebang=1
	line="${line:2}"
	;;
'//' | ';;')
	line="${line:2}"
	;;
esac

# strip whitespaces
line="${line##*([[:blank:]])}"
line="${line%%*([[:blank:]])}"

# more sanity check
if [ -z "$line" ]; then
	echo "$0: invalid shebang line: script $script, line $lineno" 1>&2
	exit 1
fi

# print out shebang line for inspection
[ -n "$real_shebang" ] && line="$line $script"
shebang="$line $*"
printf "\e[7m$ %s\e[27m\n" "$shebang" 1>&2

# actually run the shebang line and collect result
__="$script" /bin/bash -c "$shebang"
errno=$?

# deal with incomplete output line
read -s -p $'\e[6n' -d R linepos
[[ "$linepos" == *";1" ]] || echo -ne "\e[48;5;233m\e[K\n\e[0m\e[K" 1>&2

# show exit code
printf "\e[7m[ Process exited with code %d (0x%02x) ]\e[27m\n" $errno $errno 1>&2
exit $errno

