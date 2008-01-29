/**
 * \file InsetMathFont.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "InsetMathFont.h"
#include "MathData.h"
#include "MathStream.h"
#include "MathParser.h"
#include "LaTeXFeatures.h"
#include "support/std_ostream.h"


namespace lyx {

using std::auto_ptr;


InsetMathFont::InsetMathFont(latexkeys const * key)
	: InsetMathNest(1), key_(key)
{}


auto_ptr<Inset> InsetMathFont::doClone() const
{
	return auto_ptr<Inset>(new InsetMathFont(*this));
}


InsetMath::mode_type InsetMathFont::currentMode() const
{
	if (key_->extra == "mathmode")
		return MATH_MODE;
	if (key_->extra == "textmode")
		return TEXT_MODE;
	return UNDECIDED_MODE;
}


bool InsetMathFont::metrics(MetricsInfo & mi, Dimension & dim) const
{
	FontSetChanger dummy(mi.base, key_->name);
	cell(0).metrics(mi, dim);
	metricsMarkers(dim);
	if (dim_ == dim)
		return false;
	dim_ = dim;
	return true;
}


void InsetMathFont::draw(PainterInfo & pi, int x, int y) const
{
	FontSetChanger dummy(pi.base, key_->name.c_str());
	cell(0).draw(pi, x + 1, y);
	drawMarkers(pi, x, y);
	setPosCache(pi, x, y);
}


void InsetMathFont::metricsT(TextMetricsInfo const & mi, Dimension &) const
{
	cell(0).metricsT(mi, dim_);
}


void InsetMathFont::drawT(TextPainter & pain, int x, int y) const
{
	cell(0).drawT(pain, x, y);
}


docstring InsetMathFont::name() const
{
	return key_->name;
}


void InsetMathFont::validate(LaTeXFeatures & features) const
{
	InsetMathNest::validate(features);
	// Make sure amssymb is put in preamble if Blackboard Bold or
	// Fraktur used:
	if (key_->name == "mathfrak" || key_->name == "mathbb")
		features.require("amssymb");
	if (key_->name == "text")
		features.require("amsmath");
	if (key_->name == "textipa")
		features.require("tipa");
}


void InsetMathFont::infoize(odocstream & os) const
{
	os << "Font: " << key_->name;
}


} // namespace lyx