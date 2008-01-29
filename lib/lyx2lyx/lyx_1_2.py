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

""" Convert files to the file format generated by lyx 1.2"""

import re

from parser_tools import find_token, find_token_backwards, \
                         find_tokens, find_tokens_backwards, \
                         find_beginning_of, find_end_of, find_re, \
                         is_nonempty_line, find_nonempty_line, \
                         get_value, check_token

####################################################################
# Private helper functions

def get_layout(line, default_layout):
    " Get layout, if empty return the default layout."
    tokens = line.split()
    if len(tokens) > 1:
        return tokens[1]
    return default_layout


def get_paragraph(lines, i, format):
    " Finds the paragraph that contains line i."
    begin_layout = "\\layout"

    while i != -1:
        i = find_tokens_backwards(lines, ["\\end_inset", begin_layout], i)
        if i == -1: return -1
        if check_token(lines[i], begin_layout):
            return i
        i = find_beginning_of_inset(lines, i)
    return -1


def get_next_paragraph(lines, i, format):
    " Finds the paragraph after the paragraph that contains line i."
    tokens = ["\\begin_inset", "\\layout", "\\end_float", "\\the_end"]

    while i != -1:
        i = find_tokens(lines, tokens, i)
        if not check_token(lines[i], "\\begin_inset"):
            return i
        i = find_end_of_inset(lines, i)
    return -1


def find_beginning_of_inset(lines, i):
    " Find beginning of inset, where lines[i] is included."
    return find_beginning_of(lines, i, "\\begin_inset", "\\end_inset")


def find_end_of_inset(lines, i):
    " Finds the matching \end_inset"
    return find_end_of(lines, i, "\\begin_inset", "\\end_inset")


def find_end_of_tabular(lines, i):
    " Finds the matching end of tabular."
    return find_end_of(lines, i, "<lyxtabular", "</lyxtabular")


def get_tabular_lines(lines, i):
    " Returns a lists of tabular lines."
    result = []
    i = i+1
    j = find_end_of_tabular(lines, i)
    if j == -1:
        return []

    while i <= j:
        if check_token(lines[i], "\\begin_inset"):
            i = find_end_of_inset(lines, i)+1
        else:
            result.append(i)
            i = i+1
    return result

# End of helper functions
####################################################################


floats = {
    "footnote": ["\\begin_inset Foot",
                 "collapsed true"],
    "margin":   ["\\begin_inset Marginal",
                 "collapsed true"],
    "fig":      ["\\begin_inset Float figure",
                 "wide false",
                 "collapsed false"],
    "tab":      ["\\begin_inset Float table",
                 "wide false",
                 "collapsed false"],
    "alg":      ["\\begin_inset Float algorithm",
                 "wide false",
                 "collapsed false"],
    "wide-fig": ["\\begin_inset Float figure",
                 "wide true",
                 "collapsed false"],
    "wide-tab": ["\\begin_inset Float table",
                 "wide true",
                 "collapsed false"]
}

font_tokens = ["\\family", "\\series", "\\shape", "\\size", "\\emph",
               "\\bar", "\\noun", "\\color", "\\lang", "\\latex"]

pextra_type3_rexp = re.compile(r".*\\pextra_type\s+3")
pextra_rexp = re.compile(r"\\pextra_type\s+(\S+)"+\
                         r"(\s+\\pextra_alignment\s+(\S+))?"+\
                         r"(\s+\\pextra_hfill\s+(\S+))?"+\
                         r"(\s+\\pextra_start_minipage\s+(\S+))?"+\
                         r"(\s+(\\pextra_widthp?)\s+(\S*))?")


def get_width(mo):
    " Get width from a regular expression. "
    if mo.group(10):
        if mo.group(9) == "\\pextra_widthp":
            return mo.group(10)+"col%"
        else:
            return mo.group(10)
    else:
        return "100col%"


