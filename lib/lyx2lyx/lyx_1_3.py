# This file is part of lyx2lyx
# -*- coding: utf-8 -*-
# Copyright (C) 2002 Dekel Tsur <dekel@lyx.org>
# Copyright (C) 2004 José Matos <jamatos@lyx.org>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

""" Convert files to the file format generated by lyx 1.3"""

import re
from parser_tools import find_token, find_end_of, get_value,\
                         find_token_exact

####################################################################
# Private helper functions

def find_end_of_inset(lines, i):
    "Finds the matching \end_inset"
    return find_end_of(lines, i, "\\begin_inset", "\\end_inset")


def del_token(lines, token, start, end):
    """ del_token(lines, token, start, end) -> int

    Find the lower line in lines where token is the first element and
    delete that line.

    Returns the number of lines remaining."""

    k = find_token_exact(lines, token, start, end)
    if k == -1:
        return end
    else:
        del lines[k]
        return end - 1

# End of helper functions
####################################################################


def change_insetgraphics(document):
    " Change inset Graphics."
    lines = document.body
    i = 0
    while 1:
        i = find_token(lines, "\\begin_inset Graphics", i)
        if i == -1:
            break
        j = find_end_of_inset(lines, i)

        lines[i] = "\\begin_inset Graphics"

        if get_value(lines, "display", i, j) == "default":
            j = del_token(lines, "display", i, j)
        if get_value(lines, "rotateOrigin", i, j) == "leftBaseline":
            j = del_token(lines, "rotateOrigin", i, j)

        k = find_token_exact(lines, "rotate", i, j)
        if k != -1:
            del lines[k]
            j = j-1
        else:
            j = del_token(lines, "rotateAngle", i, j)

        k = find_token_exact(lines, "size_type", i, j)
        if k == -1:
            k = find_token_exact(lines, "size_kind", i, j)
        if k != -1:
            size_type = lines[k].split()[1]
            del lines[k]
            j = j-1
            if size_type in ["0", "original"]:
                j = del_token(lines, "width", i, j)
                j = del_token(lines, "height", i, j)
                j = del_token(lines, "scale", i, j)
            elif size_type in ["2", "scale"]:
                j = del_token(lines, "width", i, j)
                j = del_token(lines, "height", i, j)
                if get_value(lines, "scale", i, j) == "100":
                    j = del_token(lines, "scale", i, j)
            else:
                j = del_token(lines, "scale", i, j)

        k = find_token_exact(lines, "lyxsize_type", i, j)
        if k == -1:
            k = find_token_exact(lines, "lyxsize_kind", i, j)
        if k != -1:
            lyxsize_type = lines[k].split()[1]
            del lines[k]
            j = j-1
            j = del_token(lines, "lyxwidth", i, j)
            j = del_token(lines, "lyxheight", i, j)
            if lyxsize_type not in ["2", "scale"] or \
               get_value(lines, "lyxscale", i, j) == "100":
                j = del_token(lines, "lyxscale", i, j)

        i = i+1


def change_tabular(document):
    " Change tabular."
    lines = document.body
    i = 0
    while 1:
        i = find_token(lines, "<column", i)
        if i == -1:
            break
        if not re.search('width="0pt"', lines[i]):
            lines[i] = re.sub(' alignment=".*?"',' alignment="block"',lines[i])
        i = i+1


supported_versions = ["1.3.%d" % i for i in range(8)] + ["1.3"]
convert = [[221, [change_insetgraphics, change_tabular]]]
revert  = []


if __name__ == "__main__":
    pass
