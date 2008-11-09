/**
 * \file InsetIndex.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Lars Gullik Bj�nnes
 *
 * Full author contact details are available in file CREDITS.
 */
#include <config.h>

#include "InsetIndex.h"

#include "Buffer.h"
#include "DispatchResult.h"
#include "FuncRequest.h"
#include "FuncStatus.h"
#include "LaTeXFeatures.h"
#include "MetricsInfo.h"
#include "sgml.h"
#include "TocBackend.h"

#include "support/docstream.h"
#include "support/gettext.h"
#include "support/lstrings.h"

#include <ostream>

using namespace std;
using namespace lyx::support;

namespace lyx {

/////////////////////////////////////////////////////////////////////
//
// InsetIndex
//
///////////////////////////////////////////////////////////////////////


InsetIndex::InsetIndex(Buffer const & buf)
	: InsetCollapsable(buf)
{}


int InsetIndex::latex(odocstream & os,
			  OutputParams const & runparams) const
{
	os << "\\index";
	os << '{';
	odocstringstream ods;
	int i = InsetText::latex(ods, runparams);
	bool sorted = false;
	// correctly sort macros and formatted strings
	// if we do find a command, prepend a plain text
	// version of the content to get sorting right,
	// e.g. \index{LyX@\LyX}, \index{text@\textbf{text}}
	// Don't do that if the user entered '@' himself, though.
	if (contains(ods.str(), '\\') && !contains(ods.str(), '@')) {
		odocstringstream odss;
		if (InsetText::plaintext(odss, runparams) > 0) {
			// remove remaining \'s for the sorting part
			os << subst(odss.str(), from_ascii("\\"), docstring());
			os << '@';
			sorted = true;
		}
	}
	// if a hierarchy tag '!' is used, ommit this in the post-@ part.
	if (sorted && contains(ods.str(), '!')) {
		string dummy;
		// FIXME unicode
		os << from_utf8(rsplit(to_utf8(ods.str()), dummy, '!'));
	} else
		i = InsetText::latex(os, runparams);
	os << '}';
	return i;
}


int InsetIndex::docbook(odocstream & os, OutputParams const & runparams) const
{
	os << "<indexterm><primary>";
	int const i = InsetText::docbook(os, runparams);
	os << "</primary></indexterm>";
	return i;
}


void InsetIndex::write(ostream & os) const
{
	os << to_utf8(name()) << "\n";
	InsetCollapsable::write(os);
}


void InsetIndex::addToToc(DocIterator const & cpit)
{
	DocIterator pit = cpit;
	pit.push_back(CursorSlice(*this));

	Toc & toc = buffer().tocBackend().toc("index");
	docstring str;
	str = getNewLabel(str);
	toc.push_back(TocItem(pit, 0, str));
	// Proceed with the rest of the inset.
	InsetCollapsable::addToToc(cpit);
}


/////////////////////////////////////////////////////////////////////
//
// InsetPrintIndex
//
///////////////////////////////////////////////////////////////////////

InsetPrintIndex::InsetPrintIndex(InsetCommandParams const & p)
	: InsetCommand(p, string())
{}


ParamInfo const & InsetPrintIndex::findInfo(string const & /* cmdName */)
{
	static ParamInfo param_info_;
	if (param_info_.empty())
		param_info_.add("name", ParamInfo::LATEX_REQUIRED);
	return param_info_;
}


docstring InsetPrintIndex::screenLabel() const
{
	return _("Index");
}


void InsetPrintIndex::validate(LaTeXFeatures & features) const
{
	features.require("makeidx");
}


InsetCode InsetPrintIndex::lyxCode() const
{
	return INDEX_PRINT_CODE;
}

} // namespace lyx
