// -*- C++ -*-
/**
 * \file MathFactory.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef MATH_FACTORY_H
#define MATH_FACTORY_H

#include "MathParser.h"

#include "support/strfwd.h"

#include <map>

namespace lyx {

class MathAtom;
class MathData;


MathAtom createInsetMath(docstring const &);
MathAtom createInsetMath(char const * const);

/** Fills ar with the contents of str.
 *  str is created by the frontend dialog's and returned to the LyX core.
 *  The function returns true if successful.
 */
bool createInsetMath_fromDialogStr(docstring const &, MathData &);

/** Tells whether the argument is an ascii character or is marked as
  * mathalpha in the unicodesymbols file.
  */
bool isAsciiOrMathAlpha(char_type);

typedef std::map<docstring, latexkeys> MathWordList;
MathWordList const & mathedWordList();

} // namespace lyx

#endif