// -*- C++ -*-
/**
 * \file src/FontInfo.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Lars Gullik Bj�nnes
 * \author Jean-Marc Lasgouttes
 * \author Angus Leeming
 * \author Dekel Tsur
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef FONT_PROPERTIES_H
#define FONT_PROPERTIES_H

#ifdef TEX2LYX
#include "tex2lyx/Font.h"
#else

#include "Color.h"
#include "ColorCode.h"
#include "FontEnums.h"

namespace lyx {

///
class FontInfo
{
public:
	///
	FontInfo();
	///
	FontInfo(
		FontFamily family,
		FontSeries series,
		FontShape shape,
		FontSize size,
		ColorCode color,
		ColorCode background,
		FontState emph,
		FontState underbar,
		FontState noun,
		FontState number)
		: family_(family), series_(series), shape_(shape), size_(size), 
		color_(color), background_(background), paint_color_(), emph_(emph),
		underbar_(underbar), noun_(noun), number_(number)
	{}

	/// Decreases font size by one
	FontInfo & decSize();
	/// Increases font size by one
	FontInfo & incSize();

	/// Accessor methods.
	///@{
	FontFamily family() const { return family_; }
	void setFamily(FontFamily f) { family_ = f; }
	FontSeries series() const { return series_; }
	void setSeries(FontSeries s) { series_ = s; }
	FontShape shape() const { return shape_; }
	void setShape(FontShape s) { shape_ = s; }
	FontSize size() const { return size_; }
	void setSize(FontSize s) { size_ = s; }
	FontState emph() const { return emph_; }
	void setEmph(FontState e) { emph_ = e; }
	FontState underbar() const { return underbar_; }
	void setUnderbar(FontState u) { underbar_ = u; }
	FontState noun() const { return noun_; }
	void setNoun(FontState n) { noun_ = n; }
	FontState number() const { return number_; }
	void setNumber(FontState n) { number_ = n; }
	ColorCode color() const { return color_; }
	void setColor(ColorCode c) { color_ = c; }
	ColorCode background() const { return background_; }
	void setBackground(ColorCode b) { background_ = b; }
	///@}

	///
	void update(FontInfo const & newfont, bool toggleall);

	/** Reduce font to fall back to template where possible.
	    Equal fields are reduced to INHERIT */
	void reduce(FontInfo const & tmplt);

	/// Realize font from a template (INHERIT are realized)
	FontInfo & realize(FontInfo const & tmplt);
	/// Is a given font fully resolved?
	bool resolved() const;

	/// The real color of the font. This can be the color that is 
	/// set for painting, the color of the font or a default color.
	Color realColor() const;
	/// Sets the color which is used during painting
	void setPaintColor(Color c) { paint_color_ = c; }

	/// Converts logical attributes to concrete shape attribute
	/// Try hard to inline this as it shows up with 4.6 % in the profiler.
	FontShape realShape() const
	{
		if (noun_ == FONT_ON)
			return SMALLCAPS_SHAPE;
		if (emph_ == FONT_ON)
			return (shape_ == UP_SHAPE) ? ITALIC_SHAPE : UP_SHAPE;
		return shape_;
	}

	bool isSymbolFont() const
	{
		switch (family_) {
		case SYMBOL_FAMILY:
		case CMSY_FAMILY:
		case CMM_FAMILY:
		case CMEX_FAMILY:
		case MSA_FAMILY:
		case MSB_FAMILY:
		case WASY_FAMILY:
		case ESINT_FAMILY:
			return true;
		default:
			return false;
		}
	}

private:
	friend bool operator==(FontInfo const & lhs, FontInfo const & rhs);
	///
	FontFamily family_;
	///
	FontSeries series_;
	///
	FontShape shape_;
	///
	FontSize size_;
	///
	ColorCode color_;
	///
	ColorCode background_;
	/// The color used for painting
	Color paint_color_;
	///
	FontState emph_;
	///
	FontState underbar_;
	///
	FontState noun_;
	///
	FontState number_;
};


inline bool operator==(FontInfo const & lhs, FontInfo const & rhs)
{
	return lhs.family_ == rhs.family_
		&& lhs.series_ == rhs.series_
		&& lhs.shape_ == rhs.shape_
		&& lhs.size_ == rhs.size_
		&& lhs.color_ == rhs.color_
		&& lhs.background_ == rhs.background_
		&& lhs.emph_ == rhs.emph_
		&& lhs.underbar_ == rhs.underbar_
		&& lhs.noun_ == rhs.noun_
		&& lhs.number_ == rhs.number_;
}


inline bool operator!=(FontInfo const & lhs, FontInfo const & rhs)
{
	return !operator==(lhs, rhs);
}

/// Sane font.
extern FontInfo const sane_font;
/// All inherit font.
extern FontInfo const inherit_font;
/// All ignore font.
extern FontInfo const ignore_font;

} // namespace lyx

#endif // TEX2LYX_FONT_H
#endif