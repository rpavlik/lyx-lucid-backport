// -*- C++ -*-
/**
 * \file InsetSpace.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Asger Alstrup Nielsen
 * \author Jean-Marc Lasgouttes
 * \author Lars Gullik Bj�nnes
 * \author J�rgen Spitzm�ller
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef INSET_SPACE_H
#define INSET_SPACE_H


#include "Inset.h"


namespace lyx {

class LaTeXFeatures;

///  Used to insert different kinds of spaces
class InsetSpace : public Inset {
public:

	/// The different kinds of spaces we support
	enum Kind {
		/// Normal space ('\ ')
		NORMAL,
		/// Protected (no break) space ('~')
		PROTECTED,
		/// Thin space ('\,')
		THIN,
		/// \quad (1em)
		QUAD,
		/// \qquad (2em)
		QQUAD,
		/// \enspace (0.5em unbreakable)
		ENSPACE,
		/// \enspace (0.5em breakable)
		ENSKIP,
		/// Negative thin space ('\negthinspace')
		NEGTHIN
	};

	///
	InsetSpace();

	///
	explicit
	InsetSpace(Kind k);
	///
	Kind kind() const;
	///
	bool metrics(MetricsInfo &, Dimension &) const;
	///
	void draw(PainterInfo & pi, int x, int y) const;
	///
	void write(Buffer const &, std::ostream &) const;
	/// Will not be used when lyxf3
	void read(Buffer const &, Lexer & lex);
	///
	int latex(Buffer const &, odocstream &,
		  OutputParams const &) const;
	///
	int plaintext(Buffer const &, odocstream &,
		      OutputParams const &) const;
	///
	int docbook(Buffer const &, odocstream &,
		    OutputParams const &) const;
	/// the string that is passed to the TOC
	void textString(Buffer const &, odocstream &) const;
	///
	Inset::Code lyxCode() const { return Inset::SPACE_CODE; }
	/// We don't need \begin_inset and \end_inset
	bool directWrite() const { return true; }

	// should this inset be handled like a normal charater
	bool isChar() const;
	/// is this equivalent to a letter?
	bool isLetter() const;
	/// is this equivalent to a space (which is BTW different from
	// a line separator)?
	bool isSpace() const;
private:
	virtual std::auto_ptr<Inset> doClone() const;

	/// And which kind is this?
	Kind kind_;
};


} // namespace lyx

#endif // INSET_SPACE_H