def remove_oldfloat(document):
    " Change \begin_float .. \end_float into \begin_inset Float .. \end_inset"
    lines = document.body
    i = 0
    while 1:
        i = find_token(lines, "\\begin_float", i)
        if i == -1:
            break
        # There are no nested floats, so finding the end of the float is simple
        j = find_token(lines, "\\end_float", i+1)

        floattype = lines[i].split()[1]
        if not floats.has_key(floattype):
            document.warning("Error! Unknown float type " + floattype)
            floattype = "fig"

        # skip \end_deeper tokens
        i2 = i+1
        while check_token(lines[i2], "\\end_deeper"):
            i2 = i2+1
        if i2 > i+1:
            j2 = get_next_paragraph(lines, j + 1, document.format + 1)
            lines[j2:j2] = ["\\end_deeper "]*(i2-(i+1))

        new = floats[floattype]+[""]

        # Check if the float is floatingfigure
        k = find_re(lines, pextra_type3_rexp, i, j)
        if k != -1:
            mo = pextra_rexp.search(lines[k])
            width = get_width(mo)
            lines[k] = re.sub(pextra_rexp, "", lines[k])
            new = ["\\begin_inset Wrap figure",
                   'width "%s"' % width,
                   "collapsed false",
                   ""]

        new = new+lines[i2:j]+["\\end_inset ", ""]

        # After a float, all font attributes are reseted.
        # We need to output '\foo default' for every attribute foo
        # whose value is not default before the float.
        # The check here is not accurate, but it doesn't matter
        # as extra '\foo default' commands are ignored.
        # In fact, it might be safer to output '\foo default' for all
        # font attributes.
        k = get_paragraph(lines, i, document.format + 1)
        flag = 0
        for token in font_tokens:
            if find_token(lines, token, k, i) != -1:
                if not flag:
                    # This is not necessary, but we want the output to be
                    # as similar as posible to the lyx format
                    flag = 1
                    new.append("")
                if token == "\\lang":
                    new.append(token+" "+ document.language)
                else:
                    new.append(token+" default ")

        lines[i:j+1] = new
        i = i+1


pextra_type2_rexp = re.compile(r".*\\pextra_type\s+[12]")
pextra_type2_rexp2 = re.compile(r".*(\\layout|\\pextra_type\s+2)")
pextra_widthp = re.compile(r"\\pextra_widthp")

def remove_pextra(document):
    " Remove pextra token."
    lines = document.body
    i = 0
    flag = 0
    while 1:
        i = find_re(lines, pextra_type2_rexp, i)
        if i == -1:
            break

        # Sometimes the \pextra_widthp argument comes in it own
        # line. If that happens insert it back in this line.
        if pextra_widthp.search(lines[i+1]):
            lines[i] = lines[i] + ' ' + lines[i+1]
            del lines[i+1]

        mo = pextra_rexp.search(lines[i])
        width = get_width(mo)

        if mo.group(1) == "1":
            # handle \pextra_type 1 (indented paragraph)
            lines[i] = re.sub(pextra_rexp, "\\leftindent "+width+" ", lines[i])
            i = i+1
            continue

        # handle \pextra_type 2 (minipage)
        position = mo.group(3)
        hfill = mo.group(5)
        lines[i] = re.sub(pextra_rexp, "", lines[i])

        start = ["\\begin_inset Minipage",
                 "position " + position,
                 "inner_position 0",
                 'height "0pt"',
                 'width "%s"' % width,
                 "collapsed false"
                 ]
        if flag:
            flag = 0
            if hfill:
                start = ["","\hfill",""]+start
        else:
            start = ['\\layout %s' % document.default_layout,''] + start

        j0 = find_token_backwards(lines,"\\layout", i-1)
        j = get_next_paragraph(lines, i, document.format + 1)

        count = 0
        while 1:
            # collect more paragraphs to the minipage
            count = count+1
            if j == -1 or not check_token(lines[j], "\\layout"):
                break
            i = find_re(lines, pextra_type2_rexp2, j+1)
            if i == -1:
                break
            mo = pextra_rexp.search(lines[i])
            if not mo:
                break
            if mo.group(7) == "1":
                flag = 1
                break
            lines[i] = re.sub(pextra_rexp, "", lines[i])
            j = find_tokens(lines, ["\\layout", "\\end_float"], i+1)

        mid = lines[j0:j]
        end = ["\\end_inset "]

        lines[j0:j] = start+mid+end
        i = i+1


def is_empty(lines):
    " Are all the lines empty?"
    return filter(is_nonempty_line, lines) == []


move_rexp =  re.compile(r"\\(family|series|shape|size|emph|numeric|bar|noun|end_deeper)")
ert_rexp = re.compile(r"\\begin_inset|\\hfill|.*\\SpecialChar")
spchar_rexp = re.compile(r"(.*)(\\SpecialChar.*)")


