#!/bin/bash

# -M 1 -G 0 -H -6 --pga-gain-right 10 --pga-gain-left 10

PROJECT_ROOT="~"
while [ -n "$1" ]
do
	case $1 in
		-a)
			shift;
			export BBB_HOSTNAME="$1";
		;;
        -p)
			shift;
			PROJECT_ROOT="$1";
		;;
	esac
	shift
done

"$PROJECT_ROOT/Bela/scripts/build_project.sh" "$PROJECT_ROOT/wooten" --force \
-c "-p 64 -M 1 -G 0 -H -4 --pga-gain-right 5 --pga-gain-left 5" \
-m 'CPPFLAGS="-fno-strict-aliasing -D_FORTIFY_SOURCE=2 -g0 -fstack-protector-strong -Wformat -Werror=format-security -DNDEBUG -fwrapv -O3 -Wall -Wstrict-prototypes" LDLIBS="-lliquid -lfftw3f -lfftw3f_threads -ldl -lutil -lm -Xlinker -export-dynamic -Wl,-O2 -Wl,-Bsymbolic-functions"'

