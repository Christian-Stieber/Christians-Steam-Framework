#! /bin/sh

TEMPFILENAME=/tmp/Steam-Framework-Tool-`id --user`-Preamble.cpp
cat >$TEMPFILENAME <<EOF
/*
 * This file is part of "Christians-Steam-Framework"
 * Copyright (C) 2023- Christian Stieber
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.  If not see
 * <http://www.gnu.org/licenses/>.
 */

EOF

head -n `wc -l <$TEMPFILENAME` "$1" | cmp -s - $TEMPFILENAME
case $? in
    0)
        exit 0
        ;;
    1)
        mv "$1" "$1.tmp" || exit 1
        cat $TEMPFILENAME "$1.tmp" > "$1"
        rm -f "$1.tmp"
        exit 0
        ;;
esac
exit 1
