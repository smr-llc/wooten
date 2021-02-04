#!/bin/bash

# -M 1 -G 0 -H -6 --pga-gain-right 10 --pga-gain-left 10

~/Bela/scripts/build_project.sh ~/wooten --force \
-c "-p 64 -M 1 -G 0 -H -4 --pga-gain-right 10 --pga-gain-left 10" \
-m 'CPPFLAGS="-fno-strict-aliasing -D_FORTIFY_SOURCE=2 -g0 -fstack-protector-strong -Wformat -Werror=format-security -DNDEBUG -fwrapv -O3 -Wall -Wstrict-prototypes" LDLIBS="-lliquid -lfftw3f -lfftw3f_threads -ldl -lutil -lm -Xlinker -export-dynamic -Wl,-O2 -Wl,-Bsymbolic-functions"'

