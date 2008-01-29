/**
 * \file InsetMathBoxed.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "InsetMathBoxed.h"
#include "MathData.h"
#include "MathStream.h"
#include "LaTeXFeatures.h"
#include "Color.h"

#include "support/std_ostream.h"
#include "frontends/Painter.h"


namespace lyx {

using std::auto_ptr;


InsetMathBoxed::InsetMathBoxed()
	: InsetMathNest(1)
{}


auto_ptr<Inset> InsetMathBoxed::doClone() const
{
	return auto_ptr<Inset>(new InsetMathBoxed(*this));
}


bool InsetMathBoxed::metrics(MetricsInfo & mi, Dimension & dim) const
{
	cell(0).metrics(mi, dim);
	metricsMarkers2(dim, 3); // 1 pixel space, 1 frame, 1 space
	if (dim_ == dim)
		return false;
	dim_ = dim;
	return true;
}


void InsetMathBoxed::draw(PainterInfo & pi, int x, int y) const
{
	pi.pain.rectangle(x + 1, y - dim_.ascent() + 1,
		dim_.width() - 2, dim_.height() - 2, Color::foreground);
	cell(0).draw(pi, x + 3, y);
	setPosCache(pi, x, y);
}


void InsetMathBoxed::write(WriteStream & os) const
{
	os << "\\boxed{" << cell(0) << '}';
}


void InsetMathBoxed::normalize(NormalStream & os) const
{
	os << "[boxed " << cell(0) << ']';
}


void InsetMathBoxed::infoize(odocstream & os) const
{
	os << "Boxed: ";
}


void InsetMathBoxed::validate(LaTeXFeatures & features) const
{
	features.require("amsmath");
}


} // namespace lyx