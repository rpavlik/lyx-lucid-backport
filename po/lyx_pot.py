#!/usr/bin/env python
# -*- coding: utf-8 -*-

# file lyx_pot.py
# This file is part of LyX, the document processor.
# Licence details can be found in the file COPYING.
#
# \author Bo Peng
#
# Full author contact details are available in file CREDITS

# Usage: use
#     lyx_pot.py -h
# to get usage message

# This script will extract translatable strings from input files and write
# to output in gettext .pot format.
#
import sys, os, re, getopt
if sys.version_info < (2, 4, 0):
    from sets import Set as set

def relativePath(path, base):
    '''return relative path from top source dir'''
    # full pathname of path
    path1 = os.path.normpath(os.path.realpath(path)).split(os.sep)
    path2 = os.path.normpath(os.path.realpath(base)).split(os.sep)
    if path1[:len(path2)] != path2:
        print "Path %s is not under top source directory" % path
    path3 = os.path.join(*path1[len(path2):]);
    # replace all \ by / such that we get the same comments on Windows and *nix
    path3 = path3.replace('\\', '/')
    return path3


def writeString(outfile, infile, basefile, lineno, string):
    string = string.replace('\\', '\\\\').replace('"', '')
    if string == "":
        return
    print >> outfile, '#: %s:%d\nmsgid "%s"\nmsgstr ""\n' % \
        (relativePath(infile, basefile), lineno, string)


def ui_l10n(input_files, output, base):
    '''Generate pot file from lib/ui/*'''
    output = open(output, 'w')
    Submenu = re.compile(r'^[^#]*Submenu\s+"([^"]*)"', re.IGNORECASE)
    Popupmenu = re.compile(r'^[^#]*PopupMenu\s+"[^"]+"\s+"([^"]*)"', re.IGNORECASE)
    IconPalette = re.compile(r'^[^#]*IconPalette\s+"[^"]+"\s+"([^"]*)"', re.IGNORECASE)
    Toolbar = re.compile(r'^[^#]*Toolbar\s+"[^"]+"\s+"([^"]*)"', re.IGNORECASE)
    Item = re.compile(r'[^#]*Item\s+"([^"]*)"', re.IGNORECASE)
    TableInsert = re.compile(r'[^#]*TableInsert\s+"([^"]*)"', re.IGNORECASE)
    for src in input_files:
        input = open(src)
        for lineno, line in enumerate(input.readlines()):
            if Submenu.match(line):
                (string,) = Submenu.match(line).groups()
                string = string.replace('_', ' ')
            elif Popupmenu.match(line):
                (string,) = Popupmenu.match(line).groups()
            elif IconPalette.match(line):
                (string,) = IconPalette.match(line).groups()
            elif Toolbar.match(line):
                (string,) = Toolbar.match(line).groups()
            elif Item.match(line):
                (string,) = Item.match(line).groups()
            elif TableInsert.match(line):
                (string,) = TableInsert.match(line).groups()
            else:
                continue
            string = string.replace('"', '')
            if string != "":
                print >> output, '#: %s:%d\nmsgid "%s"\nmsgstr ""\n' % \
                    (relativePath(src, base), lineno+1, string)
        input.close()
    output.close()


