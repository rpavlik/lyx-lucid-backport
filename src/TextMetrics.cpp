/**
 * \file src/TextMetrics.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Asger Alstrup
 * \author Lars Gullik Bjønnes
 * \author Jean-Marc Lasgouttes
 * \author John Levon
 * \author André Pönitz
 * \author Dekel Tsur
 * \author Jürgen Vigna
 * \author Abdelrazak Younes
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "TextMetrics.h"

#include "Bidi.h"
#include "Buffer.h"
#include "buffer_funcs.h"
#include "BufferParams.h"
#include "BufferView.h"
#include "CoordCache.h"
#include "Cursor.h"
#include "CutAndPaste.h"
#include "HSpace.h"
#include "InsetList.h"
#include "Layout.h"
#include "Length.h"
#include "LyXRC.h"
#include "MetricsInfo.h"
#include "ParagraphParameters.h"
#include "ParIterator.h"
#include "rowpainter.h"
#include "Text.h"
#include "TextClass.h"
#include "VSpace.h"
#include "WordLangTuple.h"

#include "insets/InsetText.h"

#include "mathed/MacroTable.h"
#include "mathed/MathMacroTemplate.h"

#include "frontends/FontMetrics.h"
#include "frontends/Painter.h"

#include "support/debug.h"
#include "support/docstring_list.h"
#include "support/lassert.h"

#include <cstdlib>

using namespace std;


namespace lyx {

using frontend::FontMetrics;

static int numberOfSeparators(Paragraph const & par, Row const & row)
{
	pos_type const first = max(row.pos(), par.beginOfBody());
	pos_type const last = row.endpos() - 1;
	int n = 0;
	for (pos_type p = first; p < last; ++p) {
		if (par.isSeparator(p))
			++n;
	}
	return n;
}


static int numberOfLabelHfills(Paragraph const & par, Row const & row)
{
	pos_type last = row.endpos() - 1;
	pos_type first = row.pos();

	// hfill *DO* count at the beginning of paragraphs!
	if (first) {
		while (first < last && par.isHfill(first))
			++first;
	}

	last = min(last, par.beginOfBody());
	int n = 0;
	for (pos_type p = first; p < last; ++p) {
		if (par.isHfill(p))
			++n;
	}
	return n;
}


static int numberOfHfills(Paragraph const & par, Row const & row)
{
	pos_type const last = row.endpos();
	pos_type first = row.pos();

	// hfill *DO* count at the beginning of paragraphs!
	if (first) {
		while (first < last && par.isHfill(first))
			++first;
	}

	first = max(first, par.beginOfBody());

	int n = 0;
	for (pos_type p = first; p < last; ++p) {
		if (par.isHfill(p))
			++n;
	}
	return n;
}


/////////////////////////////////////////////////////////////////////
//
// TextMetrics
//
/////////////////////////////////////////////////////////////////////


TextMetrics::TextMetrics(BufferView * bv, Text * text)
	: bv_(bv), text_(text)
{
	LASSERT(bv_, /**/);
	max_width_ = bv_->workWidth();
	dim_.wid = max_width_;
	dim_.asc = 10;
	dim_.des = 10;
}


bool TextMetrics::contains(pit_type pit) const
{
	return par_metrics_.find(pit) != par_metrics_.end();
}


ParagraphMetrics const & TextMetrics::parMetrics(pit_type pit) const
{
	return const_cast<TextMetrics *>(this)->parMetrics(pit, true);
}



pair<pit_type, ParagraphMetrics const *> TextMetrics::first() const
{
	ParMetricsCache::const_iterator it = par_metrics_.begin();
	return make_pair(it->first, &it->second);
}


pair<pit_type, ParagraphMetrics const *> TextMetrics::last() const
{
	LASSERT(!par_metrics_.empty(), /**/);
	ParMetricsCache::const_reverse_iterator it = par_metrics_.rbegin();
	return make_pair(it->first, &it->second);
}


ParagraphMetrics & TextMetrics::parMetrics(pit_type pit, bool redo)
{
	ParMetricsCache::iterator pmc_it = par_metrics_.find(pit);
	if (pmc_it == par_metrics_.end()) {
		pmc_it = par_metrics_.insert(
			make_pair(pit, ParagraphMetrics(text_->getPar(pit)))).first;
	}
	if (pmc_it->second.rows().empty() && redo)
		redoParagraph(pit);
	return pmc_it->second;
}


bool TextMetrics::metrics(MetricsInfo & mi, Dimension & dim, int min_width)
{
	LASSERT(mi.base.textwidth > 0, /**/);
	max_width_ = mi.base.textwidth;
	// backup old dimension.
	Dimension const old_dim = dim_;
	// reset dimension.
	dim_ = Dimension();
	dim_.wid = min_width;
	pit_type const npar = text_->paragraphs().size();
	if (npar > 1)
		// If there is more than one row, expand the text to
		// the full allowable width.
		dim_.wid = max_width_;

	//lyxerr << "TextMetrics::metrics: width: " << mi.base.textwidth
	//	<< " maxWidth: " << max_width_ << "\nfont: " << mi.base.font << endl;

	bool changed = false;
	unsigned int h = 0;
	for (pit_type pit = 0; pit != npar; ++pit) {
		changed |= redoParagraph(pit);
		ParagraphMetrics const & pm = par_metrics_[pit];
		h += pm.height();
		if (dim_.wid < pm.width())
			dim_.wid = pm.width();
	}

	dim_.asc = par_metrics_[0].ascent();
	dim_.des = h - dim_.asc;
	//lyxerr << "dim_.wid " << dim_.wid << endl;
	//lyxerr << "dim_.asc " << dim_.asc << endl;
	//lyxerr << "dim_.des " << dim_.des << endl;

	changed |= dim_ != old_dim;
	dim = dim_;
	return changed;
}


int TextMetrics::rightMargin(ParagraphMetrics const & pm) const
{
	return main_text_? pm.rightMargin(*bv_) : 0;
}


int TextMetrics::rightMargin(pit_type const pit) const
{
	return main_text_? par_metrics_[pit].rightMargin(*bv_) : 0;
}


void TextMetrics::applyOuterFont(Font & font) const
{
	FontInfo lf(font_.fontInfo());
	lf.reduce(bv_->buffer().params().getFont().fontInfo());
	font.fontInfo().realize(lf); 
}


Font TextMetrics::displayFont(pit_type pit, pos_type pos) const
{
	LASSERT(pos >= 0, /**/);

	ParagraphList const & pars = text_->paragraphs();
	Paragraph const & par = pars[pit];
	Layout const & layout = par.layout();
	Buffer const & buffer = bv_->buffer();
	// FIXME: broken?
	BufferParams const & params = buffer.params();
	pos_type const body_pos = par.beginOfBody();

	// We specialize the 95% common case:
	if (!par.getDepth()) {
		Font f = par.getFontSettings(params, pos);
		if (!text_->isMainText())
			applyOuterFont(f);
		bool lab = layout.labeltype == LABEL_MANUAL && pos < body_pos;

		FontInfo const & lf = lab ? layout.labelfont : layout.font;
		FontInfo rlf = lab ? layout.reslabelfont : layout.resfont;

		// In case the default family has been customized
		if (lf.family() == INHERIT_FAMILY)
			rlf.setFamily(params.getFont().fontInfo().family());
		f.fontInfo().realize(rlf);
		return f;
	}

	// The uncommon case need not be optimized as much
	FontInfo const & layoutfont = pos < body_pos ?
		layout.labelfont : layout.font;

	Font font = par.getFontSettings(params, pos);
	font.fontInfo().realize(layoutfont);

	if (!text_->isMainText())
		applyOuterFont(font);

	// Realize against environment font information
	// NOTE: the cast to pit_type should be removed when pit_type
	// changes to a unsigned integer.
	if (pit < pit_type(pars.size()))
		font.fontInfo().realize(text_->outerFont(pit).fontInfo());

	// Realize with the fonts of lesser depth.
	font.fontInfo().realize(params.getFont().fontInfo());

	return font;
}


bool TextMetrics::isRTL(CursorSlice const & sl, bool boundary) const
{
	if (!lyxrc.rtl_support || !sl.text())
		return false;

	int correction = 0;
	if (boundary && sl.pos() > 0)
		correction = -1;

	return displayFont(sl.pit(), sl.pos() + correction).isVisibleRightToLeft();
}


bool TextMetrics::isRTLBoundary(pit_type pit, pos_type pos) const
{
	// no RTL boundary at paragraph start
	if (!lyxrc.rtl_support || pos == 0)
		return false;

	Font const & left_font = displayFont(pit, pos - 1);

	return isRTLBoundary(pit, pos, left_font);
}


