// -*- C++ -*-
/**
 * \file bufferview_funcs.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Lars Gullik Bj�nnes
 * \author Jean-Marc Lasgouttes
 * \author Angus Leeming
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef BUFFERVIEW_FUNCS_H
#define BUFFERVIEW_FUNCS_H

#include <string>
#include <vector>

namespace lyx {

class Point;
class BufferView;
class DocIterator;
class Inset_code;
class Font;

namespace bv_funcs {

/// Set \param data using \param font and \param toggle. Return success.
bool font2string(Font const & font, bool toggle, std::string & data);

/// Set \param font and \param toggle using \param data. Return success.
bool string2font(std::string const & data, Font & font, bool & toggle);

/** Returns the current freefont, encoded as a std::string to be passed to the
 *  frontends.
 */
std::string const freefont2string();

Point getPos(BufferView const & bv, DocIterator const & dit, bool boundary);

enum CurStatus {
	CUR_INSIDE,
	CUR_ABOVE,
	CUR_BELOW
};


CurStatus status(BufferView const * bv, DocIterator const & dit);


Point coordOffset(BufferView const & bv, DocIterator const & dit, bool boundary);

/// Moves cursor to the next inset with one of the given codes.
void gotoInset(BufferView * bv, std::vector<Inset_code> const & codes,
	       bool same_content);

/// Moves cursor to the next inset with given code.
void gotoInset(BufferView * bv, Inset_code code, bool same_content);

/// Looks for next inset with one of the the given code
bool findInset(DocIterator & dit, std::vector<Inset_code> const & codes,
	       bool same_content);

/// Looks for next inset with the given code
void findInset(DocIterator & dit, Inset_code code, bool same_content);


} // namespace bv_funcs


} // namespace lyx

#endif