def layouts_l10n(input_files, output, base, layouttranslations):
    '''Generate pot file from lib/layouts/*.{layout,inc,module}'''
    Style = re.compile(r'^\s*Style\s+(.*)\s*$', re.IGNORECASE)
    # match LabelString, EndLabelString, LabelStringAppendix and maybe others but no comments
    LabelString = re.compile(r'^[^#]*LabelString\S*\s+(.*)\s*$', re.IGNORECASE)
    GuiName = re.compile(r'^\s*GuiName\s+(.*)\s*$', re.IGNORECASE)
    ListName = re.compile(r'^\s*ListName\s+(.*)\s*$', re.IGNORECASE)
    CategoryName = re.compile(r'^\s*Category\s+(.*)\s*$', re.IGNORECASE)
    NameRE = re.compile(r'^\s*#\s*\\DeclareLyXModule.*{(.*)}$', re.IGNORECASE)
    InsetLayout = re.compile(r'^InsetLayout\s+\"?(.*)\"?\s*$', re.IGNORECASE)
    FlexCheck = re.compile(r'^Flex:(.*)', re.IGNORECASE)
    DescBegin = re.compile(r'^\s*#DescriptionBegin\s*$', re.IGNORECASE)
    DescEnd = re.compile(r'^\s*#\s*DescriptionEnd\s*$', re.IGNORECASE)
    Category = re.compile(r'^\s*#\s*Category:\s+(.*)\s*$', re.IGNORECASE)
    I18nPreamble = re.compile(r'^\s*((Lang)|(Babel))Preamble\s*$', re.IGNORECASE)
    EndI18nPreamble = re.compile(r'^\s*End((Lang)|(Babel))Preamble\s*$', re.IGNORECASE)
    I18nString = re.compile(r'_\(([^\)]+)\)')
    CounterFormat = re.compile(r'^\s*PrettyFormat\s+"?(.*)"?\s*$', re.IGNORECASE)
    CiteFormat = re.compile(r'^\s*CiteFormat', re.IGNORECASE)
    KeyVal = re.compile(r'^\s*_\w+\s+(.*)\s*$')
    Float = re.compile(r'^\s*Float\s*$', re.IGNORECASE)
    UsesFloatPkg = re.compile(r'^\s*UsesFloatPkg\s+(.*)\s*$', re.IGNORECASE)
    IsPredefined = re.compile(r'^\s*IsPredefined\s+(.*)\s*$', re.IGNORECASE)
    End = re.compile(r'^\s*End', re.IGNORECASE)
    Comment = re.compile(r'^(.*)#')
    Translation = re.compile(r'^\s*Translation\s+(.*)\s*$', re.IGNORECASE)
    KeyValPair = re.compile(r'\s*"(.*)"\s+"(.*)"')

    oldlanguages = []
    languages = []
    keyset = set()
    oldtrans = dict()
    if layouttranslations:
        linguas_file = os.path.join(base, 'po/LINGUAS')
        for line in open(linguas_file).readlines():
            res = Comment.search(line)
            if res:
                line = res.group(1)
            if line.strip() != '':
                languages.extend(line.split())

        # read old translations if available
        try:
            input = open(output)
            lang = ''
            for line in input.readlines():
                res = Comment.search(line)
                if res:
                    line = res.group(1)
                if line.strip() == '':
                    continue
                res = Translation.search(line)
                if res:
                    lang = res.group(1)
                    if lang not in languages:
                        oldlanguages.append(lang)
                        languages.append(lang)
                    oldtrans[lang] = dict()
                    continue
                res = End.search(line)
                if res:
                    lang = ''
                    continue
                res = KeyValPair.search(line)
                if res and lang != '':
                    key = res.group(1).decode('utf-8')
                    val = res.group(2).decode('utf-8')
                    key = key.replace('\\"', '"').replace('\\\\', '\\')
                    val = val.replace('\\"', '"').replace('\\\\', '\\')
                    oldtrans[lang][key] = val
                    keyset.add(key)
                    continue
                print "Error: Unable to handle line:"
                print line
        except IOError:
            print "Warning: Unable to open %s for reading." % output
            print "         Old translations will be lost."

        # walon is not a known document language
        # FIXME: Do not hardcode, read from lib/languages!
        if 'wa' in languages:
            languages.remove('wa')

    out = open(output, 'w')
    for src in input_files:
        readingDescription = False
        readingI18nPreamble = False
        readingFloat = False
        readingCiteFormats = False
        isPredefined = False
        usesFloatPkg = True
        listname = ''
        floatname = ''
        descStartLine = -1
        descLines = []
        lineno = 0
        for line in open(src).readlines():
            lineno += 1
            if readingDescription:
                res = DescEnd.search(line)
                if res != None:
                    readingDescription = False
                    desc = " ".join(descLines)
                    if not layouttranslations:
                        writeString(out, src, base, lineno + 1, desc)
                    continue
                descLines.append(line[1:].strip())
                continue
            res = DescBegin.search(line)
            if res != None:
                readingDescription = True
                descStartLine = lineno
                continue
            if readingI18nPreamble:
                res = EndI18nPreamble.search(line)
                if res != None:
                    readingI18nPreamble = False
                    continue
                res = I18nString.search(line)
                if res != None:
                    string = res.group(1)
                    if layouttranslations:
                        keyset.add(string)
                    else:
                        writeString(out, src, base, lineno, string)
                continue
            res = I18nPreamble.search(line)
            if res != None:
                readingI18nPreamble = True
                continue
            res = NameRE.search(line)
            if res != None:
                string = res.group(1)
                if not layouttranslations:
                    writeString(out, src, base, lineno + 1, string)
                continue
            res = Style.search(line)
            if res != None:
                string = res.group(1)
                string = string.replace('_', ' ')
                # Style means something else inside a float definition
                if not readingFloat:
                    if not layouttranslations:
                        writeString(out, src, base, lineno, string)
                continue
            res = LabelString.search(line)
            if res != None:
                string = res.group(1)
                if not layouttranslations:
                    writeString(out, src, base, lineno, string)
                continue
            res = GuiName.search(line)
            if res != None:
                string = res.group(1)
                if layouttranslations:
                    # gui name must only be added for floats
                    if readingFloat:
                        floatname = string
                else:
                    writeString(out, src, base, lineno, string)
                continue
            res = CategoryName.search(line)
            if res != None:
                string = res.group(1)
                if not layouttranslations:
                    writeString(out, src, base, lineno, string)
                continue
            res = ListName.search(line)
            if res != None:
                string = res.group(1)
                if layouttranslations:
                    listname = string.strip('"')
                else:
                    writeString(out, src, base, lineno, string)
                continue
            res = InsetLayout.search(line)
            if res != None:
                string = res.group(1)
                string = string.replace('_', ' ')
                #Flex:xxx is not used in translation
                #if not layouttranslations:
                #    writeString(out, src, base, lineno, string)
                m = FlexCheck.search(string)
                if m:
                    if not layouttranslations:
                        writeString(out, src, base, lineno, m.group(1))
                continue
            res = Category.search(line)
            if res != None:
                string = res.group(1)
                if not layouttranslations:
                    writeString(out, src, base, lineno, string)
                continue
            res = CounterFormat.search(line)
            if res != None:
                string = res.group(1)
                if not layouttranslations:
                    writeString(out, src, base, lineno, string)
                continue
            res = Float.search(line)
            if res != None:
                readingFloat = True
                continue
            res = IsPredefined.search(line)
            if res != None:
                string = res.group(1).lower()
                if string == 'true':
                    isPredefined = True
                else:
                    isPredefined = False
                continue
            res = UsesFloatPkg.search(line)
            if res != None:
                string = res.group(1).lower()
                if string == 'true':
                    usesFloatPkg = True
                else:
                    usesFloatPkg = False
                continue
            res = CiteFormat.search(line)
            if res != None:
                readingCiteFormats = True
                continue
            res = End.search(line)
            if res != None:
                # If a float is predefined by the package and it does not need
                # the float package then it uses the standard babel translations.
                # This is even true for MarginFigure, MarginTable (both from
                # tufte-book.layout) and Planotable, Plate (both from aguplus.inc).
                if layouttranslations and readingFloat and usesFloatPkg and not isPredefined:
                    if floatname != '':
                        keyset.add(floatname)
                    if listname != '':
                        keyset.add(listname)
                isPredefined = False
                usesFloatPkg = True
                listname = ''
                floatname = ''
                readingCiteFormats = False
                readingFloat = False
                continue
            if readingCiteFormats:
                res = KeyVal.search(line)
                if res != None:
                    val = res.group(1)
                    if not layouttranslations:
                        writeString(out, src, base, lineno, val)

    if layouttranslations:
        # Extract translations of layout files
        import polib

        # Sort languages and key to minimize the diff between different runs
        # with changed translations
        languages.sort()
        keys = []
        for key in keyset:
            keys.append(key)
        keys.sort()

        print >> out, '''# This file has been automatically generated by po/lyx_pot.py.
# PLEASE MODIFY ONLY THE LAGUAGES HAVING NO .po FILE! If you want to regenerate
# this file from the translations, run `make ../lib/layouttranslations' in po.
# Python polib library is needed for building the output file.
#
# This file should remain fixed during minor LyX releases.
# For more comments see README.localization file.'''
        for lang in languages:
            print >> out, '\nTranslation %s' % lang
            if lang in oldtrans.keys():
                trans = oldtrans[lang]
            else:
                trans = dict()
            if not lang in oldlanguages:
                poname = os.path.join(base, 'po/' + lang + '.po')
                po = polib.pofile(poname)
                # Iterate through po entries and not keys for speed reasons.
                # FIXME: The code is still too slow
                for entry in po:
                    if not entry.translated():
                        continue
                    if entry.msgid in keys:
                        key = entry.msgid
                        val = entry.msgstr
                        # some translators keep untranslated entries
                        if val != key:
                            trans[key] = val
            for key in keys:
                if key in trans.keys():
                    val = trans[key].replace('\\', '\\\\').replace('"', '\\"')
                    key = key.replace('\\', '\\\\').replace('"', '\\"')
                    print >> out, '\t"%s" "%s"' % \
                             (key.encode('utf-8'), val.encode('utf-8'))
                # also print untranslated entries to help translators
                elif not lang in oldlanguages:
                    key = key.replace('\\', '\\\\').replace('"', '\\"')
                    print >> out, '\t"%s" "%s"' % \
                             (key.encode('utf-8'), key.encode('utf-8'))
            print >> out, 'End'

    out.close()