// isRTLBoundary returns false on a real end-of-line boundary,
// because otherwise the two boundary types get mixed up.
// This is the whole purpose of this being in TextMetrics.
bool TextMetrics::isRTLBoundary(pit_type pit, pos_type pos,
		Font const & font) const
{
	if (!lyxrc.rtl_support
	    // no RTL boundary at paragraph start
	    || pos == 0
	    // if the metrics have not been calculated, then we are not
	    // on screen and can safely ignore issues about boundaries.
	    || !contains(pit))
		return false;

	ParagraphMetrics & pm = par_metrics_[pit];
	// no RTL boundary in empty paragraph
	if (pm.rows().empty())
		return false;

	pos_type endpos = pm.getRow(pos - 1, false).endpos();
	pos_type startpos = pm.getRow(pos, false).pos();
	// no RTL boundary at line start:
	// abc\n   -> toggle to RTL ->    abc\n     (and not:    abc\n|
	// |                              |                               )
	if (pos == startpos && pos == endpos) // start of cur row, end of prev row
		return false;

	Paragraph const & par = text_->getPar(pit);
	// no RTL boundary at line break:
	// abc|\n    -> move right ->   abc\n       (and not:    abc\n|
	// FED                          FED|                     FED     )
	if (startpos == pos && endpos == pos && endpos != par.size() 
		&& (par.isNewline(pos - 1) 
			|| par.isLineSeparator(pos - 1) 
			|| par.isSeparator(pos - 1)))
		return false;
	
	bool left = font.isVisibleRightToLeft();
	bool right;
	if (pos == par.size())
		right = par.isRTL(bv_->buffer().params());
	else
		right = displayFont(pit, pos).isVisibleRightToLeft();
	
	return left != right;
}


bool TextMetrics::redoParagraph(pit_type const pit)
{
	Paragraph & par = text_->getPar(pit);
	// IMPORTANT NOTE: We pass 'false' explicitly in order to not call
	// redoParagraph() recursively inside parMetrics.
	Dimension old_dim = parMetrics(pit, false).dim();
	ParagraphMetrics & pm = par_metrics_[pit];
	pm.reset(par);

	Buffer & buffer = bv_->buffer();
	main_text_ = (text_ == &buffer.text());
	bool changed = false;

	// FIXME: This check ought to be done somewhere else. It is the reason
	// why text_ is not	const. But then, where else to do it?
	// Well, how can you end up with either (a) a biblio environment that
	// has no InsetBibitem or (b) a biblio environment with more than one
	// InsetBibitem? I think the answer is: when paragraphs are merged;
	// when layout is set; when material is pasted.
	int const moveCursor = par.checkBiblio(buffer);
	if (moveCursor > 0)
		const_cast<Cursor &>(bv_->cursor()).posForward();
	else if (moveCursor < 0) {
		Cursor & cursor = const_cast<Cursor &>(bv_->cursor());
		if (cursor.pos() >= -moveCursor)
			cursor.posBackward();
	}

	// Optimisation: this is used in the next two loops
	// so better to calculate that once here.
	int const right_margin = rightMargin(pm);

	// iterator pointing to paragraph to resolve macros
	DocIterator parPos = text_->macrocontextPosition();
	if (!parPos.empty())
		parPos.pit() = pit;
	else {
		LYXERR(Debug::INFO, "MacroContext not initialised!"
			<< " Going through the buffer again and hope"
			<< " the context is better then.");
		// FIXME audit updateBuffer calls
		// This should not be here, but it is not clear yet where else it
		// should be.
		bv_->buffer().updateBuffer();
		parPos = text_->macrocontextPosition();
		LASSERT(!parPos.empty(), /**/);
		parPos.pit() = pit;
	}

	// redo insets
	Font const bufferfont = buffer.params().getFont();
	InsetList::const_iterator ii = par.insetList().begin();
	InsetList::const_iterator iend = par.insetList().end();
	for (; ii != iend; ++ii) {
		// FIXME Doesn't this HAVE to be non-empty?
		// position already initialized?
		if (!parPos.empty()) {
			parPos.pos() = ii->pos;

			// A macro template would normally not be visible
			// by itself. But the tex macro semantics allow
			// recursion, so we artifically take the context
			// after the macro template to simulate this.
			if (ii->inset->lyxCode() == MATHMACRO_CODE)
				parPos.pos()++;
		}

		// do the metric calculation
		Dimension dim;
		int const w = max_width_ - leftMargin(max_width_, pit, ii->pos)
			- right_margin;
		Font const & font = ii->inset->inheritFont() ?
			displayFont(pit, ii->pos) : bufferfont;
		MacroContext mc(&buffer, parPos);
		MetricsInfo mi(bv_, font.fontInfo(), w, mc);
		ii->inset->metrics(mi, dim);
		Dimension const old_dim = pm.insetDimension(ii->inset);
		if (old_dim != dim) {
			pm.setInsetDimension(ii->inset, dim);
			changed = true;
		}
	}

	par.setBeginOfBody();
	pos_type first = 0;
	size_t row_index = 0;
	// maximum pixel width of a row
	int width = max_width_ - right_margin; // - leftMargin(max_width_, pit, row);
	do {
		Dimension dim;
		pos_type end = rowBreakPoint(width, pit, first);
		if (row_index || end < par.size())
			// If there is more than one row, expand the text to
			// the full allowable width. This setting here is needed
			// for the computeRowMetrics() below.
			dim_.wid = max_width_;

		dim = rowHeight(pit, first, end);
		dim.wid = rowWidth(right_margin, pit, first, end);
		if (row_index == pm.rows().size())
			pm.rows().push_back(Row());
		Row & row = pm.rows()[row_index];
		row.setChanged(false);
		row.pos(first);
		row.endpos(end);
		row.setDimension(dim);
		int const max_row_width = max(dim_.wid, dim.wid);
		computeRowMetrics(pit, row, max_row_width);
		first = end;
		++row_index;

		pm.dim().wid = max(pm.dim().wid, dim.wid);
		pm.dim().des += dim.height();
	} while (first < par.size());

	if (row_index < pm.rows().size())
		pm.rows().resize(row_index);

	// Make sure that if a par ends in newline, there is one more row
	// under it
	if (first > 0 && par.isNewline(first - 1)) {
		Dimension dim = rowHeight(pit, first, first);
		dim.wid = rowWidth(right_margin, pit, first, first);
		if (row_index == pm.rows().size())
			pm.rows().push_back(Row());
		Row & row = pm.rows()[row_index];
		row.setChanged(false);
		row.pos(first);
		row.endpos(first);
		row.setDimension(dim);
		int const max_row_width = max(dim_.wid, dim.wid);
		computeRowMetrics(pit, row, max_row_width);
		pm.dim().des += dim.height();
	}

	pm.dim().asc += pm.rows()[0].ascent();
	pm.dim().des -= pm.rows()[0].ascent();

	changed |= old_dim.height() != pm.dim().height();

	return changed;
}


void TextMetrics::computeRowMetrics(pit_type const pit,
		Row & row, int width) const
{
	row.label_hfill = 0;
	row.separator = 0;

	Paragraph const & par = text_->getPar(pit);

	double w = width - row.width();
	// FIXME: put back this assertion when the crash on new doc is solved.
	//LASSERT(w >= 0, /**/);

	//lyxerr << "\ndim_.wid " << dim_.wid << endl;
	//lyxerr << "row.width() " << row.width() << endl;
	//lyxerr << "w " << w << endl;

	bool const is_rtl = text_->isRTL(par);
	if (is_rtl)
		row.x = rightMargin(pit);
	else
		row.x = leftMargin(max_width_, pit, row.pos());

	// is there a manual margin with a manual label
	Layout const & layout = par.layout();

	if (layout.margintype == MARGIN_MANUAL
	    && layout.labeltype == LABEL_MANUAL) {
		/// We might have real hfills in the label part
		int nlh = numberOfLabelHfills(par, row);

		// A manual label par (e.g. List) has an auto-hfill
		// between the label text and the body of the
		// paragraph too.
		// But we don't want to do this auto hfill if the par
		// is empty.
		if (!par.empty())
			++nlh;

		if (nlh && !par.getLabelWidthString().empty())
			row.label_hfill = labelFill(pit, row) / double(nlh);
	}

	double hfill = 0;
	// are there any hfills in the row?
	if (int const nh = numberOfHfills(par, row)) {
		if (w > 0)
			hfill = w / double(nh);
	// we don't have to look at the alignment if it is ALIGN_LEFT and
	// if the row is already larger then the permitted width as then
	// we force the LEFT_ALIGN'edness!
	} else if (int(row.width()) < max_width_) {
		// is it block, flushleft or flushright?
		// set x how you need it
		int align;
		if (par.params().align() == LYX_ALIGN_LAYOUT)
			align = layout.align;
		else
			align = par.params().align();

		// handle alignment inside tabular cells
		Inset const & owner = text_->inset();
		switch (owner.contentAlignment()) {
			case LYX_ALIGN_CENTER:
			case LYX_ALIGN_LEFT:
			case LYX_ALIGN_RIGHT:
				if (align == LYX_ALIGN_NONE 
				    || align == LYX_ALIGN_BLOCK)
					align = owner.contentAlignment();
				break;
			default:
				// unchanged (use align)
				break;
		}

		// Display-style insets should always be on a centered row
		if (Inset const * inset = par.getInset(row.pos())) {
			switch (inset->display()) {
				case Inset::AlignLeft:
					align = LYX_ALIGN_BLOCK;
					break;
				case Inset::AlignCenter:
					align = LYX_ALIGN_CENTER;
					break;
				case Inset::Inline:
					// unchanged (use align)
					break;
				case Inset::AlignRight:
					align = LYX_ALIGN_RIGHT;
					break;
			}
		}

		switch (align) {
		case LYX_ALIGN_BLOCK: {
			int const ns = numberOfSeparators(par, row);
			bool disp_inset = false;
			if (row.endpos() < par.size()) {
				Inset const * in = par.getInset(row.endpos());
				if (in)
					disp_inset = in->display();
			}
			// If we have separators, this is not the last row of a
			// par, does not end in newline, and is not row above a
			// display inset... then stretch it
			if (ns
			    && row.endpos() < par.size()
			    && !par.isNewline(row.endpos() - 1)
			    && !disp_inset
				) {
				row.separator = w / ns;
				//lyxerr << "row.separator " << row.separator << endl;
				//lyxerr << "ns " << ns << endl;
			} else if (is_rtl) {
				row.x += w;
			}
			break;
		}
		case LYX_ALIGN_RIGHT:
			row.x += w;
			break;
		case LYX_ALIGN_CENTER:
			row.x += w / 2;
			break;
		}
	}

	if (is_rtl) {
		pos_type body_pos = par.beginOfBody();
		pos_type end = row.endpos();

		if (body_pos > 0
		    && (body_pos > end || !par.isLineSeparator(body_pos - 1)))
		{
			row.x += theFontMetrics(text_->labelFont(par)).
				width(layout.labelsep);
			if (body_pos <= end)
				row.x += row.label_hfill;
		}
	}

	pos_type const endpos = row.endpos();
	pos_type body_pos = par.beginOfBody();
	if (body_pos > 0
		&& (body_pos > endpos || !par.isLineSeparator(body_pos - 1)))
		body_pos = 0;

	ParagraphMetrics & pm = par_metrics_[pit];
	InsetList::const_iterator ii = par.insetList().begin();
	InsetList::const_iterator iend = par.insetList().end();
	for ( ; ii != iend; ++ii) {
		if (ii->pos >= endpos || ii->pos < row.pos()
			|| (ii->inset->lyxCode() != SPACE_CODE ||
			    !ii->inset->isStretchableSpace()))
			continue;
		Dimension dim = row.dimension();
		if (pm.hfillExpansion(row, ii->pos))
			dim.wid = int(ii->pos >= body_pos ?
				max(hfill, 5.0) : row.label_hfill);
		else
			dim.wid = 5;
		// Cache the inset dimension.
		bv_->coordCache().insets().add(ii->inset, dim);
		pm.setInsetDimension(ii->inset, dim);
	}
}


