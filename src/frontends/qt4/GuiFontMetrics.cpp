/**
 * \file GuiFontMetrics.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author unknown
 * \author John Levon
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "GuiFontMetrics.h"

#include "qt_helpers.h"

#include "Language.h"
#include "Dimension.h"

#include "support/unicode.h"

using std::string;

namespace lyx {
namespace frontend {

// Caution: When using ucs4_to_qchar() in these methods, this is no
// real conversion but a simple cast in reality. This is the reason
// why this works well for symbol fonts used in mathed too, even though
// these are not real ucs4 characters. These are codepoints in the
// modern fonts used, nothing unicode related.
// See comment in QLPainter::text() for more explanation.

GuiFontMetrics::GuiFontMetrics(QFont const & font)
: metrics_(font), smallcaps_metrics_(font), smallcaps_shape_(false)
{
}


GuiFontMetrics::GuiFontMetrics(QFont const & font, QFont const & smallcaps_font)
: metrics_(font), smallcaps_metrics_(smallcaps_font), smallcaps_shape_(true)
{
}


int GuiFontMetrics::maxAscent() const
{
	return metrics_.ascent();
}


int GuiFontMetrics::maxDescent() const
{
	// We add 1 as the value returned by QT is different than X
	// See http://doc.trolltech.com/2.3/qfontmetrics.html#200b74
	return metrics_.descent() + 1;
}


int GuiFontMetrics::lbearing(char_type c) const
{
	if (!is_utf16(c))
		// FIXME: QFontMetrics::leftBearingdoes not support the
		//        full unicode range. Once it does, we could use:
		//return metrics_.leftBearing(toqstr(docstring(1,c)));
		return 0;

	return metrics_.leftBearing(ucs4_to_qchar(c));
}


int GuiFontMetrics::rbearing(char_type c) const
{
	if (!rbearing_cache_.contains(c)) {
		// Qt rbearing is from the right edge of the char's width().
		int rb;
		if (is_utf16(c)) {
			QChar sc = ucs4_to_qchar(c);
			rb = width(c) - metrics_.rightBearing(sc);
		} else
			// FIXME: QFontMetrics::leftBearingdoes not support the
			//        full unicode range. Once it does, we could use:
			// metrics_.rightBearing(toqstr(docstring(1,c)));
			rb = width(c);

		rbearing_cache_.insert(c, rb);
	}
	return rbearing_cache_.value(c);
}


int GuiFontMetrics::smallcapsWidth(char_type c) const
{
	// FIXME: Optimisation probably needed: we don't use the width cache.
	if (is_utf16(c)) {
		QChar const qc = ucs4_to_qchar(c);
		QChar const uc = qc.toUpper();
		if (qc != uc)
			return smallcaps_metrics_.width(uc);
		else
			return metrics_.width(qc);
	} else {
		QString const s = toqstr(docstring(1,c));
		QString const us = s.toUpper();
		if (s != us)
			return smallcaps_metrics_.width(us);
		else
			return metrics_.width(s);
	}
}


int GuiFontMetrics::width(docstring const & s) const
{
	size_t ls = s.size();
	int w = 0;
	for (unsigned int i = 0; i < ls; ++i)
		w += width(s[i]);

	return w;
}


int GuiFontMetrics::width(QString const & ucs2) const
{
	return width(qstring_to_ucs4(ucs2));
}


int GuiFontMetrics::signedWidth(docstring const & s) const
{
	if (s.empty())
		return 0;

	if (s[0] == '-')
		return -width(s.substr(1, s.size() - 1));
	else
		return width(s);
}


void GuiFontMetrics::rectText(docstring const & str,
	int & w, int & ascent, int & descent) const
{
	static int const d = 2;
	w = width(str) + d * 2 + 2;
	ascent = metrics_.ascent() + d;
	descent = metrics_.descent() + d;
}



void GuiFontMetrics::buttonText(docstring const & str,
	int & w, int & ascent, int & descent) const
{
	static int const d = 3;
	w = width(str) + d * 2 + 2;
	ascent = metrics_.ascent() + d;
	descent = metrics_.descent() + d;
}


Dimension const GuiFontMetrics::defaultDimension() const
{
	return Dimension(0, maxAscent(), maxDescent());
}


Dimension const GuiFontMetrics::dimension(char_type c) const
{
	return Dimension(width(c), ascent(c), descent(c));
}


void GuiFontMetrics::fillMetricsCache(char_type c) const
{
	QRect r;
	if (is_utf16(c))
		r = metrics_.boundingRect(ucs4_to_qchar(c));
	else
		r = metrics_.boundingRect(toqstr(docstring(1,c)));

	AscendDescend ad = { -r.top(), r.bottom() + 1};
	// We could as well compute the width but this is not really
	// needed for now as it is done directly in width() below.
	metrics_cache_.insert(c, ad);
}


int GuiFontMetrics::width(char_type c) const
{
	if (smallcaps_shape_)
		return smallcapsWidth(c);

	if (!width_cache_.contains(c)) {
		if (is_utf16(c))
			width_cache_.insert(c, metrics_.width(ucs4_to_qchar(c)));
		else
			width_cache_.insert(c, metrics_.width(toqstr(docstring(1,c))));
	}

	return width_cache_.value(c);
}


int GuiFontMetrics::ascent(char_type c) const
{
	if (!metrics_cache_.contains(c))
		fillMetricsCache(c);

	return metrics_cache_.value(c).ascent;
}


int GuiFontMetrics::descent(char_type c) const
{
	if (!metrics_cache_.contains(c))
		fillMetricsCache(c);

	return metrics_cache_.value(c).descent;
}

} // frontend
} // lyx