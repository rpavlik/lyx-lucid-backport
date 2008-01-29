/**
 * \file InsetMathNest.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "InsetMathNest.h"

#include "InsetMathArray.h"
#include "InsetMathBig.h"
#include "InsetMathBox.h"
#include "InsetMathBrace.h"
#include "InsetMathColor.h"
#include "InsetMathComment.h"
#include "InsetMathDelim.h"
#include "InsetMathHull.h"
//#include "InsetMathMBox.h"
#include "InsetMathRef.h"
#include "InsetMathScript.h"
#include "InsetMathSpace.h"
#include "InsetMathSymbol.h"
#include "InsetMathUnknown.h"
#include "MathData.h"
#include "MathFactory.h"
#include "MathMacroArgument.h"
#include "MathParser.h"
#include "MathStream.h"
#include "MathSupport.h"

#include "bufferview_funcs.h"
#include "BufferView.h"
#include "Color.h"
#include "CoordCache.h"
#include "Cursor.h"
#include "CutAndPaste.h"
#include "debug.h"
#include "DispatchResult.h"
#include "FuncRequest.h"
#include "FuncStatus.h"
#include "gettext.h"
#include "Text.h"
#include "OutputParams.h"
#include "Undo.h"

#include "support/lstrings.h"
#include "support/textutils.h"

#include "frontends/Clipboard.h"
#include "frontends/Painter.h"
#include "frontends/Selection.h"

#include "FuncRequest.h"

#include <sstream>


namespace lyx {

using cap::copySelection;
using cap::grabAndEraseSelection;
using cap::cutSelection;
using cap::replaceSelection;
using cap::selClearOrDel;

using std::endl;
using std::string;
using std::istringstream;


InsetMathNest::InsetMathNest(idx_type nargs)
	: cells_(nargs), lock_(false), mouse_hover_(false)
{}


InsetMathNest::InsetMathNest(InsetMathNest const & inset)
	: InsetMath(inset), cells_(inset.cells_), lock_(inset.lock_),
	  mouse_hover_(false)
{}


InsetMathNest & InsetMathNest::operator=(InsetMathNest const & inset)
{
	cells_ = inset.cells_;
	lock_ = inset.lock_;
	mouse_hover_ = false;
	InsetMath::operator=(inset);
	return *this;
}


InsetMath::idx_type InsetMathNest::nargs() const
{
	return cells_.size();
}


void InsetMathNest::cursorPos(BufferView const & bv,
		CursorSlice const & sl, bool /*boundary*/,
		int & x, int & y) const
{
// FIXME: This is a hack. Ideally, the coord cache should not store
// absolute positions, but relative ones. This would mean to call
// setXY() not in MathData::draw(), but in the parent insets' draw()
// with the correctly adjusted x,y values. But this means that we'd have
// to touch all (math)inset's draw() methods. Right now, we'll store
// absolute value, and make them here relative, only to make them
// absolute again when actually drawing the cursor. What a mess.
	BOOST_ASSERT(ptr_cmp(&sl.inset(), this));
	MathData const & ar = sl.cell();
	CoordCache const & coord_cache = bv.coordCache();
	if (!coord_cache.getArrays().has(&ar)) {
		// this can (semi-)legally happen if we just created this cell
		// and it never has been drawn before. So don't ASSERT.
		//lyxerr << "no cached data for array " << &ar << endl;
		x = 0;
		y = 0;
		return;
	}
	Point const pt = coord_cache.getArrays().xy(&ar);
	if (!coord_cache.getInsets().has(this)) {
		// same as above
		//lyxerr << "no cached data for inset " << this << endl;
		x = 0;
		y = 0;
		return;
	}
	Point const pt2 = coord_cache.getInsets().xy(this);
	//lyxerr << "retrieving position cache for MathData "
	//	<< pt.x_ << ' ' << pt.y_ << std::endl;
	x = pt.x_ - pt2.x_ + ar.pos2x(sl.pos());
	y = pt.y_ - pt2.y_;
//	lyxerr << "pt.y_ : " << pt.y_ << " pt2_.y_ : " << pt2.y_
//		<< " asc: " << ascent() << "  des: " << descent()
//		<< " ar.asc: " << ar.ascent() << " ar.des: " << ar.descent() << endl;
	// move cursor visually into empty cells ("blue rectangles");
	if (ar.empty())
		x += 2;
}


void InsetMathNest::metrics(MetricsInfo const & mi) const
{
	MetricsInfo m = mi;
	for (idx_type i = 0, n = nargs(); i != n; ++i)
		cell(i).metrics(m);
}


bool InsetMathNest::idxNext(Cursor & cur) const
{
	BOOST_ASSERT(ptr_cmp(&cur.inset(), this));
	if (cur.idx() == cur.lastidx())
		return false;
	++cur.idx();
	cur.pos() = 0;
	return true;
}


bool InsetMathNest::idxRight(Cursor & cur) const
{
	return idxNext(cur);
}


bool InsetMathNest::idxPrev(Cursor & cur) const
{
	BOOST_ASSERT(ptr_cmp(&cur.inset(), this));
	if (cur.idx() == 0)
		return false;
	--cur.idx();
	cur.pos() = cur.lastpos();
	return true;
}


bool InsetMathNest::idxLeft(Cursor & cur) const
{
	return idxPrev(cur);
}


bool InsetMathNest::idxFirst(Cursor & cur) const
{
	BOOST_ASSERT(ptr_cmp(&cur.inset(), this));
	if (nargs() == 0)
		return false;
	cur.idx() = 0;
	cur.pos() = 0;
	return true;
}


bool InsetMathNest::idxLast(Cursor & cur) const
{
	BOOST_ASSERT(ptr_cmp(&cur.inset(), this));
	if (nargs() == 0)
		return false;
	cur.idx() = cur.lastidx();
	cur.pos() = cur.lastpos();
	return true;
}