def remove_oldert(document):
    " Remove old ERT inset."
    ert_begin = ["\\begin_inset ERT",
                 "status Collapsed",
                 "",
                 '\\layout %s' % document.default_layout,
                 ""]
    lines = document.body
    i = 0
    while 1:
        i = find_tokens(lines, ["\\latex latex", "\\layout LaTeX"], i)
        if i == -1:
            break
        j = i+1
        while 1:
            # \end_inset is for ert inside a tabular cell. The other tokens
            # are obvious.
            j = find_tokens(lines, ["\\latex default", "\\layout", "\\begin_inset", "\\end_inset", "\\end_float", "\\the_end"],
                            j)
            if check_token(lines[j], "\\begin_inset"):
                j = find_end_of_inset(lines, j)+1
            else:
                break

        if check_token(lines[j], "\\layout"):
            while j-1 >= 0 and check_token(lines[j-1], "\\begin_deeper"):
                j = j-1

        # We need to remove insets, special chars & font commands from ERT text
        new = []
        new2 = []
        if check_token(lines[i], "\\layout LaTeX"):
            new = ['\layout %s' % document.default_layout, "", ""]

        k = i+1
        while 1:
            k2 = find_re(lines, ert_rexp, k, j)
            inset = hfill = specialchar = 0
            if k2 == -1:
                k2 = j
            elif check_token(lines[k2], "\\begin_inset"):
                inset = 1
            elif check_token(lines[k2], "\\hfill"):
                hfill = 1
                del lines[k2]
                j = j-1
            else:
                specialchar = 1
                mo = spchar_rexp.match(lines[k2])
                lines[k2] = mo.group(1)
                specialchar_str = mo.group(2)
                k2 = k2+1

            tmp = []
            for line in lines[k:k2]:
                # Move some lines outside the ERT inset:
                if move_rexp.match(line):
                    if new2 == []:
                        # This is not necessary, but we want the output to be
                        # as similar as posible to the lyx format
                        new2 = [""]
                    new2.append(line)
                elif not check_token(line, "\\latex"):
                    tmp.append(line)

            if is_empty(tmp):
                if filter(lambda x:x != "", tmp) != []:
                    if new == []:
                        # This is not necessary, but we want the output to be
                        # as similar as posible to the lyx format
                        lines[i-1] = lines[i-1]+" "
                    else:
                        new = new+[" "]
            else:
                new = new+ert_begin+tmp+["\\end_inset ", ""]

            if inset:
                k3 = find_end_of_inset(lines, k2)
                new = new+[""]+lines[k2:k3+1]+[""] # Put an empty line after \end_inset
                k = k3+1
                # Skip the empty line after \end_inset
                if not is_nonempty_line(lines[k]):
                    k = k+1
                    new.append("")
            elif hfill:
                new = new + ["\\hfill", ""]
                k = k2
            elif specialchar:
                if new == []:
                    # This is not necessary, but we want the output to be
                    # as similar as posible to the lyx format
                    lines[i-1] = lines[i-1]+specialchar_str
                    new = [""]
                else:
                    new = new+[specialchar_str, ""]
                k = k2
            else:
                break

        new = new+new2
        if not check_token(lines[j], "\\latex "):
            new = new+[""]+[lines[j]]
        lines[i:j+1] = new
        i = i+1

    # Delete remaining "\latex xxx" tokens
    i = 0
    while 1:
        i = find_token(lines, "\\latex ", i)
        if i == -1:
            break
        del lines[i]


def remove_oldertinset(document):
    " ERT insert are hidden feature of lyx 1.1.6. This might be removed in the future."
    lines = document.body
    i = 0
    while 1:
        i = find_token(lines, "\\begin_inset ERT", i)
        if i == -1:
            break
        j = find_end_of_inset(lines, i)
        k = find_token(lines, "\\layout", i+1)
        l = get_paragraph(lines, i, document.format + 1)
        if lines[k] == lines[l]: # same layout
            k = k+1
        new = lines[k:j]
        lines[i:j+1] = new
        i = i+1


def is_ert_paragraph(document, i):
    " Is this a ert paragraph? "
    lines = document.body
    if not check_token(lines[i], "\\layout"):
        return 0
    if not document.is_default_layout(get_layout(lines[i], document.default_layout)):
        return 0

    i = find_nonempty_line(lines, i+1)
    if not check_token(lines[i], "\\begin_inset ERT"):
        return 0

    j = find_end_of_inset(lines, i)
    k = find_nonempty_line(lines, j+1)
    return check_token(lines[k], "\\layout")


