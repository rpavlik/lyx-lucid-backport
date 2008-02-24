#! /usr/bin/env python
# -*- coding: utf-8 -*-
# This file is part of the LyX Documentation
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

# This script creates a "master table of contents" for a set of LyX docs.
# It does so by going through the files and printing out all of the
# chapter, section, and sub(sub)section headings out. (It numbers the
# sections sequentially; hopefully noone's using Section* in the docs.)
# It calls the script doc_toc.py using this syntax:
# depend.py doc_toc.py SetOfDocuments
# where SetOfDocuments is a set of documents

import sys
import os

import re

possible_documents = ("Customization", "EmbeddedObjects", "Extended", "FAQ", "Intro", "Math", "Tutorial", "UserGuide")

def documents(srcdir, lang, dir_prefix = None):
    '''Return documents for specified language. Translated files are in lang 
       directory.
    '''
    result = []
    if dir_prefix is None:
        dir_prefix = srcdir
    for file in possible_documents:
        fname = os.path.join(srcdir, lang, file + '.lyx')
        if os.access(fname, os.F_OK):
            result.append(os.path.join(dir_prefix, lang, file + '.lyx'))
        else:
            result.append(os.path.join(dir_prefix, file + '.lyx'))
    return result


def all_documents(srcdir, dir_prefix = None):
    '''Return available languages and its documents'''
    languages = {}
    if dir_prefix is None:
        dir_prefix = srcdir
    for dir in os.listdir(srcdir):
        if os.path.isdir(os.path.join(srcdir, dir)) and len(dir) == 2:
            languages[dir] = documents(srcdir, dir, dir_prefix)
    # general, English language
    if 'en' not in languages.keys():
        languages['en'] = documents(srcdir, 'en', dir_prefix)
    return languages
  
    
def main(argv):
    print """# This is a Makefile for the TOC.lyx files.
# It was automatically generated by %s
#
# First come the rules for each xx/TOC.lyx file. Then comes the
# TOCs target, which prints all the TOC files.
""" % os.path.basename(argv[0])

    # What are the languages available? And its documents?
    languages = all_documents(os.path.dirname(argv[0]), '')

    # sort languages alphabetically
    langs = languages.keys()
    langs.sort()

    tocs = []

    # Write rules for other languages
    for lang in langs:
        if lang != 'en':
            toc_name = os.path.join(lang, 'TOC.lyx')
        else:
            # for English, because there is no 'en' directory
            toc_name = 'TOC.lyx'
        tocs.append(toc_name)

        if toc_name in languages[lang]:
            languages[lang].remove(toc_name)
        languages[lang].sort()

        print toc_name + ': $(srcdir)/' + ' $(srcdir)/'.join(languages[lang])
        print '\tPYTHONPATH=$(top_builddir)/lib/lyx2lyx python -tt $(srcdir)/doc_toc.py %s .' % lang
        print

    # Write meta-rule to call all the other rules
    print 'tocfiles =', ' '.join(tocs)


if __name__ == "__main__":
    main(sys.argv)
