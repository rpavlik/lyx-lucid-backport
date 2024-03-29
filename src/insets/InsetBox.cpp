/**
 * \file InsetBox.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Angus Leeming
 * \author Martin Vermeer
 * \author Jürgen Spitzmüller
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "InsetBox.h"

#include "Buffer.h"
#include "BufferParams.h"
#include "BufferView.h"
#include "Cursor.h"
#include "DispatchResult.h"
#include "FuncStatus.h"
#include "FuncRequest.h"
#include "LaTeXFeatures.h"
#include "Lexer.h"
#include "MetricsInfo.h"
#include "output_xhtml.h"
#include "TextClass.h"

#include "support/debug.h"
#include "support/docstream.h"
#include "support/gettext.h"
#include "support/lstrings.h"
#include "support/Translator.h"

#include "frontends/Application.h"

#include <sstream>

using namespace std;
using namespace lyx::support;

namespace lyx {

namespace {

typedef Translator<string, InsetBox::BoxType> BoxTranslator;
typedef Translator<docstring, InsetBox::BoxType> BoxTranslatorLoc;

BoxTranslator initBoxtranslator()
{
	BoxTranslator translator("Boxed", InsetBox::Boxed);
	translator.addPair("Frameless", InsetBox::Frameless);
	translator.addPair("Framed", InsetBox::Framed);
	translator.addPair("ovalbox", InsetBox::ovalbox);
	translator.addPair("Ovalbox", InsetBox::Ovalbox);
	translator.addPair("Shadowbox", InsetBox::Shadowbox);
	translator.addPair("Shaded", InsetBox::Shaded);
	translator.addPair("Doublebox",InsetBox::Doublebox);
	return translator;
}


BoxTranslatorLoc initBoxtranslatorLoc()
{
	BoxTranslatorLoc translator(_("simple frame"), InsetBox::Boxed);
	translator.addPair(_("frameless"), InsetBox::Frameless);
	translator.addPair(_("simple frame, page breaks"), InsetBox::Framed);
	translator.addPair(_("oval, thin"), InsetBox::ovalbox);
	translator.addPair(_("oval, thick"), InsetBox::Ovalbox);
	translator.addPair(_("drop shadow"), InsetBox::Shadowbox);
	translator.addPair(_("shaded background"), InsetBox::Shaded);
	translator.addPair(_("double frame"), InsetBox::Doublebox);
	return translator;
}


BoxTranslator const & boxtranslator()
{
	static BoxTranslator translator = initBoxtranslator();
	return translator;
}


BoxTranslatorLoc const & boxtranslator_loc()
{
	static BoxTranslatorLoc translator = initBoxtranslatorLoc();
	return translator;
}

} // namespace anon


/////////////////////////////////////////////////////////////////////////
//
// InsetBox
//
/////////////////////////////////////////////////////////////////////////

InsetBox::InsetBox(Buffer * buffer, string const & label)
	: InsetCollapsable(buffer), params_(label)
{}


docstring InsetBox::layoutName() const
{
	// FIXME: UNICODE
	return from_ascii("Box:" + params_.type);
}


void InsetBox::write(ostream & os) const
{
	params_.write(os);
	InsetCollapsable::write(os);
}


void InsetBox::read(Lexer & lex)
{
	params_.read(lex);
	InsetCollapsable::read(lex);
}


void InsetBox::setButtonLabel()
{
	BoxType const btype = boxtranslator().find(params_.type);

	docstring const type = _("Box");

	docstring inner;
	if (params_.inner_box) {
		if (params_.use_parbox)
			inner = _("Parbox");
		else if (params_.use_makebox)
			inner = _("Makebox");
		else
			inner = _("Minipage");
	}

	docstring frame;
	if (btype != Frameless)
		frame = boxtranslator_loc().find(btype);

	docstring label;
	if (inner.empty() && frame.empty())
		label = type;
	else if (inner.empty())
		label = bformat(_("%1$s (%2$s)"),
			type, frame);
	else if (frame.empty())
		label = bformat(_("%1$s (%2$s)"),
			type, inner);
	else
		label = bformat(_("%1$s (%2$s, %3$s)"),
			type, inner, frame);
	setLabel(label);
}


bool InsetBox::hasFixedWidth() const
{
	return params_.inner_box || params_.special != "width";
}


void InsetBox::metrics(MetricsInfo & m, Dimension & dim) const
{
	// back up textwidth.
	int textwidth_backup = m.base.textwidth;
	if (hasFixedWidth())
		m.base.textwidth = params_.width.inPixels(m.base.textwidth);
	InsetCollapsable::metrics(m, dim);
	// retore textwidth.
	m.base.textwidth = textwidth_backup;
}


bool InsetBox::forcePlainLayout(idx_type) const
{
	return (!params_.inner_box || params_.use_makebox)
		&& params_.type != "Shaded" && params_.type != "Framed";
}


void InsetBox::doDispatch(Cursor & cur, FuncRequest & cmd)
{
	switch (cmd.action()) {

	case LFUN_INSET_MODIFY: {
		//lyxerr << "InsetBox::dispatch MODIFY" << endl;
		cur.recordUndoInset(ATOMIC_UNDO, this);
		if (cmd.getArg(0) == "changetype") {
			params_.type = cmd.getArg(1);
		} else
			string2params(to_utf8(cmd.argument()), params_);
		setButtonLabel();
		break;
	}

	default:
		InsetCollapsable::doDispatch(cur, cmd);
		break;
	}
}


bool InsetBox::getStatus(Cursor & cur, FuncRequest const & cmd,
		FuncStatus & flag) const
{
	switch (cmd.action()) {

	case LFUN_INSET_MODIFY:
		if (cmd.getArg(0) == "changetype")
			flag.setOnOff(cmd.getArg(1) == params_.type);
		flag.setEnabled(true);
		return true;

	case LFUN_INSET_DIALOG_UPDATE:
		flag.setEnabled(true);
		return true;

	case LFUN_BREAK_PARAGRAPH:
		if ((params_.inner_box && !params_.use_makebox)
		     || params_.type == "Shaded" || params_.type == "Framed")
			return InsetCollapsable::getStatus(cur, cmd, flag);
		flag.setEnabled(false);
		return true;

	default:
		return InsetCollapsable::getStatus(cur, cmd, flag);
	}
}


void InsetBox::latex(otexstream & os, OutputParams const & runparams) const
{
	BoxType btype = boxtranslator().find(params_.type);

	string width_string = params_.width.asLatexString();
	bool stdwidth = false;
	if (params_.inner_box &&
			(width_string.find("1.0\\columnwidth") != string::npos
			|| width_string.find("1.0\\textwidth") != string::npos)) {
		stdwidth = true;
		switch (btype) {
		case Frameless:
		case Framed:
			break;
		case Boxed:
		case Shaded:
			width_string += " - 2\\fboxsep - 2\\fboxrule";
			break;
		case ovalbox:
			width_string += " - 2\\fboxsep - 0.8pt";
			break;
		case Ovalbox:
			width_string += " - 2\\fboxsep - 1.6pt";
			break;
		case Shadowbox:
			// Shadow falls outside right margin... opinions?
			width_string += " - 2\\fboxsep - 2\\fboxrule"/* "-\\shadowsize"*/;
			break;
		case Doublebox:
			width_string += " - 2\\fboxsep - 7.5\\fboxrule - 1pt";
			break;
		}
	}

	os << safebreakln;
	if (runparams.lastid != -1)
		os.texrow().start(runparams.lastid, runparams.lastpos);

	// Adapt to column/text width correctly also if paragraphs indented:
	if (stdwidth)
		os << "\\noindent";

	switch (btype) {
	case Frameless:
		break;
	case Framed:
		os << "\\begin{framed}%\n";
		break;
	case Boxed:
		os << "\\framebox";
		if (!params_.inner_box) {
			// Special widths, see usrguide §3.5
			// FIXME UNICODE
			if (params_.special != "none") {
				os << "[" << params_.width.value()
				   << '\\' << from_utf8(params_.special)
				   << ']';
			} else
				os << '[' << from_ascii(width_string)
				   << ']';
			if (params_.hor_pos != 'c')
				os << "[" << params_.hor_pos << "]";
		}
		os << "{";
		break;
	case ovalbox:
		os << "\\ovalbox{";
		break;
	case Ovalbox:
		os << "\\Ovalbox{";
		break;
	case Shadowbox:
		os << "\\shadowbox{";
		break;
	case Shaded:
		// later
		break;
	case Doublebox:
		os << "\\doublebox{";
		break;
	}

	if (params_.inner_box) {
		if (params_.use_parbox)
			os << "\\parbox";
		else if (params_.use_makebox) {
			os << "\\makebox";
			// FIXME UNICODE
			// output the width and horizontal position
			if (params_.special != "none") {
				os << "[" << params_.width.value()
				   << '\\' << from_utf8(params_.special)
				   << ']';
			} else
				os << '[' << from_ascii(width_string)
				   << ']';
			if (params_.hor_pos != 'c')
				os << "[" << params_.hor_pos << "]";
			os << "{";
		}
		else
			os << "\\begin{minipage}";

		// output parameters for parbox and minipage
		if (!params_.use_makebox) {
			os << "[" << params_.pos << "]";
			if (params_.height_special == "none") {
				// FIXME UNICODE
				os << "[" << from_ascii(params_.height.asLatexString()) << "]";
			} else {
				// Special heights
				// set no optional argument when the value is the default "1\height"
				// (special units like \height are handled as "in")
				// but when the user has chosen a non-default inner_pos, the height
				// must be given: \minipage[pos][height][inner-pos]{width}
				if ((params_.height != Length("1in") ||
					params_.height_special != "totalheight") ||
					params_.inner_pos != params_.pos) {
						// FIXME UNICODE
						os << "[" << params_.height.value()
							<< "\\" << from_utf8(params_.height_special) << "]";
				}
			}
			if (params_.inner_pos != params_.pos)
				os << "[" << params_.inner_pos << "]";
			// FIXME UNICODE
			os << '{' << from_ascii(width_string) << '}';
			if (params_.use_parbox)
				os << "{";
		}

		os << "%\n";
	} // end if inner_box

	if (btype == Shaded) {
		os << "\\begin{shaded}%\n";
	}

	InsetText::latex(os, runparams);

	if (btype == Shaded)
		os << "\\end{shaded}";

	if (params_.inner_box) {
		if (params_.use_parbox || params_.use_makebox)
			os << "%\n}";
		else
			os << "%\n\\end{minipage}";
	}

	switch (btype) {
	case Frameless:
		break;
	case Framed:
		os << "\\end{framed}";
		break;
	case Boxed:
		os << "}";
		break;
	case ovalbox:
	case Ovalbox:
	case Doublebox:
	case Shadowbox:
		os << "}";
		break;
	case Shaded:
		// already done
		break;
	}
}


