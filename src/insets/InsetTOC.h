// -*- C++ -*-
/**
 * \file InsetTOC.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Lars Gullik Bjønnes
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef INSET_TOC_H
#define INSET_TOC_H

#include "InsetCommand.h"


namespace lyx {


/// Used to insert table of contents and similar lists
/// at present, supports only \tableofcontents. Other
/// such commands, such as \listoffigures, are supported
/// by InsetFloatList.
class InsetTOC : public InsetCommand {
public:
	///
	InsetTOC(Buffer * buf, InsetCommandParams const &);

	/// \name Public functions inherited from Inset class
	//@{
	///
	InsetCode lyxCode() const { return TOC_CODE; }
	///
	DisplayType display() const { return AlignCenter; }
	///
	int plaintext(odocstream &, OutputParams const &) const;
	///
	int docbook(odocstream &, OutputParams const &) const;
	///
	docstring xhtml(XHTMLStream & xs, OutputParams const &) const;
	///
	void doDispatch(Cursor & cur, FuncRequest & cmd);
	//@}

	/// \name Static public methods obligated for InsetCommand derived classes
	//@{
	///
	static ParamInfo const & findInfo(std::string const &);
	///
	static std::string defaultCommand() { return "tableofcontents"; }
	///
	static bool isCompatibleCommand(std::string const & cmd)
		{ return cmd == defaultCommand(); }
	//@}

private:
	/// \name Private functions inherited from Inset class
	//@{
	///
	Inset * clone() const { return new InsetTOC(*this); }
	//@}

	/// \name Private functions inherited from InsetCommand class
	//@{
	///
	docstring screenLabel() const;
	//@}
};


} // namespace lyx

#endif