void InsetMathNest::dump() const
{
	odocstringstream oss;
	WriteStream os(oss);
	os << "---------------------------------------------\n";
	write(os);
	os << "\n";
	for (idx_type i = 0, n = nargs(); i != n; ++i)
		os << cell(i) << "\n";
	os << "---------------------------------------------\n";
	lyxerr << to_utf8(oss.str());
}


void InsetMathNest::draw(PainterInfo & pi, int x, int y) const
{
#if 0
	if (lock_)
		pi.pain.fillRectangle(x, y - ascent(), width(), height(),
					Color::mathlockbg);
#endif
	setPosCache(pi, x, y);
}


void InsetMathNest::drawSelection(PainterInfo & pi, int x, int y) const
{
	BufferView & bv = *pi.base.bv;
	// this should use the x/y values given, not the cached values
	Cursor & cur = bv.cursor();
	if (!cur.selection())
		return;
	if (!ptr_cmp(&cur.inset(), this))
		return;

	// FIXME: hack to get position cache warm
	pi.pain.setDrawingEnabled(false);
	draw(pi, x, y);
	pi.pain.setDrawingEnabled(true);

	CursorSlice s1 = cur.selBegin();
	CursorSlice s2 = cur.selEnd();

	//lyxerr << "InsetMathNest::drawing selection: "
	//	<< " s1: " << s1 << " s2: " << s2 << endl;
	if (s1.idx() == s2.idx()) {
		MathData const & c = cell(s1.idx());
		int x1 = c.xo(bv) + c.pos2x(s1.pos());
		int y1 = c.yo(bv) - c.ascent();
		int x2 = c.xo(bv) + c.pos2x(s2.pos());
		int y2 = c.yo(bv) + c.descent();
		pi.pain.fillRectangle(x1, y1, x2 - x1, y2 - y1, Color::selection);
	//lyxerr << "InsetMathNest::drawing selection 3: "
	//	<< " x1: " << x1 << " x2: " << x2
	//	<< " y1: " << y1 << " y2: " << y2 << endl;
	} else {
		for (idx_type i = 0; i < nargs(); ++i) {
			if (idxBetween(i, s1.idx(), s2.idx())) {
				MathData const & c = cell(i);
				int x1 = c.xo(bv);
				int y1 = c.yo(bv) - c.ascent();
				int x2 = c.xo(bv) + c.width();
				int y2 = c.yo(bv) + c.descent();
				pi.pain.fillRectangle(x1, y1, x2 - x1, y2 - y1, Color::selection);
			}
		}
	}
}


void InsetMathNest::validate(LaTeXFeatures & features) const
{
	for (idx_type i = 0; i < nargs(); ++i)
		cell(i).validate(features);
}


void InsetMathNest::replace(ReplaceData & rep)
{
	for (idx_type i = 0; i < nargs(); ++i)
		cell(i).replace(rep);
}


bool InsetMathNest::contains(MathData const & ar) const
{
	for (idx_type i = 0; i < nargs(); ++i)
		if (cell(i).contains(ar))
			return true;
	return false;
}


bool InsetMathNest::lock() const
{
	return lock_;
}


void InsetMathNest::lock(bool l)
{
	lock_ = l;
}


bool InsetMathNest::isActive() const
{
	return nargs() > 0;
}


MathData InsetMathNest::glue() const
{
	MathData ar;
	for (size_t i = 0; i < nargs(); ++i)
		ar.append(cell(i));
	return ar;
}


void InsetMathNest::write(WriteStream & os) const
{
	os << '\\' << name().c_str();
	for (size_t i = 0; i < nargs(); ++i)
		os << '{' << cell(i) << '}';
	if (nargs() == 0)
		os.pendingSpace(true);
	if (lock_ && !os.latex()) {
		os << "\\lyxlock";
		os.pendingSpace(true);
	}
}


void InsetMathNest::normalize(NormalStream & os) const
{
	os << '[' << name().c_str();
	for (size_t i = 0; i < nargs(); ++i)
		os << ' ' << cell(i);
	os << ']';
}


int InsetMathNest::latex(Buffer const &, odocstream & os,
			OutputParams const & runparams) const
{
	WriteStream wi(os, runparams.moving_arg, true);
	write(wi);
	return wi.line();
}


bool InsetMathNest::setMouseHover(bool mouse_hover)
{
	mouse_hover_ = mouse_hover;
	return true;
}


bool InsetMathNest::notifyCursorLeaves(Cursor & /*cur*/)
{
#ifdef WITH_WARNINGS
#warning look here
#endif
#if 0
	MathData & ar = cur.cell();
	// remove base-only "scripts"
	for (pos_type i = 0; i + 1 < ar.size(); ++i) {
		InsetMathScript * p = operator[](i).nucleus()->asScriptInset();
		if (p && p->nargs() == 1) {
			MathData ar = p->nuc();
			erase(i);
			insert(i, ar);
			cur.adjust(i, ar.size() - 1);
		}
	}

	// glue adjacent font insets of the same kind
	for (pos_type i = 0; i + 1 < size(); ++i) {
		InsetMathFont * p = operator[](i).nucleus()->asFontInset();
		InsetMathFont const * q = operator[](i + 1)->asFontInset();
		if (p && q && p->name() == q->name()) {
			p->cell(0).append(q->cell(0));
			erase(i + 1);
			cur.adjust(i, -1);
		}
	}
#endif
	return false;
}


void InsetMathNest::handleFont
	(Cursor & cur, docstring const & arg, char const * const font)
{
	handleFont(cur, arg, from_ascii(font));
}


void InsetMathNest::handleFont
	(Cursor & cur, docstring const & arg, docstring const & font)
{
	// this whole function is a hack and won't work for incremental font
	// changes...

	if (cur.inset().asInsetMath()->name() == font) {
		recordUndoInset(cur, Undo::ATOMIC);
		cur.handleFont(to_utf8(font));
	} else {
		recordUndo(cur, Undo::ATOMIC);
		cur.handleNest(createInsetMath(font));
		cur.insert(arg);
	}
}


