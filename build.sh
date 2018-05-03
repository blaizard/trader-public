#!/bin/bash

CMAKE_DIR=build
CMAKE_BUILD_TYPE=debug
CMAKE_GENERATOR='Sublime Text 2 - Unix Makefiles'

FILE_NAME=`basename "$0"`
FILE_FULLPATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/${FILE_NAME}"
PROJECT_FULLPATH=`dirname "${FILE_FULLPATH}"`

CMD_PREFIX=
CMD_SUFFIX=

function usage
{
cat <<EOF
Usage: ${FILE_NAME} [options] [type]

options:
	-h, -?        Print this help, then exit.
	-s            Setup a project.
	-c            Clean.
	-d            Generate documentation.
	-b <path>     Backup the whole project to a specific path. Transfer the increment
	              to the remote directory and create an archive dated only if there are
	              changes from the previous backup.
	-v            Use Valgrind to check for memory leaks.
	-e <command>  Run the command endlessly (break if the program fails).
	-m            Stop the output when reached the full screen, use more to scroll down.

type:
	debug (default)
	release

example:
	`basename "$0"` -e ./build/debug/bin/tests
EOF
}

while getopts "h?e:scvb:md" opt; do
	case "$opt" in
	h|\?)
		usage
		exit 0
		;;
	d)
		;;
	s)
		rm -rfd "$CMAKE_DIR"
		CUR_DIR=`pwd`
		# Create the git directories
		git_path=$(find . -name '.gitmodules')
		for path in $git_path; do
			cd "$CUR_DIR"
			cd "$(dirname $path)"
			# If no git directory is present, create it
			if [ ! -d ".git" ]; then
				git init
				# Update the index with submodules
				git config -f .gitmodules --get-regexp '^submodule\..*\.path$' |
				while read path_key path
				do
					url_key=$(echo $path_key | sed 's/\.path/.url/')
					url=$(git config -f .gitmodules --get "$url_key")
					git submodule add -f $url $path
				done
			fi
		done
		git submodule update --init --recursive
		cd "${CUR_DIR}" && mkdir -p "$CMAKE_DIR/debug" && cd "$CMAKE_DIR/debug" && cmake -G "$CMAKE_GENERATOR" -DCMAKE_BUILD_TYPE=Debug ../..
		cd "${CUR_DIR}" && mkdir -p "$CMAKE_DIR/release" && cd "$CMAKE_DIR/release" && cmake -G "$CMAKE_GENERATOR" -DCMAKE_BUILD_TYPE=Release ../..
		# Create a config.hpp if it does not exists
		if [ ! -e "${PROJECT_FULLPATH}/config.hpp" ]; then
			cp "${PROJECT_FULLPATH}/config.example.hpp" "${PROJECT_FULLPATH}/config.hpp"
		fi
		exit 0
		;;
	c)
		rm -rfd "$CMAKE_DIR/bin" && mkdir -p "$CMAKE_DIR/bin"
		rm -rfd "$CMAKE_DIR/lib" && mkdir -p "$CMAKE_DIR/lib"
		exit 0
		;;

	b)
		mkdir -p "${OPTARG}"
		BACKUP_FULLPATH=`cd "${OPTARG}" && pwd`
		echo "Backup to '${BACKUP_FULLPATH}'..."
		raw_output=`rsync -icrzl --inplace --stats --filter=':- .gitignore' --exclude='.git' --delete "${PROJECT_FULLPATH}" "${BACKUP_FULLPATH}"`
		nb_files=`echo "$raw_output" | awk '/Number of .* files|Number of files transferred/{print $NF}' | awk '{s+=$1} END {printf "%.0f", s}'`
		echo "Number of files modified: ${nb_files}"

		if [ "${nb_files}" -ne "0" ]; then
			DATE=`date +%Y-%m-%d-%H%M%S`
			BACKUP_DIR="${BACKUP_FULLPATH}/`basename ${PROJECT_FULLPATH}`"
			echo "Backup '${BACKUP_DIR}' -> ${DATE}.zip"
			zip -qr "${OPTARG}/${DATE}.zip" "${BACKUP_DIR}"
		fi
		exit 1
		;;
	v)
		CMD_PREFIX='valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes'
		;;
	m)
		CMD_SUFFIX='2>&1 | more'
		;;
	e)
		shift
		while ${CMD_PREFIX} $@; do :; done
		exit 1
		;;
	esac

done

shift $((OPTIND-1))
[ "$1" = "--" ] && shift

if [ "$#" -eq "1" ]; then
	CMAKE_BUILD_TYPE=$1
elif [ "$?" -gt "1" ]; then
	usage
	exit 1
fi

# Build the program
echo "Build type: '$CMAKE_BUILD_TYPE'"
if [ ! -f "${CMAKE_DIR}/.buildtype" ] || [ ! "`cat "${CMAKE_DIR}/.buildtype"`" == "$CMAKE_BUILD_TYPE" ]; then
	${FILE_FULLPATH} -c
fi
echo -n "$CMAKE_BUILD_TYPE" > "${CMAKE_DIR}/.buildtype"
eval "cmake --build \"$CMAKE_DIR/$CMAKE_BUILD_TYPE\" -- -j3 ${CMD_SUFFIX}"