int TextMetrics::labelFill(pit_type const pit, Row const & row) const
{
	Paragraph const & par = text_->getPar(pit);

	pos_type last = par.beginOfBody();
	LASSERT(last > 0, /**/);

	// -1 because a label ends with a space that is in the label
	--last;

	// a separator at this end does not count
	if (par.isLineSeparator(last))
		--last;

	int w = 0;
	for (pos_type i = row.pos(); i <= last; ++i)
		w += singleWidth(pit, i);

	docstring const & label = par.params().labelWidthString();
	if (label.empty())
		return 0;

	FontMetrics const & fm
		= theFontMetrics(text_->labelFont(par));

	return max(0, fm.width(label) - w);
}


// this needs special handling - only newlines count as a break point
static pos_type addressBreakPoint(pos_type i, Paragraph const & par)
{
	pos_type const end = par.size();

	for (; i < end; ++i)
		if (par.isNewline(i))
			return i + 1;

	return end;
}


int TextMetrics::labelEnd(pit_type const pit) const
{
	// labelEnd is only needed if the layout fills a flushleft label.
	if (text_->getPar(pit).layout().margintype != MARGIN_MANUAL)
		return 0;
	// return the beginning of the body
	return leftMargin(max_width_, pit);
}

namespace {

/**
 * Calling Text::getFont is slow. While rebreaking we scan a
 * paragraph from left to right calling getFont for every char.  This
 * simple class address this problem by hidding an optimization trick
 * (not mine btw -AB): the font is reused in the whole font span.  The
 * class handles transparently the "hidden" (not part of the fontlist)
 * label font (as getFont does).
 **/
class FontIterator
{
public:
	///
	FontIterator(TextMetrics const & tm,
		Paragraph const & par, pit_type pit, pos_type pos)
		: tm_(tm), par_(par), pit_(pit), pos_(pos),
		font_(tm.displayFont(pit, pos)),
		endspan_(par.fontSpan(pos).last),
		bodypos_(par.beginOfBody())
	{}

	///
	Font const & operator*() const { return font_; }

	///
	FontIterator & operator++()
	{
		++pos_;
		if (pos_ > endspan_ || pos_ == bodypos_) {
			font_ = tm_.displayFont(pit_, pos_);
			endspan_ = par_.fontSpan(pos_).last;
		}
		return *this;
	}

	///
	Font * operator->() { return &font_; }

private:
	///
	TextMetrics const & tm_;
	///
	Paragraph const & par_;
	///
	pit_type pit_;
	///
	pos_type pos_;
	///
	Font font_;
	///
	pos_type endspan_;
	///
	pos_type bodypos_;
};

} // anon namespace

pos_type TextMetrics::rowBreakPoint(int width, pit_type const pit,
		pos_type pos) const
{
	ParagraphMetrics const & pm = par_metrics_[pit];
	Paragraph const & par = text_->getPar(pit);
	pos_type const end = par.size();
	if (pos == end || width < 0)
		return end;

	Layout const & layout = par.layout();

	if (layout.margintype == MARGIN_RIGHT_ADDRESS_BOX)
		return addressBreakPoint(pos, par);

	pos_type const body_pos = par.beginOfBody();

	// check for possible inline completion
	DocIterator const & inlineCompletionPos = bv_->inlineCompletionPos();
	pos_type inlineCompletionLPos = -1;
	if (inlineCompletionPos.inTexted()
	    && inlineCompletionPos.text() == text_
	    && inlineCompletionPos.pit() == pit) {
		// draw logically behind the previous character
		inlineCompletionLPos = inlineCompletionPos.pos() - 1;
	}

	// Now we iterate through until we reach the right margin
	// or the end of the par, then choose the possible break
	// nearest that.

	int label_end = labelEnd(pit);
	int const left = leftMargin(max_width_, pit, pos);
	int x = left;

	// pixel width since last breakpoint
	int chunkwidth = 0;

	docstring const s(1, char_type(0x00B6));
	Font f;
	int par_marker_width = theFontMetrics(f).width(s);

	FontIterator fi = FontIterator(*this, par, pit, pos);
	pos_type point = end;
	pos_type i = pos;

	ParagraphList const & pars_ = text_->paragraphs();
	bool const draw_par_end_marker = lyxrc.paragraph_markers
		&& size_type(pit + 1) < pars_.size();
				
	for ( ; i < end; ++i, ++fi) {
		int thiswidth = pm.singleWidth(i, *fi);
		
		if (draw_par_end_marker && i == end - 1)
			// enlarge the last character to hold the end-of-par marker
			thiswidth += par_marker_width;

		// add inline completion width
		if (inlineCompletionLPos == i) {
			docstring const & completion = bv_->inlineCompletion();
			if (completion.length() > 0)
				thiswidth += theFontMetrics(*fi).width(completion);
		}

		// add the auto-hfill from label end to the body
		if (body_pos && i == body_pos) {
			FontMetrics const & fm = theFontMetrics(
				text_->labelFont(par));
			int add = fm.width(layout.labelsep);
			if (par.isLineSeparator(i - 1))
				add -= singleWidth(pit, i - 1);

			add = max(add, label_end - x);
			thiswidth += add;
		}

		x += thiswidth;
		chunkwidth += thiswidth;

		// break before a character that will fall off
		// the right of the row
		if (x >= width) {
			// if no break before, break here
			if (point == end || chunkwidth >= width - left) {
				if (i > pos)
					point = i;
				else
					point = i + 1;
			}
			// exit on last registered breakpoint:
			break;
		}

		if (par.isNewline(i)) {
			point = i + 1;
			break;
		}
		Inset const * inset = 0;
		// Break before...
		if (i + 1 < end) {
			if ((inset = par.getInset(i + 1)) && inset->display()) {
				point = i + 1;
				break;
			}
			// ...and after.
			if ((inset = par.getInset(i)) && inset->display()) {
				point = i + 1;
				break;
			}
		}

		inset = par.getInset(i);
		if (!inset || inset->isChar()) {
			// some insets are line separators too
			if (par.isLineSeparator(i)) {
				// register breakpoint:
				point = i + 1;
				chunkwidth = 0;
			}
		}
	}

	// maybe found one, but the par is short enough.
	if (i == end && x < width)
		point = end;

	// manual labels cannot be broken in LaTeX. But we
	// want to make our on-screen rendering of footnotes
	// etc. still break
	if (body_pos && point < body_pos)
		point = body_pos;

	return point;
}


