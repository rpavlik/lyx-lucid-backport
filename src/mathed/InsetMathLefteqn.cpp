/**
 * \file InsetMathLefteqn.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "InsetMathLefteqn.h"
#include "MathData.h"
#include "support/std_ostream.h"


namespace lyx {


using std::string;
using std::auto_ptr;


InsetMathLefteqn::InsetMathLefteqn()
	: InsetMathNest(1)
{}


auto_ptr<Inset> InsetMathLefteqn::doClone() const
{
	return auto_ptr<Inset>(new InsetMathLefteqn(*this));
}


bool InsetMathLefteqn::metrics(MetricsInfo & mi, Dimension & dim) const
{
	cell(0).metrics(mi, dim);
	dim.asc += 2;
	dim.des += 2;
	dim.wid = 4;
	metricsMarkers(dim);
	if (dim_ == dim)
		return false;
	dim_ = dim;
	return true;
}


void InsetMathLefteqn::draw(PainterInfo & pi, int x, int y) const
{
	cell(0).draw(pi, x + 2, y);
	drawMarkers(pi, x, y);
}


docstring InsetMathLefteqn::name() const
{
	return from_ascii("lefteqn");
}


void InsetMathLefteqn::infoize(odocstream & os) const
{
	os << "Lefteqn ";
}


} // namespace lyx