void InsetMathNest::handleFont2(Cursor & cur, docstring const & arg)
{
	recordUndo(cur, Undo::ATOMIC);
	Font font;
	bool b;
	bv_funcs::string2font(to_utf8(arg), font, b);
	if (font.color() != Color::inherit) {
		MathAtom at = MathAtom(new InsetMathColor(true, font.color()));
		cur.handleNest(at, 0);
	}
}


void InsetMathNest::doDispatch(Cursor & cur, FuncRequest & cmd)
{
	//lyxerr << "InsetMathNest: request: " << cmd << std::endl;
	//CursorSlice sl = cur.current();

	switch (cmd.action) {

	case LFUN_PASTE: {
		recordUndo(cur);
		cur.message(_("Paste"));
		replaceSelection(cur);
		docstring topaste;
		if (cmd.argument().empty() && !theClipboard().isInternal())
			topaste = theClipboard().getAsText();
		else {
			size_t n = 0;
			idocstringstream is(cmd.argument());
			is >> n;
			topaste = cap::getSelection(cur.buffer(), n);
		}
		cur.niceInsert(topaste);
		cur.clearSelection(); // bug 393
		finishUndo();
		break;
	}

	case LFUN_CUT:
		recordUndo(cur);
		cutSelection(cur, true, true);
		cur.message(_("Cut"));
		// Prevent stale position >= size crash
		// Probably not necessary anymore, see eraseSelection (gb 2005-10-09)
		cur.normalize();
		break;

	case LFUN_COPY:
		copySelection(cur);
		cur.message(_("Copy"));
		break;

	case LFUN_MOUSE_PRESS:
		lfunMousePress(cur, cmd);
		break;

	case LFUN_MOUSE_MOTION:
		lfunMouseMotion(cur, cmd);
		break;

	case LFUN_MOUSE_RELEASE:
		lfunMouseRelease(cur, cmd);
		break;

	case LFUN_FINISHED_LEFT:
		cur.bv().cursor() = cur;
		break;

	case LFUN_FINISHED_RIGHT:
		++cur.pos();
		cur.bv().cursor() = cur;
		break;

	case LFUN_CHAR_FORWARD:
		cur.updateFlags(Update::Decoration | Update::FitCursor);
	case LFUN_CHAR_FORWARD_SELECT:
		cur.selHandle(cmd.action == LFUN_CHAR_FORWARD_SELECT);
		cur.autocorrect() = false;
		cur.clearTargetX();
		cur.macroModeClose();
		if (reverseDirectionNeeded(cur))
			goto goto_char_backwards;

goto_char_forwards:
		if (cur.pos() != cur.lastpos() && cur.openable(cur.nextAtom())) {
			cur.pushLeft(*cur.nextAtom().nucleus());
			cur.inset().idxFirst(cur);
		} else if (cur.posRight() || idxRight(cur)
			|| cur.popRight() || cur.selection())
			;
		else {
			cmd = FuncRequest(LFUN_FINISHED_RIGHT);
			cur.undispatched();
		}
		break;

	case LFUN_CHAR_BACKWARD:
		cur.updateFlags(Update::Decoration | Update::FitCursor);
	case LFUN_CHAR_BACKWARD_SELECT:
		cur.selHandle(cmd.action == LFUN_CHAR_BACKWARD_SELECT);
		cur.autocorrect() = false;
		cur.clearTargetX();
		cur.macroModeClose();
		if (reverseDirectionNeeded(cur))
			goto goto_char_forwards;

goto_char_backwards:
		if (cur.pos() != 0 && cur.openable(cur.prevAtom())) {
			cur.posLeft();
			cur.push(*cur.nextAtom().nucleus());
			cur.inset().idxLast(cur);
		} else if (cur.posLeft() || idxLeft(cur)
			|| cur.popLeft() || cur.selection())
			;
		else {
			cmd = FuncRequest(LFUN_FINISHED_LEFT);
			cur.undispatched();
		}
		break;

	case LFUN_DOWN:
	case LFUN_UP:
		cur.updateFlags(Update::Decoration | Update::FitCursor);
	case LFUN_DOWN_SELECT: 
	case LFUN_UP_SELECT: {
		// close active macro
		if (cur.inMacroMode()) {
			cur.macroModeClose();
			break;
		}
		
		// stop/start the selection
		bool select = cmd.action == LFUN_DOWN_SELECT ||
			cmd.action == LFUN_UP_SELECT;
		cur.selHandle(select);
		
		// go up/down
		bool up = cmd.action == LFUN_UP || cmd.action == LFUN_UP_SELECT;
		bool successful = cur.upDownInMath(up);
		if (successful) {
			// notify left insets and give them chance to set update flags
			lyx::notifyCursorLeaves(cur.beforeDispatchCursor(), cur);
			cur.fixIfBroken();
			break;
		}
		
		if (cur.fixIfBroken())
			// FIXME: Something bad happened. We pass the corrected Cursor
			// instead of letting things go worse.
			break;

		// We did not manage to move the cursor.
		cur.undispatched();
		break;
	}

	case LFUN_MOUSE_DOUBLE:
	case LFUN_MOUSE_TRIPLE:
	case LFUN_WORD_SELECT:
		cur.pos() = 0;
		cur.idx() = 0;
		cur.resetAnchor();
		cur.selection() = true;
		cur.pos() = cur.lastpos();
		cur.idx() = cur.lastidx();
		break;

	case LFUN_PARAGRAPH_UP:
	case LFUN_PARAGRAPH_DOWN:
		cur.updateFlags(Update::Decoration | Update::FitCursor);
	case LFUN_PARAGRAPH_UP_SELECT:
	case LFUN_PARAGRAPH_DOWN_SELECT:
		break;

	case LFUN_LINE_BEGIN:
	case LFUN_WORD_BACKWARD:
		cur.updateFlags(Update::Decoration | Update::FitCursor);
	case LFUN_LINE_BEGIN_SELECT:
	case LFUN_WORD_BACKWARD_SELECT:
		cur.selHandle(cmd.action == LFUN_WORD_BACKWARD_SELECT ||
				cmd.action == LFUN_LINE_BEGIN_SELECT);
		cur.macroModeClose();
		if (cur.pos() != 0) {
			cur.pos() = 0;
		} else if (cur.col() != 0) {
			cur.idx() -= cur.col();
			cur.pos() = 0;
		} else if (cur.idx() != 0) {
			cur.idx() = 0;
			cur.pos() = 0;
		} else {
			cmd = FuncRequest(LFUN_FINISHED_LEFT);
			cur.undispatched();
		}
		break;

	case LFUN_WORD_FORWARD:
	case LFUN_LINE_END:
		cur.updateFlags(Update::Decoration | Update::FitCursor);
	case LFUN_WORD_FORWARD_SELECT:
	case LFUN_LINE_END_SELECT:
		cur.selHandle(cmd.action == LFUN_WORD_FORWARD_SELECT ||
				cmd.action == LFUN_LINE_END_SELECT);
		cur.macroModeClose();
		cur.clearTargetX();
		if (cur.pos() != cur.lastpos()) {
			cur.pos() = cur.lastpos();
		} else if (ncols() && (cur.col() != cur.lastcol())) {
			cur.idx() = cur.idx() - cur.col() + cur.lastcol();
			cur.pos() = cur.lastpos();
		} else if (cur.idx() != cur.lastidx()) {
			cur.idx() = cur.lastidx();
			cur.pos() = cur.lastpos();
		} else {
			cmd = FuncRequest(LFUN_FINISHED_RIGHT);
			cur.undispatched();
		}
		break;

	case LFUN_SCREEN_UP_SELECT:
	case LFUN_SCREEN_UP:
		cmd = FuncRequest(LFUN_FINISHED_LEFT);
		cur.undispatched();
		break;

	case LFUN_SCREEN_DOWN_SELECT:
	case LFUN_SCREEN_DOWN:
		cmd = FuncRequest(LFUN_FINISHED_RIGHT);
		cur.undispatched();
		break;

	case LFUN_CELL_FORWARD:
		cur.updateFlags(Update::Decoration | Update::FitCursor);
		cur.inset().idxNext(cur);
		break;

	case LFUN_CELL_BACKWARD:
		cur.updateFlags(Update::Decoration | Update::FitCursor);
		cur.inset().idxPrev(cur);
		break;

	case LFUN_WORD_DELETE_BACKWARD:
	case LFUN_CHAR_DELETE_BACKWARD:
		if (cur.pos() == 0)
			// May affect external cell:
			recordUndoInset(cur, Undo::ATOMIC);
		else
			recordUndo(cur, Undo::ATOMIC);
		// if the inset can not be removed from within, delete it
		if (!cur.backspace()) {
			FuncRequest cmd = FuncRequest(LFUN_CHAR_DELETE_FORWARD);
			cur.innerText()->dispatch(cur, cmd);
		}
		break;

	case LFUN_WORD_DELETE_FORWARD:
	case LFUN_CHAR_DELETE_FORWARD:
		if (cur.pos() == cur.lastpos())
			// May affect external cell:
			recordUndoInset(cur, Undo::ATOMIC);
		else
			recordUndo(cur, Undo::ATOMIC);
		// if the inset can not be removed from within, delete it
		if (!cur.erase()) {
			FuncRequest cmd = FuncRequest(LFUN_CHAR_DELETE_FORWARD);
			cur.innerText()->dispatch(cur, cmd);
		}
		break;

	case LFUN_ESCAPE:
		if (cur.selection())
			cur.clearSelection();
		else  {
			cmd = FuncRequest(LFUN_FINISHED_RIGHT);
			cur.undispatched();
		}
		break;

	case LFUN_INSET_TOGGLE:
		recordUndo(cur);
		lock(!lock());
		cur.popRight();
		break;

	case LFUN_SELF_INSERT:
		if (cmd.argument().size() != 1) {
			recordUndo(cur);
			docstring const arg = cmd.argument();
			if (!interpretString(cur, arg))
				cur.insert(arg);
			break;
		}
		// Don't record undo steps if we are in macro mode and
		// cmd.argument is the next character of the macro name.
		// Otherwise we'll get an invalid cursor if we undo after
		// the macro was finished and the macro is a known command,
		// e.g. sqrt. Cursor::macroModeClose replaces in this case
		// the InsetMathUnknown with name "frac" by an empty
		// InsetMathFrac -> a pos value > 0 is invalid.
		// A side effect is that an undo before the macro is finished
		// undoes the complete macro, not only the last character.
		if (!cur.inMacroMode())
			recordUndo(cur);

		// spacial handling of space. If we insert an inset
		// via macro mode, we want to put the cursor inside it
		// if relevant. Think typing "\frac<space>".
		if (cmd.argument()[0] == ' '
		    && cur.inMacroMode() && cur.macroName() != "\\"
		    && cur.macroModeClose()) {
			MathAtom const atom = cur.prevAtom();
			if (atom->asNestInset() && atom->isActive()) {
				cur.posLeft();
				cur.pushLeft(*cur.nextInset());
			}
		} else if (!interpretChar(cur, cmd.argument()[0])) {
			cmd = FuncRequest(LFUN_FINISHED_RIGHT);
			cur.undispatched();
		}
		break;

	//case LFUN_SERVER_GET_XY:
	//	sprintf(dispatch_buffer, "%d %d",);
	//	break;

	case LFUN_SERVER_SET_XY: {
		lyxerr << "LFUN_SERVER_SET_XY broken!" << endl;
		int x = 0;
		int y = 0;
		istringstream is(to_utf8(cmd.argument()));
		is >> x >> y;
		cur.setScreenPos(x, y);
		break;
	}

	// Special casing for superscript in case of LyX handling
	// dead-keys:
	case LFUN_ACCENT_CIRCUMFLEX:
		if (cmd.argument().empty()) {
			// do superscript if LyX handles
			// deadkeys
			recordUndo(cur, Undo::ATOMIC);
			script(cur, true, grabAndEraseSelection(cur));
		}
		break;

	case LFUN_ACCENT_UMLAUT:
	case LFUN_ACCENT_ACUTE:
	case LFUN_ACCENT_GRAVE:
	case LFUN_ACCENT_BREVE:
	case LFUN_ACCENT_DOT:
	case LFUN_ACCENT_MACRON:
	case LFUN_ACCENT_CARON:
	case LFUN_ACCENT_TILDE:
	case LFUN_ACCENT_CEDILLA:
	case LFUN_ACCENT_CIRCLE:
	case LFUN_ACCENT_UNDERDOT:
	case LFUN_ACCENT_TIE:
	case LFUN_ACCENT_OGONEK:
	case LFUN_ACCENT_HUNGARIAN_UMLAUT:
		break;

	//  Math fonts
	case LFUN_FONT_FREE_APPLY:
	case LFUN_FONT_FREE_UPDATE:
		handleFont2(cur, cmd.argument());
		break;

	case LFUN_FONT_BOLD:
		if (currentMode() == TEXT_MODE)
			handleFont(cur, cmd.argument(), "textbf");
		else
			handleFont(cur, cmd.argument(), "mathbf");
		break;
	case LFUN_FONT_SANS:
		if (currentMode() == TEXT_MODE)
			handleFont(cur, cmd.argument(), "textsf");
		else
			handleFont(cur, cmd.argument(), "mathsf");
		break;
	case LFUN_FONT_EMPH:
		if (currentMode() == TEXT_MODE)
			handleFont(cur, cmd.argument(), "emph");
		else
			handleFont(cur, cmd.argument(), "mathcal");
		break;
	case LFUN_FONT_ROMAN:
		if (currentMode() == TEXT_MODE)
			handleFont(cur, cmd.argument(), "textrm");
		else
			handleFont(cur, cmd.argument(), "mathrm");
		break;
	case LFUN_FONT_CODE:
		if (currentMode() == TEXT_MODE)
			handleFont(cur, cmd.argument(), "texttt");
		else
			handleFont(cur, cmd.argument(), "mathtt");
		break;
	case LFUN_FONT_FRAK:
		handleFont(cur, cmd.argument(), "mathfrak");
		break;
	case LFUN_FONT_ITAL:
		if (currentMode() == TEXT_MODE)
			handleFont(cur, cmd.argument(), "textit");
		else
			handleFont(cur, cmd.argument(), "mathit");
		break;
	case LFUN_FONT_NOUN:
		if (currentMode() == TEXT_MODE)
			// FIXME: should be "noun"
			handleFont(cur, cmd.argument(), "textsc");
		else
			handleFont(cur, cmd.argument(), "mathbb");
		break;
	/*
	case LFUN_FONT_FREE_APPLY:
		handleFont(cur, cmd.argument(), "textrm");
		break;
	*/
	case LFUN_FONT_DEFAULT:
		handleFont(cur, cmd.argument(), "textnormal");
		break;

	case LFUN_MATH_MODE: {
#if 1
		// ignore math-mode on when already in math mode
		if (currentMode() == Inset::MATH_MODE && cmd.argument() == "on")
			break;
		cur.macroModeClose();
		docstring const save_selection = grabAndEraseSelection(cur);
		selClearOrDel(cur);
		//cur.plainInsert(MathAtom(new InsetMathMBox(cur.bv())));
		cur.plainInsert(MathAtom(new InsetMathBox(from_ascii("mbox"))));
		cur.posLeft();
		cur.pushLeft(*cur.nextInset());
		cur.niceInsert(save_selection);
#else
		if (currentMode() == Inset::TEXT_MODE) {
			cur.niceInsert(MathAtom(new InsetMathHull("simple")));
			cur.message(_("create new math text environment ($...$)"));
		} else {
			handleFont(cur, cmd.argument(), "textrm");
			cur.message(_("entered math text mode (textrm)"));
		}
#endif
		break;
	}

	case LFUN_MATH_SIZE:
#if 0
		recordUndo(cur);
		cur.setSize(arg);
#endif
		break;

	case LFUN_MATH_MATRIX: {
		recordUndo(cur, Undo::ATOMIC);
		unsigned int m = 1;
		unsigned int n = 1;
		docstring v_align;
		docstring h_align;
		idocstringstream is(cmd.argument());
		is >> m >> n >> v_align >> h_align;
		if (m < 1)
			m = 1;
		if (n < 1)
			n = 1;
		v_align += 'c';
		cur.niceInsert(
			MathAtom(new InsetMathArray(from_ascii("array"), m, n, (char)v_align[0], h_align)));
		break;
	}

	case LFUN_MATH_DELIM: {
		docstring ls;
		docstring rs = support::split(cmd.argument(), ls, ' ');
		// Reasonable default values
		if (ls.empty())
			ls = '(';
		if (rs.empty())
			rs = ')';
		recordUndo(cur, Undo::ATOMIC);
		cur.handleNest(MathAtom(new InsetMathDelim(ls, rs)));
		break;
	}

	case LFUN_MATH_BIGDELIM: {
		docstring const lname  = from_utf8(cmd.getArg(0));
		docstring const ldelim = from_utf8(cmd.getArg(1));
		docstring const rname  = from_utf8(cmd.getArg(2));
		docstring const rdelim = from_utf8(cmd.getArg(3));
		latexkeys const * l = in_word_set(lname);
		bool const have_l = l && l->inset == "big" &&
				    InsetMathBig::isBigInsetDelim(ldelim);
		l = in_word_set(rname);
		bool const have_r = l && l->inset == "big" &&
				    InsetMathBig::isBigInsetDelim(rdelim);
		// We mimic LFUN_MATH_DELIM in case we have an empty left
		// or right delimiter.
		if (have_l || have_r) {
			recordUndo(cur, Undo::ATOMIC);
			docstring const selection = grabAndEraseSelection(cur);
			selClearOrDel(cur);
			if (have_l)
				cur.insert(MathAtom(new InsetMathBig(lname,
								ldelim)));
			cur.niceInsert(selection);
			if (have_r)
				cur.insert(MathAtom(new InsetMathBig(rname,
								rdelim)));
		}
		// Don't call cur.undispatched() if we did nothing, this would
		// lead to infinite recursion via Text::dispatch().
		break;
	}

	case LFUN_SPACE_INSERT:
	case LFUN_MATH_SPACE:
		recordUndo(cur, Undo::ATOMIC);
		cur.insert(MathAtom(new InsetMathSpace(from_ascii(","))));
		break;

	case LFUN_ERT_INSERT:
		// interpret this as if a backslash was typed
		recordUndo(cur, Undo::ATOMIC);
		interpretChar(cur, '\\');
		break;

	case LFUN_MATH_SUBSCRIPT:
		// interpret this as if a _ was typed
		recordUndo(cur, Undo::ATOMIC);
		interpretChar(cur, '_');
		break;

	case LFUN_MATH_SUPERSCRIPT:
		// interpret this as if a ^ was typed
		recordUndo(cur, Undo::ATOMIC);
		interpretChar(cur, '^');
		break;

	case LFUN_QUOTE_INSERT:
		// interpret this as if a straight " was typed
		recordUndo(cur, Undo::ATOMIC);
		interpretChar(cur, '\"');
		break;

// FIXME: We probably should swap parts of "math-insert" and "self-insert"
// handling such that "self-insert" works on "arbitrary stuff" too, and
// math-insert only handles special math things like "matrix".
	case LFUN_MATH_INSERT: {
		recordUndo(cur, Undo::ATOMIC);
		if (cmd.argument() == "^" || cmd.argument() == "_") {
			interpretChar(cur, cmd.argument()[0]);
		} else
			cur.niceInsert(cmd.argument());
		break;
		}

	case LFUN_DIALOG_SHOW_NEW_INSET: {
		docstring const & name = cmd.argument();
		string data;
		if (name == "ref") {
			InsetMathRef tmp(name);
			data = tmp.createDialogStr(to_utf8(name));
		}
		cur.bv().showInsetDialog(to_utf8(name), data, 0);
		break;
	}

	case LFUN_INSET_INSERT: {
		MathData ar;
		if (createInsetMath_fromDialogStr(cmd.argument(), ar)) {
			recordUndo(cur);
			cur.insert(ar);
		} else
			cur.undispatched();
		break;
	}
	case LFUN_INSET_DISSOLVE:
		if (!asHullInset()) {
			recordUndoInset(cur, Undo::ATOMIC);
			cur.pullArg();
		}
		break;

	default:
		InsetMath::doDispatch(cur, cmd);
		break;
	}
}