def qt4_l10n(input_files, output, base):
    '''Generate pot file from src/frontends/qt4/ui/*.ui'''
    output = open(output, 'w')
    pat = re.compile(r'\s*<string>(.*)</string>')
    prop = re.compile(r'\s*<property.*name.*=.*shortcut')
    for src in input_files:
        input = open(src)
        skipNextLine = False
        for lineno, line in enumerate(input.readlines()):
            # skip the line after <property name=shortcut>
            if skipNextLine:
                skipNextLine = False
                continue
            if prop.match(line):
                skipNextLine = True
                continue
            # get lines that match <string>...</string>
            if pat.match(line):
                (string,) = pat.match(line).groups()
                string = string.replace('&amp;', '&').replace('&quot;', '"')
                string = string.replace('&lt;', '<').replace('&gt;', '>')
                string = string.replace('\\', '\\\\').replace('"', r'\"')
                string = string.replace('&#x0a;', r'\n')
                print >> output, '#: %s:%d\nmsgid "%s"\nmsgstr ""\n' % \
                    (relativePath(src, base), lineno+1, string)
        input.close()
    output.close()


def languages_l10n(input_files, output, base):
    '''Generate pot file from lib/languages'''
    out = open(output, 'w')
    GuiName = re.compile(r'^[^#]*GuiName\s+(.*)', re.IGNORECASE)
    
    for src in input_files:
        descStartLine = -1
        descLines = []
        lineno = 0
        for line in open(src).readlines():
            lineno += 1
            res = GuiName.search(line)
            if res != None:
                string = res.group(1)
                writeString(out, src, base, lineno, string)
                continue
               
    out.close()


