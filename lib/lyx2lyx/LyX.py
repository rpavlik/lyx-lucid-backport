# This file is part of lyx2lyx
# -*- coding: utf-8 -*-
# Copyright (C) 2002-2004 Dekel Tsur <dekel@lyx.org>
# Copyright (C) 2002-2006 José Matos <jamatos@lyx.org>
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

from parser_tools import get_value, check_token, find_token,\
     find_tokens, find_end_of
import os.path
import gzip
import locale
import sys
import re
import time

try:
    import lyx2lyx_version
    version_lyx2lyx = lyx2lyx_version.version
except: # we are running from build directory so assume the last version
    version_lyx2lyx = '1.5.0svn'

default_debug_level = 2

####################################################################
# Private helper functions

def find_end_of_inset(lines, i):
    " Find beginning of inset, where lines[i] is included."
    return find_end_of(lines, i, "\\begin_inset", "\\end_inset")

def generate_minor_versions(major, last_minor_version):
    """ Generate minor versions, using major as prefix and minor
    versions from 0 until last_minor_version, plus the generic version.

    Example:

      generate_minor_versions("1.2", 4) ->
      [ "1.2", "1.2.0", "1.2.1", "1.2.2", "1.2.3"]
    """
    return [major] + [major + ".%d" % i for i in range(last_minor_version + 1)]


# End of helper functions
####################################################################


# Regular expressions used
format_re = re.compile(r"(\d)[\.,]?(\d\d)")
fileformat = re.compile(r"\\lyxformat\s*(\S*)")
original_version = re.compile(r".*?LyX ([\d.]*)")

##
# file format information:
#  file, supported formats, stable release versions
format_relation = [("0_06",    [200], generate_minor_versions("0.6" , 4)),
                   ("0_08",    [210], generate_minor_versions("0.8" , 6) + ["0.7"]),
                   ("0_10",    [210], generate_minor_versions("0.10", 7) + ["0.9"]),
                   ("0_12",    [215], generate_minor_versions("0.12", 1) + ["0.11"]),
                   ("1_0",     [215], generate_minor_versions("1.0" , 4)),
                   ("1_1",     [215], generate_minor_versions("1.1" , 4)),
                   ("1_1_5",   [216], ["1.1.5","1.1.5.1","1.1.5.2","1.1"]),
                   ("1_1_6_0", [217], ["1.1.6","1.1.6.1","1.1.6.2","1.1"]),
                   ("1_1_6_3", [218], ["1.1.6.3","1.1.6.4","1.1"]),
                   ("1_2",     [220], generate_minor_versions("1.2" , 4)),
                   ("1_3",     [221], generate_minor_versions("1.3" , 7)),
                   ("1_4", range(222,246), generate_minor_versions("1.4" , 5)),
                   ("1_5", range(246,277), generate_minor_versions("1.5" , 0))]


def formats_list():
    " Returns a list with supported file formats."
    formats = []
    for version in format_relation:
        for format in version[1]:
            if format not in formats:
                formats.append(format)
    return formats


def get_end_format():
    " Returns the more recent file format available."
    return format_relation[-1][1][-1]


def get_backend(textclass):
    " For _textclass_ returns its backend."
    if textclass == "linuxdoc" or textclass == "manpage":
        return "linuxdoc"
    if textclass[:7] == "docbook":
        return "docbook"
    return "latex"


def trim_eol(line):
    " Remove end of line char(s)."
    if line[-2:-1] == '\r':
        return line[:-2]
    else:
        return line[:-1]


def get_encoding(language, inputencoding, format, cjk_encoding):
    if format > 248:
        return "utf8"
    # CJK-LyX encodes files using the current locale encoding.
    # This means that files created by CJK-LyX can only be converted using
    # the correct locale settings unless the encoding is given as commandline
    # argument.
    if cjk_encoding == 'auto':
        return locale.getpreferredencoding()
    elif cjk_encoding != '':
        return cjk_encoding
    from lyx2lyx_lang import lang
    if inputencoding == "auto" or inputencoding == "default":
        return lang[language][3]
    if inputencoding == "":
        return "latin1"
    # python does not know the alias latin9
    if inputencoding == "latin9":
        return "iso-8859-15"
    return inputencoding

