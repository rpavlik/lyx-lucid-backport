/**
 * \file InsetFloat.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author J�rgen Vigna
 * \author Lars Gullik Bj�nnes
 * \author J�rgen Spitzm�ller
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "InsetFloat.h"
#include "InsetCaption.h"

#include "Buffer.h"
#include "BufferParams.h"
#include "BufferView.h"
#include "Counters.h"
#include "Cursor.h"
#include "DispatchResult.h"
#include "Floating.h"
#include "FloatList.h"
#include "FuncRequest.h"
#include "FuncStatus.h"
#include "InsetList.h"
#include "LaTeXFeatures.h"
#include "Lexer.h"
#include "OutputParams.h"
#include "ParIterator.h"
#include "TextClass.h"

#include "support/convert.h"
#include "support/debug.h"
#include "support/docstream.h"
#include "support/gettext.h"
#include "support/lstrings.h"

#include "frontends/Application.h"

using namespace std;


namespace lyx {

// With this inset it will be possible to support the latex package
// float.sty, and I am sure that with this and some additional support
// classes we can support similar functionality in other formats
// (read DocBook).
// By using float.sty we will have the same handling for all floats, both
// for those already in existance (table and figure) and all user created
// ones�. So suddenly we give the users the possibility of creating new
// kinds of floats on the fly. (and with a uniform look)
//
// API to float.sty:
//   \newfloat{type}{placement}{ext}[within]
//     type      - The "type" of the new class of floats, like program or
//                 algorithm. After the appropriate \newfloat, commands
//                 such as \begin{program} or \end{algorithm*} will be
//                 available.
//     placement - The default placement for the given class of floats.
//                 They are like in standard LaTeX: t, b, p and h for top,
//                 bottom, page, and here, respectively. On top of that
//                 there is a new type, H, which does not really correspond
//                 to a float, since it means: put it "here" and nowhere else.
//                 Note, however that the H specifier is special and, because
//                 of implementation details cannot be used in the second
//                 argument of \newfloat.
//     ext       - The file name extension of an auxiliary file for the list
//                 of figures (or whatever). LaTeX writes the captions to
//                 this file.
//     within    - This (optional) argument determines whether floats of this
//                 class will be numbered within some sectional unit of the
//                 document. For example, if within is equal to chapter, the
//                 floats will be numbered within chapters.
//   \floatstyle{style}
//     style -  plain, boxed, ruled
//   \floatname{float}{floatname}
//     float     -
//     floatname -
//   \floatplacement{float}{placement}
//     float     -
//     placement -
//   \restylefloat{float}
//     float -
//   \listof{type}{title}
//     title -

// � the algorithm float is defined using the float.sty package. Like this
//   \floatstyle{ruled}
//   \newfloat{algorithm}{htbp}{loa}[<sect>]
//   \floatname{algorithm}{Algorithm}
//
// The intention is that floats should be definable from two places:
//          - layout files
//          - the "gui" (i.e. by the user)
//
// From layout files.
// This should only be done for floats defined in a documentclass and that
// does not need any additional packages. The two most known floats in this
// category is "table" and "figure". Floats defined in layout files are only
// stored in lyx files if the user modifies them.
//
// By the user.
// There should be a gui dialog (and also a collection of lyxfuncs) where
// the user can modify existing floats and/or create new ones.
//
// The individual floats will also have some settable
// variables: wide and placement.
//
// Lgb


InsetFloat::InsetFloat(Buffer const & buf, string const & type)
	: InsetCollapsable(buf), name_(from_utf8(type))
{
	setLabel(_("float: ") + floatName(type, buf.params()));
	params_.type = type;
}


InsetFloat::~InsetFloat()
{
	hideDialogs("float", this);
}


docstring InsetFloat::name() const 
{ 
	return "Float:" + name_; 
}


docstring InsetFloat::toolTip(BufferView const & bv, int x, int y) const
{
	if (InsetCollapsable::toolTip(bv, x, y).empty() || isOpen(bv))
		return docstring();

	OutputParams rp(&buffer().params().encoding());
	return getCaptionText(rp);
}


void InsetFloat::doDispatch(Cursor & cur, FuncRequest & cmd)
{
	switch (cmd.action) {

	case LFUN_INSET_MODIFY: {
		InsetFloatParams params;
		string2params(to_utf8(cmd.argument()), params);

		// placement, wide and sideways are not used for subfloats
		if (!params_.subfloat) {
			params_.placement = params.placement;
			params_.wide      = params.wide;
			params_.sideways  = params.sideways;
			setWide(params_.wide, cur.buffer().params(), false);
			setSideways(params_.sideways, cur.buffer().params(), false);
		}

		setNewLabel(cur.buffer().params());
		break;
	}

	case LFUN_INSET_DIALOG_UPDATE: {
		cur.bv().updateDialog("float", params2string(params()));
		break;
	}

	default:
		InsetCollapsable::doDispatch(cur, cmd);
		break;
	}
}


bool InsetFloat::getStatus(Cursor & cur, FuncRequest const & cmd,
		FuncStatus & flag) const
{
	switch (cmd.action) {

	case LFUN_INSET_MODIFY:
	case LFUN_INSET_DIALOG_UPDATE:
		flag.setEnabled(true);
		return true;

	case LFUN_INSET_SETTINGS:
		if (InsetCollapsable::getStatus(cur, cmd, flag)) {
			flag.setEnabled(flag.enabled() && !params_.subfloat);
			return true;
		} else
			return false;

	default:
		return InsetCollapsable::getStatus(cur, cmd, flag);
	}
}


void InsetFloat::updateLabels(ParIterator const & it)
{
	Counters & cnts =
		buffer().masterBuffer()->params().documentClass().counters();
	string const saveflt = cnts.current_float();
	bool const savesubflt = cnts.isSubfloat();

	bool const subflt = (it.innerInsetOfType(FLOAT_CODE)
			     || it.innerInsetOfType(WRAP_CODE));
	// floats can only embed subfloats of their own kind
	if (subflt)
		params_.type = saveflt;
	setSubfloat(subflt, buffer().params());

	// Tell to captions what the current float is
	cnts.current_float(params().type);
	cnts.isSubfloat(subflt);

	InsetCollapsable::updateLabels(it);

	//reset afterwards
	cnts.current_float(saveflt);
	cnts.isSubfloat(savesubflt);
}


void InsetFloatParams::write(ostream & os) const
{
	os << "Float " << type << '\n';

	if (!placement.empty())
		os << "placement " << placement << "\n";

	if (wide)
		os << "wide true\n";
	else
		os << "wide false\n";

	if (sideways)
		os << "sideways true\n";
	else
		os << "sideways false\n";
}


void InsetFloatParams::read(Lexer & lex)
{
	lex.setContext("InsetFloatParams::read");
	if (lex.checkFor("placement"))
		lex >> placement;
	lex >> "wide" >> wide;
	lex >> "sideways" >> sideways;
}


void InsetFloat::write(ostream & os) const
{
	params_.write(os);
	InsetCollapsable::write(os);
}


void InsetFloat::read(Lexer & lex)
{
	params_.read(lex);
	setWide(params_.wide, buffer().params());
	setSideways(params_.sideways, buffer().params());
	InsetCollapsable::read(lex);
}


void InsetFloat::validate(LaTeXFeatures & features) const
{
	if (support::contains(params_.placement, 'H'))
		features.require("float");

	if (params_.sideways)
		features.require("rotfloat");

	if (features.inFloat())
		features.require("subfig");

	features.useFloat(params_.type, features.inFloat());
	features.inFloat(true);
	InsetCollapsable::validate(features);
	features.inFloat(false);
}


docstring InsetFloat::editMessage() const
{
	return _("Opened Float Inset");
}


int InsetFloat::latex(odocstream & os, OutputParams const & runparams_in) const
{
	if (runparams_in.inFloat != OutputParams::NONFLOAT) {
		if (runparams_in.moving_arg)
			os << "\\protect";
		os << "\\subfloat";
	
		OutputParams rp = runparams_in;
		docstring const caption = getCaption(rp);
		if (!caption.empty()) {
			os << caption;
		}
		os << '{';
		rp.inFloat = OutputParams::SUBFLOAT;
		int const i = InsetText::latex(os, rp);
		os << "}";
	
		return i + 1;
	}
	OutputParams runparams(runparams_in);
	runparams.inFloat = OutputParams::MAINFLOAT;

	FloatList const & floats = buffer().params().documentClass().floats();
	string tmptype = params_.type;
	if (params_.sideways)
		tmptype = "sideways" + params_.type;
	if (params_.wide && (!params_.sideways ||
			     params_.type == "figure" ||
			     params_.type == "table"))
		tmptype += "*";
	// Figure out the float placement to use.
	// From lowest to highest:
	// - float default placement
	// - document wide default placement
	// - specific float placement
	string placement;
	string const buf_placement = buffer().params().float_placement;
	string const def_placement = floats.defaultPlacement(params_.type);
	if (!params_.placement.empty()
	    && params_.placement != def_placement) {
		placement = params_.placement;
	} else if (params_.placement.empty()
		   && !buf_placement.empty()
		   && buf_placement != def_placement) {
		placement = buf_placement;
	}

	// The \n is used to force \begin{<floatname>} to appear in a new line.
	// The % is needed to prevent two consecutive \n chars in the case
	// when the current output line is empty.
	os << "%\n\\begin{" << from_ascii(tmptype) << '}';
	// We only output placement if different from the def_placement.
	// sidewaysfloats always use their own page
	if (!placement.empty() && !params_.sideways) {
		os << '[' << from_ascii(placement) << ']';
	}
	os << '\n';

	int const i = InsetText::latex(os, runparams);

	// The \n is used to force \end{<floatname>} to appear in a new line.
	// In this case, we do not case if the current output line is empty.
	os << "\n\\end{" << from_ascii(tmptype) << "}\n";

	return i + 4;
}


int InsetFloat::plaintext(odocstream & os, OutputParams const & runparams) const
{
	os << '[' << buffer().B_("float") << ' '
		<< floatName(params_.type, buffer().params()) << ":\n";
	InsetText::plaintext(os, runparams);
	os << "\n]";

	return PLAINTEXT_NEWLINE + 1; // one char on a separate line
}


int InsetFloat::docbook(odocstream & os, OutputParams const & runparams) const
{
	// FIXME Implement subfloat!
	// FIXME UNICODE
	os << '<' << from_ascii(params_.type) << '>';
	int const i = InsetText::docbook(os, runparams);
	os << "</" << from_ascii(params_.type) << '>';

	return i;
}


bool InsetFloat::insetAllowed(InsetCode code) const
{
	// The case that code == FLOAT_CODE is handled in Text3.cpp, 
	// because we need to know what type of float is meant.
	switch(code) {
	case WRAP_CODE:
	case FOOT_CODE:
	case MARGIN_CODE:
		return false;
	default:
		return InsetCollapsable::insetAllowed(code);
	}
}


bool InsetFloat::showInsetDialog(BufferView * bv) const
{
	if (!InsetText::showInsetDialog(bv))
		bv->showDialog("float", params2string(params()),
			const_cast<InsetFloat *>(this));
	return true;
}


void InsetFloat::setWide(bool w, BufferParams const & bp, bool update_label)
{
	params_.wide = w;
	if (update_label)
		setNewLabel(bp);
}


void InsetFloat::setSideways(bool s, BufferParams const & bp, bool update_label)
{
	params_.sideways = s;
	if (update_label)
		setNewLabel(bp);
}


void InsetFloat::setSubfloat(bool s, BufferParams const & bp, bool update_label)
{
	params_.subfloat = s;
	if (update_label)
		setNewLabel(bp);
}


void InsetFloat::setNewLabel(BufferParams const & bp)
{
	docstring lab = _("float: ");

	if (params_.subfloat)
		lab = _("subfloat: ");

	lab += floatName(params_.type, bp);

	if (params_.wide)
		lab += '*';

	if (params_.sideways)
		lab += _(" (sideways)");

	setLabel(lab);
}


docstring InsetFloat::getCaption(OutputParams const & runparams) const
{
	if (paragraphs().empty())
		return docstring();

	ParagraphList::const_iterator pit = paragraphs().begin();
	for (; pit != paragraphs().end(); ++pit) {
		InsetList::const_iterator it = pit->insetList().begin();
		for (; it != pit->insetList().end(); ++it) {
			Inset & inset = *it->inset;
			if (inset.lyxCode() == CAPTION_CODE) {
				odocstringstream ods;
				InsetCaption * ins =
					static_cast<InsetCaption *>(it->inset);
				ins->getOptArg(ods, runparams);
				ods << '[';
				ins->getArgument(ods, runparams);
				ods << ']';
				return ods.str();
			}
		}
	}
	return docstring();
}


docstring InsetFloat::getCaptionText(OutputParams const & runparams) const
{
	if (paragraphs().empty())
		return docstring();

	ParagraphList::const_iterator pit = paragraphs().begin();
	for (; pit != paragraphs().end(); ++pit) {
		InsetList::const_iterator it = pit->insetList().begin();
		for (; it != pit->insetList().end(); ++it) {
			Inset & inset = *it->inset;
			if (inset.lyxCode() == CAPTION_CODE) {
				odocstringstream ods;
				InsetCaption * ins =
					static_cast<InsetCaption *>(it->inset);
				ins->getCaptionText(ods, runparams);
				return ods.str();
			}
		}
	}
	return docstring();
}


void InsetFloat::string2params(string const & in, InsetFloatParams & params)
{
	params = InsetFloatParams();
	if (in.empty())
		return;

	istringstream data(in);
	Lexer lex;
	lex.setStream(data);
	lex.setContext("InsetFloat::string2params");
	lex >> "float" >> "Float";
	lex >> params.type; // We have to read the type here!
	params.read(lex);
}


string InsetFloat::params2string(InsetFloatParams const & params)
{
	ostringstream data;
	data << "float" << ' ';
	params.write(data);
	return data.str();
}


} // namespace lyx