def combine_ert(document):
    " Combine ERT paragraphs."
    lines = document.body
    i = 0
    while 1:
        i = find_token(lines, "\\begin_inset ERT", i)
        if i == -1:
            break
        j = get_paragraph(lines, i, document.format + 1)
        count = 0
        text = []
        while is_ert_paragraph(document, j):

            count = count+1
            i2 = find_token(lines, "\\layout", j+1)
            k = find_token(lines, "\\end_inset", i2+1)
            text = text+lines[i2:k]
            j = find_token(lines, "\\layout", k+1)
            if j == -1:
                break

        if count >= 2:
            j = find_token(lines, "\\layout", i+1)
            lines[j:k] = text

        i = i+1


oldunits = ["pt", "cm", "in", "text%", "col%"]

def get_length(lines, name, start, end):
    " Get lenght."
    i = find_token(lines, name, start, end)
    if i == -1:
        return ""
    x = lines[i].split()
    return x[2]+oldunits[int(x[1])]


def write_attribute(x, token, value):
    " Write attribute."
    if value != "":
        x.append("\t"+token+" "+value)


def remove_figinset(document):
    " Remove figinset."
    lines = document.body
    i = 0
    while 1:
        i = find_token(lines, "\\begin_inset Figure", i)
        if i == -1:
            break
        j = find_end_of_inset(lines, i)

        if ( len(lines[i].split()) > 2 ):
            lyxwidth = lines[i].split()[3]+"pt"
            lyxheight = lines[i].split()[4]+"pt"
        else:
            lyxwidth = ""
            lyxheight = ""

        filename = get_value(lines, "file", i+1, j)

        width = get_length(lines, "width", i+1, j)
        # what does width=5 mean ?
        height = get_length(lines, "height", i+1, j)
        rotateAngle = get_value(lines, "angle", i+1, j)
        if width == "" and height == "":
            size_type = "0"
        else:
            size_type = "1"

        flags = get_value(lines, "flags", i+1, j)
        x = int(flags)%4
        if x == 1:
            display = "monochrome"
        elif x == 2:
            display = "gray"
        else:
            display = "color"

        subcaptionText = ""
        subcaptionLine = find_token(lines, "subcaption", i+1, j)
        if subcaptionLine != -1:
            subcaptionText = lines[subcaptionLine][11:]
            if subcaptionText != "":
                subcaptionText = '"'+subcaptionText+'"'

        k = find_token(lines, "subfigure", i+1,j)
        if k == -1:
            subcaption = 0
        else:
            subcaption = 1

        new = ["\\begin_inset Graphics FormatVersion 1"]
        write_attribute(new, "filename", filename)
        write_attribute(new, "display", display)
        if subcaption:
            new.append("\tsubcaption")
        write_attribute(new, "subcaptionText", subcaptionText)
        write_attribute(new, "size_type", size_type)
        write_attribute(new, "width", width)
        write_attribute(new, "height", height)
        if rotateAngle != "":
            new.append("\trotate")
            write_attribute(new, "rotateAngle", rotateAngle)
        write_attribute(new, "rotateOrigin", "leftBaseline")
        write_attribute(new, "lyxsize_type", "1")
        write_attribute(new, "lyxwidth", lyxwidth)
        write_attribute(new, "lyxheight", lyxheight)
        new = new + ["\\end_inset"]
        lines[i:j+1] = new


attr_re = re.compile(r' \w*="(false|0|)"')
line_re = re.compile(r'<(features|column|row|cell)')

def update_tabular(document):
    " Convert tabular format 2 to 3."
    regexp = re.compile(r'^\\begin_inset\s+Tabular')
    lines = document.body
    i = 0
    while 1:
        i = find_re(lines, regexp, i)
        if i == -1:
            break

        for k in get_tabular_lines(lines, i):
            if check_token(lines[k], "<lyxtabular"):
                lines[k] = lines[k].replace('version="2"', 'version="3"')
            elif check_token(lines[k], "<column"):
                lines[k] = lines[k].replace('width=""', 'width="0pt"')

            if line_re.match(lines[k]):
                lines[k] = re.sub(attr_re, "", lines[k])

        i = i+1


