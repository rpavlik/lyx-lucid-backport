// -*- C++ -*-
/**
 * \file InsetMathRef.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef REF_INSET_H
#define REF_INSET_H


#include "CommandInset.h"


namespace lyx {
class Buffer;

// for \ref
class InsetMathRef : public CommandInset {
public:
	///
	InsetMathRef(Buffer * buf);
	///
	explicit InsetMathRef(Buffer * buf, docstring const & data);
	///
	//void write(WriteStream & os) const;
	///
	void infoize(odocstream & os) const;
	///
	mode_type currentMode() const { return TEXT_MODE; }
	///
	bool lockedMode() const { return true; }
	///
	bool asciiOnly() const { return true; }
	///
	docstring const screenLabel() const;
	///
	void validate(LaTeXFeatures & features) const;
	///
	virtual InsetMathRef * asRefInset() { return this; }

	/// docbook output
	int docbook(odocstream & os, OutputParams const &) const;
	/// generate something that will be understood by the Dialogs.
	std::string const createDialogStr(std::string const & name) const;

	struct ref_type_info {
		///
		docstring latex_name;
		///
		docstring gui_name;
		///
		docstring short_gui_name;
	};
	static ref_type_info types[];
	///
	static int getType(docstring const & name);
	///
	static docstring const & getName(int type);
protected:
	///
	virtual void doDispatch(Cursor & cur, FuncRequest & cmd);
	///
	bool getStatus(Cursor &, FuncRequest const &, FuncStatus &) const;
private:
	///
	virtual Inset * clone() const;
};



} // namespace lyx
#endif
