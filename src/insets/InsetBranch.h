// -*- C++ -*-
/**
 * \file InsetBranch.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Martin Vermeer
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef INSETBRANCH_H
#define INSETBRANCH_H

#include "InsetCollapsable.h"

namespace lyx {

class InsetBranchParams {
public:
	///
	explicit InsetBranchParams(docstring const & b = docstring())
		: branch(b) {}
	///
	void write(std::ostream & os) const;
	///
	void read(Lexer & lex);
	///
	docstring branch;
};


/////////////////////////////////////////////////////////////////////////
//
// InsetBranch
//
/////////////////////////////////////////////////////////////////////////

/// The Branch inset for alternative, conditional output.

class InsetBranch : public InsetCollapsable
{
public:
	///
	InsetBranch(Buffer *, InsetBranchParams const &);

	///
	static std::string params2string(InsetBranchParams const &);
	///
	static void string2params(std::string const &, InsetBranchParams &);
	///
	docstring branch() const { return params_.branch; }
	///
	void rename(docstring const & newname) { params_.branch = newname; }

private:
	///
	InsetCode lyxCode() const { return BRANCH_CODE; }
	///
	void write(std::ostream &) const;
	///
	void read(Lexer & lex);
	///
	docstring const buttonLabel(BufferView const & bv) const;
	///
	ColorCode backgroundColor(PainterInfo const &) const;
	///
	void latex(otexstream &, OutputParams const &) const;
	///
	int plaintext(odocstream &, OutputParams const &) const;
	///
	int docbook(odocstream &, OutputParams const &) const;
	///
	docstring xhtml(XHTMLStream &, OutputParams const &) const;
	///
	void toString(odocstream &) const;
	///
	void forToc(docstring &, size_t) const;
	///
	void validate(LaTeXFeatures &) const;
	///
	docstring contextMenuName() const;
	///
	void addToToc(DocIterator const &) const;
	///
	InsetBranchParams const & params() const { return params_; }
	///
	void setParams(InsetBranchParams const & params) { params_ = params; }

	/** \returns true if params_.branch is listed as 'selected' in
	    \c buffer. This handles the case of child documents.
	 */
	bool isBranchSelected() const;
	/*!
	 * Is the content of this inset part of the output document?
	 *
	 * Note that Branch insets are only considered part of the
	 * document when they are selected.
	 */
	bool producesOutput() const { return isBranchSelected(); }
	///
	bool getStatus(Cursor &, FuncRequest const &, FuncStatus &) const;
	///
	bool isMacroScope() const;
	///
	docstring toolTip(BufferView const & bv, int x, int y) const;
	///
	bool usePlainLayout() const { return false; }
	///
	void doDispatch(Cursor & cur, FuncRequest & cmd);
	///
	docstring layoutName() const { return from_ascii("Branch"); }
	///
	Inset * clone() const { return new InsetBranch(*this); }

	///
	friend class InsetBranchParams;
	///
	InsetBranchParams params_;
};

} // namespace lyx

#endif
