// -*- C++ -*-
/**
 * \file lyxfind.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Lars Gullik Bjønnes
 * \author John Levon
 * \author Jürgen Vigna
 * \author Alfredo Braunstein
 * \author Tommaso Cucinotta
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef LYXFIND_H
#define LYXFIND_H

#include "support/strfwd.h"

// FIXME
#include "support/docstring.h"

namespace lyx {



class Buffer;
class BufferView;
class DocIterator;
class FuncRequest;
class Text;

/** Encode the parameters needed to find \c search as a string
 *  that can be dispatched to the LyX core in a FuncRequest wrapper.
 */
docstring const find2string(docstring const & search,
			      bool casesensitive,
			      bool matchword,
			      bool forward);

/** Encode the parameters needed to replace \c search with \c replace
 *  as a string that can be dispatched to the LyX core in a FuncRequest
 *  wrapper.
 */
docstring const replace2string(docstring const & replace,
				 docstring const & search,
				 bool casesensitive,
				 bool matchword,
				 bool all,
				 bool forward);

/** Parse the string encoding of the find request that is found in
 *  \c ev.argument and act on it.
 * The string is encoded by \c find2string.
 * \return true if the string was found.
 */
bool lyxfind(BufferView * bv, FuncRequest const & ev);

/** Parse the string encoding of the replace request that is found in
 *  \c ev.argument and act on it.
 * The string is encoded by \c replace2string.
 * \return whether we did anything
 */
bool lyxreplace(BufferView * bv, 
		FuncRequest const &, bool has_deleted = false);

/// find the next change in the buffer
bool findNextChange(BufferView * bv);

/// find the previous change in the buffer
bool findPreviousChange(BufferView * bv);

/// find the change in the buffer
/// \param next true to find the next change, otherwise the previous
bool findChange(BufferView * bv, bool next);

// Hopefully, nobody will ever replace with something like this
#define LYX_FR_NULL_STRING "__LYX__F&R__NULL__STRING__"

class FindAndReplaceOptions {
public:
	typedef enum {
		S_BUFFER,
		S_DOCUMENT,
		S_OPEN_BUFFERS,
		S_ALL_MANUALS
	} SearchScope;
	FindAndReplaceOptions(
		docstring const & search,
		bool casesensitive,
		bool matchword,
		bool forward,
		bool expandmacros,
		bool ignoreformat,
		bool regexp,
		docstring const & replace,
		bool keep_case,
		SearchScope scope = S_BUFFER
	);
	FindAndReplaceOptions() {  }
	docstring search;
	bool casesensitive;
	bool matchword;
	bool forward;
	bool expandmacros;
	bool ignoreformat;
	bool regexp;
	docstring replace;
	bool keep_case;
	SearchScope scope;
};

/// Write a FindAdvOptions instance to a stringstream
std::ostringstream & operator<<(std::ostringstream & os, lyx::FindAndReplaceOptions const & opt);

/// Read a FindAdvOptions instance from a stringstream
std::istringstream & operator>>(std::istringstream & is, lyx::FindAndReplaceOptions & opt);

/// Perform a FindAdv operation.
bool findAdv(BufferView * bv, FindAndReplaceOptions const & opt);
	
/** Computes the simple-text or LaTeX export (depending on opt) of buf starting
 ** from cur and ending len positions after cur, if len is positive, or at the
 ** paragraph or innermost inset end if len is -1.
 **
 ** This is useful for computing opt.search from the SearchAdvDialog controller (ControlSearchAdv).
 ** Ideally, this should not be needed, and the opt.search field should become a Text const &.
 **/
docstring stringifyFromForSearch(
	FindAndReplaceOptions const & opt,
	DocIterator const & cur,
	int len = -1);

} // namespace lyx

#endif // LYXFIND_H