##
# Convert tabular format 2 to 3
#
# compatibility read for old longtable options. Now we can make any
# row part of the header/footer type we want before it was strict
# sequential from the first row down (as LaTeX does it!). So now when
# we find a header/footer line we have to go up the rows and set it
# on all preceding rows till the first or one with already a h/f option
# set. If we find a firstheader on the same line as a header or a
# lastfooter on the same line as a footer then this should be set empty.
# (Jug 20011220)

# just for compatibility with old python versions
# python >= 2.3 has real booleans (False and True)
false = 0
true = 1

class row:
    " Simple data structure to deal with long table info."
    def __init__(self):
        self.endhead = false                # header row
        self.endfirsthead = false        # first header row
        self.endfoot = false                # footer row
        self.endlastfoot = false        # last footer row


def haveLTFoot(row_info):
    " Does row has LTFoot?"
    for row_ in row_info:
        if row_.endfoot:
            return true
    return false


def setHeaderFooterRows(hr, fhr, fr, lfr, rows_, row_info):
    " Set Header/Footer rows."
    endfirsthead_empty = false
    endlastfoot_empty = false
    # set header info
    while (hr > 0):
        hr = hr - 1
        row_info[hr].endhead = true

    # set firstheader info
    if fhr and fhr < rows_:
        if row_info[fhr].endhead:
            while fhr > 0:
                fhr = fhr - 1
                row_info[fhr].endfirsthead = true
                row_info[fhr].endhead = false
        elif row_info[fhr - 1].endhead:
            endfirsthead_empty = true
        else:
            while fhr > 0 and not row_info[fhr - 1].endhead:
                fhr = fhr - 1
                row_info[fhr].endfirsthead = true

    # set footer info
    if fr and fr < rows_:
        if row_info[fr].endhead and row_info[fr - 1].endhead:
            while fr > 0 and not row_info[fr - 1].endhead:
                fr = fr - 1
                row_info[fr].endfoot = true
                row_info[fr].endhead = false
        elif row_info[fr].endfirsthead and row_info[fr - 1].endfirsthead:
            while fr > 0 and not row_info[fr - 1].endfirsthead:
                fr = fr - 1
                row_info[fr].endfoot = true
                row_info[fr].endfirsthead = false
        elif not row_info[fr - 1].endhead and not row_info[fr - 1].endfirsthead:
            while fr > 0 and not row_info[fr - 1].endhead and not row_info[fr - 1].endfirsthead:
                fr = fr - 1
                row_info[fr].endfoot = true

    # set lastfooter info
    if lfr and lfr < rows_:
        if row_info[lfr].endhead and row_info[lfr - 1].endhead:
            while lfr > 0 and not row_info[lfr - 1].endhead:
                lfr = lfr - 1
                row_info[lfr].endlastfoot = true
                row_info[lfr].endhead = false
        elif row_info[lfr].endfirsthead and row_info[lfr - 1].endfirsthead:
            while lfr > 0 and not row_info[lfr - 1].endfirsthead:
                lfr = lfr - 1
                row_info[lfr].endlastfoot = true
                row_info[lfr].endfirsthead = false
        elif row_info[lfr].endfoot and row_info[lfr - 1].endfoot:
            while lfr > 0 and not row_info[lfr - 1].endfoot:
                lfr = lfr - 1
                row_info[lfr].endlastfoot = true
                row_info[lfr].endfoot = false
        elif not row_info[fr - 1].endhead and not row_info[fr - 1].endfirsthead and not row_info[fr - 1].endfoot:
            while lfr > 0 and not row_info[lfr - 1].endhead and not row_info[lfr - 1].endfirsthead and not row_info[lfr - 1].endfoot:
                lfr = lfr - 1
                row_info[lfr].endlastfoot = true
        elif haveLTFoot(row_info):
            endlastfoot_empty = true

    return endfirsthead_empty, endlastfoot_empty


def insert_attribute(lines, i, attribute):
    " Insert attribute in lines[i]."
    last = lines[i].find('>')
    lines[i] = lines[i][:last] + ' ' + attribute + lines[i][last:]


