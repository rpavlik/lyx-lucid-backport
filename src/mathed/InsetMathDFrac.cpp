/**
 * \file InsetMathDFrac.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "InsetMathDFrac.h"
#include "MathData.h"
#include "MathStream.h"
#include "LaTeXFeatures.h"
#include "Color.h"
#include "frontends/Painter.h"


namespace lyx {


using std::string;
using std::max;
using std::auto_ptr;


InsetMathDFrac::InsetMathDFrac()
	: InsetMathFrac()
{}


auto_ptr<Inset> InsetMathDFrac::doClone() const
{
	return auto_ptr<Inset>(new InsetMathDFrac(*this));
}


bool InsetMathDFrac::metrics(MetricsInfo & mi, Dimension & dim) const
{
	cell(0).metrics(mi);
	cell(1).metrics(mi);
	dim.wid = max(cell(0).width(), cell(1).width()) + 2;
	dim.asc = cell(0).height() + 2 + 5;
	dim.des = cell(1).height() + 2 - 5;
	if (dim_ == dim)
		return false;
	dim_ = dim;
	return true;
}


void InsetMathDFrac::draw(PainterInfo & pi, int x, int y) const
{
	int m = x + dim_.wid / 2;
	cell(0).draw(pi, m - cell(0).width() / 2, y - cell(0).descent() - 2 - 5);
	cell(1).draw(pi, m - cell(1).width() / 2, y + cell(1).ascent()  + 2 - 5);
	pi.pain.line(x + 1, y - 5, x + dim_.wid - 2, y - 5, Color::math);
	setPosCache(pi, x, y);
}


docstring InsetMathDFrac::name() const
{
	return from_ascii("dfrac");
}


void InsetMathDFrac::mathmlize(MathStream & os) const
{
	os << MTag("mdfrac") << cell(0) << cell(1) << ETag("mdfrac");
}


void InsetMathDFrac::validate(LaTeXFeatures & features) const
{
	features.require("amsmath");
	InsetMathNest::validate(features);
}


} // namespace lyx