bool InsetMathNest::getStatus(Cursor & cur, FuncRequest const & cmd,
		FuncStatus & flag) const
{
	// the font related toggles
	//string tc = "mathnormal";
	bool ret = true;
	string const arg = to_utf8(cmd.argument());
	switch (cmd.action) {
	case LFUN_TABULAR_FEATURE:
		flag.enabled(false);
		break;
#if 0
	case LFUN_TABULAR_FEATURE:
		// FIXME: check temporarily disabled
		// valign code
		char align = mathcursor::valign();
		if (align == '\0') {
			enable = false;
			break;
		}
		if (cmd.argument().empty()) {
			flag.clear();
			break;
		}
		if (!contains("tcb", cmd.argument()[0])) {
			enable = false;
			break;
		}
		flag.setOnOff(cmd.argument()[0] == align);
		break;
#endif
	/// We have to handle them since 1.4 blocks all unhandled actions
	case LFUN_FONT_ITAL:
	case LFUN_FONT_BOLD:
	case LFUN_FONT_SANS:
	case LFUN_FONT_EMPH:
	case LFUN_FONT_CODE:
	case LFUN_FONT_NOUN:
	case LFUN_FONT_ROMAN:
	case LFUN_FONT_DEFAULT:
		flag.enabled(true);
		break;
	case LFUN_MATH_MUTATE:
		//flag.setOnOff(mathcursor::formula()->hullType() == to_utf8(cmd.argument()));
		flag.setOnOff(false);
		break;

	// we just need to be in math mode to enable that
	case LFUN_MATH_SIZE:
	case LFUN_MATH_SPACE:
	case LFUN_MATH_LIMITS:
	case LFUN_MATH_NONUMBER:
	case LFUN_MATH_NUMBER:
	case LFUN_MATH_EXTERN:
		flag.enabled(true);
		break;

	case LFUN_FONT_FRAK:
		flag.enabled(currentMode() != TEXT_MODE);
		break;

	case LFUN_MATH_INSERT: {
		bool const textarg =
			arg == "\\textbf"   || arg == "\\textsf" ||
			arg == "\\textrm"   || arg == "\\textmd" ||
			arg == "\\textit"   || arg == "\\textsc" ||
			arg == "\\textsl"   || arg == "\\textup" ||
			arg == "\\texttt"   || arg == "\\textbb" ||
			arg == "\\textnormal";
		flag.enabled(currentMode() != TEXT_MODE || textarg);
		break;
	}

	case LFUN_MATH_MATRIX:
		flag.enabled(currentMode() == MATH_MODE);
		break;

	case LFUN_INSET_INSERT: {
		// Don't test createMathInset_fromDialogStr(), since
		// getStatus is not called with a valid reference and the
		// dialog would not be applyable.
		string const name = cmd.getArg(0);
		flag.enabled(name == "ref");
		break;
	}

	case LFUN_MATH_DELIM:
	case LFUN_MATH_BIGDELIM:
		// Don't do this with multi-cell selections
		flag.enabled(cur.selBegin().idx() == cur.selEnd().idx());
		break;

	case LFUN_HYPHENATION_POINT_INSERT:
	case LFUN_LIGATURE_BREAK_INSERT:
	case LFUN_MENU_SEPARATOR_INSERT:
	case LFUN_DOTS_INSERT:
	case LFUN_END_OF_SENTENCE_PERIOD_INSERT:
		// FIXME: These would probably make sense in math-text mode
		flag.enabled(false);
		break;

	case LFUN_INSET_DISSOLVE:
		flag.enabled(!asHullInset());
		break;

	default:
		ret = false;
		break;
	}
	return ret;
}


