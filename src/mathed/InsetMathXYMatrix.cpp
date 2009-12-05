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

#include "LaTeXFeatures.h"
#include "MathStream.h"

#include <ostream>

namespace lyx {


InsetMathXYMatrix::InsetMathXYMatrix(Buffer * buf, Length const & s, char c)
	: InsetMathGrid(buf, 1, 1), spacing_(s), spacing_code_(c)
{}


Inset * InsetMathXYMatrix::clone() const
{
	return new InsetMathXYMatrix(*this);
}


int InsetMathXYMatrix::colsep() const
{
	return 40;
}


int InsetMathXYMatrix::rowsep() const
{
	return 40;
}


void InsetMathXYMatrix::metrics(MetricsInfo & mi, Dimension & dim) const
{
	if (mi.base.style == LM_ST_DISPLAY)
		mi.base.style = LM_ST_TEXT;
	InsetMathGrid::metrics(mi, dim);
}


void InsetMathXYMatrix::write(WriteStream & os) const
{
	MathEnsurer ensurer(os);
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
