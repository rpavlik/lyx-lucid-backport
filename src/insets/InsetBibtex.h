// -*- C++ -*-
/**
 * \file InsetBibtex.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Alejandro Aguilar Sierra
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef INSET_BIBTEX_H
#define INSET_BIBTEX_H


#include <vector>
#include "InsetCommand.h"

#include "support/FileName.h"


namespace lyx {

/** Used to insert BibTeX's information
  */
class InsetBibtex : public InsetCommand {
public:
	///
	InsetBibtex(InsetCommandParams const &);
	///
	docstring const getScreenLabel(Buffer const &) const;
	///
	EDITABLE editable() const { return IS_EDITABLE; }
	///
	Inset::Code lyxCode() const { return Inset::BIBTEX_CODE; }
	///
	DisplayType display() const { return AlignCenter; }
	///
	int latex(Buffer const &, odocstream &, OutputParams const &) const;
	///
	void fillWithBibKeys(Buffer const & buffer,
		std::vector<std::pair<std::string, docstring> > & keys) const;
	///
	std::vector<support::FileName> const getFiles(Buffer const &) const;
	///
	bool addDatabase(std::string const &);
	///
	bool delDatabase(std::string const &);
	///
	void validate(LaTeXFeatures &) const;
protected:
	virtual void doDispatch(Cursor & cur, FuncRequest & cmd);
private:
	virtual std::auto_ptr<Inset> doClone() const;

};


} // namespace lyx

#endif // INSET_BIBTEX_H