void InsetMathNest::edit(Cursor & cur, bool left)
{
	cur.push(*this);
	cur.idx() = left ? 0 : cur.lastidx();
	cur.pos() = left ? 0 : cur.lastpos();
	cur.resetAnchor();
	//lyxerr << "InsetMathNest::edit, cur:\n" << cur << endl;
}


Inset * InsetMathNest::editXY(Cursor & cur, int x, int y)
{
	int idx_min = 0;
	int dist_min = 1000000;
	for (idx_type i = 0, n = nargs(); i != n; ++i) {
		int const d = cell(i).dist(cur.bv(), x, y);
		if (d < dist_min) {
			dist_min = d;
			idx_min = i;
		}
	}
	MathData & ar = cell(idx_min);
	cur.push(*this);
	cur.idx() = idx_min;
	cur.pos() = ar.x2pos(x - ar.xo(cur.bv()));

	//lyxerr << "found cell : " << idx_min << " pos: " << cur.pos() << endl;
	if (dist_min == 0) {
		// hit inside cell
		for (pos_type i = 0, n = ar.size(); i < n; ++i)
			if (ar[i]->covers(cur.bv(), x, y))
				return ar[i].nucleus()->editXY(cur, x, y);
	}
	return this;
}


void InsetMathNest::lfunMousePress(Cursor & cur, FuncRequest & cmd)
{
	//lyxerr << "## lfunMousePress: buttons: " << cmd.button() << endl;
	BufferView & bv = cur.bv();
	bv.mouseSetCursor(cur);
	if (cmd.button() == mouse_button::button1) {
		//lyxerr << "## lfunMousePress: setting cursor to: " << cur << endl;
		// Update the cursor update flags as needed:
		//
		// Update::Decoration: tells to update the decoration
		//                     (visual box corners that define
		//                     the inset)/
		// Update::FitCursor: adjust the screen to the cursor
		//                    position if needed
		// cur.result().update(): don't overwrite previously set flags.
		cur.updateFlags(Update::Decoration | Update::FitCursor 
				| cur.result().update());
	} else if (cmd.button() == mouse_button::button2) {
		if (cap::selection()) {
			// See comment in Text::dispatch why we do this
			cap::copySelectionToStack();
			cmd = FuncRequest(LFUN_PASTE, "0");
			doDispatch(bv.cursor(), cmd);
		} else {
			MathData ar;
			asArray(theSelection().get(), ar);
			bv.cursor().insert(ar);
		}
	}
}


