/**
 * \file os_cygwin.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Ruurd A. Reitsma
 * \author Claus Hentschel
 * \author Angus Leeming
 *
 * Full author contact details are available in file CREDITS.
 *
 * Various OS specific functions
 */

#include <config.h>

#include "support/os.h"

#include "support/FileName.h"
#include "support/lstrings.h"
#include "support/debug.h"

#include <windows.h>
#include <io.h>
#include <windef.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <limits.h>
#include <stdlib.h>

#include <sys/cygwin.h>

using namespace std;

namespace lyx {

void emergencyCleanup();

namespace support {
namespace os {

namespace {

bool windows_style_tex_paths_ = false;

// In both is_posix_path() and is_windows_path() it is assumed that
// a valid posix or pseudo-windows path is passed. They simply tell
// whether the path looks posix/pseudo-windows or not.

bool is_posix_path(string const & p)
{
	return  p.empty() ||
		(!contains(p, '\\') && (p.length() <= 1 || p[1] != ':'));
}

// This is a test for a win32 style path with forward slashes (pseudo-windows).

bool is_windows_path(string const & p)
{
	return p.empty() || (!contains(p, '\\') && p[0] != '/');
}


enum PathStyle {
	posix,
	windows
};


/// Convert a path to or from posix style.
/// \p p is encoded in local 8bit encoding or utf8.
/// The result is returned in the same encoding as \p p.
string convert_path(string const & p, PathStyle const & target)
{
	char path_buf[PATH_MAX];

	if ((target == posix && is_posix_path(p)) ||
	    (target == windows && is_windows_path(p)))
		return p;

	path_buf[0] = '\0';

	// cygwin_conv_to_posix_path and cygwin_conv_to_win32_path do not
	// care about the encoding.
	if (target == posix)
		cygwin_conv_to_posix_path(p.c_str(), path_buf);
	else
		cygwin_conv_to_win32_path(p.c_str(), path_buf);

	return subst(path_buf[0] ? path_buf : p, '\\', '/');
}


/// Convert a path list to or from posix style.
/// \p p is encoded in local 8bit encoding or utf8.
/// The result is returned in the same encoding as \p p.
string convert_path_list(string const & p, PathStyle const & target)
{
	if (p.empty())
		return p;

	char const * const pc = p.c_str();
	PathStyle const actual = cygwin_posix_path_list_p(pc) ? posix : windows;

	if (target != actual) {
		int const target_size = (target == posix) ?
				cygwin_win32_to_posix_path_list_buf_size(pc) :
				cygwin_posix_to_win32_path_list_buf_size(pc);

		char * ptr = new char[target_size];

		if (ptr) {
			// FIXME: See comment in convert_path() above
			if (target == posix)
				cygwin_win32_to_posix_path_list(pc, ptr);
			else
				cygwin_posix_to_win32_path_list(pc, ptr);

			string path_list = subst(ptr, '\\', '/');
			delete ptr;
			return path_list;
		}
	}

	return subst(p, '\\', '/');
}


BOOL terminate_handler(DWORD event)
{
	if (event == CTRL_CLOSE_EVENT
	    || event == CTRL_LOGOFF_EVENT
	    || event == CTRL_SHUTDOWN_EVENT) {
		lyx::emergencyCleanup();
		return TRUE;
	}
	return FALSE;
}

} // namespace anon

void init(int, char *[])
{
	// Make sure that the TEMP variable is set
	// and sync the Windows environment.
	setenv("TEMP", "/tmp", false);
	cygwin_internal(CW_SYNC_WINENV);

	// Catch shutdown events.
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)terminate_handler, TRUE);
}


string current_root()
{
	return string("/");
}


bool isFilesystemCaseSensitive()
{
	return false;
}


docstring::size_type common_path(docstring const & p1, docstring const & p2)
{
	docstring::size_type i = 0;
	docstring::size_type const p1_len = p1.length();
	docstring::size_type const p2_len = p2.length();
	while (i < p1_len && i < p2_len && uppercase(p1[i]) == uppercase(p2[i]))
		++i;
	if ((i < p1_len && i < p2_len)
	    || (i < p1_len && p1[i] != '/' && i == p2_len)
	    || (i < p2_len && p2[i] != '/' && i == p1_len))
	{
		if (i)
			--i;     // here was the last match
		while (i && p1[i] != '/')
			--i;
	}
	return i;
}


bool path_prefix_is(string const & path, string const & pre)
{
	return path_prefix_is(const_cast<string &>(path), pre, CASE_UNCHANGED);
}


bool path_prefix_is(string & path, string const & pre, path_case how)
{
	docstring const p1 = from_utf8(path);
	docstring const p2 = from_utf8(pre);
	docstring::size_type const p1_len = p1.length();
	docstring::size_type const p2_len = p2.length();
	docstring::size_type common_len = common_path(p1, p2);

	if (p2[p2_len - 1] == '/' && p1_len != p2_len)
		++common_len;

	if (common_len != p2_len)
		return false;

	if (how == CASE_ADJUSTED && !prefixIs(path, pre)) {
		if (p1_len < common_len)
			path = to_utf8(p2.substr(0, p1_len));
		else
			path = to_utf8(p2 + p1.substr(common_len,
							p1_len - common_len));
	}

	return true;
}


string external_path(string const & p)
{
	return convert_path(p, PathStyle(posix));
}


string internal_path(string const & p)
{
	return convert_path(p, PathStyle(posix));
}


string external_path_list(string const & p)
{
	return convert_path_list(p, PathStyle(posix));
}


string internal_path_list(string const & p)
{
	return convert_path_list(p, PathStyle(posix));
}


string latex_path(string const & p)
{
	// We may need a posix style path or a windows style path (depending
	// on windows_style_tex_paths_), but we use always forward slashes,
	// since it gets written into a .tex file.

	if (windows_style_tex_paths_ && FileName::isAbsolute(p)) {
		string dos_path = convert_path(p, PathStyle(windows));
		LYXERR(Debug::LATEX, "<Path correction for LaTeX> ["
			<< p << "]->>[" << dos_path << ']');
		return dos_path;
	}

	return convert_path(p, PathStyle(posix));
}


bool is_valid_strftime(string const & p)
{
	string::size_type pos = p.find_first_of('%');
	while (pos != string::npos) {
		if (pos + 1 == string::npos)
			break;
		if (!containsOnly(p.substr(pos + 1, 1),
			"aAbBcCdDeEFgGhHIjklmMnOpPrRsStTuUVwWxXyYzZ%+"))
			return false;
		if (pos + 2 == string::npos)
		      break;
		pos = p.find_first_of('%', pos + 2);
	}
	return true;
}


// returns a string suitable to be passed to popen when
// reading a pipe
char const * popen_read_mode()
{
	return "r";
}


string const & nulldev()
{
	static string const nulldev_ = "/dev/null";
	return nulldev_;
}


shell_type shell()
{
	return UNIX;
}


char path_separator()
{
	return ':';
}


void windows_style_tex_paths(bool use_windows_paths)
{
	windows_style_tex_paths_ = use_windows_paths;
}


bool canAutoOpenFile(string const & ext, auto_open_mode const mode)
{
	if (ext.empty())
		return false;

	string const full_ext = "." + ext;

	DWORD bufSize = MAX_PATH + 100;
	TCHAR buf[MAX_PATH + 100];
	// reference: http://msdn.microsoft.com/en-us/library/bb773471.aspx
	char const * action = (mode == VIEW) ? "open" : "edit";
	return S_OK == AssocQueryString(ASSOCF_INIT_IGNOREUNKNOWN,
		ASSOCSTR_EXECUTABLE, full_ext.c_str(), action, buf, &bufSize);
}


bool autoOpenFile(string const & filename, auto_open_mode const mode)
{
	// reference: http://msdn.microsoft.com/en-us/library/bb762153.aspx
	string const win_path = to_local8bit(from_utf8(convert_path(filename, PathStyle(windows))));
	char const * action = (mode == VIEW) ? "open" : "edit";
	return reinterpret_cast<int>(ShellExecute(NULL, action,
		win_path.c_str(), NULL, NULL, 1)) > 32;
}


string real_path(string const & path)
{
	char rpath[PATH_MAX + 1];
	char * result = realpath(path.c_str(), rpath);
	return FileName::fromFilesystemEncoding(result ? rpath : path).absFilename();
}

} // namespace os
} // namespace support
} // namespace lyx