int InsetBox::plaintext(odocstream & os, OutputParams const & runparams) const
{
	BoxType const btype = boxtranslator().find(params_.type);

	switch (btype) {
		case Frameless:
			break;
		case Framed:
		case Boxed:
			os << "[\n";
			break;
		case ovalbox:
			os << "(\n";
			break;
		case Ovalbox:
			os << "((\n";
			break;
		case Shadowbox:
		case Shaded:
			os << "[/\n";
			break;
		case Doublebox:
			os << "[[\n";
			break;
	}

	InsetText::plaintext(os, runparams);

	int len = 0;
	switch (btype) {
		case Frameless:
			os << "\n";
			break;
		case Framed:
		case Boxed:
			os << "\n]";
			len = 1;
			break;
		case ovalbox:
			os << "\n)";
			len = 1;
			break;
		case Ovalbox:
			os << "\n))";
			len = 2;
			break;
		case Shadowbox:
		case Shaded:
			os << "\n/]";
			len = 2;
			break;
		case Doublebox:
			os << "\n]]";
			len = 2;
			break;
	}

	return PLAINTEXT_NEWLINE + len; // len chars on a separate line
}


int InsetBox::docbook(odocstream & os, OutputParams const & runparams) const
{
	return InsetText::docbook(os, runparams);
}