void InsetMathNest::lfunMouseMotion(Cursor & cur, FuncRequest & cmd)
{
	// only select with button 1
	if (cmd.button() == mouse_button::button1) {
		Cursor & bvcur = cur.bv().cursor();
		if (bvcur.anchor_.hasPart(cur)) {
			//lyxerr << "## lfunMouseMotion: cursor: " << cur << endl;
			bvcur.setCursor(cur);
			bvcur.selection() = true;
			//lyxerr << "MOTION " << bvcur << endl;
		} else
			cur.undispatched();
	}
}


void InsetMathNest::lfunMouseRelease(Cursor & cur, FuncRequest & cmd)
{
	//lyxerr << "## lfunMouseRelease: buttons: " << cmd.button() << endl;

	if (cmd.button() == mouse_button::button1) {
		if (!cur.selection())
			cur.noUpdate();
		else {
			Cursor & bvcur = cur.bv().cursor();
			bvcur.selection() = true;
		}
		return;
	}

	cur.undispatched();
}


bool InsetMathNest::interpretChar(Cursor & cur, char_type c)
{
	//lyxerr << "interpret 2: '" << c << "'" << endl;
	docstring save_selection;
	if (c == '^' || c == '_')
		save_selection = grabAndEraseSelection(cur);

	cur.clearTargetX();

	// handle macroMode
	if (cur.inMacroMode()) {
		docstring name = cur.macroName();

		/// are we currently typing '#1' or '#2' or...?
		if (name == "\\#") {
			cur.backspace();
			int n = c - '0';
			if (n >= 1 && n <= 9)
				cur.insert(new MathMacroArgument(n));
			return true;
		}

		if (isAlphaASCII(c)) {
			cur.activeMacro()->setName(name + docstring(1, c));
			return true;
		}

		// handle 'special char' macros
		if (name == "\\") {
			// remove the '\\'
			if (c == '\\') {
				cur.backspace();
				if (currentMode() == InsetMath::TEXT_MODE)
					cur.niceInsert(createInsetMath("textbackslash"));
				else
					cur.niceInsert(createInsetMath("backslash"));
			} else if (c == '{') {
				cur.backspace();
				cur.niceInsert(MathAtom(new InsetMathBrace));
			} else if (c == '%') {
				cur.backspace();
				cur.niceInsert(MathAtom(new InsetMathComment));
			} else if (c == '#') {
				BOOST_ASSERT(cur.activeMacro());
				cur.activeMacro()->setName(name + docstring(1, c));
			} else {
				cur.backspace();
				cur.niceInsert(createInsetMath(docstring(1, c)));
			}
			return true;
		}

		// One character big delimiters. The others are handled in
		// interpretString().
		latexkeys const * l = in_word_set(name.substr(1));
		if (name[0] == '\\' && l && l->inset == "big") {
			docstring delim;
			switch (c) {
			case '{':
				delim = from_ascii("\\{");
				break;
			case '}':
				delim = from_ascii("\\}");
				break;
			default:
				delim = docstring(1, c);
				break;
			}
			if (InsetMathBig::isBigInsetDelim(delim)) {
				// name + delim ared a valid InsetMathBig.
				// We can't use cur.macroModeClose() because
				// it does not handle delim.
				InsetMathUnknown * p = cur.activeMacro();
				p->finalize();
				--cur.pos();
				cur.cell().erase(cur.pos());
				cur.plainInsert(MathAtom(
					new InsetMathBig(name.substr(1), delim)));
				return true;
			}
		}

		// leave macro mode and try again if necessary
		cur.macroModeClose();
		if (c == '{')
			cur.niceInsert(MathAtom(new InsetMathBrace));
		else if (c != ' ')
			interpretChar(cur, c);
		return true;
	}

	// This is annoying as one has to press <space> far too often.
	// Disable it.

#if 0
		// leave autocorrect mode if necessary
		if (autocorrect() && c == ' ') {
			autocorrect() = false;
			return true;
		}
#endif

	// just clear selection on pressing the space bar
	if (cur.selection() && c == ' ') {
		cur.selection() = false;
		return true;
	}

	selClearOrDel(cur);

	if (c == '\\') {
		//lyxerr << "starting with macro" << endl;
		cur.insert(MathAtom(new InsetMathUnknown(from_ascii("\\"), false)));
		return true;
	}

	if (c == '\n') {
		if (currentMode() == InsetMath::TEXT_MODE)
			cur.insert(c);
		return true;
	}

	if (c == ' ') {
		if (currentMode() == InsetMath::TEXT_MODE) {
			// insert spaces in text mode,
			// but suppress direct insertion of two spaces in a row
			// the still allows typing  '<space>a<space>' and deleting the 'a', but
			// it is better than nothing...
			if (!cur.pos() != 0 || cur.prevAtom()->getChar() != ' ') {
				cur.insert(c);
				// FIXME: we have to enable full redraw here because of the
				// visual box corners that define the inset. If we know for
				// sure that we stay within the same cell we can optimize for
				// that using:
				//cur.updateFlags(Update::SinglePar | Update::FitCursor);
			}
			return true;
		}
		if (cur.pos() != 0 && cur.prevAtom()->asSpaceInset()) {
			cur.prevAtom().nucleus()->asSpaceInset()->incSpace();
			// FIXME: we have to enable full redraw here because of the
			// visual box corners that define the inset. If we know for
			// sure that we stay within the same cell we can optimize for
			// that using:
			//cur.updateFlags(Update::SinglePar | Update::FitCursor);
			return true;
		}

		if (cur.popRight()) {
			// FIXME: we have to enable full redraw here because of the
			// visual box corners that define the inset. If we know for
			// sure that we stay within the same cell we can optimize for
			// that using:
			//cur.updateFlags(Update::FitCursor);
			return true;
		}

		// if we are at the very end, leave the formula
		return cur.pos() != cur.lastpos();
	}

	// These shouldn't work in text mode:
	if (currentMode() != InsetMath::TEXT_MODE) {
		if (c == '_') {
			script(cur, false, save_selection);
			return true;
		}
		if (c == '^') {
			script(cur, true, save_selection);
			return true;
		}
		if (c == '~') {
			cur.niceInsert(createInsetMath("sim"));
			return true;
		}
	}

	if (c == '{' || c == '}' || c == '&' || c == '$' || c == '#' ||
	    c == '%' || c == '_' || c == '^') {
		cur.niceInsert(createInsetMath(docstring(1, c)));
		return true;
	}


	// try auto-correction
	//if (autocorrect() && hasPrevAtom() && math_autocorrect(prevAtom(), c))
	//	return true;

	// no special circumstances, so insert the character without any fuss
	cur.insert(c);
	cur.autocorrect() = true;
	return true;
}


