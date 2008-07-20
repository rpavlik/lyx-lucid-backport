/**
 * \file boost.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Lars Gullik Bj�nnes
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "debug.h"

#include <boost/assert.hpp>

#include <cstdlib>
#include <exception>


using std::endl;

namespace boost {

void throw_exception(std::exception const & e)
{
	lyx::lyxerr << "Exception caught:\n"
	       << e.what() << endl;
	BOOST_ASSERT(false);
}


void assertion_failed(char const * expr, char const * function,
		      char const * file, long line)
{
	lyx::lyxerr << "Assertion triggered in " << function
	       << " by failing check \"" << expr << "\""
	       << " in file " << file << ":" << line << endl;
	::abort();
}

} // namespace boost
