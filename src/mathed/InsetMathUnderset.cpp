/**
 * \file InsetMathUnderset.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "InsetMathUnderset.h"
#include "MathData.h"
#include "MathStream.h"

#include "Cursor.h"
#include "LaTeXFeatures.h"

using namespace std;

namespace lyx {

Inset * InsetMathUnderset::clone() const
{
	return new InsetMathUnderset(*this);
}


void InsetMathUnderset::metrics(MetricsInfo & mi, Dimension & dim) const
{
	Dimension dim1;
	cell(1).metrics(mi, dim1);
	FracChanger dummy(mi.base);
	Dimension dim0;
	cell(0).metrics(mi, dim0);
	dim.wid = max(dim0.width(), dim1.width()) + 4;
	dim.asc = dim1.ascent();
	dim.des = dim1.descent() + dim0.height() + 4;
	metricsMarkers(dim);
}


void InsetMathUnderset::draw(PainterInfo & pi, int x, int y) const
{
	Dimension const dim = dimension(*pi.base.bv);
	Dimension const & dim0 = cell(0).dimension(*pi.base.bv);
	Dimension const & dim1 = cell(1).dimension(*pi.base.bv);
	int m  = x + dim.wid / 2;
	int yo = y + dim1.descent() + dim0.ascent() + 1;
	cell(1).draw(pi, m - dim1.width() / 2, y);
	FracChanger dummy(pi.base);
	cell(0).draw(pi, m - dim0.width() / 2, yo);
	drawMarkers(pi, x, y);
}


bool InsetMathUnderset::idxFirst(Cursor & cur) const
{
	cur.idx() = 1;
	cur.pos() = 0;
	return true;
}


bool InsetMathUnderset::idxLast(Cursor & cur) const
{
	cur.idx() = 1;
	cur.pos() = cur.lastpos();
	return true;
}


bool InsetMathUnderset::idxUpDown(Cursor & cur, bool up) const
{
	idx_type target = up; // up ? 1 : 0, since upper cell has idx 1
	if (cur.idx() == target)
		return false;
	cur.idx() = target;
	cur.pos() = cur.cell().x2pos(&cur.bv(), cur.x_target());
	return true;
}


void InsetMathUnderset::write(WriteStream & os) const
{
	os << "\\underset{" << cell(0) << "}{" << cell(1) << '}';
}


void InsetMathUnderset::normalize(NormalStream & os) const
{
	os << "[underset " << cell(0) << ' ' << cell(1) << ']';
}


void InsetMathUnderset::validate(LaTeXFeatures & features) const
{
	features.require("amsmath");
	InsetMathNest::validate(features);
}


} // namespace lyx
