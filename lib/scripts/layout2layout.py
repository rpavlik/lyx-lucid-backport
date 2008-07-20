#! /usr/bin/env python
# -*- coding: utf-8 -*-

# file layout2layout.py
# This file is part of LyX, the document processor.
# Licence details can be found in the file COPYING.

# author Georg Baum

# Full author contact details are available in file CREDITS

# This script will update a .layout file to format 6


import os, re, string, sys

# Incremented to format 4, 6 April 2007, lasgouttes
# Introduction of generic "Provides" declaration

# Incremented to format 5, 22 August 2007 by vermeer
# InsetLayout material

# Incremented to format 6, 7 January 2008 by spitz
# Requires tag added to layout files

# Incremented to format 7, 24 March 2008 by rgh
# AddToPreamble tag added to layout files
currentFormat = 7


def usage(prog_name):
    return ("Usage: %s inputfile outputfile\n" % prog_name +
            "or     %s <inputfile >outputfile" % prog_name)


def error(message):
    sys.stderr.write(message + '\n')
    sys.exit(1)


def trim_eol(line):
    " Remove end of line char(s)."
    if line[-2:-1] == '\r':
        return line[:-2]
    elif line[-1:] == '\r' or line[-1:] == '\n':
        return line[:-1]
    else:
        # file with no EOL in last line
        return line


def read(input):
    " Read input file and strip lineendings."
    lines = list()
    while 1:
        line = input.readline()
        if not line:
            break
        lines.append(trim_eol(line))
    return lines


def write(output, lines):
    " Write output file with native lineendings."
    for line in lines:
        output.write(line + os.linesep)


# Concatenates old and new in an intelligent way:
# If old is wrapped in ", they are stripped. The result is wrapped in ".
def concatenate_label(old, new):
    # Don't use strip as long as we support python 1.5.2
    if old[0] == '"':
        return old[0:-1] + new + '"'
    else:
        return '"' + old + new + '"'


