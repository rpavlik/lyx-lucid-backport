/**
 * \file InsetVSpace.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author various
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "InsetVSpace.h"

#include "Buffer.h"
#include "Cursor.h"
#include "DispatchResult.h"
#include "FuncRequest.h"
#include "gettext.h"
#include "Color.h"
#include "Lexer.h"
#include "Text.h"
#include "MetricsInfo.h"
#include "OutputParams.h"

#include "frontends/FontMetrics.h"
#include "frontends/Painter.h"

#include <sstream>


namespace lyx {

using std::istringstream;
using std::ostream;
using std::ostringstream;
using std::string;
using std::max;


namespace {

int const ADD_TO_VSPACE_WIDTH = 5;

} // namespace anon


InsetVSpace::InsetVSpace(VSpace const & space)
	: space_(space)
{}


InsetVSpace::~InsetVSpace()
{
	InsetVSpaceMailer(*this).hideDialog();
}


std::auto_ptr<Inset> InsetVSpace::doClone() const
{
	return std::auto_ptr<Inset>(new InsetVSpace(*this));
}


void InsetVSpace::doDispatch(Cursor & cur, FuncRequest & cmd)
{
	switch (cmd.action) {

	case LFUN_INSET_MODIFY: {
		InsetVSpaceMailer::string2params(to_utf8(cmd.argument()), space_);
		break;
	}

	case LFUN_MOUSE_RELEASE:
		if (!cur.selection())
			InsetVSpaceMailer(*this).showDialog(&cur.bv());
		break;

	default:
		Inset::doDispatch(cur, cmd);
		break;
	}
}


void InsetVSpace::read(Buffer const &, Lexer & lex)
{
	BOOST_ASSERT(lex.isOK());
	string vsp;
	lex >> vsp;
	if (lex)
		space_ = VSpace(vsp);

	string end_token;
	lex >> end_token;
	if (end_token != "\\end_inset")
		lex.printError("Missing \\end_inset at this point. "
			       "Read: `$$Token'");
}


void InsetVSpace::write(Buffer const &, ostream & os) const
{
	os << "VSpace " << space_.asLyXCommand();
}


docstring const InsetVSpace::label() const
{
	static docstring const label = _("Vertical Space");
	return label + " (" + space_.asGUIName() + ')';
}


namespace {
int const arrow_size = 4;
}


bool InsetVSpace::metrics(MetricsInfo & mi, Dimension & dim) const
{
	int height = 3 * arrow_size;
	if (space_.length().len().value() >= 0.0)
		height = max(height, space_.inPixels(*mi.base.bv));

	Font font;
	font.decSize();
	font.decSize();

	int w = 0;
	int a = 0;
	int d = 0;
	theFontMetrics(font).rectText(label(), w, a, d);

	height = max(height, a + d);

	dim.asc = height / 2 + (a - d) / 2; // align cursor with the
	dim.des = height - dim.asc;         // label text
	dim.wid = ADD_TO_VSPACE_WIDTH + 2 * arrow_size + 5 + w;
	bool const changed = dim_ != dim;
	dim_ = dim;
	return changed;
}


void InsetVSpace::draw(PainterInfo & pi, int x, int y) const
{
	setPosCache(pi, x, y);

	x += ADD_TO_VSPACE_WIDTH;

	int const start = y - dim_.asc;
	int const end   = y + dim_.des;

	// y-values for top arrow
	int ty1, ty2;
	// y-values for bottom arrow
	int by1, by2;

	if (space_.kind() == VSpace::VFILL) {
		ty1 = ty2 = start;
		by1 = by2 = end;
	} else {
		// adding or removing space
		bool const added = space_.kind() != VSpace::LENGTH ||
				   space_.length().len().value() >= 0.0;
		ty1 = added ? (start + arrow_size) : start;
		ty2 = added ? start : (start + arrow_size);
		by1 = added ? (end - arrow_size) : end;
		by2 = added ? end : (end - arrow_size);
	}

	int const midx = x + arrow_size;
	int const rightx = midx + arrow_size;

	// first the string
	int w = 0;
	int a = 0;
	int d = 0;

	Font font;
	font.setColor(Color::added_space);
	font.decSize();
	font.decSize();
	docstring const lab = label();
	theFontMetrics(font).rectText(lab, w, a, d);

	pi.pain.rectText(x + 2 * arrow_size + 5,
			 start + (end - start) / 2 + (a - d) / 2,
			 lab, font, Color::none, Color::none);

	// top arrow
	pi.pain.line(x, ty1, midx, ty2, Color::added_space);
	pi.pain.line(midx, ty2, rightx, ty1, Color::added_space);

	// bottom arrow
	pi.pain.line(x, by1, midx, by2, Color::added_space);
	pi.pain.line(midx, by2, rightx, by1, Color::added_space);

	// joining line
	pi.pain.line(midx, ty2, midx, by2, Color::added_space);
}


int InsetVSpace::latex(Buffer const & buf, odocstream & os,
		       OutputParams const &) const
{
	os << from_ascii(space_.asLatexCommand(buf.params())) << '\n';
	return 1;
}


int InsetVSpace::plaintext(Buffer const &, odocstream & os,
			   OutputParams const &) const
{
	os << "\n\n";
	return PLAINTEXT_NEWLINE;
}


int InsetVSpace::docbook(Buffer const &, odocstream & os,
			 OutputParams const &) const
{
	os << '\n';
	return 1;
}


string const InsetVSpaceMailer::name_ = "vspace";


InsetVSpaceMailer::InsetVSpaceMailer(InsetVSpace & inset)
	: inset_(inset)
{}


string const InsetVSpaceMailer::inset2string(Buffer const &) const
{
	return params2string(inset_.space());
}


void InsetVSpaceMailer::string2params(string const & in, VSpace & vspace)
{
	vspace = VSpace();
	if (in.empty())
		return;

	istringstream data(in);
	Lexer lex(0,0);
	lex.setStream(data);

	string name;
	lex >> name;
	if (!lex || name != name_)
		return print_mailer_error("InsetVSpaceMailer", in, 1, name_);

	string vsp;
	lex >> vsp;
	if (lex)
		vspace = VSpace(vsp);
}


string const InsetVSpaceMailer::params2string(VSpace const & vspace)
{
	ostringstream data;
	data << name_ << ' ' << vspace.asLyXCommand();
	return data.str();
}


} // namespace lyx