docstring InsetBox::xhtml(XHTMLStream & xs, OutputParams const & runparams) const
{
	// construct attributes
	string attrs = "class='" + params_.type + "'";
	string style;
	if (!params_.width.empty()) {
		string w = params_.width.asHTMLString();
		if (w != "100%")
			style += ("width: " + params_.width.asHTMLString() + "; ");
	}
	// The special heights don't really mean anything for us.
	if (!params_.height.empty() && params_.height_special == "none")
		style += ("height: " + params_.height.asHTMLString() + "; ");
	if (!style.empty())
		attrs += " style='" + style + "'";

	xs << html::StartTag("div", attrs);
	XHTMLOptions const opts = InsetText::WriteLabel | InsetText::WriteInnerTag;
	docstring defer = InsetText::insetAsXHTML(xs, runparams, opts);
	xs << html::EndTag("div");
	xs << defer;
	return docstring();
}


void InsetBox::validate(LaTeXFeatures & features) const
{
	BoxType btype = boxtranslator().find(params_.type);
	switch (btype) {
	case Frameless:
		break;
	case Framed:
		features.require("framed");
		break;
	case Boxed:
		features.require("calc");
		break;
	case ovalbox:
	case Ovalbox:
	case Shadowbox:
	case Doublebox:
		features.require("calc");
		features.require("fancybox");
		break;
	case Shaded:
		features.require("color");
		features.require("framed");
		break;
	}
	InsetCollapsable::validate(features);
}


