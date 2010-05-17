// -*- C++ -*-
/**
 * \file WordLangTuple.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author John Levon
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef WORD_LANG_TUPLE_H
#define WORD_LANG_TUPLE_H

#include "support/docstring.h"


namespace lyx {


/**
 * A word and its given language code ("en_US")
 * plus a variety if needed.
 * This is used for spellchecking.
 */
class WordLangTuple {
public:
	WordLangTuple() {}

	WordLangTuple(docstring const & w, std::string const & c,
		      std::string const & v = std::string())
		: word_(w), code_(c), variety_(v)
	{}

	/// return the word
	docstring const & word() const {
		return word_;
	}

	/// return its language code
	std::string const & lang_code() const {
		return code_;
	}

	/// return the language variety
	std::string const & lang_variety() const {
		return variety_;
	}

private:
	/// the word
	docstring word_;
	/// language code of word
	std::string code_;
	/// language variety of word
	std::string variety_;
};


} // namespace lyx

#endif // WORD_LANG_TUPLE_H