##
# Class
#
class LyX_Base:
    """This class carries all the information of the LyX file."""
    
    def __init__(self, end_format = 0, input = "", output = "", error
                 = "", debug = default_debug_level, try_hard = 0, cjk_encoding = '',
                 language = "english", encoding = "auto"):

        """Arguments:
        end_format: final format that the file should be converted. (integer)
        input: the name of the input source, if empty resort to standard input.
        output: the name of the output file, if empty use the standard output.
        error: the name of the error file, if empty use the standard error.
        debug: debug level, O means no debug, as its value increases be more verbose.
        """
        self.choose_io(input, output)

        if error:
            self.err = open(error, "w")
        else:
            self.err = sys.stderr

        self.debug = debug
        self.try_hard = try_hard
        self.cjk_encoding = cjk_encoding

        if end_format:
            self.end_format = self.lyxformat(end_format)
        else:
            self.end_format = get_end_format()

        self.backend = "latex"
        self.textclass = "article"
        # This is a hack: We use '' since we don't know the default
        # layout of the text class. LyX will parse it as default layout.
        # FIXME: Read the layout file and use the real default layout
        self.default_layout = ''
        self.header = []
        self.preamble = []
        self.body = []
        self.status = 0
        self.encoding = encoding
        self.language = language


    def warning(self, message, debug_level= default_debug_level):
        " Emits warning to self.error, if the debug_level is less than the self.debug."
        if debug_level <= self.debug:
            self.err.write("Warning: " + message + "\n")


    def error(self, message):
        " Emits a warning and exits if not in try_hard mode."
        self.warning(message)
        if not self.try_hard:
            self.warning("Quiting.")
            sys.exit(1)

        self.status = 2


    def read(self):
        """Reads a file into the self.header and self.body parts, from self.input."""

        while 1:
            line = self.input.readline()
            if not line:
                self.error("Invalid LyX file.")

            line = trim_eol(line)
            if check_token(line, '\\begin_preamble'):
                while 1:
                    line = self.input.readline()
                    if not line:
                        self.error("Invalid LyX file.")

                    line = trim_eol(line)
                    if check_token(line, '\\end_preamble'):
                        break
                    
                    if line.split()[:0] in ("\\layout", "\\begin_layout", "\\begin_body"):
                        self.warning("Malformed LyX file: Missing '\\end_preamble'.")
                        self.warning("Adding it now and hoping for the best.")

                    self.preamble.append(line)

            if check_token(line, '\\end_preamble'):
                continue

            line = line.strip()
            if not line:
                continue

            if line.split()[0] in ("\\layout", "\\begin_layout", "\\begin_body", "\\begin_deeper"):
                self.body.append(line)
                break

            self.header.append(line)

        self.textclass = get_value(self.header, "\\textclass", 0)
        self.backend = get_backend(self.textclass)
        self.format  = self.read_format()
        self.language = get_value(self.header, "\\language", 0, default = "english")
        self.inputencoding = get_value(self.header, "\\inputencoding", 0, default = "auto")
        self.encoding = get_encoding(self.language, self.inputencoding, self.format, self.cjk_encoding)
        self.initial_version = self.read_version()

        # Second pass over header and preamble, now we know the file encoding
        for i in range(len(self.header)):
            self.header[i] = self.header[i].decode(self.encoding)
        for i in range(len(self.preamble)):
            self.preamble[i] = self.preamble[i].decode(self.encoding)

        # Read document body
        while 1:
            line = self.input.readline().decode(self.encoding)
            if not line:
                break
            self.body.append(trim_eol(line))


    def write(self):
        " Writes the LyX file to self.output."
        self.set_version()
        self.set_format()
        self.set_textclass()
        if self.encoding == "auto":
            self.encoding = get_encoding(self.language, self.encoding, self.format, self.cjk_encoding)

        if self.preamble:
            i = find_token(self.header, '\\textclass', 0) + 1
            preamble = ['\\begin_preamble'] + self.preamble + ['\\end_preamble']
            if i == 0:
                self.error("Malformed LyX file: Missing '\\textclass'.")
            else:
                header = self.header[:i] + preamble + self.header[i:]
        else:
            header = self.header

        for line in header + [''] + self.body:
            self.output.write(line.encode(self.encoding)+"\n")


    def choose_io(self, input, output):
        """Choose input and output streams, dealing transparently with
        compressed files."""

        if output:
            self.output = open(output, "wb")
        else:
            self.output = sys.stdout

        if input and input != '-':
            self.dir = os.path.dirname(os.path.abspath(input))
            try:
                gzip.open(input).readline()
                self.input = gzip.open(input)
                self.output = gzip.GzipFile(mode="wb", fileobj=self.output) 
            except:
                self.input = open(input)
        else:
            self.dir = ''
            self.input = sys.stdin


    def lyxformat(self, format):
        " Returns the file format representation, an integer."
        result = format_re.match(format)
        if result:
            format = int(result.group(1) + result.group(2))
        elif format == '2':
            format = 200
        else:
            self.error(str(format) + ": " + "Invalid LyX file.")

        if format in formats_list():
            return format

        self.error(str(format) + ": " + "Format not supported.")
        return None


    def read_version(self):
        """ Searchs for clues of the LyX version used to write the file, returns the
        most likely value, or None otherwise."""
        for line in self.header:
            if line[0] != "#":
                return None

            line = line.replace("fix",".")
            result = original_version.match(line)
            if result:
                # Special know cases: reLyX and KLyX
                if line.find("reLyX") != -1 or line.find("KLyX") != -1:
                    return "0.12"

                res = result.group(1)
                if not res:
                    self.warning(line)
                #self.warning("Version %s" % result.group(1))
                return res
        self.warning(str(self.header[:2]))
        return None


    def set_version(self):
        " Set the header with the version used."
        self.header[0] = "#LyX %s created this file. For more info see http://www.lyx.org/" % version_lyx2lyx
        if self.header[1][0] == '#':
            del self.header[1]


    def read_format(self):
        " Read from the header the fileformat of the present LyX file."
        for line in self.header:
            result = fileformat.match(line)
            if result:
                return self.lyxformat(result.group(1))
        else:
            self.error("Invalid LyX File.")
        return None


    def set_format(self):
        " Set the file format of the file, in the header."
        if self.format <= 217:
            format = str(float(self.format)/100)
        else:
            format = str(self.format)
        i = find_token(self.header, "\\lyxformat", 0)
        self.header[i] = "\\lyxformat %s" % format


    def set_textclass(self):
        i = find_token(self.header, "\\textclass", 0)
        self.header[i] = "\\textclass %s" % self.textclass


    def set_parameter(self, param, value):
        " Set the value of the header parameter."
        i = find_token(self.header, '\\' + param, 0)
        if i == -1:
            self.warning('Parameter not found in the header: %s' % param, 3)
            return
        self.header[i] = '\\%s %s' % (param, str(value))


    def is_default_layout(self, layout):
        " Check whether a layout is the default layout of this class."
        # FIXME: Check against the real text class default layout
        if layout == 'Standard' or layout == self.default_layout:
            return 1
        return 0


    def convert(self):
        "Convert from current (self.format) to self.end_format."
        mode, convertion_chain = self.chain()
        self.warning("convertion chain: " + str(convertion_chain), 3)

        for step in convertion_chain:
            steps = getattr(__import__("lyx_" + step), mode)

            self.warning("Convertion step: %s - %s" % (step, mode), default_debug_level + 1)
            if not steps:
                    self.error("The convertion to an older format (%s) is not implemented." % self.format)

            multi_conv = len(steps) != 1
            for version, table in steps:
                if multi_conv and \
                   (self.format >= version and mode == "convert") or\
                   (self.format <= version and mode == "revert"):
                    continue

                for conv in table:
                    init_t = time.time()
                    try:
                        conv(self)
                    except:
                        self.warning("An error ocurred in %s, %s" % (version, str(conv)),
                                     default_debug_level)
                        if not self.try_hard:
                            raise
                        self.status = 2
                    else:
                        self.warning("%lf: Elapsed time on %s"  % (time.time() - init_t, str(conv)),
                                     default_debug_level + 1)

                self.format = version
                if self.end_format == self.format:
                    return


    def chain(self):
        """ This is where all the decisions related with the convertion are taken.
        It returns a list of modules needed to convert the LyX file from
        self.format to self.end_format"""

        self.start =  self.format
        format = self.format
        correct_version = 0

        for rel in format_relation:
            if self.initial_version in rel[2]:
                if format in rel[1]:
                    initial_step = rel[0]
                    correct_version = 1
                    break

        if not correct_version:
            if format <= 215:
                self.warning("Version does not match file format, discarding it. (Version %s, format %d)" %(self.initial_version, self.format))
            for rel in format_relation:
                if format in rel[1]:
                    initial_step = rel[0]
                    break
            else:
                # This should not happen, really.
                self.error("Format not supported.")

        # Find the final step
        for rel in format_relation:
            if self.end_format in rel[1]:
                final_step = rel[0]
                break
        else:
            self.error("Format not supported.")

        # Convertion mode, back or forth
        steps = []
        if (initial_step, self.start) < (final_step, self.end_format):
            mode = "convert"
            first_step = 1
            for step in format_relation:
                if  initial_step <= step[0] <= final_step:
                    if first_step and len(step[1]) == 1:
                        first_step = 0
                        continue
                    steps.append(step[0])
        else:
            mode = "revert"
            relation_format = format_relation[:]
            relation_format.reverse()
            last_step = None

            for step in relation_format:
                if  final_step <= step[0] <= initial_step:
                    steps.append(step[0])
                    last_step = step

            if last_step[1][-1] == self.end_format:
                steps.pop()

        return mode, steps


    def get_toc(self, depth = 4):
        " Returns the TOC of this LyX document."
        paragraphs_filter = {'Title' : 0,'Chapter' : 1, 'Section' : 2, 'Subsection' : 3, 'Subsubsection': 4}
        allowed_insets = ['Quotes']
        allowed_parameters = '\\paragraph_spacing', '\\noindent', '\\align', '\\labelwidthstring', "\\start_of_appendix", "\\leftindent"

        sections = []
        for section in paragraphs_filter.keys():
            sections.append('\\begin_layout %s' % section)

        toc_par = []
        i = 0
        while 1:
            i = find_tokens(self.body, sections, i)
            if i == -1:
                break

            j = find_end_of(self.body,  i + 1, '\\begin_layout', '\\end_layout')
            if j == -1:
                self.warning('Incomplete file.', 0)
                break

            section = self.body[i].split()[1]
            if section[-1] == '*':
                section = section[:-1]

            par = []

            k = i + 1
            # skip paragraph parameters
            while not self.body[k].strip() or self.body[k].split()[0] in allowed_parameters:
                k = k +1

            while k < j:
                if check_token(self.body[k], '\\begin_inset'):
                    inset = self.body[k].split()[1]
                    end = find_end_of_inset(self.body, k)
                    if end == -1 or end > j:
                        self.warning('Malformed file.', 0)

                    if inset in allowed_insets:
                        par.extend(self.body[k: end+1])
                    k = end + 1
                else:
                    par.append(self.body[k])
                    k = k + 1

            # trim empty lines in the end.
            while par and par[-1].strip() == '':
                par.pop()

            toc_par.append(Paragraph(section, par))

            i = j + 1

        return toc_par