docstring InsetBox::contextMenuName() const
{
	return from_ascii("context-box");
}


string InsetBox::params2string(InsetBoxParams const & params)
{
	ostringstream data;
	data << "box" << ' ';
	params.write(data);
	return data.str();
}


void InsetBox::string2params(string const & in, InsetBoxParams & params)
{
	params = InsetBoxParams(string());
	if (in.empty())
		return;

	istringstream data(in);
	Lexer lex;
	lex.setStream(data);

	string name;
	lex >> name;
	if (!lex || name != "box") {
		LYXERR0("InsetBox::string2params(" << in << ")\n"
					  "Expected arg 1 to be \"box\"\n");
		return;
	}

	// This is part of the inset proper that is usually swallowed
	// by Text::readInset
	string id;
	lex >> id;
	if (!lex || id != "Box") {
		LYXERR0("InsetBox::string2params(" << in << ")\n"
					  "Expected arg 2 to be \"Box\"\n");
	}

	params.read(lex);
}


/////////////////////////////////////////////////////////////////////////
//
// InsetBoxParams
//
/////////////////////////////////////////////////////////////////////////

InsetBoxParams::InsetBoxParams(string const & label)
	: type(label),
	  use_parbox(false),
	  use_makebox(false),
	  inner_box(true),
	  width(Length("100col%")),
	  special("none"),
	  pos('t'),
	  hor_pos('c'),
	  inner_pos('t'),
	  height(Length("1in")),
	  height_special("totalheight") // default is 1\\totalheight
{}


void InsetBoxParams::write(ostream & os) const
{
	os << "Box " << type << "\n";
	os << "position \"" << pos << "\"\n";
	os << "hor_pos \"" << hor_pos << "\"\n";
	os << "has_inner_box " << inner_box << "\n";
	os << "inner_pos \"" << inner_pos << "\"\n";
	os << "use_parbox " << use_parbox << "\n";
	os << "use_makebox " << use_makebox << "\n";
	os << "width \"" << width.asString() << "\"\n";
	os << "special \"" << special << "\"\n";
	os << "height \"" << height.asString() << "\"\n";
	os << "height_special \"" << height_special << "\"\n";
}


void InsetBoxParams::read(Lexer & lex)
{
	lex.setContext("InsetBoxParams::read");
	lex >> type;
	lex >> "position" >> pos;
	lex >> "hor_pos" >> hor_pos;
	lex >> "has_inner_box" >> inner_box;
	if (type == "Framed")
		inner_box = false;
	lex >> "inner_pos" >> inner_pos;
	lex >> "use_parbox" >> use_parbox;
	lex >> "use_makebox" >> use_makebox;
	lex >> "width" >> width;
	lex >> "special" >> special;
	lex >> "height" >> height;
	lex >> "height_special" >> height_special;
}


} // namespace lyx