def external_l10n(input_files, output, base):
    '''Generate pot file from lib/external_templates'''
    output = open(output, 'w')
    Template = re.compile(r'^Template\s+(.*)', re.IGNORECASE)
    GuiName = re.compile(r'\s*GuiName\s+(.*)', re.IGNORECASE)
    HelpTextStart = re.compile(r'\s*HelpText\s', re.IGNORECASE)
    HelpTextSection = re.compile(r'\s*(\S.*)\s*$')
    HelpTextEnd = re.compile(r'\s*HelpTextEnd\s', re.IGNORECASE)
    i = -1
    for src in input_files:
        input = open(src)
        inHelp = False
        hadHelp = False
        prev_help_string = ''
        for lineno, line in enumerate(input.readlines()):
            if Template.match(line):
                (string,) = Template.match(line).groups()
            elif GuiName.match(line):
                (string,) = GuiName.match(line).groups()
            elif inHelp:
                if HelpTextEnd.match(line):
                    if hadHelp:
                        print >> output, '\nmsgstr ""\n'
                    inHelp = False
                    hadHelp = False
                    prev_help_string = ''
                elif HelpTextSection.match(line):
                    (help_string,) = HelpTextSection.match(line).groups()
                    help_string = help_string.replace('"', '')
                    if help_string != "" and prev_help_string == '':
                        print >> output, '#: %s:%d\nmsgid ""\n"%s\\n"' % \
                            (relativePath(src, base), lineno+1, help_string)
                        hadHelp = True
                    elif help_string != "":
                        print >> output, '"%s\\n"' % help_string
                    prev_help_string = help_string
            elif HelpTextStart.match(line):
                inHelp = True
                prev_help_string = ''
            else:
                continue
            string = string.replace('"', '')
            if string != "" and not inHelp:
                print >> output, '#: %s:%d\nmsgid "%s"\nmsgstr ""\n' % \
                    (relativePath(src, base), lineno+1, string)
        input.close()
    output.close()


