/**
 * \file InsetMathSymbol.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author André Pönitz
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "InsetMathSymbol.h"

#include "Dimension.h"
#include "LaTeXFeatures.h"
#include "MathAtom.h"
#include "MathParser.h"
#include "MathStream.h"
#include "MathSupport.h"

#include "support/debug.h"
#include "support/docstream.h"
#include "support/textutils.h"


namespace lyx {

InsetMathSymbol::InsetMathSymbol(latexkeys const * l)
	: sym_(l), h_(0), scriptable_(false)
{}


InsetMathSymbol::InsetMathSymbol(char const * name)
	: sym_(in_word_set(from_ascii(name))), h_(0), scriptable_(false)
{}


InsetMathSymbol::InsetMathSymbol(docstring const & name)
	: sym_(in_word_set(name)), h_(0), scriptable_(false)
{}


Inset * InsetMathSymbol::clone() const
{
	return new InsetMathSymbol(*this);
}


docstring InsetMathSymbol::name() const
{
	return sym_->name;
}


void InsetMathSymbol::metrics(MetricsInfo & mi, Dimension & dim) const
{
	//lyxerr << "metrics: symbol: '" << sym_->name
	//	<< "' in font: '" << sym_->inset
	//	<< "' drawn as: '" << sym_->draw
	//	<< "'" << endl;

	bool const italic_upcase_greek = sym_->inset == "cmr" &&
					 sym_->extra == "mathalpha" &&
					 mi.base.fontname == "mathit";
	docstring const font = italic_upcase_greek ? from_ascii("cmm") : sym_->inset;
	int const em = mathed_char_width(mi.base.font, 'M');
	FontSetChanger dummy(mi.base, font);
	mathed_string_dim(mi.base.font, sym_->draw, dim);
	docstring::const_reverse_iterator rit = sym_->draw.rbegin();
	kerning_ = mathed_char_kerning(mi.base.font, *rit);
	// correct height for broken cmex and wasy font
	if (sym_->inset == "cmex" || sym_->inset == "wasy") {
		h_ = 4 * dim.des / 5;
		dim.asc += h_;
		dim.des -= h_;
	}
	// seperate things a bit
	if (isRelOp())
		dim.wid += static_cast<int>(0.5 * em + 0.5);
	else
		dim.wid += static_cast<int>(0.1667 * em + 0.5);

	scriptable_ = false;
	if (mi.base.style == LM_ST_DISPLAY)
		if (sym_->inset == "cmex" || sym_->inset == "esint" ||
		    sym_->extra == "funclim")
			scriptable_ = true;
}


void InsetMathSymbol::draw(PainterInfo & pi, int x, int y) const
{
	//lyxerr << "metrics: symbol: '" << sym_->name
	//	<< "' in font: '" << sym_->inset
	//	<< "' drawn as: '" << sym_->draw
	//	<< "'" << endl;

	bool const italic_upcase_greek = sym_->inset == "cmr" &&
					 sym_->extra == "mathalpha" &&
					 pi.base.fontname == "mathit";
	docstring const font = italic_upcase_greek ? from_ascii("cmm") : sym_->inset;
	int const em = mathed_char_width(pi.base.font, 'M');
	if (isRelOp())
		x += static_cast<int>(0.25*em+0.5);
	else
		x += static_cast<int>(0.0833*em+0.5);

	FontSetChanger dummy(pi.base, font);
	pi.draw(x, y - h_, sym_->draw);
}


bool InsetMathSymbol::isRelOp() const
{
	return sym_->extra == "mathrel";
}


bool InsetMathSymbol::isOrdAlpha() const
{
	return sym_->extra == "mathord" || sym_->extra == "mathalpha";
}


bool InsetMathSymbol::isScriptable() const
{
	return scriptable_;
}


bool InsetMathSymbol::takesLimits() const
{
	return
		sym_->inset == "cmex" ||
		sym_->inset == "lyxboldsymb" ||
		sym_->inset == "esint" ||
		sym_->extra == "funclim";
}


void InsetMathSymbol::normalize(NormalStream & os) const
{
	os << "[symbol " << name() << ']';
}


void InsetMathSymbol::maple(MapleStream & os) const
{
	if (name() == "cdot")
		os << '*';
	else if (name() == "infty")
		os << "infinity";
	else
		os << name();
}

void InsetMathSymbol::maxima(MaximaStream & os) const
{
	if (name() == "cdot")
		os << '*';
	else if (name() == "infty")
		os << "inf";
	else if (name() == "pi")
		os << "%pi";
	else
		os << name();
}


void InsetMathSymbol::mathematica(MathematicaStream & os) const
{
	if ( name() == "pi")    { os << "Pi"; return;}
	if ( name() == "infty") { os << "Infinity"; return;}
	if ( name() == "cdot")  { os << '*'; return;}
	os << name();
}


// FIXME This will likely need some work.
char const * MathMLtype(docstring const & s)
{
	if (s == "mathord")
		return "mi";
	return "mo";
}


void InsetMathSymbol::mathmlize(MathStream & os) const
{
	// FIXME We may need to do more interesting things 
	// with MathMLtype.
	char const * type = MathMLtype(sym_->extra);
	os << '<' << type << "> ";
	if (sym_->xmlname == "x") 
		// unknown so far
		os << name();
	else
		os << sym_->xmlname;
	os << " </" << type << '>';
}


void InsetMathSymbol::htmlize(HtmlStream & os, bool spacing) const
{
	// FIXME We may need to do more interesting things 
	// with MathMLtype.
	char const * type = MathMLtype(sym_->extra);
	bool op = (std::string(type) == "mo");
	
	if (sym_->xmlname == "x") 
		// unknown so far
		os << ' ' << name() << ' ';
	else if (op && spacing) 
		os << ' ' << sym_->xmlname << ' ';
	else
		os << sym_->xmlname;
}


void InsetMathSymbol::htmlize(HtmlStream & os) const
{
	htmlize(os, true);
}


void InsetMathSymbol::octave(OctaveStream & os) const
{
	if (name() == "cdot")
		os << '*';
	else
		os << name();
}


void InsetMathSymbol::write(WriteStream & os) const
{
	MathEnsurer ensurer(os);
	os << '\\' << name();

	// $,#, etc. In theory the restriction based on catcodes, but then
	// we do not handle catcodes very well, let alone cat code changes,
	// so being outside the alpha range is good enough.
	if (name().size() == 1 && !isAlphaASCII(name()[0]))
		return;

	os.pendingSpace(true);
}


void InsetMathSymbol::infoize2(odocstream & os) const
{
	os << from_ascii("Symbol: ") << name();
}


void InsetMathSymbol::validate(LaTeXFeatures & features) const
{
	// this is not really the ideal place to do this, but we can't
	// validate in InsetMathExInt.
	if (features.runparams().math_flavor == OutputParams::MathAsHTML
	    && sym_->name == from_ascii("int")) {
		features.addPreambleSnippet("<style type=\"text/css\">\n"
			"span.limits{display: inline-block; vertical-align: middle; text-align:center; font-size: 75%;}\n"
			"span.limits span{display: block;}\n"
			"span.intsym{font-size: 150%;}\n"
			"sub.limit{font-size: 75%;}\n"
			"sup.limit{font-size: 75%;}\n"
			"</style>");
	} else {
		if (!sym_->requires.empty())
			features.require(to_utf8(sym_->requires));
	}
}

} // namespace lyx
