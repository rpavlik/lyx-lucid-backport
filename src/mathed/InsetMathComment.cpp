/**
 * \file InsetMathComment.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "InsetMathComment.h"
#include "MathData.h"
#include "MathStream.h"
#include "MathSupport.h"
#include "support/std_ostream.h"


namespace lyx {

using std::string;
using std::auto_ptr;


InsetMathComment::InsetMathComment()
	: InsetMathNest(1)
{}


InsetMathComment::InsetMathComment(docstring const & str)
	: InsetMathNest(1)
{
	// FIXME UNICODE
	asArray(str, cell(0));
}


auto_ptr<Inset> InsetMathComment::doClone() const
{
	return auto_ptr<Inset>(new InsetMathComment(*this));
}


bool InsetMathComment::metrics(MetricsInfo & mi, Dimension & dim) const
{
	cell(0).metrics(mi, dim);
	metricsMarkers(dim);
	if (dim_ == dim)
		return false;
	dim_ = dim;
	return true;
}


void InsetMathComment::draw(PainterInfo & pi, int x, int y) const
{
	cell(0).draw(pi, x + 1, y);
	drawMarkers(pi, x, y);
}


void InsetMathComment::metricsT(TextMetricsInfo const & mi, Dimension & dim) const
{
	cell(0).metricsT(mi, dim);
}


void InsetMathComment::drawT(TextPainter & pain, int x, int y) const
{
	cell(0).drawT(pain, x, y);
}


void InsetMathComment::write(WriteStream & os) const
{
	os << '%' << cell(0) << "\n";
}


void InsetMathComment::maple(MapleStream & os) const
{
	os << '#' << cell(0) << "\n";
}


void InsetMathComment::mathematica(MathematicaStream &) const
{}


void InsetMathComment::octave(OctaveStream &) const
{}


void InsetMathComment::mathmlize(MathStream & os) const
{
	os << MTag("comment") << cell(0) << cell(1) << ETag("comment");
}


void InsetMathComment::infoize(odocstream & os) const
{
	os << "Comment";
}


} // namespace lyx