def formats_l10n(input_files, output, base):
    '''Generate pot file from configure.py'''
    output = open(output, 'w')
    GuiName = re.compile(r'.*\\Format\s+\S+\s+\S+\s+"([^"]*)"\s+(\S*)\s+.*', re.IGNORECASE)
    GuiName2 = re.compile(r'.*\\Format\s+\S+\s+\S+\s+([^"]\S+)\s+(\S*)\s+.*', re.IGNORECASE)
    input = open(input_files[0])
    for lineno, line in enumerate(input.readlines()):
        label = ""
        labelsc = ""
        if GuiName.match(line):
            label = GuiName.match(line).group(1)
            shortcut = GuiName.match(line).group(2).replace('"', '')
        elif GuiName2.match(line):
            label = GuiName2.match(line).group(1)
            shortcut = GuiName2.match(line).group(2).replace('"', '')
        else:
            continue
        label = label.replace('\\', '\\\\').replace('"', '')
        if shortcut != "":
            labelsc = label + "|" + shortcut
        if label != "":
            print >> output, '#: %s:%d\nmsgid "%s"\nmsgstr ""\n' % \
                (relativePath(input_files[0], base), lineno+1, label)
        if labelsc != "":
            print >> output, '#: %s:%d\nmsgid "%s"\nmsgstr ""\n' % \
                (relativePath(input_files[0], base), lineno+1, labelsc)
    input.close()
    output.close()


def encodings_l10n(input_files, output, base):
    '''Generate pot file from lib/encodings'''
    output = open(output, 'w')
    # assuming only one encodings file
    #                 Encoding utf8      utf8    "Unicode (utf8)" UTF-8    variable inputenc
    reg = re.compile('Encoding [\w-]+\s+[\w-]+\s+"([\w \-\(\)]+)"\s+[\w-]+\s+(fixed|variable)\s+\w+.*')
    input = open(input_files[0])
    for lineno, line in enumerate(input.readlines()):
        if not line.startswith('Encoding'):
            continue
        if reg.match(line):
            print >> output, '#: %s:%d\nmsgid "%s"\nmsgstr ""\n' % \
                (relativePath(input_files[0], base), lineno+1, reg.match(line).groups()[0])
        else:
            print "Error: Unable to handle line:"
            print line
            # No need to abort if the parsing fails
            # sys.exit(1)
    input.close()
    output.close()



Usage = '''
lyx_pot.py [-b|--base top_src_dir] [-o|--output output_file] [-h|--help] [-s|src_file filename] -t|--type input_type input_files

where
    --base:
        path to the top source directory. default to '.'
    --output:
        output pot file, default to './lyx.pot'
    --src_file
        filename that contains a list of input files in each line
    --input_type can be
        ui: lib/ui/*
        layouts: lib/layouts/*
        layouttranslations: create lib/layouttranslations from po/*.po and lib/layouts/*
        qt4: qt4 ui files
        languages: file lib/languages
        encodings: file lib/encodings
        external: external templates file
        formats: formats predefined in lib/configure.py
'''

if __name__ == '__main__':
    input_type = None
    output = 'lyx.pot'
    base = '.'
    input_files = []
    #
    optlist, args = getopt.getopt(sys.argv[1:], 'ht:o:b:s:',
        ['help', 'type=', 'output=', 'base=', 'src_file='])
    for (opt, value) in optlist:
        if opt in ['-h', '--help']:
            print Usage
            sys.exit(0)
        elif opt in ['-o', '--output']:
            output = value
        elif opt in ['-b', '--base']:
            base = value
        elif opt in ['-t', '--type']:
            input_type = value
        elif opt in ['-s', '--src_file']:
            input_files = [f.strip() for f in open(value)]

    if input_type not in ['ui', 'layouts', 'layouttranslations', 'qt4', 'languages', 'encodings', 'external', 'formats'] or output is None:
        print 'Wrong input type or output filename.'
        sys.exit(1)

    input_files += args

    if input_type == 'ui':
        ui_l10n(input_files, output, base)
    elif input_type == 'layouts':
        layouts_l10n(input_files, output, base, False)
    elif input_type == 'layouttranslations':
        layouts_l10n(input_files, output, base, True)
    elif input_type == 'qt4':
        qt4_l10n(input_files, output, base)
    elif input_type == 'external':
        external_l10n(input_files, output, base)
    elif input_type == 'formats':
        formats_l10n(input_files, output, base)
    elif input_type == 'encodings':
        encodings_l10n(input_files, output, base)
    else:
        languages_l10n(input_files, output, base)


