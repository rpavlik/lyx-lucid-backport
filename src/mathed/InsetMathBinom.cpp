/**
 * \file InsetMathBinom.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "InsetMathBinom.h"
#include "MathData.h"
#include "MathSupport.h"
#include "MathStream.h"


namespace lyx {


using std::max;
using std::auto_ptr;


InsetMathBinom::InsetMathBinom(bool choose)
	: choose_(choose)
{}


auto_ptr<Inset> InsetMathBinom::doClone() const
{
	return auto_ptr<Inset>(new InsetMathBinom(*this));
}


int InsetMathBinom::dw() const
{
	int w = dim_.height() / 5;
	if (w > 15)
		w = 15;
	if (w < 6)
		w = 6;
	return w;
}


bool InsetMathBinom::metrics(MetricsInfo & mi, Dimension & dim) const
{
	FracChanger dummy(mi.base);
	cell(0).metrics(mi);
	cell(1).metrics(mi);
	dim.asc = cell(0).height() + 4 + 5;
	dim.des = cell(1).height() + 4 - 5;
	dim.wid = max(cell(0).width(), cell(1).width()) + 2 * dw() + 4;
	metricsMarkers2(dim);
	bool const changed = dim_ != dim;
	dim_ = dim;
	return changed;
}


void InsetMathBinom::draw(PainterInfo & pi, int x, int y) const
{
	int m = x + dim_.width() / 2;
	FracChanger dummy(pi.base);
	cell(0).draw(pi, m - cell(0).width() / 2, y - cell(0).descent() - 3 - 5);
	cell(1).draw(pi, m - cell(1).width() / 2, y + cell(1).ascent()  + 3 - 5);
	mathed_draw_deco(pi, x, y - dim_.ascent(), dw(), dim_.height(), from_ascii("("));
	mathed_draw_deco(pi, x + dim_.width() - dw(), y - dim_.ascent(),
		dw(), dim_.height(), from_ascii(")"));
	drawMarkers2(pi, x, y);
}


bool InsetMathBinom::extraBraces() const
{
	return choose_;
}


void InsetMathBinom::write(WriteStream & os) const
{
	if (choose_)
		os << '{' << cell(0) << " \\choose " << cell(1) << '}';
	else
		os << "\\binom{" << cell(0) << "}{" << cell(1) << '}';
}


void InsetMathBinom::normalize(NormalStream & os) const
{
	os << "[binom " << cell(0) << ' ' << cell(1) << ']';
}


} // namespace lyx