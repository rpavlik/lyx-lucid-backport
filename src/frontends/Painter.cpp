/**
 * \file Painter.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author unknown
 * \author John Levon
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "frontends/Painter.h"

#include "frontends/FontMetrics.h"

#include "Color.h"
#include "Font.h"

using lyx::docstring;

using std::max;
using std::string;

namespace lyx {
namespace frontend {

void Painter::button(int x, int y, int w, int h, bool mouseHover)
{
	if (mouseHover)
		fillRectangle(x, y, w, h, Color::buttonhoverbg);
	else
		fillRectangle(x, y, w, h, Color::buttonbg);
	buttonFrame(x, y, w, h);
}


void Painter::buttonFrame(int x, int y, int w, int h)
{
	line(x, y, x, y + h - 1, Color::buttonframe);
	line(x - 1 + w, y, x - 1 + w, y + h - 1, Color::buttonframe);
	line(x, y - 1, x - 1 + w, y - 1, Color::buttonframe);
	line(x, y + h - 1, x - 1 + w, y + h - 1, Color::buttonframe);
}


void Painter::rectText(int x, int y,
	docstring const & str,
	Font const & font,
	Color_color back,
	Color_color frame)
{
	int width;
	int ascent;
	int descent;

	FontMetrics const & fm = theFontMetrics(font);
	fm.rectText(str, width, ascent, descent);

	if (back != Color::none)
		fillRectangle(x + 1, y - ascent + 1, width - 1,
			      ascent + descent - 1, back);

	if (frame != Color::none)
		rectangle(x, y - ascent, width, ascent + descent, frame);

	text(x + 3, y, str, font);
}


void Painter::buttonText(int x, int y, docstring const & str,
	Font const & font, bool mouseHover)
{
	int width;
	int ascent;
	int descent;

	FontMetrics const & fm = theFontMetrics(font);
	fm.buttonText(str, width, ascent, descent);

	button(x, y - ascent, width, descent + ascent, mouseHover);
	text(x + 3, y - 1, str, font);
}


int Painter::preeditText(int x, int y, char_type c,
	Font const & font, preedit_style style)
{
	Font temp_font = font;
	FontMetrics const & fm = theFontMetrics(font);
	int ascent = fm.maxAscent();
	int descent = fm.maxDescent();
	int height = ascent + descent;
	int width = fm.width(c);

	switch (style) {
		case preedit_default:
			// default unselecting mode.
			fillRectangle(x, y - height + 1, width, height, Color::background);
			dashedUnderline(font, x, y - descent + 1, width);
			break;
		case preedit_selecting:
			// We are in selecting mode: white text on black background.
			fillRectangle(x, y - height + 1, width, height, Color::black);
			temp_font.setColor(Color::white);
			break;
		case preedit_cursor:
			// The character comes with a cursor.
			fillRectangle(x, y - height + 1, width, height, Color::background);
			underline(font, x, y - descent + 1, width);
			break;
	}
	text(x, y - descent + 1, c, temp_font);

	return width;
}


void Painter::underline(Font const & f, int x, int y, int width)
{
	FontMetrics const & fm = theFontMetrics(f);

	int const below = max(fm.maxDescent() / 2, 2);
	int const height = max((fm.maxDescent() / 4) - 1, 1);

	if (height < 2)
		line(x, y + below, x + width, y + below, f.color());
	else
		fillRectangle(x, y + below, width, below + height, f.color());
}


void Painter::dashedUnderline(Font const & f, int x, int y, int width)
{
	FontMetrics const & fm = theFontMetrics(f);

	int const below = max(fm.maxDescent() / 2, 2);
	int height = max((fm.maxDescent() / 4) - 1, 1);

	if (height >= 2)
		height += below;

	for (int n = 0; n < height; ++n)
		line(x, y + below + n, x + width, y + below + n, f.color(), line_onoffdash);
}

} // namespace frontend
} // namespace lyx