int TextMetrics::rowWidth(int right_margin, pit_type const pit,
		pos_type const first, pos_type const end) const
{
	// get the pure distance
	ParagraphMetrics const & pm = par_metrics_[pit];
	Paragraph const & par = text_->getPar(pit);
	int w = leftMargin(max_width_, pit, first);
	int label_end = labelEnd(pit);

	// check for possible inline completion
	DocIterator const & inlineCompletionPos = bv_->inlineCompletionPos();
	pos_type inlineCompletionLPos = -1;
	if (inlineCompletionPos.inTexted()
	    && inlineCompletionPos.text() == text_
	    && inlineCompletionPos.pit() == pit) {
		// draw logically behind the previous character
		inlineCompletionLPos = inlineCompletionPos.pos() - 1;
	}

	pos_type const body_pos = par.beginOfBody();
	pos_type i = first;

	if (i < end) {
		FontIterator fi = FontIterator(*this, par, pit, i);
		for ( ; i < end; ++i, ++fi) {
			if (body_pos > 0 && i == body_pos) {
				FontMetrics const & fm = theFontMetrics(
					text_->labelFont(par));
				w += fm.width(par.layout().labelsep);
				if (par.isLineSeparator(i - 1))
					w -= singleWidth(pit, i - 1);
				w = max(w, label_end);
			}
			
			// a line separator at the end of a line (but not at the end of a 
			// paragraph) will not be drawn and should therefore not count for
			// the row width.
			if (!par.isLineSeparator(i) || i != end - 1 || end == par.size())
				w += pm.singleWidth(i, *fi);

			// add inline completion width
			if (inlineCompletionLPos == i) {
				docstring const & completion = bv_->inlineCompletion();
				if (completion.length() > 0)
					w += theFontMetrics(*fi).width(completion);
			}
		}
	}

	// count the paragraph end marker.
	if (end == par.size() && lyxrc.paragraph_markers) {
		ParagraphList const & pars_ = text_->paragraphs();
		if (size_type(pit + 1) < pars_.size()) {
			// enlarge the last character to hold the
			// end-of-par marker
			docstring const s(1, char_type(0x00B6));
			Font f;
			w += theFontMetrics(f).width(s);
		}
	}

	if (body_pos > 0 && body_pos >= end) {
		FontMetrics const & fm = theFontMetrics(
			text_->labelFont(par));
		w += fm.width(par.layout().labelsep);
		if (end > 0 && par.isLineSeparator(end - 1))
			w -= singleWidth(pit, end - 1);
		w = max(w, label_end);
	}

	return w + right_margin;
}


Dimension TextMetrics::rowHeight(pit_type const pit, pos_type const first,
		pos_type const end, bool topBottomSpace) const
{
	Paragraph const & par = text_->getPar(pit);
	// get the maximum ascent and the maximum descent
	double layoutasc = 0;
	double layoutdesc = 0;
	double const dh = defaultRowHeight();

	// ok, let us initialize the maxasc and maxdesc value.
	// Only the fontsize count. The other properties
	// are taken from the layoutfont. Nicer on the screen :)
	Layout const & layout = par.layout();

	// as max get the first character of this row then it can
	// increase but not decrease the height. Just some point to
	// start with so we don't have to do the assignment below too
	// often.
	Buffer const & buffer = bv_->buffer();
	Font font = displayFont(pit, first);
	FontSize const tmpsize = font.fontInfo().size();
	font.fontInfo() = text_->layoutFont(pit);
	FontSize const size = font.fontInfo().size();
	font.fontInfo().setSize(tmpsize);

	FontInfo labelfont = text_->labelFont(par);

	FontMetrics const & labelfont_metrics = theFontMetrics(labelfont);
	FontMetrics const & fontmetrics = theFontMetrics(font);

	// these are minimum values
	double const spacing_val = layout.spacing.getValue()
		* text_->spacing(par);
	//lyxerr << "spacing_val = " << spacing_val << endl;
	int maxasc  = int(fontmetrics.maxAscent()  * spacing_val);
	int maxdesc = int(fontmetrics.maxDescent() * spacing_val);

	// insets may be taller
	ParagraphMetrics const & pm = par_metrics_[pit];
	InsetList::const_iterator ii = par.insetList().begin();
	InsetList::const_iterator iend = par.insetList().end();
	for ( ; ii != iend; ++ii) {
		if (ii->pos >= first && ii->pos < end) {
			Dimension const & dim = pm.insetDimension(ii->inset);
			maxasc  = max(maxasc,  dim.ascent());
			maxdesc = max(maxdesc, dim.descent());
		}
	}

	// Check if any custom fonts are larger (Asger)
	// This is not completely correct, but we can live with the small,
	// cosmetic error for now.
	int labeladdon = 0;

	FontSize maxsize =
		par.highestFontInRange(first, end, size);
	if (maxsize > font.fontInfo().size()) {
		// use standard paragraph font with the maximal size
		FontInfo maxfont = font.fontInfo();
		maxfont.setSize(maxsize);
		FontMetrics const & maxfontmetrics = theFontMetrics(maxfont);
		maxasc  = max(maxasc,  maxfontmetrics.maxAscent());
		maxdesc = max(maxdesc, maxfontmetrics.maxDescent());
	}

	// This is nicer with box insets:
	++maxasc;
	++maxdesc;

	ParagraphList const & pars = text_->paragraphs();
	Inset const & inset = text_->inset();

	// is it a top line?
	if (first == 0 && topBottomSpace) {
		BufferParams const & bufparams = buffer.params();
		// some parskips VERY EASY IMPLEMENTATION
		if (bufparams.paragraph_separation == BufferParams::ParagraphSkipSeparation
		    && !inset.getLayout().parbreakIsNewline()
		    && !par.layout().parbreak_is_newline
		    && pit > 0
		    && ((layout.isParagraph() && par.getDepth() == 0)
			|| (pars[pit - 1].layout().isParagraph()
			    && pars[pit - 1].getDepth() == 0))) {
			maxasc += bufparams.getDefSkip().inPixels(*bv_);
		}

		if (par.params().startOfAppendix())
			maxasc += int(3 * dh);

		// This is special code for the chapter, since the label of this
		// layout is printed in an extra row
		if (layout.counter == "chapter"
		    && !par.params().labelString().empty()) {
			labeladdon = int(labelfont_metrics.maxHeight()
				     * layout.spacing.getValue()
				     * text_->spacing(par));
		}

		// special code for the top label
		if ((layout.labeltype == LABEL_TOP_ENVIRONMENT
		     || layout.labeltype == LABEL_BIBLIO
		     || layout.labeltype == LABEL_CENTERED_TOP_ENVIRONMENT)
		    && text_->isFirstInSequence(pit)
		    && !par.labelString().empty())
		{
			labeladdon = int(
				  labelfont_metrics.maxHeight()
					* layout.spacing.getValue()
					* text_->spacing(par)
				+ (layout.topsep + layout.labelbottomsep) * dh);
		}

		// Add the layout spaces, for example before and after
		// a section, or between the items of a itemize or enumerate
		// environment.

		pit_type prev = text_->depthHook(pit, par.getDepth());
		Paragraph const & prevpar = pars[prev];
		if (prev != pit
		    && prevpar.layout() == layout
		    && prevpar.getDepth() == par.getDepth()
		    && prevpar.getLabelWidthString()
					== par.getLabelWidthString()) {
			layoutasc = layout.itemsep * dh;
		} else if (pit != 0 || first != 0) {
			if (layout.topsep > 0)
				layoutasc = layout.topsep * dh;
		}

		prev = text_->outerHook(pit);
		if (prev != pit_type(pars.size())) {
			maxasc += int(pars[prev].layout().parsep * dh);
		} else if (pit != 0) {
			Paragraph const & prevpar = pars[pit - 1];
			if (prevpar.getDepth() != 0 ||
					prevpar.layout() == layout) {
				maxasc += int(layout.parsep * dh);
			}
		}
	}

	// is it a bottom line?
	if (end >= par.size() && topBottomSpace) {
		// add the layout spaces, for example before and after
		// a section, or between the items of a itemize or enumerate
		// environment
		pit_type nextpit = pit + 1;
		if (nextpit != pit_type(pars.size())) {
			pit_type cpit = pit;
			double usual = 0;
			double unusual = 0;

			if (pars[cpit].getDepth() > pars[nextpit].getDepth()) {
				usual = pars[cpit].layout().bottomsep * dh;
				cpit = text_->depthHook(cpit, pars[nextpit].getDepth());
				if (pars[cpit].layout() != pars[nextpit].layout()
					|| pars[nextpit].getLabelWidthString() != pars[cpit].getLabelWidthString())
				{
					unusual = pars[cpit].layout().bottomsep * dh;
				}
				layoutdesc = max(unusual, usual);
			} else if (pars[cpit].getDepth() == pars[nextpit].getDepth()) {
				if (pars[cpit].layout() != pars[nextpit].layout()
					|| pars[nextpit].getLabelWidthString() != pars[cpit].getLabelWidthString())
					layoutdesc = int(pars[cpit].layout().bottomsep * dh);
			}
		}
	}

	// incalculate the layout spaces
	maxasc  += int(layoutasc  * 2 / (2 + pars[pit].getDepth()));
	maxdesc += int(layoutdesc * 2 / (2 + pars[pit].getDepth()));

	// FIXME: the correct way is to do the following is to move the
	// following code in another method specially tailored for the
	// main Text. The following test is thus bogus.
	// Top and bottom margin of the document (only at top-level)
	if (main_text_ && topBottomSpace) {
		if (pit == 0 && first == 0)
			maxasc += 20;
		if (pit + 1 == pit_type(pars.size()) &&
		    end == par.size() &&
				!(end > 0 && par.isNewline(end - 1)))
			maxdesc += 20;
	}

	return Dimension(0, maxasc + labeladdon, maxdesc);
}


