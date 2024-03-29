/**
 * \file os.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Ruurd A. Reitsma
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#if defined(__CYGWIN__) || defined(__CYGWIN32__)
#include "support/os_cygwin.cpp"
#elif defined(_WIN32)
#include "support/os_win32.cpp"
#else
#include "support/os_unix.cpp"
#endif

// Static assert to break compilation on platforms where
// int/unsigned int is not 4 bytes. Added to make sure that
// e.g., the author hash is always 32-bit.
template<bool Condition> struct static_assert_helper;
template <> struct static_assert_helper<true> {};
enum { 
	dummy = sizeof(static_assert_helper<sizeof(int) == 4>)
};

namespace lyx {
namespace support {
namespace os {

string const python()
{
	// Use the -tt switch so that mixed tab/whitespace indentation is
	// an error
	static string const command("python -tt");
	return command;
}

}
}
}
