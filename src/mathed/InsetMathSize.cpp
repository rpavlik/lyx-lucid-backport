/**
 * \file InsetMathSize.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "InsetMathSize.h"
#include "MathData.h"
#include "MathParser.h"
#include "MathStream.h"

#include "support/convert.h"
#include "support/std_ostream.h"


namespace lyx {

using std::auto_ptr;


InsetMathSize::InsetMathSize(latexkeys const * l)
	: InsetMathNest(1), key_(l), style_(Styles(convert<int>(l->extra)))
{}


auto_ptr<Inset> InsetMathSize::doClone() const
{
	return auto_ptr<Inset>(new InsetMathSize(*this));
}


bool InsetMathSize::metrics(MetricsInfo & mi, Dimension & dim) const
{
	StyleChanger dummy(mi.base, style_);
	cell(0).metrics(mi, dim);
	metricsMarkers(dim);
	if (dim_ == dim)
		return false;
	dim_ = dim;
	return true;
}


void InsetMathSize::draw(PainterInfo & pi, int x, int y) const
{
	StyleChanger dummy(pi.base, style_);
	cell(0).draw(pi, x + 1, y);
	drawMarkers(pi, x, y);
}


void InsetMathSize::write(WriteStream & os) const
{
	os << "{\\" << key_->name << ' ' << cell(0) << '}';
}


void InsetMathSize::normalize(NormalStream & os) const
{
	os << '[' << key_->name << ' ' << cell(0) << ']';
}


void InsetMathSize::infoize(odocstream & os) const
{
	os << "Size: " << key_->name;
}


} // namespace lyx