// x is an absolute screen coord
// returns the column near the specified x-coordinate of the row
// x is set to the real beginning of this column
pos_type TextMetrics::getColumnNearX(pit_type const pit,
		Row const & row, int & x, bool & boundary) const
{
	Buffer const & buffer = bv_->buffer();

	/// For the main Text, it is possible that this pit is not
	/// yet in the CoordCache when moving cursor up.
	/// x Paragraph coordinate is always 0 for main text anyway.
	int const xo = origin_.x_;
	x -= xo;
	Paragraph const & par = text_->getPar(pit);
	Bidi bidi;
	bidi.computeTables(par, buffer, row);

	pos_type vc = row.pos();
	pos_type end = row.endpos();
	pos_type c = 0;
	Layout const & layout = par.layout();

	bool left_side = false;

	pos_type body_pos = par.beginOfBody();

	double tmpx = row.x;
	double last_tmpx = tmpx;

	if (body_pos > 0 &&
	    (body_pos > end || !par.isLineSeparator(body_pos - 1)))
		body_pos = 0;

	// check for empty row
	if (vc == end) {
		x = int(tmpx) + xo;
		return 0;
	}

	// This (rtl_support test) is not needed, but gives
	// some speedup if rtl_support == false
	bool const lastrow = lyxrc.rtl_support && row.endpos() == par.size();

	// If lastrow is false, we don't need to compute
	// the value of rtl.
	bool const rtl = lastrow ? text_->isRTL(par) : false;

	// if the first character is a separator, and we are in RTL
	// text, this character will not be painted on screen
	// and thus we should not count it and skip to the next.
	if (rtl && par.isSeparator(bidi.vis2log(vc)))
		++vc;

	while (vc < end && tmpx <= x) {
		c = bidi.vis2log(vc);
		last_tmpx = tmpx;
		if (body_pos > 0 && c == body_pos - 1) {
			FontMetrics const & fm = theFontMetrics(
				text_->labelFont(par));
			tmpx += row.label_hfill + fm.width(layout.labelsep);
			if (par.isLineSeparator(body_pos - 1))
				tmpx -= singleWidth(pit, body_pos - 1);
		}

		tmpx += singleWidth(pit, c);
		if (par.isSeparator(c) && c >= body_pos)
				tmpx += row.separator;
		++vc;
	}

	if ((tmpx + last_tmpx) / 2 > x) {
		tmpx = last_tmpx;
		left_side = true;
	}

	LASSERT(vc <= end, /**/);  // This shouldn't happen.

	boundary = false;

	if (lastrow &&
	    ((rtl  &&  left_side && vc == row.pos() && x < tmpx - 5) ||
	     (!rtl && !left_side && vc == end  && x > tmpx + 5))) {
		if (!par.isNewline(end - 1))
			c = end;
	} else if (vc == row.pos()) {
		c = bidi.vis2log(vc);
		if (bidi.level(c) % 2 == 1)
			++c;
	} else {
		c = bidi.vis2log(vc - 1);
		bool const rtl = (bidi.level(c) % 2 == 1);
		if (left_side == rtl) {
			++c;
			boundary = isRTLBoundary(pit, c);
		}
	}

// I believe this code is not needed anymore (Jug 20050717)
#if 0
	// The following code is necessary because the cursor position past
	// the last char in a row is logically equivalent to that before
	// the first char in the next row. That's why insets causing row
	// divisions -- Newline and display-style insets -- must be treated
	// specially, so cursor up/down doesn't get stuck in an air gap -- MV
	// Newline inset, air gap below:
	if (row.pos() < end && c >= end && par.isNewline(end - 1)) {
		if (bidi.level(end -1) % 2 == 0)
			tmpx -= singleWidth(pit, end - 1);
		else
			tmpx += singleWidth(pit, end - 1);
		c = end - 1;
	}

	// Air gap above display inset:
	if (row.pos() < end && c >= end && end < par.size()
	    && par.isInset(end) && par.getInset(end)->display()) {
		c = end - 1;
	}
	// Air gap below display inset:
	if (row.pos() < end && c >= end && par.isInset(end - 1)
	    && par.getInset(end - 1)->display()) {
		c = end - 1;
	}
#endif

	x = int(tmpx) + xo;
	pos_type const col = c - row.pos();

	if (!c || end == par.size())
		return col;

	if (c==end && !par.isLineSeparator(c-1) && !par.isNewline(c-1)) {
		boundary = true;
		return col;
	}

	return min(col, end - 1 - row.pos());
}


pos_type TextMetrics::x2pos(pit_type pit, int row, int x) const
{
	// We play safe and use parMetrics(pit) to make sure the
	// ParagraphMetrics will be redone and OK to use if needed.
	// Otherwise we would use an empty ParagraphMetrics in
	// upDownInText() while in selection mode.
	ParagraphMetrics const & pm = parMetrics(pit);

	LASSERT(row < int(pm.rows().size()), /**/);
	bool bound = false;
	Row const & r = pm.rows()[row];
	return r.pos() + getColumnNearX(pit, r, x, bound);
}


void TextMetrics::newParMetricsDown()
{
	pair<pit_type, ParagraphMetrics> const & last = *par_metrics_.rbegin();
	pit_type const pit = last.first + 1;
	if (pit == int(text_->paragraphs().size()))
		return;

	// do it and update its position.
	redoParagraph(pit);
	par_metrics_[pit].setPosition(last.second.position()
		+ last.second.descent() + par_metrics_[pit].ascent());
}


void TextMetrics::newParMetricsUp()
{
	pair<pit_type, ParagraphMetrics> const & first = *par_metrics_.begin();
	if (first.first == 0)
		return;

	pit_type const pit = first.first - 1;
	// do it and update its position.
	redoParagraph(pit);
	par_metrics_[pit].setPosition(first.second.position()
		- first.second.ascent() - par_metrics_[pit].descent());
}

// y is screen coordinate
pit_type TextMetrics::getPitNearY(int y)
{
	LASSERT(!text_->paragraphs().empty(), return -1);
	LASSERT(!par_metrics_.empty(), return -1);
	LYXERR(Debug::DEBUG, "y: " << y << " cache size: " << par_metrics_.size());

	// look for highest numbered paragraph with y coordinate less than given y
	pit_type pit = -1;
	int yy = -1;
	ParMetricsCache::const_iterator it = par_metrics_.begin();
	ParMetricsCache::const_iterator et = par_metrics_.end();
	ParMetricsCache::const_iterator last = et; last--;

	ParagraphMetrics const & pm = it->second;

	if (y < it->second.position() - int(pm.ascent())) {
		// We are looking for a position that is before the first paragraph in
		// the cache (which is in priciple off-screen, that is before the
		// visible part.
		if (it->first == 0)
			// We are already at the first paragraph in the inset.
			return 0;
		// OK, this is the paragraph we are looking for.
		pit = it->first - 1;
		newParMetricsUp();
		return pit;
	}

	ParagraphMetrics const & pm_last = par_metrics_[last->first];

	if (y >= last->second.position() + int(pm_last.descent())) {
		// We are looking for a position that is after the last paragraph in
		// the cache (which is in priciple off-screen), that is before the
		// visible part.
		pit = last->first + 1;
		if (pit == int(text_->paragraphs().size()))
			//  We are already at the last paragraph in the inset.
			return last->first;
		// OK, this is the paragraph we are looking for.
		newParMetricsDown();
		return pit;
	}

	for (; it != et; ++it) {
		LYXERR(Debug::DEBUG, "examining: pit: " << it->first
			<< " y: " << it->second.position());

		ParagraphMetrics const & pm = par_metrics_[it->first];

		if (it->first >= pit && int(it->second.position()) - int(pm.ascent()) <= y) {
			pit = it->first;
			yy = it->second.position();
		}
	}

	LYXERR(Debug::DEBUG, "found best y: " << yy << " for pit: " << pit);

	return pit;
}


Row const & TextMetrics::getPitAndRowNearY(int & y, pit_type & pit,
	bool assert_in_view, bool up)
{
	ParagraphMetrics const & pm = par_metrics_[pit];

	int yy = pm.position() - pm.ascent();
	LASSERT(!pm.rows().empty(), /**/);
	RowList::const_iterator rit = pm.rows().begin();
	RowList::const_iterator rlast = pm.rows().end();
	--rlast;
	for (; rit != rlast; yy += rit->height(), ++rit)
		if (yy + rit->height() > y)
			break;

	if (assert_in_view) {
		if (!up && yy + rit->height() > y) {
			if (rit != pm.rows().begin()) {
				y = yy;
				--rit;
			} else if (pit != 0) {
				--pit;
				newParMetricsUp();
				ParagraphMetrics const & pm2 = par_metrics_[pit];
				rit = pm2.rows().end();
				--rit;
				y = yy;
			}
		} else if (up && yy != y) {
			if (rit != rlast) {
				y = yy + rit->height();
				++rit;
			} else if (pit < int(text_->paragraphs().size()) - 1) {
				++pit;
				newParMetricsDown();
				ParagraphMetrics const & pm2 = par_metrics_[pit];
				rit = pm2.rows().begin();
				y = pm2.position();
			}
		}
	}
	return *rit;
}


