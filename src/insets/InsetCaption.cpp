/**
 * \file InsetCaption.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Lars Gullik Bjønnes
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "InsetCaption.h"
#include "InsetFloat.h"
#include "InsetWrap.h"

#include "Buffer.h"
#include "BufferParams.h"
#include "BufferView.h"
#include "Counters.h"
#include "Cursor.h"
#include "Dimension.h"
#include "Floating.h"
#include "FloatList.h"
#include "FuncRequest.h"
#include "FuncStatus.h"
#include "InsetList.h"
#include "Language.h"
#include "MetricsInfo.h"
#include "output_latex.h"
#include "output_xhtml.h"
#include "OutputParams.h"
#include "Paragraph.h"
#include "TextClass.h"
#include "TextMetrics.h"
#include "TocBackend.h"

#include "frontends/FontMetrics.h"
#include "frontends/Painter.h"

#include "support/gettext.h"
#include "support/lstrings.h"

#include <sstream>

using namespace std;
using namespace lyx::support;

namespace lyx {


InsetCaption::InsetCaption(Buffer * buf)
	: InsetText(buf, InsetText::PlainLayout)
{
	setAutoBreakRows(true);
	setDrawFrame(true);
	setFrameColor(Color_collapsableframe);
}


void InsetCaption::write(ostream & os) const
{
	os << "Caption\n";
	text().write(os);
}


docstring InsetCaption::layoutName() const
{
	if (type_.empty())
		return from_ascii("Caption");
	return from_utf8("Caption:" + type_);
}


void InsetCaption::cursorPos(BufferView const & bv,
		CursorSlice const & sl, bool boundary, int & x, int & y) const
{
	InsetText::cursorPos(bv, sl, boundary, x, y);
	x += labelwidth_;
}


void InsetCaption::setCustomLabel(docstring const & label)
{
	if (!isAscii(label) || label.empty())
		// This must be a user defined layout. We cannot translate
		// this, since gettext accepts only ascii keys.
		custom_label_ = label;
	else
		custom_label_ = _(to_ascii(label));
}


void InsetCaption::addToToc(DocIterator const & cpit) const
{
	if (type_.empty())
		return;

	DocIterator pit = cpit;
	pit.push_back(CursorSlice(const_cast<InsetCaption &>(*this)));

	Toc & toc = buffer().tocBackend().toc(type_);
	docstring str = full_label_ + ". ";
	text().forToc(str, TOC_ENTRY_LENGTH);
	toc.push_back(TocItem(pit, 0, str));

	// Proceed with the rest of the inset.
	InsetText::addToToc(cpit);
}


void InsetCaption::metrics(MetricsInfo & mi, Dimension & dim) const
{
	FontInfo tmpfont = mi.base.font;
	mi.base.font = mi.base.bv->buffer().params().getFont().fontInfo();
	labelwidth_ = theFontMetrics(mi.base.font).width(full_label_);
	// add some space to separate the label from the inset text
	labelwidth_ += 2 * TEXT_TO_INSET_OFFSET;
	dim.wid = labelwidth_;
	Dimension textdim;
	// Correct for button and label width
	mi.base.textwidth -= dim.wid;
	InsetText::metrics(mi, textdim);
	mi.base.font = tmpfont;
	mi.base.textwidth += dim.wid;
	dim.des = max(dim.des - textdim.asc + dim.asc, textdim.des);
	dim.asc = textdim.asc;
	dim.wid += textdim.wid;
}


void InsetCaption::drawBackground(PainterInfo & pi, int x, int y) const
{
	TextMetrics & tm = pi.base.bv->textMetrics(&text());
	int const h = tm.height() + 2 * TEXT_TO_INSET_OFFSET;
	int const yy = y - TEXT_TO_INSET_OFFSET - tm.ascent();
	pi.pain.fillRectangle(x, yy, labelwidth_, h, pi.backgroundColor(this));
}


void InsetCaption::draw(PainterInfo & pi, int x, int y) const
{
	// We must draw the label, we should get the label string
	// from the enclosing float inset.
	// The question is: Who should draw the label, the caption inset,
	// the text inset or the paragraph?
	// We should also draw the float number (Lgb)

	// Answer: the text inset (in buffer_funcs.cpp: setCaption).

	FontInfo tmpfont = pi.base.font;
	pi.base.font = pi.base.bv->buffer().params().getFont().fontInfo();
	pi.base.font.setColor(pi.textColor(pi.base.font.color()).baseColor);
	int const xx = x + TEXT_TO_INSET_OFFSET;
	pi.pain.text(xx, y, full_label_, pi.base.font);
	InsetText::draw(pi, x + labelwidth_, y);
	pi.base.font = tmpfont;
}


void InsetCaption::edit(Cursor & cur, bool front, EntryDirection entry_from)
{
	cur.push(*this);
	InsetText::edit(cur, front, entry_from);
}


Inset * InsetCaption::editXY(Cursor & cur, int x, int y)
{
	cur.push(*this);
	return InsetText::editXY(cur, x, y);
}


bool InsetCaption::insetAllowed(InsetCode code) const
{
	switch (code) {
	// code that is not allowed in a caption
	case CAPTION_CODE:
	case FLOAT_CODE:
	case FOOT_CODE:
	case NEWPAGE_CODE:
	case MARGIN_CODE:
	case MATHMACRO_CODE:
	case TABULAR_CODE:
	case WRAP_CODE:
		return false;
	default:
		return InsetText::insetAllowed(code);
	}
}


bool InsetCaption::getStatus(Cursor & cur, FuncRequest const & cmd,
	FuncStatus & status) const
{
	switch (cmd.action()) {

	case LFUN_BREAK_PARAGRAPH:
		status.setEnabled(false);
		return true;

	case LFUN_ARGUMENT_INSERT:
		status.setEnabled(cur.paragraph().insetList().find(ARG_CODE) == -1);
		return true;

	case LFUN_INSET_TOGGLE:
		// pass back to owner
		cur.undispatched();
		return false;

	default:
		return InsetText::getStatus(cur, cmd, status);
	}
}


void InsetCaption::latex(otexstream & os,
			 OutputParams const & runparams_in) const
{
	if (runparams_in.inFloat == OutputParams::SUBFLOAT)
		// caption is output as an optional argument
		return;
	// This is a bit too simplistic to take advantage of
	// caption options we must add more later. (Lgb)
	// This code is currently only able to handle the simple
	// \caption{...}, later we will make it take advantage
	// of the one of the caption packages. (Lgb)
	OutputParams runparams = runparams_in;
	// FIXME: actually, it is moving only when there is no
	// optional argument.
	runparams.moving_arg = true;
	os << "\\caption";
	latexArgInsets(paragraphs()[0], os, runparams, 0, 1);
	os << '{';
	InsetText::latex(os, runparams);
	os << "}\n";
	runparams_in.encoding = runparams.encoding;
}


int InsetCaption::plaintext(odocstream & os,
			    OutputParams const & runparams) const
{
	os << '[' << full_label_ << "\n";
	InsetText::plaintext(os, runparams);
	os << "\n]";

	return PLAINTEXT_NEWLINE + 1; // one char on a separate line
}


int InsetCaption::docbook(odocstream & os,
			  OutputParams const & runparams) const
{
	int ret;
	os << "<title>";
	ret = InsetText::docbook(os, runparams);
	os << "</title>\n";
	return ret;
}


docstring InsetCaption::xhtml(XHTMLStream & xs, OutputParams const & rp) const
{
	if (rp.html_disable_captions)
		return docstring();
	string attr = "class='float-caption";
	if (!type_.empty())
		attr += " float-caption-" + type_;
	attr += "'";
	xs << html::StartTag("div", attr);
	docstring def = getCaptionAsHTML(xs, rp);
	xs << html::EndTag("div");
	return def;
}


void InsetCaption::getArgument(otexstream & os,
			OutputParams const & runparams) const
{
	InsetText::latex(os, runparams);
}


void InsetCaption::getOptArg(otexstream & os,
			OutputParams const & runparams) const
{
	latexArgInsets(paragraphs()[0], os, runparams, 0, 1);
}


int InsetCaption::getCaptionAsPlaintext(odocstream & os,
			OutputParams const & runparams) const
{
	os << full_label_ << ' ';
	return InsetText::plaintext(os, runparams);
}


docstring InsetCaption::getCaptionAsHTML(XHTMLStream & xs,
			OutputParams const & runparams) const
{
	xs << full_label_ << ' ';
	InsetText::XHTMLOptions const opts = 
		InsetText::WriteLabel | InsetText::WriteInnerTag;
	return InsetText::insetAsXHTML(xs, runparams, opts);
}


void InsetCaption::updateBuffer(ParIterator const & it, UpdateType utype)
{
	Buffer const & master = *buffer().masterBuffer();
	DocumentClass const & tclass = master.params().documentClass();
	string const & lang = it.paragraph().getParLanguage(master.params())->code();
	Counters & cnts = tclass.counters();
	string const & type = cnts.current_float();
	if (utype == OutputUpdate) {
		// counters are local to the caption
		cnts.saveLastCounter();
	}
	// Memorize type for addToToc().
	type_ = type;
	if (type.empty())
		full_label_ = master.B_("Senseless!!! ");
	else {
		// FIXME: life would be _much_ simpler if listings was
		// listed in Floating.
		docstring name;
		if (type == "listing")
			name = master.B_("Listing");
		else
			name = master.B_(tclass.floats().getType(type).name());
		docstring counter = from_utf8(type);
		if (cnts.isSubfloat()) {
			counter = "sub-" + from_utf8(type);
			name = bformat(_("Sub-%1$s"),
				       master.B_(tclass.floats().getType(type).name()));
		}
		if (cnts.hasCounter(counter)) {
			cnts.step(counter, utype);
			full_label_ = bformat(from_ascii("%1$s %2$s:"), 
					      name,
					      cnts.theCounter(counter, lang));
		} else
			full_label_ = bformat(from_ascii("%1$s #:"), name);	
	}

	// Do the real work now.
	InsetText::updateBuffer(it, utype);
	if (utype == OutputUpdate)
		cnts.restoreLastCounter();
}


} // namespace lyx