def convert(lines):
    " Convert to new format."
    re_Comment = re.compile(r'^(\s*)#')
    re_Empty = re.compile(r'^(\s*)$')
    re_Format = re.compile(r'^(\s*)(Format)(\s+)(\S+)', re.IGNORECASE)
    re_Preamble = re.compile(r'^(\s*)Preamble', re.IGNORECASE)
    re_EndPreamble = re.compile(r'^(\s*)EndPreamble', re.IGNORECASE)
    re_MaxCounter = re.compile(r'^(\s*)(MaxCounter)(\s+)(\S+)', re.IGNORECASE)
    re_LabelType = re.compile(r'^(\s*)(LabelType)(\s+)(\S+)', re.IGNORECASE)
    re_LabelString = re.compile(r'^(\s*)(LabelString)(\s+)(("[^"]+")|(\S+))', re.IGNORECASE)
    re_LabelStringAppendix = re.compile(r'^(\s*)(LabelStringAppendix)(\s+)(("[^"]+")|(\S+))', re.IGNORECASE)
    re_LatexType = re.compile(r'^(\s*)(LatexType)(\s+)(\S+)', re.IGNORECASE)
    re_Style = re.compile(r'^(\s*)(Style)(\s+)(\S+)', re.IGNORECASE)
    re_CopyStyle = re.compile(r'^(\s*)(CopyStyle)(\s+)(\S+)', re.IGNORECASE)
    re_NoStyle = re.compile(r'^(\s*)(NoStyle)(\s+)(\S+)', re.IGNORECASE)
    re_End = re.compile(r'^(\s*)(End)(\s*)$', re.IGNORECASE)
    re_Provides = re.compile(r'^(\s*)Provides(\S+)(\s+)(\S+)', re.IGNORECASE)
    re_CharStyle = re.compile(r'^(\s*)CharStyle(\s+)(\S+)$', re.IGNORECASE)

    # counters for sectioning styles (hardcoded in 1.3)
    counters = {"part"          : "\\Roman{part}",
                "chapter"       : "\\arabic{chapter}",
                "section"       : "\\arabic{section}",
                "subsection"    : "\\arabic{section}.\\arabic{subsection}",
                "subsubsection" : "\\arabic{section}.\\arabic{subsection}.\\arabic{subsubsection}",
                "paragraph"     : "\\arabic{section}.\\arabic{subsection}.\\arabic{subsubsection}.\\arabic{paragraph}",
                "subparagraph"  : "\\arabic{section}.\\arabic{subsection}.\\arabic{subsubsection}.\\arabic{paragraph}.\\arabic{subparagraph}"}

    # counters for sectioning styles in appendix (hardcoded in 1.3)
    appendixcounters = {"chapter"       : "\\Alph{chapter}",
                        "section"       : "\\Alph{section}",
                        "subsection"    : "\\arabic{section}.\\arabic{subsection}",
                        "subsubsection" : "\\arabic{section}.\\arabic{subsection}.\\arabic{subsubsection}",
                        "paragraph"     : "\\arabic{section}.\\arabic{subsection}.\\arabic{subsubsection}.\\arabic{paragraph}",
                        "subparagraph"  : "\\arabic{section}.\\arabic{subsection}.\\arabic{subsubsection}.\\arabic{paragraph}.\\arabic{subparagraph}"}

    # Value of TocLevel for sectioning styles
    toclevels = {"part"          : 0,
                 "chapter"       : 0,
                 "section"       : 1,
                 "subsection"    : 2,
                 "subsubsection" : 3,
                 "paragraph"     : 4,
                 "subparagraph"  : 5}

    i = 0
    only_comment = 1
    counter = ""
    label = ""
    labelstring = ""
    labelstringappendix = ""
    space1 = ""
    labelstring_line = -1
    labelstringappendix_line = -1
    labeltype_line = -1
    latextype = ""
    latextype_line = -1
    style = ""
    maxcounter = 0
    format = 1
    while i < len(lines):

        # Skip comments and empty lines
        if re_Comment.match(lines[i]) or re_Empty.match(lines[i]):
            i += 1
            continue

        # insert file format if not already there
        if (only_comment):
                match = re_Format.match(lines[i])
                if match:
                        format = int(match.group(4))
                        if format > 1 and format < currentFormat:
                            lines[i] = "Format %d" % (format + 1)
                            only_comment = 0
                        elif format == currentFormat:
                                # nothing to do
                                return format
                        else:
                            error('Cannot convert file format %s' % format)
                else:
                        lines.insert(i, "Format 2")
                        only_comment = 0
                        continue

        # Don't get confused by LaTeX code
        if re_Preamble.match(lines[i]):
            i += 1
            while i < len(lines) and not re_EndPreamble.match(lines[i]):
                i += 1
            continue

        if format == 6:
          i += 1
          continue

        if format == 5:
          i += 1
          continue

        if format == 4:
            # Handle conversion to long CharStyle names
            match = re_CharStyle.match(lines[i])
            if match:
                lines[i] = "InsetLayout CharStyle:%s" % (match.group(3))
                i += 1
                lines.insert(i, "\tLyXType charstyle")
                i += 1
                lines.insert(i, "")
                lines[i] = "\tLabelString %s" % (match.group(3))
            i += 1
            continue

        if format == 3:
            # convert 'providesamsmath x',  'providesmakeidx x',  'providesnatbib x',  'providesurl x' to
            #         'provides amsmath x', 'provides makeidx x', 'provides natbib x', 'provides url x'
            # x is either 0 or 1
            match = re_Provides.match(lines[i])
            if match:
                lines[i] = "%sProvides %s%s%s" % (match.group(1), match.group(2).lower(),
                                                  match.group(3), match.group(4))
            i += 1
            continue

        if format == 2:
            caption = []

            # delete caption styles
            match = re_Style.match(lines[i])
            if match:
                style = string.lower(match.group(4))
                if style == "caption":
                    del lines[i]
                    while i < len(lines) and not re_End.match(lines[i]):
                        caption.append(lines[i])
                        del lines[i]
                    if i == len(lines):
                        error('Incomplete caption style.')
                    else:
                        del lines[i]
                        continue

            # delete undefinition of caption styles
            match = re_NoStyle.match(lines[i])
            if match:
                style = string.lower(match.group(4))
                if style == "caption":
                    del lines[i]
                    continue

            # replace the CopyStyle statement with the definition of the real
            # style. This may result in duplicate statements, but that is OK
            # since the second one will overwrite the first one.
            match = re_CopyStyle.match(lines[i])
            if match:
                style = string.lower(match.group(4))
                if style == "caption":
                    if len(caption) > 0:
                        lines[i:i+1] = caption
                    else:
                        # FIXME: This style comes from an include file, we
                        # should replace the real style and not this default.
                        lines[i:i+1] = ['	Margin                First_Dynamic',
                                        '	LatexType             Command',
                                        '	LatexName             caption',
                                        '	NeedProtect           1',
                                        '	LabelSep              xx',
                                        '	ParSkip               0.4',
                                        '	TopSep                0.5',
                                        '	Align                 Center',
                                        '	AlignPossible         Center',
                                        '	LabelType             Sensitive',
                                        '	LabelString           "Senseless!"',
                                        '	OptionalArgs          1',
                                        '	LabelFont',
                                        '	  Series              Bold',
                                        '	EndFont']

            i += 1
            continue

        # Delete MaxCounter and remember the value of it
        match = re_MaxCounter.match(lines[i])
        if match:
            level = match.group(4)
            if string.lower(level) == "counter_chapter":
                maxcounter = 0
            elif string.lower(level) == "counter_section":
                maxcounter = 1
            elif string.lower(level) == "counter_subsection":
                maxcounter = 2
            elif string.lower(level) == "counter_subsubsection":
                maxcounter = 3
            elif string.lower(level) == "counter_paragraph":
                maxcounter = 4
            elif string.lower(level) == "counter_subparagraph":
                maxcounter = 5
            elif string.lower(level) == "counter_enumi":
                maxcounter = 6
            elif string.lower(level) == "counter_enumii":
                maxcounter = 7
            elif string.lower(level) == "counter_enumiii":
                maxcounter = 8
            del lines[i]
            continue

        # Replace line
        #
        # LabelType Counter_EnumI
        #
        # with two lines
        #
        # LabelType Counter
        # LabelCounter EnumI
        #
        match = re_LabelType.match(lines[i])
        if match:
            label = match.group(4)
            # Remember indenting space for later reuse in added lines
            space1 = match.group(1)
            # Remember the line for adding the LabelCounter later.
            # We can't do it here because it could shift latextype_line etc.
            labeltype_line = i
            if string.lower(label[:8]) == "counter_":
                counter = string.lower(label[8:])
                lines[i] = re_LabelType.sub(r'\1\2\3Counter', lines[i])

        # Remember the LabelString line
        match = re_LabelString.match(lines[i])
        if match:
            labelstring = match.group(4)
            labelstring_line = i

        # Remember the LabelStringAppendix line
        match = re_LabelStringAppendix.match(lines[i])
        if match:
            labelstringappendix = match.group(4)
            labelstringappendix_line = i

        # Remember the LatexType line
        match = re_LatexType.match(lines[i])
        if match:
            latextype = string.lower(match.group(4))
            latextype_line = i

        # Reset variables at the beginning of a style definition
        match = re_Style.match(lines[i])
        if match:
            style = string.lower(match.group(4))
            counter = ""
            label = ""
            space1 = ""
            labelstring = ""
            labelstringappendix = ""
            labelstring_line = -1
            labelstringappendix_line = -1
            labeltype_line = -1
            latextype = ""
            latextype_line = -1

        if re_End.match(lines[i]):

            # Add a line "LatexType Bib_Environment" if LabelType is Bibliography
            # (or change the existing LatexType)
            if string.lower(label) == "bibliography":
                if (latextype_line < 0):
                    lines.insert(i, "%sLatexType Bib_Environment" % space1)
                    i += 1
                else:
                    lines[latextype_line] = re_LatexType.sub(r'\1\2\3Bib_Environment', lines[latextype_line])

            # Change "LabelType Static" to "LabelType Itemize" for itemize environments
            if latextype == "item_environment" and string.lower(label) == "static":
                lines[labeltype_line] = re_LabelType.sub(r'\1\2\3Itemize', lines[labeltype_line])

            # Change "LabelType Counter_EnumI" to "LabelType Enumerate" for enumerate environments
            if latextype == "item_environment" and string.lower(label) == "counter_enumi":
                lines[labeltype_line] = re_LabelType.sub(r'\1\2\3Enumerate', lines[labeltype_line])
                # Don't add the LabelCounter line later
                counter = ""

            # Replace
            #
            # LabelString "Chapter"
            #
            # with
            #
            # LabelString "Chapter \arabic{chapter}"
            #
            # if this style has a counter. Ditto for LabelStringAppendix.
            # This emulates the hardcoded article style numbering of 1.3
            #
            if counter != "":
                if counters.has_key(style):
                    if labelstring_line < 0:
                        lines.insert(i, '%sLabelString "%s"' % (space1, counters[style]))
                        i += 1
                    else:
                        new_labelstring = concatenate_label(labelstring, counters[style])
                        lines[labelstring_line] = re_LabelString.sub(
                                r'\1\2\3%s' % new_labelstring.replace("\\", "\\\\"),
                                lines[labelstring_line])
                if appendixcounters.has_key(style):
                    if labelstringappendix_line < 0:
                        lines.insert(i, '%sLabelStringAppendix "%s"' % (space1, appendixcounters[style]))
                        i += 1
                    else:
                        new_labelstring = concatenate_label(labelstring, appendixcounters[style])
                        lines[labelstringappendix_line] = re_LabelStringAppendix.sub(
                                r'\1\2\3%s' % new_labelstring.replace("\\", "\\\\"),
                                lines[labelstringappendix_line])

                # Now we can safely add the LabelCounter line
                lines.insert(labeltype_line + 1, "%sLabelCounter %s" % (space1, counter))
                i += 1

            # Add the TocLevel setting for sectioning styles
            if toclevels.has_key(style) and maxcounter <= toclevels[style]:
                lines.insert(i, '%sTocLevel %d' % (space1, toclevels[style]))
                i += 1

        i += 1

    return format + 1


def main(argv):

    # Open files
    if len(argv) == 1:
        input = sys.stdin
        output = sys.stdout
    elif len(argv) == 3:
        input = open(argv[1], 'rb')
        output = open(argv[2], 'wb')
    else:
        error(usage(argv[0]))

    # Do the real work
    lines = read(input)
    format = 1
    while (format < currentFormat):
        format = convert(lines)
    write(output, lines)

    # Close files
    if len(argv) == 3:
        input.close()
        output.close()

    return 0


if __name__ == "__main__":
    main(sys.argv)