// x,y are absolute screen coordinates
// sets cursor recursively descending into nested editable insets
Inset * TextMetrics::editXY(Cursor & cur, int x, int y,
	bool assert_in_view, bool up)
{
	if (lyxerr.debugging(Debug::WORKAREA)) {
		LYXERR0("TextMetrics::editXY(cur, " << x << ", " << y << ")");
		cur.bv().coordCache().dump();
	}
	pit_type pit = getPitNearY(y);
	LASSERT(pit != -1, return 0);
	
	int yy = y; // is modified by getPitAndRowNearY
	Row const & row = getPitAndRowNearY(yy, pit, assert_in_view, up);

	bool bound = false; // is modified by getColumnNearX
	int xx = x; // is modified by getColumnNearX
	pos_type const pos = row.pos()
		+ getColumnNearX(pit, row, xx, bound);
	cur.pit() = pit;
	cur.pos() = pos;
	cur.boundary(bound);
	cur.setTargetX(x);

	// try to descend into nested insets
	Inset * inset = checkInsetHit(x, yy);
	//lyxerr << "inset " << inset << " hit at x: " << x << " y: " << y << endl;
	if (!inset) {
		// Either we deconst editXY or better we move current_font
		// and real_current_font to Cursor
		// FIXME: what is needed now that current_font and real_current_font
		// are transferred?
		cur.setCurrentFont();
		return 0;
	}

	ParagraphList const & pars = text_->paragraphs();
	Inset const * inset_before = pos ? pars[pit].getInset(pos - 1) : 0;

	// This should be just before or just behind the
	// cursor position set above.
	LASSERT(inset == inset_before 
		|| inset == pars[pit].getInset(pos), return 0);

	// Make sure the cursor points to the position before
	// this inset.
	if (inset == inset_before) {
		--cur.pos();
		cur.boundary(false);
	}

	// Try to descend recursively inside the inset.
	inset = inset->editXY(cur, x, yy);

	if (cur.top().text() == text_)
		cur.setCurrentFont();
	return inset;
}


void TextMetrics::setCursorFromCoordinates(Cursor & cur, int const x, int const y)
{
	LASSERT(text_ == cur.text(), /**/);
	pit_type pit = getPitNearY(y);
	LASSERT(pit != -1, return);

	ParagraphMetrics const & pm = par_metrics_[pit];

	int yy = pm.position() - pm.ascent();
	LYXERR(Debug::DEBUG, "x: " << x << " y: " << y <<
		" pit: " << pit << " yy: " << yy);

	int r = 0;
	LASSERT(pm.rows().size(), /**/);
	for (; r < int(pm.rows().size()) - 1; ++r) {
		Row const & row = pm.rows()[r];
		if (int(yy + row.height()) > y)
			break;
		yy += row.height();
	}

	Row const & row = pm.rows()[r];

	LYXERR(Debug::DEBUG, "row " << r << " from pos: " << row.pos());

	bool bound = false;
	int xx = x;
	pos_type const pos = row.pos() + getColumnNearX(pit, row, xx, bound);

	LYXERR(Debug::DEBUG, "setting cursor pit: " << pit << " pos: " << pos);

	text_->setCursor(cur, pit, pos, true, bound);
	// remember new position.
	cur.setTargetX();
}


//takes screen x,y coordinates
Inset * TextMetrics::checkInsetHit(int x, int y)
{
	pit_type pit = getPitNearY(y);
	LASSERT(pit != -1, return 0);

	Paragraph const & par = text_->paragraphs()[pit];
	ParagraphMetrics const & pm = par_metrics_[pit];

	LYXERR(Debug::DEBUG, "x: " << x << " y: " << y << "  pit: " << pit);

	InsetList::const_iterator iit = par.insetList().begin();
	InsetList::const_iterator iend = par.insetList().end();
	for (; iit != iend; ++iit) {
		Inset * inset = iit->inset;

		LYXERR(Debug::DEBUG, "examining inset " << inset);

		if (!bv_->coordCache().getInsets().has(inset)) {
			LYXERR(Debug::DEBUG, "inset has no cached position");
			return 0;
		}

		Dimension const & dim = pm.insetDimension(inset);
		Point p = bv_->coordCache().getInsets().xy(inset);

		LYXERR(Debug::DEBUG, "xo: " << p.x_ << "..." << p.x_ + dim.wid
			<< " yo: " << p.y_ - dim.asc << "..." << p.y_ + dim.des);

		if (x >= p.x_
			&& x <= p.x_ + dim.wid
			&& y >= p.y_ - dim.asc
			&& y <= p.y_ + dim.des) {
			LYXERR(Debug::DEBUG, "Hit inset: " << inset);
			return inset;
		}
	}

	LYXERR(Debug::DEBUG, "No inset hit. ");
	return 0;
}


int TextMetrics::cursorX(CursorSlice const & sl,
		bool boundary) const
{
	LASSERT(sl.text() == text_, /**/);
	pit_type const pit = sl.pit();
	Paragraph const & par = text_->paragraphs()[pit];
	ParagraphMetrics const & pm = par_metrics_[pit];
	if (pm.rows().empty())
		return 0;

	pos_type ppos = sl.pos();
	// Correct position in front of big insets
	bool const boundary_correction = ppos != 0 && boundary;
	if (boundary_correction)
		--ppos;

	Row const & row = pm.getRow(sl.pos(), boundary);

	pos_type cursor_vpos = 0;

	Buffer const & buffer = bv_->buffer();
	double x = row.x;
	Bidi bidi;
	bidi.computeTables(par, buffer, row);

	pos_type const row_pos  = row.pos();
	pos_type const end      = row.endpos();
	// Spaces at logical line breaks in bidi text must be skipped during
	// cursor positioning. However, they may appear visually in the middle
	// of a row; they must be skipped, wherever they are...
	// * logically "abc_[HEBREW_\nHEBREW]"
	// * visually "abc_[_WERBEH\nWERBEH]"
	pos_type skipped_sep_vpos = -1;

	if (end <= row_pos)
		cursor_vpos = row_pos;
	else if (ppos >= end)
		cursor_vpos = text_->isRTL(par) ? row_pos : end;
	else if (ppos > row_pos && ppos >= end)
		//FIXME: this code is never reached!
		//       (see http://www.lyx.org/trac/changeset/8251)
		// Place cursor after char at (logical) position pos - 1
		cursor_vpos = (bidi.level(ppos - 1) % 2 == 0)
			? bidi.log2vis(ppos - 1) + 1 : bidi.log2vis(ppos - 1);
	else
		// Place cursor before char at (logical) position ppos
		cursor_vpos = (bidi.level(ppos) % 2 == 0)
			? bidi.log2vis(ppos) : bidi.log2vis(ppos) + 1;

	pos_type body_pos = par.beginOfBody();
	if (body_pos > 0 &&
	    (body_pos > end || !par.isLineSeparator(body_pos - 1)))
		body_pos = 0;

	// check for possible inline completion in this row
	DocIterator const & inlineCompletionPos = bv_->inlineCompletionPos();
	pos_type inlineCompletionVPos = -1;
	if (inlineCompletionPos.inTexted()
	    && inlineCompletionPos.text() == text_
	    && inlineCompletionPos.pit() == pit
	    && inlineCompletionPos.pos() - 1 >= row_pos
	    && inlineCompletionPos.pos() - 1 < end) {
		// draw logically behind the previous character
		inlineCompletionVPos = bidi.log2vis(inlineCompletionPos.pos() - 1);
	}

	// Use font span to speed things up, see below
	FontSpan font_span;
	Font font;

	// If the last logical character is a separator, skip it, unless
	// it's in the last row of a paragraph; see skipped_sep_vpos declaration
	if (end > 0 && end < par.size() && par.isSeparator(end - 1))
		skipped_sep_vpos = bidi.log2vis(end - 1);

	if (lyxrc.paragraph_markers && text_->isRTL(par)) {
		ParagraphList const & pars_ = text_->paragraphs();
		if (size_type(pit + 1) < pars_.size()) {
			FontInfo f;
			docstring const s = docstring(1, char_type(0x00B6));
			x += theFontMetrics(f).width(s);
		}
	}

	// Inline completion RTL special case row_pos == cursor_pos:
	// "__|b" => cursor_pos is right of __
	if (row_pos == inlineCompletionVPos && row_pos == cursor_vpos) {
		font = displayFont(pit, row_pos + 1);
		docstring const & completion = bv_->inlineCompletion();
		if (font.isRightToLeft() && completion.length() > 0)
			x += theFontMetrics(font.fontInfo()).width(completion);
	}

	for (pos_type vpos = row_pos; vpos < cursor_vpos; ++vpos) {
		// Skip the separator which is at the logical end of the row
		if (vpos == skipped_sep_vpos)
			continue;
		pos_type pos = bidi.vis2log(vpos);
		if (body_pos > 0 && pos == body_pos - 1) {
			FontMetrics const & labelfm = theFontMetrics(
				text_->labelFont(par));
			x += row.label_hfill + labelfm.width(par.layout().labelsep);
			if (par.isLineSeparator(body_pos - 1))
				x -= singleWidth(pit, body_pos - 1);
		}

		// Use font span to speed things up, see above
		if (pos < font_span.first || pos > font_span.last) {
			font_span = par.fontSpan(pos);
			font = displayFont(pit, pos);
		}

		x += pm.singleWidth(pos, font);

		// Inline completion RTL case:
		// "a__|b", __ of b => non-boundary a-pos is right of __
		if (vpos + 1 == inlineCompletionVPos
		    && (vpos + 1 < cursor_vpos || !boundary_correction)) {
			font = displayFont(pit, vpos + 1);
			docstring const & completion = bv_->inlineCompletion();
			if (font.isRightToLeft() && completion.length() > 0)
				x += theFontMetrics(font.fontInfo()).width(completion);
		}

		//  Inline completion LTR case:
		// "b|__a", __ of b => non-boundary a-pos is in front of __
		if (vpos == inlineCompletionVPos
		    && (vpos + 1 < cursor_vpos || boundary_correction)) {
			font = displayFont(pit, vpos);
			docstring const & completion = bv_->inlineCompletion();
			if (!font.isRightToLeft() && completion.length() > 0)
				x += theFontMetrics(font.fontInfo()).width(completion);
		}

		if (par.isSeparator(pos) && pos >= body_pos)
			x += row.separator;
	}

	// see correction above
	if (boundary_correction) {
		if (isRTL(sl, boundary))
			x -= singleWidth(pit, ppos);
		else
			x += singleWidth(pit, ppos);
	}

	return int(x);
}


