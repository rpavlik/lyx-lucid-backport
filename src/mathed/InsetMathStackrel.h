// -*- C++ -*-
/**
 * \file InsetMathStackrel.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef MATH_STACKRELINSET_H
#define MATH_STACKRELINSET_H

#include "InsetMathFracBase.h"


namespace lyx {


/** Stackrel objects
 *  \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */
class InsetMathStackrel : public InsetMathFracBase {
public:
	///
	InsetMathStackrel();
	///
	bool metrics(MetricsInfo & mi, Dimension & dim) const;
	///
	void draw(PainterInfo & pi, int x, int y) const;

	///
	void write(WriteStream & os) const;
	///
	void normalize(NormalStream &) const;
private:
	virtual std::auto_ptr<Inset> doClone() const;
};



} // namespace lyx
#endif
