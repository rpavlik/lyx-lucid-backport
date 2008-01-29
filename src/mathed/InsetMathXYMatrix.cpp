/**
 * \file InsetMathXYMatrix.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "InsetMathXYMatrix.h"
#include "MathStream.h"

#include "LaTeXFeatures.h"
#include "support/std_ostream.h"


namespace lyx {


InsetMathXYMatrix::InsetMathXYMatrix(Length const & s, char c)
	: InsetMathGrid(1, 1), spacing_(s), spacing_code_(c)
{}


std::auto_ptr<Inset> InsetMathXYMatrix::doClone() const
{
	return std::auto_ptr<Inset>(new InsetMathXYMatrix(*this));
}


int InsetMathXYMatrix::colsep() const
{
	return 40;
}


int InsetMathXYMatrix::rowsep() const
{
	return 40;
}


bool InsetMathXYMatrix::metrics(MetricsInfo & mi, Dimension & dim) const
{
	if (mi.base.style == LM_ST_DISPLAY)
		mi.base.style = LM_ST_TEXT;
	InsetMathGrid::metrics(mi, dim);
	if (dim_ == dim)
		return false;
	dim_ = dim;
	return true;
}


void InsetMathXYMatrix::write(WriteStream & os) const
{
	os << "\\xymatrix";
	switch (spacing_code_) {
	case 'R':
	case 'C':
	case 'M':
	case 'W':
	case 'H':
	case 'L':
		os << '@' << spacing_code_ << '='
		   << from_ascii(spacing_.asLatexString());
		break;
	default:
		if (!spacing_.empty())
			os << "@=" << from_ascii(spacing_.asLatexString());
	}
	os << '{';
	InsetMathGrid::write(os);
	os << "}\n";
}


void InsetMathXYMatrix::infoize(odocstream & os) const
{
	os << "xymatrix ";
	switch (spacing_code_) {
	case 'R':
	case 'C':
	case 'M':
	case 'W':
	case 'H':
	case 'L':
		os << spacing_code_ << ' '
		   << from_ascii(spacing_.asLatexString()) << ' ';
		break;
	default:
		if (!spacing_.empty())
			os << from_ascii(spacing_.asLatexString()) << ' ';
	}
	InsetMathGrid::infoize(os);
}


void InsetMathXYMatrix::normalize(NormalStream & os) const
{
	os << "[xymatrix ";
	InsetMathGrid::normalize(os);
	os << ']';
}


void InsetMathXYMatrix::maple(MapleStream & os) const
{
	os << "xymatrix(";
	InsetMathGrid::maple(os);
	os << ')';
}


void InsetMathXYMatrix::validate(LaTeXFeatures & features) const
{
	features.require("xy");
	InsetMathGrid::validate(features);
}


} // namespace lyx