class File(LyX_Base):
    " This class reads existing LyX files."
    def __init__(self, end_format = 0, input = "", output = "", error = "", debug = default_debug_level, try_hard = 0, cjk_encoding = ''):
        LyX_Base.__init__(self, end_format, input, output, error, debug, try_hard, cjk_encoding)
        self.read()


class NewFile(LyX_Base):
    " This class is to create new LyX files."
    def set_header(self, **params):
        # set default values
        self.header.extend([
            "#LyX xxxx created this file. For more info see http://www.lyx.org/",
            "\\lyxformat xxx",
            "\\begin_document",
            "\\begin_header",
            "\\textclass article",
            "\\language english",
            "\\inputencoding auto",
            "\\font_roman default",
            "\\font_sans default",
            "\\font_typewriter default",
            "\\font_default_family default",
            "\\font_sc false",
            "\\font_osf false",
            "\\font_sf_scale 100",
            "\\font_tt_scale 100",
            "\\graphics default",
            "\\paperfontsize default",
            "\\papersize default",
            "\\use_geometry false",
            "\\use_amsmath 1",
            "\\cite_engine basic",
            "\\use_bibtopic false",
            "\\paperorientation portrait",
            "\\secnumdepth 3",
            "\\tocdepth 3",
            "\\paragraph_separation indent",
            "\\defskip medskip",
            "\\quotes_language english",
            "\\papercolumns 1",
            "\\papersides 1",
            "\\paperpagestyle default",
            "\\tracking_changes false",
            "\\end_header"])

        self.format = get_end_format()
        for param in params:
            self.set_parameter(param, params[param])


    def set_body(self, paragraphs):
        self.body.extend(['\\begin_body',''])

        for par in paragraphs:
            self.body.extend(par.asLines())

        self.body.extend(['','\\end_body', '\\end_document'])


class Paragraph:
    # unfinished implementation, it is missing the Text and Insets representation.
    " This class represents the LyX paragraphs."
    def __init__(self, name, body=[], settings = [], child = []):
        """ Parameters:
        name: paragraph name.
        body: list of lines of body text.
        child: list of paragraphs that descend from this paragraph.
        """
        self.name = name
        self.body = body
        self.settings = settings
        self.child = child

    def asLines(self):
        " Converts the paragraph to a list of strings, representing it in the LyX file."
        result = ['','\\begin_layout %s' % self.name]
        result.extend(self.settings)
        result.append('')
        result.extend(self.body)
        result.append('\\end_layout')

        if not self.child:
            return result

        result.append('\\begin_deeper')
        for node in self.child:
            result.extend(node.asLines())
        result.append('\\end_deeper')

        return result


class Inset:
    " This class represents the LyX insets."
    pass


class Text:
    " This class represents simple chuncks of text."
    pass