include $(top_srcdir)/config/common.am

EXTRA_DIST = tex2lyx.1in $(TEST_FILES) pch.h

#noinst_LTLIBRARIES = libtexparser.la
#
#libtexparser_la_SOURCES = \
#	Parser.cpp \
#	Parser.h
#
#tex2lyx_LDADD = libtexparser.la

man_MANS = tex2lyx.1

bin_PROGRAMS = tex2lyx

DEFAULT_INCLUDES = 

AM_CPPFLAGS += -DTEX2LYX $(PCH_FLAGS) -I$(top_srcdir)/src/tex2lyx \
	-I$(top_srcdir)/src -I$(top_builddir) $(BOOST_INCLUDES)

TEST_FILES = \
	test/box-color-size-space-align.tex \
	test/DummyDocument.tex \
	test/foo.eps \
	test/foo.png \
	test/test-insets.tex \
	test/test.ltx \
	test/test-structure.tex

LINKED_FILES = \
	../FloatList.cpp \
	../Floating.cpp \
	../Counters.cpp \
	../insets/InsetLayout.cpp \
	../LayoutFile.h \
	../Layout.h \
	../Layout.cpp \
	../TextClass.cpp \
	../TextClass.h \
	../Lexer.cpp \
	../lengthcommon.cpp \
	../Color.cpp \
	../Color.h \
	../Encoding.cpp

BUILT_SOURCES = $(PCH_FILE)

tex2lyx_SOURCES = \
	$(LINKED_FILES) \
	Spacing.h \
	boost.cpp \
	Context.cpp \
	Context.h \
	Font.cpp \
	Font.h \
	gettext.cpp \
	Parser.cpp \
	Parser.h \
	tex2lyx.cpp \
	tex2lyx.h \
	preamble.cpp \
	math.cpp \
	table.cpp \
	text.cpp

tex2lyx_LDADD = \
	$(top_builddir)/src/support/liblyxsupport.la \
	$(LIBICONV) $(BOOST_LIBS) @LIBS@ $(LIBPSAPI)