/**
 * \file InsetMathTabular.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "InsetMathTabular.h"
#include "MathData.h"
#include "MathStream.h"
#include "MathStream.h"

#include "support/lstrings.h"
#include "support/std_ostream.h"

#include <iterator>


namespace lyx {


using std::string;
using std::auto_ptr;


InsetMathTabular::InsetMathTabular(docstring const & name, int m, int n)
	: InsetMathGrid(m, n), name_(name)
{}


InsetMathTabular::InsetMathTabular(docstring const & name, int m, int n,
		char valign, docstring const & halign)
	: InsetMathGrid(m, n, valign, halign), name_(name)
{}


InsetMathTabular::InsetMathTabular(docstring const & name, char valign,
		docstring const & halign)
	: InsetMathGrid(valign, halign), name_(name)
{}


auto_ptr<Inset> InsetMathTabular::doClone() const
{
	return auto_ptr<Inset>(new InsetMathTabular(*this));
}


bool InsetMathTabular::metrics(MetricsInfo & mi, Dimension & dim) const
{
	FontSetChanger dummy(mi.base, "textnormal");
	InsetMathGrid::metrics(mi, dim);
	dim.wid += 6;
	if (dim_ == dim)
		return false;
	dim_ = dim;
	return true;
}


void InsetMathTabular::draw(PainterInfo & pi, int x, int y) const
{
	FontSetChanger dummy(pi.base, "textnormal");
	InsetMathGrid::drawWithMargin(pi, x, y, 4, 2);
}


void InsetMathTabular::write(WriteStream & os) const
{
	if (os.fragile())
		os << "\\protect";
	os << "\\begin{" << name_ << '}';

	if (v_align_ == 't' || v_align_ == 'b')
		os << '[' << char(v_align_) << ']';
	os << '{' << halign() << "}\n";

	InsetMathGrid::write(os);

	if (os.fragile())
		os << "\\protect";
	os << "\\end{" << name_ << '}';
	// adding a \n here is bad if the tabular is the last item
	// in an \eqnarray...
}


void InsetMathTabular::infoize(odocstream & os) const
{
	docstring name = name_;
	name[0] = support::uppercase(name[0]);
	os << name << ' ';
}


void InsetMathTabular::normalize(NormalStream & os) const
{
	os << '[' << name_ << ' ';
	InsetMathGrid::normalize(os);
	os << ']';
}


void InsetMathTabular::maple(MapleStream & os) const
{
	os << "array(";
	InsetMathGrid::maple(os);
	os << ')';
}


} // namespace lyx