bool InsetMathNest::interpretString(Cursor & cur, docstring const & str)
{
	// Create a InsetMathBig from cur.cell()[cur.pos() - 1] and t if
	// possible
	if (!cur.empty() && cur.pos() > 0 &&
	    cur.cell()[cur.pos() - 1]->asUnknownInset()) {
		if (InsetMathBig::isBigInsetDelim(str)) {
			docstring prev = asString(cur.cell()[cur.pos() - 1]);
			if (prev[0] == '\\') {
				prev = prev.substr(1);
				latexkeys const * l = in_word_set(prev);
				if (l && l->inset == "big") {
					cur.cell()[cur.pos() - 1] =
						MathAtom(new InsetMathBig(prev, str));
					return true;
				}
			}
		}
	}
	return false;
}


bool InsetMathNest::script(Cursor & cur, bool up,
		docstring const & save_selection)
{
	// Hack to get \^ and \_ working
	//lyxerr << "handling script: up: " << up << endl;
	if (cur.inMacroMode() && cur.macroName() == "\\") {
		if (up)
			cur.niceInsert(createInsetMath("mathcircumflex"));
		else
			interpretChar(cur, '_');
		return true;
	}

	cur.macroModeClose();
	if (asScriptInset() && cur.idx() == 0) {
		// we are in a nucleus of a script inset, move to _our_ script
		InsetMathScript * inset = asScriptInset();
		//lyxerr << " going to cell " << inset->idxOfScript(up) << endl;
		inset->ensure(up);
		cur.idx() = inset->idxOfScript(up);
		cur.pos() = 0;
	} else if (cur.pos() != 0 && cur.prevAtom()->asScriptInset()) {
		--cur.pos();
		InsetMathScript * inset = cur.nextAtom().nucleus()->asScriptInset();
		cur.push(*inset);
		inset->ensure(up);
		cur.idx() = inset->idxOfScript(up);
		cur.pos() = cur.lastpos();
	} else {
		// convert the thing to our left to a scriptinset or create a new
		// one if in the very first position of the array
		if (cur.pos() == 0) {
			//lyxerr << "new scriptinset" << endl;
			cur.insert(new InsetMathScript(up));
		} else {
			//lyxerr << "converting prev atom " << endl;
			cur.prevAtom() = MathAtom(new InsetMathScript(cur.prevAtom(), up));
		}
		--cur.pos();
		InsetMathScript * inset = cur.nextAtom().nucleus()->asScriptInset();
		// See comment in MathParser.cpp for special handling of {}-bases

		cur.push(*inset);
		cur.idx() = 1;
		cur.pos() = 0;
	}
	//lyxerr << "inserting selection 1:\n" << save_selection << endl;
	cur.niceInsert(save_selection);
	cur.resetAnchor();
	//lyxerr << "inserting selection 2:\n" << save_selection << endl;
	return true;
}


} // namespace lyx