rows_re = re.compile(r'rows="(\d*)"')
longtable_re = re.compile(r'islongtable="(\w)"')
ltvalues_re = re.compile(r'endhead="(-?\d*)" endfirsthead="(-?\d*)" endfoot="(-?\d*)" endlastfoot="(-?\d*)"')
lt_features_re = re.compile(r'(endhead="-?\d*" endfirsthead="-?\d*" endfoot="-?\d*" endlastfoot="-?\d*")')
def update_longtables(document):
    " Update longtables to new format."
    regexp = re.compile(r'^\\begin_inset\s+Tabular')
    body = document.body
    i = 0
    while 1:
        i = find_re(body, regexp, i)
        if i == -1:
            break
        i = i + 1
        i = find_token(body, "<lyxtabular", i)
        if i == -1:
            break

        # get number of rows in the table
        rows = int(rows_re.search(body[i]).group(1))

        i = i + 1
        i = find_token(body, '<features', i)
        if i == -1:
            break

        # is this a longtable?
        longtable = longtable_re.search(body[i])

        if not longtable:
            # islongtable is missing add it
            body[i] = body[i][:10] + 'islongtable="false" ' + body[i][10:]

        if not longtable or longtable.group(1) != "true":
            # remove longtable elements from features
            features = lt_features_re.search(body[i])
            if features:
                body[i] = body[i].replace(features.group(1), "")
            continue

        row_info = row() * rows
        res = ltvalues_re.search(body[i])
        if not res:
            continue

        endfirsthead_empty, endlastfoot_empty = setHeaderFooterRows(res.group(1), res.group(2), res.group(3), res.group(4), rows, row_info)

        if endfirsthead_empty:
            insert_attribute(body, i, 'firstHeadEmpty="true"')

        if endfirsthead_empty:
            insert_attribute(body, i, 'lastFootEmpty="true"')

        i = i + 1
        for j in range(rows):
            i = find_token(body, '<row', i)

            self.endfoot = false                # footer row
            self.endlastfoot = false        # last footer row
            if row_info[j].endhead:
                insert_attribute(body, i, 'endhead="true"')

            if row_info[j].endfirsthead:
                insert_attribute(body, i, 'endfirsthead="true"')

            if row_info[j].endfoot:
                insert_attribute(body, i, 'endfoot="true"')

            if row_info[j].endlastfoot:
                insert_attribute(body, i, 'endlastfoot="true"')

            i = i + 1


def fix_oldfloatinset(document):
    " Figure insert are hidden feature of lyx 1.1.6. This might be removed in the future."
    lines = document.body
    i = 0
    while 1:
        i = find_token(lines, "\\begin_inset Float ", i)
        if i == -1:
            break
        j = find_token(lines, "collapsed", i)
        if j != -1:
            lines[j:j] = ["wide false"]
        i = i+1


def change_listof(document):
    " Change listof insets."
    lines = document.body
    i = 0
    while 1:
        i = find_token(lines, "\\begin_inset LatexCommand \\listof", i)
        if i == -1:
            break
        type = re.search(r"listof(\w*)", lines[i]).group(1)[:-1]
        lines[i] = "\\begin_inset FloatList "+type
        i = i+1


def change_infoinset(document):
    " Change info inset."
    lines = document.body
    i = 0
    while 1:
        i = find_token(lines, "\\begin_inset Info", i)
        if i == -1:
            break
        txt = lines[i][18:].lstrip()
        new = ["\\begin_inset Note", "collapsed true", ""]
        j = find_token(lines, "\\end_inset", i)
        if j == -1:
            break

        note_lines = lines[i+1:j]
        if len(txt) > 0:
            note_lines = [txt]+note_lines

        for line in note_lines:
            new = new + ['\layout %s' % document.default_layout, ""]
            tmp = line.split('\\')
            new = new + [tmp[0]]
            for x in tmp[1:]:
                new = new + ["\\backslash ", x]
        lines[i:j] = new
        i = i+5


def change_header(document):
    " Update header."
    lines = document.header
    i = find_token(lines, "\\use_amsmath", 0)
    if i == -1:
        return
    lines[i+1:i+1] = ["\\use_natbib 0",
                      "\use_numerical_citations 0"]


supported_versions = ["1.2.%d" % i for i in range(5)] + ["1.2"]
convert = [[220, [change_header, change_listof, fix_oldfloatinset,
                  update_tabular, update_longtables, remove_pextra,
                  remove_oldfloat, remove_figinset, remove_oldertinset,
                  remove_oldert, combine_ert, change_infoinset]]]
revert  = []


if __name__ == "__main__":
    pass
