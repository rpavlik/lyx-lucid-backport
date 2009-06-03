/*
 * \file configCompiler.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * This is the compilation configuration file for LyX.
 * It was generated by autoconfs configure.
 * You might want to change some of the defaults if something goes wrong
 * during the compilation.
 */

#ifndef _CONFIG_COMPILER_H
#define _CONFIG_COMPILER_H


#cmakedefine HAVE_IO_H 1
#cmakedefine HAVE_LIMITS_H 1
#cmakedefine HAVE_LOCALE_H 1
#cmakedefine HAVE_PROCESS_H 1
#cmakedefine HAVE_STDLIB_H 1
#cmakedefine HAVE_SYS_STAT_H 1
#cmakedefine HAVE_SYS_TIME_H 1
#cmakedefine HAVE_SYS_TYPES_H 1
#cmakedefine HAVE_SYS_UTIME_H 1
#cmakedefine HAVE_SYS_SOCKET_H 1
#cmakedefine HAVE_UNISTD_H 1
#cmakedefine HAVE_INTTYPES_H 1
#cmakedefine HAVE_UTIME_H 1
#cmakedefine HAVE_STRING_H 1
#cmakedefine HAVE_STRINGS_H 1
#cmakedefine HAVE_ISTREAM 1
#cmakedefine HAVE_OSTREAM 1
#cmakedefine HAVE_IOS 1
#cmakedefine HAVE_LOCALE 1
#cmakedefine HAVE_OPEN 1
#cmakedefine HAVE_CHMOD 1
#cmakedefine HAVE_CLOSE 1
#cmakedefine HAVE_DCGETTEXT 1
#cmakedefine HAVE_POPEN 1
#cmakedefine HAVE_PCLOSE 1
#cmakedefine HAVE__OPEN 1
#cmakedefine HAVE__CLOSE 1
#cmakedefine HAVE__POPEN 1
#cmakedefine HAVE__PCLOSE 1
#cmakedefine HAVE_GETPID 1
#cmakedefine HAVE__GETPID 1
#cmakedefine HAVE_GETTEXT 1
#cmakedefine HAVE_MKDIR 1
#cmakedefine HAVE__MKDIR 1
#cmakedefine HAVE_PUTENV 1
#cmakedefine HAVE_MKTEMP 1
#cmakedefine HAVE_MKSTEMP 1
#cmakedefine HAVE_STRERROR 1
#cmakedefine HAVE_STD_COUNT 1
#cmakedefine HAVE_ASPRINTF 1
#cmakedefine HAVE_WPRINTF 1
#cmakedefine HAVE_SNPRINTF 1
#cmakedefine HAVE_POSIX_PRINTF 1
#cmakedefine HAVE_FCNTL 1
#cmakedefine HAVE_INTMAX_T 1
#cmakedefine HAVE_INTTYPES_H_WITH_UINTMAX 1
#cmakedefine HAVE_DECL_ISTREAMBUF_ITERATOR 1
#cmakedefine CXX_GLOBAL_CSTD 1
#cmakedefine HAVE_GETCWD 1
#cmakedefine HAVE_STPCPY 1
#cmakedefine HAVE_STRCASECMP 1
#cmakedefine HAVE_STRDUP 1
#cmakedefine HAVE_STRTOUL 1
#cmakedefine HAVE___FSETLOCKING 1
#cmakedefine HAVE_MEMPCPY 1
#cmakedefine HAVE___ARGZ_COUNT 1
#cmakedefine HAVE___ARGZ_NEXT 1
#cmakedefine HAVE___ARGZ_STRINGIFY 1
#cmakedefine HAVE_SETLOCALE 1
#cmakedefine HAVE_TSEARCH 1
#cmakedefine HAVE_GETEGID 1
#cmakedefine HAVE_GETGID 1
#cmakedefine HAVE_GETUID 1
#cmakedefine HAVE_WCSLEN 1
#cmakedefine HAVE_MKFIFO 1
#cmakedefine HAVE_WPRINTF 1
#cmakedefine HAVE_LONG_DOUBLE 1
#cmakedefine HAVE_LONG_LONG 1
#cmakedefine HAVE_WCHAR_T 1
#cmakedefine HAVE_WINT_T 1
#cmakedefine HAVE_STDINT_H_WITH_UINTMAX 1
#cmakedefine HAVE_LC_MESSAGES 1    
#cmakedefine HAVE_SSTREAM 1
#cmakedefine HAVE_ARGZ_H 1
#cmakedefine SIZEOF_WCHAR_T_IS_2 1
#cmakedefine SIZEOF_WCHAR_T_IS_4 1

#ifdef SIZEOF_WCHAR_T_IS_2
#  define SIZEOF_WCHAR_T 2
#else
#  ifdef SIZEOF_WCHAR_T_IS_4
#    define SIZEOF_WCHAR_T 4
#  endif
#endif

#cmakedefine GETTEXT_FOUND 1

#cmakedefine HAVE_ALLOCA 1
#cmakedefine HAVE_SYMBOL_ALLOCA 1
#if defined(HAVE_SYMBOL_ALLOCA) && !defined(HAVE_ALLOCA)
#define HAVE_ALLOCA
#endif

#cmakedefine HAVE_ICONV_CONST 1
#ifdef HAVE_ICONV_CONST
#define ICONV_CONST const
#else
#define ICONV_CONST
#endif

#ifdef _MSC_VER
#undef HAVE_OPEN  // use _open instead
#define pid_t int
#define PATH_MAX 512
#endif

#ifdef _WIN32 
#undef HAVE_MKDIR // use _mkdir instead
#endif

#define BOOST_ALL_NO_LIB 1

#if defined(HAVE_OSTREAM) && defined(HAVE_LOCALE) && defined(HAVE_SSTREAM)
#  define USE_BOOST_FORMAT 1
#else
#  define USE_BOOST_FORMAT 0
#endif

#ifdef _DEBUG
#  define ENABLE_ASSERTIONS 1
#endif

#ifndef ENABLE_ASSERTIONS
#  define BOOST_DISABLE_ASSERTS 1
#endif
#define BOOST_ENABLE_ASSERT_HANDLER 1

#define BOOST_DISABLE_THREADS 1
#define BOOST_NO_WSTRING 1

#ifdef __CYGWIN__
#  define BOOST_POSIX 1
#  define BOOST_POSIX_API 1
#  define BOOST_POSIX_PATH 1
#endif

#ifndef HAVE_NEWAPIS_H
#  define WANT_GETFILEATTRIBUTESEX_WRAPPER 1
#endif

/*
 * the FreeBSD libc uses UCS4, but libstdc++ has no proper wchar_t
 * support compiled in:
 * http://gcc.gnu.org/onlinedocs/libstdc++/faq/index.html#3_9
 * And we are not interested at all what libc
 * does: What we need is a 32bit wide wchar_t, and a libstdc++ that
 * has the needed wchar_t support and uses UCS4. Whether it
 * implements this with the help of libc, or whether it has own code
 * does not matter for us, because we don't use libc directly (Georg)
*/
#if defined(HAVE_WCHAR_T) && SIZEOF_WCHAR_T == 4 && !defined(__FreeBSD__) && !defined(__FreeBSD_kernel__)
#  define USE_WCHAR_T
#endif

#if defined(MAKE_INTL_LIB) && defined(_MSC_VER)
#define __attribute__(x)
#define inline
#define uintmax_t UINT_MAX
#endif

#ifdef _MSC_VER
#ifdef HAVE_CHMOD
#undef HAVE_CHMOD
#endif
#endif

#ifdef HAVE_CHMOD
#define HAVE_MODE_T
#endif



#endif
