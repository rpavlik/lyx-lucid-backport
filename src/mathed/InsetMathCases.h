// -*- C++ -*-
/**
 * \file InsetMathCases.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef MATH_CASESINSET_H
#define MATH_CASESINSET_H

#include "InsetMathGrid.h"


namespace lyx {


class LaTeXFeatures;

class InsetMathCases : public InsetMathGrid {
public:
	///
	explicit InsetMathCases(row_type rows = 1u);
	///
	void metrics(MetricsInfo & mi, Dimension & dim) const;
	///
	Dimension const dimension(BufferView const &) const;
	///
	void draw(PainterInfo & pi, int x, int y) const;
	///
	virtual void doDispatch(Cursor & cur, FuncRequest & cmd);
	///
	bool getStatus(Cursor & cur, FuncRequest const & cmd,
		FuncStatus & flag) const;

	///
	void infoize(odocstream & os) const;
	///
	void normalize(NormalStream &) const;
	///
	void maple(MapleStream &) const;
	///
	void write(WriteStream & os) const;
	///
	void validate(LaTeXFeatures & features) const;
private:
	virtual Inset * clone() const;
};


} // namespace lyx

#endif