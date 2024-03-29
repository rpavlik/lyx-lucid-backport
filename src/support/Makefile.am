include $(top_srcdir)/config/common.am

CLEANFILES += $(BUILT_SOURCES)

EXTRA_DIST = pch.h \
	os_cygwin.cpp os_unix.cpp os_win32.cpp os_win32.h

noinst_LIBRARIES = liblyxsupport.a

BUILT_SOURCES = $(PCH_FILE)

#########################  Qt stuff  #############################
#

MOCHEADER = SystemcallPrivate.h

MOCEDFILES = $(MOCHEADER:%.h=moc_%.cpp)

CLEANFILES += $(MOCEDFILES)
BUILT_SOURCES += $(MOCEDFILES)

moc_%.cpp: %.h
	$(MOC4) -o $@ $<

liblyxsupport_a_DEPENDENCIES = $(MOCEDFILES)

#
##################################################################

AM_CPPFLAGS += $(PCH_FLAGS) -I$(srcdir)/.. $(BOOST_INCLUDES)
AM_CPPFLAGS += $(QT4_CPPFLAGS) $(QT4_CORE_INCLUDES) 

liblyxsupport_a_SOURCES = \
	FileMonitor.h \
	FileMonitor.cpp \
	RandomAccessList.h \
	bind.h \
	convert.cpp \
	convert.h \
	copied_ptr.h \
	debug.cpp \
	debug.h \
	docstream.cpp \
	docstream.h \
	docstring.cpp \
	docstring.h \
	docstring_list.h \
	environment.h \
	environment.cpp \
	ExceptionMessage.h \
	FileName.cpp \
	FileName.h \
	FileNameList.h \
	filetools.cpp \
	filetools.h \
	foreach.h \
	ForkedCalls.cpp \
	ForkedCalls.h \
	functional.h \
	gettext.cpp \
	gettext.h \
	gzstream.cpp \
	gzstream.h \
	kill.cpp \
	lassert.h \
	lassert.cpp \
	limited_stack.h \
	lstrings.cpp \
	lstrings.h \
	lyxalgo.h \
	lyxlib.h \
	lyxtime.cpp \
	lyxtime.h \
	mutex.h \
	mutex.cpp \
	Messages.cpp \
	Messages.h \
	numpunct_lyx_char_type.h \
	os.cpp \
	os.h \
	Path.cpp \
	Path.h \
	Package.cpp \
	Package.h \
	ProgressInterface.h \
	qstring_helpers.cpp \
	qstring_helpers.h \
	regex.h \
	socktools.cpp \
	socktools.h \
	strfwd.h \
	Systemcall.cpp \
	Systemcall.h \
	SystemcallPrivate.h \
	shared_ptr.h \
	textutils.h \
	Translator.h \
	Timeout.cpp \
	Timeout.h \
	types.h \
	userinfo.cpp \
	userinfo.h \
	unicode.cpp \
	unicode.h \
	weighted_btree.h
if USE_INCLUDED_MYTHES
liblyxsupport_a_SOURCES += \
	mythes/mythes.cxx \
	mythes/mythes.hxx \
	mythes/license.readme
endif

#if INSTALL_MACOSX
#liblyxsupport_a_SOURCES += \
#	AppleSpellChecker.h \
#	AppleSpellChecker.mm
#endif

if INSTALL_MACOSX
liblyxsupport_a_SOURCES += \
	AppleSpeller.h \
	AppleSpeller.m \
	linkback/LinkBack.h \
	linkback/LinkBack.m \
	linkback/LinkBackProxy.h \
	linkback/LinkBackProxy.m \
	linkback/LinkBackServer.h \
	linkback/LinkBackServer.m
endif

############################## Tests ##################################

EXTRA_DIST += \
	tests/test_convert \
	tests/test_filetools \
	tests/test_lstrings \
	tests/regfiles/convert \
	tests/regfiles/filetools \
	tests/regfiles/lstrings


TESTS = \
	tests/test_convert \
	tests/test_filetools \
	tests/test_lstrings

check_PROGRAMS = \
	check_convert \
	check_filetools \
	check_lstrings

check_convert_LDADD = liblyxsupport.a $(INTLLIBS) $(LIBICONV) $(BOOST_LIBS) $(QT4_CORE_LIBS) $(LIBSHLWAPI)
check_convert_LDFLAGS = $(QT4_LDFLAGS)
check_convert_SOURCES = \
	tests/check_convert.cpp \
	tests/boost.cpp

check_filetools_LDADD = liblyxsupport.a $(INTLLIBS) $(LIBICONV) $(BOOST_LIBS) $(QT4_CORE_LIBS) $(LIBSHLWAPI)
check_filetools_LDFLAGS = $(QT4_CORE_LDFLAGS)
check_filetools_SOURCES = \
	tests/check_filetools.cpp \
	tests/boost.cpp

check_lstrings_LDADD = liblyxsupport.a $(INTLLIBS) $(LIBICONV) $(BOOST_LIBS) $(QT4_CORE_LIBS) $(LIBSHLWAPI)
check_lstrings_LDFLAGS = $(QT4_CORE_LDFLAGS)
check_lstrings_SOURCES = \
	tests/check_lstrings.cpp \
	tests/boost.cpp

makeregfiles: ${check_PROGRAMS}
	for all in ${check_PROGRAMS} ; do \
		./$$all > ${srcdir}/tests/regfiles/$$all ; \
	done