int TextMetrics::cursorY(CursorSlice const & sl, bool boundary) const
{
	//lyxerr << "TextMetrics::cursorY: boundary: " << boundary << endl;
	ParagraphMetrics const & pm = par_metrics_[sl.pit()];
	if (pm.rows().empty())
		return 0;

	int h = 0;
	h -= par_metrics_[0].rows()[0].ascent();
	for (pit_type pit = 0; pit < sl.pit(); ++pit) {
		h += par_metrics_[pit].height();
	}
	int pos = sl.pos();
	if (pos && boundary)
		--pos;
	size_t const rend = pm.pos2row(pos);
	for (size_t rit = 0; rit != rend; ++rit)
		h += pm.rows()[rit].height();
	h += pm.rows()[rend].ascent();
	return h;
}


// the cursor set functions have a special mechanism. When they
// realize you left an empty paragraph, they will delete it.

bool TextMetrics::cursorHome(Cursor & cur)
{
	LASSERT(text_ == cur.text(), /**/);
	ParagraphMetrics const & pm = par_metrics_[cur.pit()];
	Row const & row = pm.getRow(cur.pos(),cur.boundary());
	return text_->setCursor(cur, cur.pit(), row.pos());
}


bool TextMetrics::cursorEnd(Cursor & cur)
{
	LASSERT(text_ == cur.text(), /**/);
	// if not on the last row of the par, put the cursor before
	// the final space exept if I have a spanning inset or one string
	// is so long that we force a break.
	pos_type end = cur.textRow().endpos();
	if (end == 0)
		// empty text, end-1 is no valid position
		return false;
	bool boundary = false;
	if (end != cur.lastpos()) {
		if (!cur.paragraph().isLineSeparator(end-1)
		    && !cur.paragraph().isNewline(end-1))
			boundary = true;
		else
			--end;
	}
	return text_->setCursor(cur, cur.pit(), end, true, boundary);
}


void TextMetrics::deleteLineForward(Cursor & cur)
{
	LASSERT(text_ == cur.text(), /**/);
	if (cur.lastpos() == 0) {
		// Paragraph is empty, so we just go forward
		text_->cursorForward(cur);
	} else {
		cur.resetAnchor();
		cur.setSelection(true); // to avoid deletion
		cursorEnd(cur);
		cur.setSelection();
		// What is this test for ??? (JMarc)
		if (!cur.selection())
			text_->deleteWordForward(cur);
		else
			cap::cutSelection(cur, true, false);
		cur.checkBufferStructure();
	}
}


bool TextMetrics::isLastRow(pit_type pit, Row const & row) const
{
	ParagraphList const & pars = text_->paragraphs();
	return row.endpos() >= pars[pit].size()
		&& pit + 1 == pit_type(pars.size());
}


bool TextMetrics::isFirstRow(pit_type pit, Row const & row) const
{
	return row.pos() == 0 && pit == 0;
}


int TextMetrics::leftMargin(int max_width, pit_type pit) const
{
	LASSERT(pit >= 0, /**/);
	LASSERT(pit < int(text_->paragraphs().size()), /**/);
	return leftMargin(max_width, pit, text_->paragraphs()[pit].size());
}


int TextMetrics::leftMargin(int max_width,
		pit_type const pit, pos_type const pos) const
{
	ParagraphList const & pars = text_->paragraphs();

	LASSERT(pit >= 0, /**/);
	LASSERT(pit < int(pars.size()), /**/);
	Paragraph const & par = pars[pit];
	LASSERT(pos >= 0, /**/);
	LASSERT(pos <= par.size(), /**/);
	Buffer const & buffer = bv_->buffer();
	//lyxerr << "TextMetrics::leftMargin: pit: " << pit << " pos: " << pos << endl;
	DocumentClass const & tclass = buffer.params().documentClass();
	Layout const & layout = par.layout();

	docstring parindent = layout.parindent;

	int l_margin = 0;

	if (text_->isMainText())
		l_margin += bv_->leftMargin();

	l_margin += theFontMetrics(buffer.params().getFont()).signedWidth(
		tclass.leftmargin());

	if (par.getDepth() != 0) {
		// find the next level paragraph
		pit_type newpar = text_->outerHook(pit);
		if (newpar != pit_type(pars.size())) {
			if (pars[newpar].layout().isEnvironment()) {
				l_margin = leftMargin(max_width, newpar);
			}
			if (tclass.isDefaultLayout(par.layout())
			    || tclass.isPlainLayout(par.layout())) {
				if (pars[newpar].params().noindent())
					parindent.erase();
				else
					parindent = pars[newpar].layout().parindent;
			}
		}
	}

	// This happens after sections in standard classes. The 1.3.x
	// code compared depths too, but it does not seem necessary
	// (JMarc)
	if (tclass.isDefaultLayout(par.layout())
	    && pit > 0 && pars[pit - 1].layout().nextnoindent)
		parindent.erase();

	FontInfo const labelfont = text_->labelFont(par);
	FontMetrics const & labelfont_metrics = theFontMetrics(labelfont);

	switch (layout.margintype) {
	case MARGIN_DYNAMIC:
		if (!layout.leftmargin.empty()) {
			l_margin += theFontMetrics(buffer.params().getFont()).signedWidth(
				layout.leftmargin);
		}
		if (!par.labelString().empty()) {
			l_margin += labelfont_metrics.signedWidth(layout.labelindent);
			l_margin += labelfont_metrics.width(par.labelString());
			l_margin += labelfont_metrics.width(layout.labelsep);
		}
		break;

	case MARGIN_MANUAL: {
		l_margin += labelfont_metrics.signedWidth(layout.labelindent);
		// The width of an empty par, even with manual label, should be 0
		if (!par.empty() && pos >= par.beginOfBody()) {
			if (!par.getLabelWidthString().empty()) {
				docstring labstr = par.getLabelWidthString();
				l_margin += labelfont_metrics.width(labstr);
				l_margin += labelfont_metrics.width(layout.labelsep);
			}
		}
		break;
	}

	case MARGIN_STATIC: {
		l_margin += theFontMetrics(buffer.params().getFont()).
			signedWidth(layout.leftmargin) * 4	/ (par.getDepth() + 4);
		break;
	}

	case MARGIN_FIRST_DYNAMIC:
		if (layout.labeltype == LABEL_MANUAL) {
			// if we are at position 0, we are never in the body
			if (pos > 0 && pos >= par.beginOfBody())
				l_margin += labelfont_metrics.signedWidth(layout.leftmargin);
			else
				l_margin += labelfont_metrics.signedWidth(layout.labelindent);
		} else if (pos != 0
			   // Special case to fix problems with
			   // theorems (JMarc)
			   || (layout.labeltype == LABEL_STATIC
			       && layout.latextype == LATEX_ENVIRONMENT
			       && !text_->isFirstInSequence(pit))) {
			l_margin += labelfont_metrics.signedWidth(layout.leftmargin);
		} else if (layout.labeltype != LABEL_TOP_ENVIRONMENT
			   && layout.labeltype != LABEL_BIBLIO
			   && layout.labeltype !=
			   LABEL_CENTERED_TOP_ENVIRONMENT) {
			l_margin += labelfont_metrics.signedWidth(layout.labelindent);
			l_margin += labelfont_metrics.width(layout.labelsep);
			l_margin += labelfont_metrics.width(par.labelString());
		}
		break;

	case MARGIN_RIGHT_ADDRESS_BOX: {
#if 0
		// ok, a terrible hack. The left margin depends on the widest
		// row in this paragraph.
		RowList::iterator rit = par.rows().begin();
		RowList::iterator end = par.rows().end();
		// FIXME: This is wrong.
		int minfill = max_width;
		for ( ; rit != end; ++rit)
			if (rit->fill() < minfill)
				minfill = rit->fill();
		l_margin += theFontMetrics(params.getFont()).signedWidth(layout.leftmargin);
		l_margin += minfill;
#endif
		// also wrong, but much shorter.
		l_margin += max_width / 2;
		break;
	}
	}

	if (!par.params().leftIndent().zero())
		l_margin += par.params().leftIndent().inPixels(max_width);

	LyXAlignment align;

	if (par.params().align() == LYX_ALIGN_LAYOUT)
		align = layout.align;
	else
		align = par.params().align();

	// set the correct parindent
	if (pos == 0
	    && (layout.labeltype == LABEL_NO_LABEL
	       || layout.labeltype == LABEL_TOP_ENVIRONMENT
	       || layout.labeltype == LABEL_CENTERED_TOP_ENVIRONMENT
	       || (layout.labeltype == LABEL_STATIC
	           && layout.latextype == LATEX_ENVIRONMENT
	           && !text_->isFirstInSequence(pit)))
	    && (align == LYX_ALIGN_BLOCK || align == LYX_ALIGN_LEFT)
	    && !par.params().noindent()
	    // in some insets, paragraphs are never indented
	    && !text_->inset().neverIndent()
	    // display style insets are always centered, omit indentation
	    && !(!par.empty()
		    && par.isInset(pos)
		    && par.getInset(pos)->display())
			&& (!(tclass.isDefaultLayout(par.layout())
	         || tclass.isPlainLayout(par.layout()))
	        || buffer.params().paragraph_separation 
				== BufferParams::ParagraphIndentSeparation)
	    )
		{
			// use the parindent of the layout when the default indentation is
			// used otherwise use the indentation set in the document settings
			if (buffer.params().getIndentation().asLyXCommand() == "default")
				l_margin += theFontMetrics(
					buffer.params().getFont()).signedWidth(parindent);
			else
				l_margin += buffer.params().getIndentation().inPixels(*bv_);
		}
	
	return l_margin;
}


