// -*- C++ -*-
/**
 * \file InsetMathFont.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef MATH_FONTINSET_H
#define MATH_FONTINSET_H

#include "InsetMathNest.h"


namespace lyx {


class latexkeys;

/// Inset for font changes
class InsetMathFont : public InsetMathNest {
public:
	///
	explicit InsetMathFont(latexkeys const * key);
	///
	InsetMathFont * asFontInset() { return this; }
	///
	InsetMathFont const * asFontInset() const { return this; }
	/// are we in math mode, text mode, or unsure?
	mode_type currentMode() const;
	///
	docstring name() const;
	///
	bool metrics(MetricsInfo & mi, Dimension & dim) const;
	///
	void draw(PainterInfo & pi, int x, int y) const;
	///
	void metricsT(TextMetricsInfo const & mi, Dimension & dim) const;
	///
	void drawT(TextPainter & pi, int x, int y) const;
	///
	void validate(LaTeXFeatures & features) const;
	///
	void infoize(odocstream & os) const;
	///
	int kerning() const { return cell(0).kerning(); }

private:
	virtual std::auto_ptr<Inset> doClone() const;
	/// the font to be used on screen
	latexkeys const * key_;
};


} // namespace lyx
#endif