int TextMetrics::singleWidth(pit_type pit, pos_type pos) const
{
	ParagraphMetrics const & pm = par_metrics_[pit];

	return pm.singleWidth(pos, displayFont(pit, pos));
}


void TextMetrics::draw(PainterInfo & pi, int x, int y) const
{
	if (par_metrics_.empty())
		return;

	origin_.x_ = x;
	origin_.y_ = y;

	ParMetricsCache::iterator it = par_metrics_.begin();
	ParMetricsCache::iterator const pm_end = par_metrics_.end();
	y -= it->second.ascent();
	for (; it != pm_end; ++it) {
		ParagraphMetrics const & pmi = it->second;
		y += pmi.ascent();
		pit_type const pit = it->first;
		// Save the paragraph position in the cache.
		it->second.setPosition(y);
		drawParagraph(pi, pit, x, y);
		y += pmi.descent();
	}
}


void TextMetrics::drawParagraph(PainterInfo & pi, pit_type pit, int x, int y) const
{
	BufferParams const & bparams = bv_->buffer().params();
	ParagraphMetrics const & pm = par_metrics_[pit];
	if (pm.rows().empty())
		return;

	Bidi bidi;
	bool const original_drawing_state = pi.pain.isDrawingEnabled();
	int const ww = bv_->workHeight();
	size_t const nrows = pm.rows().size();

	Cursor const & cur = bv_->cursor();
	DocIterator sel_beg = cur.selectionBegin();
	DocIterator sel_end = cur.selectionEnd();
	bool selection = cur.selection()
		// This is our text.
		&& cur.text() == text_
		// if the anchor is outside, this is not our selection
		&& cur.normalAnchor().text() == text_
		&& pit >= sel_beg.pit() && pit <= sel_end.pit();

	// We store the begin and end pos of the selection relative to this par
	DocIterator sel_beg_par = cur.selectionBegin();
	DocIterator sel_end_par = cur.selectionEnd();
	
	// We care only about visible selection.
	if (selection) {
		if (pit != sel_beg.pit()) {
			sel_beg_par.pit() = pit;
			sel_beg_par.pos() = 0;
		}
		if (pit != sel_end.pit()) {
			sel_end_par.pit() = pit;
			sel_end_par.pos() = sel_end_par.lastpos();
		}
	}

	for (size_t i = 0; i != nrows; ++i) {

		Row const & row = pm.rows()[i];
		if (i)
			y += row.ascent();

		bool const inside = (y + row.descent() >= 0
			&& y - row.ascent() < ww);
		// It is not needed to draw on screen if we are not inside.
		pi.pain.setDrawingEnabled(inside && original_drawing_state);
		RowPainter rp(pi, *text_, pit, row, bidi, x, y);

		if (selection)
			row.setSelectionAndMargins(sel_beg_par, sel_end_par);
		else
			row.setSelection(-1, -1);
		
		// The row knows nothing about the paragraph, so we have to check
		// whether this row is the first or last and update the margins.
		if (row.selection()) {
			if (row.sel_beg == 0)
				row.begin_margin_sel = sel_beg.pit() < pit;
			if (row.sel_end == sel_end_par.lastpos())
				row.end_margin_sel = sel_end.pit() > pit;
		}

		// Row signature; has row changed since last paint?
		row.setCrc(pm.computeRowSignature(row, bparams));
		bool row_has_changed = row.changed();

		// Take this opportunity to spellcheck the row contents.
		if (row_has_changed && lyxrc.spellcheck_continuously) {
			text_->getPar(pit).spellCheck();
		}

		// Don't paint the row if a full repaint has not been requested
		// and if it has not changed.
		if (!pi.full_repaint && !row_has_changed) {
			// Paint only the insets if the text itself is
			// unchanged.
			rp.paintOnlyInsets();
			y += row.descent();
			continue;
		}

		// Clear background of this row if paragraph background was not
		// already cleared because of a full repaint.
		if (!pi.full_repaint && row_has_changed) {
			pi.pain.fillRectangle(x, y - row.ascent(),
				width(), row.height(), pi.background_color);
		}
		
		// Instrumentation for testing row cache (see also
		// 12 lines lower):
		if (lyxerr.debugging(Debug::PAINTING) && inside
			&& (row.selection() || pi.full_repaint || row_has_changed)) {
				string const foreword = text_->isMainText() ?
					"main text redraw " : "inset text redraw: ";
			LYXERR(Debug::PAINTING, foreword << "pit=" << pit << " row=" << i
				<< " row_selection="	<< row.selection()
				<< " full_repaint="	<< pi.full_repaint
				<< " row_has_changed="	<< row_has_changed);
		}

		// Backup full_repaint status and force full repaint
		// for inner insets as the Row has been cleared out.
		bool tmp = pi.full_repaint;
		pi.full_repaint = true;

		rp.paintSelection();
		rp.paintAppendix();
		rp.paintDepthBar();
		rp.paintChangeBar();
		bool const is_rtl = text_->isRTL(text_->getPar(pit));
		if (i == 0 && !is_rtl)
			rp.paintFirst();
		if (i == nrows - 1 && is_rtl)
			rp.paintLast();
		rp.paintText();
		if (i == nrows - 1 && !is_rtl)
			rp.paintLast();
		if (i == 0 && is_rtl)
			rp.paintFirst();
		y += row.descent();

		// Restore full_repaint status.
		pi.full_repaint = tmp;
	}
	// Re-enable screen drawing for future use of the painter.
	pi.pain.setDrawingEnabled(original_drawing_state);

	//LYXERR(Debug::PAINTING, ".");
}


void TextMetrics::completionPosAndDim(Cursor const & cur, int & x, int & y,
	Dimension & dim) const
{
	Cursor const & bvcur = cur.bv().cursor();

	// get word in front of cursor
	docstring word = text_->previousWord(bvcur.top());
	DocIterator wordStart = bvcur;
	wordStart.pos() -= word.length();

	// get position on screen of the word start and end
	//FIXME: Is it necessary to explicitly set this to false?
	wordStart.boundary(false);
	Point lxy = cur.bv().getPos(wordStart);
	Point rxy = cur.bv().getPos(bvcur);

	// calculate dimensions of the word
	dim = rowHeight(bvcur.pit(), wordStart.pos(), bvcur.pos(), false);
	dim.wid = abs(rxy.x_ - lxy.x_);

	// calculate position of word
	y = lxy.y_;
	x = min(rxy.x_, lxy.x_);

	//lyxerr << "wid=" << dim.width() << " x=" << x << " y=" << y << " lxy.x_=" << lxy.x_ << " rxy.x_=" << rxy.x_ << " word=" << word << std::endl;
	//lyxerr << " wordstart=" << wordStart << " bvcur=" << bvcur << " cur=" << cur << std::endl;
}

//int TextMetrics::pos2x(pit_type pit, pos_type pos) const
//{
//	ParagraphMetrics const & pm = par_metrics_[pit];
//	Row const & r = pm.rows()[row];
//	int x = 0;
//	pos -= r.pos();
//}


int defaultRowHeight()
{
	return int(theFontMetrics(sane_font).maxHeight() *  1.2);
}

} // namespace lyx
