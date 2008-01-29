// -*- C++ -*-
/**
 * \file InsetMathNest.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef MATH_NESTINSET_H
#define MATH_NESTINSET_H

#include "InsetMath.h"


namespace lyx {


/** Abstract base class for all math objects that contain nested items.
    This is basically everything that is not a single character or a
    single symbol.
*/

class InsetMathNest : public InsetMath {
public:
	/// nestinsets have a fixed size to start with
	explicit InsetMathNest(idx_type ncells);
	///
	virtual ~InsetMathNest() {}

	/// the size is usually some sort of convex hull of the cells
	/// hides inset::metrics() intentionally!
	void metrics(MetricsInfo const & mi) const;
	/// draw background if locked
	void draw(PainterInfo & pi, int x, int y) const;
	/// draw selection background
	void drawSelection(PainterInfo & pi, int x, int y) const;
	/// draw decorations.
	void drawDecoration(PainterInfo & pi, int x, int y) const
	{ drawMarkers(pi, x, y); }
	/// identifies NestInsets
	InsetMathNest * asNestInset() { return this; }
	/// identifies NestInsets
	InsetMathNest const * asNestInset() const { return this; }
	/// get cursor position
	void cursorPos(BufferView const & bv, CursorSlice const & sl,
		bool boundary, int & x, int & y) const;
	///
	void edit(Cursor & cur, bool left);
	///
	Inset * editXY(Cursor & cur, int x, int y);

	/// order of movement through the cells when pressing the left key
	bool idxLeft(Cursor &) const;
	/// order of movement through the cells when pressing the right key
	bool idxRight(Cursor &) const;

	/// move one physical cell up
	bool idxNext(Cursor &) const;
	/// move one physical cell down
	bool idxPrev(Cursor &) const;

	/// target pos when we enter the inset from the left by pressing "Right"
	bool idxFirst(Cursor &) const;
	/// target pos when we enter the inset from the right by pressing "Left"
	bool idxLast(Cursor &) const;

	/// number of cells currently governed by us
	idx_type nargs() const;
	/// access to the lock
	bool lock() const;
	/// access to the lock
	void lock(bool);
	/// get notification when the cursor leaves this inset
	bool notifyCursorLeaves(Cursor & cur);

	/// direct access to the cell.
	/// inlined because shows in profile.
	//@{
	MathData & cell(idx_type i) { return cells_[i]; }
	MathData const & cell(idx_type i) const { return cells_[i]; }
	//@}

	/// can we move into this cell (see macroarg.h)
	bool isActive() const;
	/// request "external features"
	void validate(LaTeXFeatures & features) const;

	/// replace in all cells
	void replace(ReplaceData &);
	/// do we contain a given pattern?
	bool contains(MathData const &) const;
	/// glue everything to a single cell
	MathData glue() const;

	/// debug helper
	void dump() const;

	/// writes \\, name(), and args in braces and '\\lyxlock' if necessary
	void write(WriteStream & os) const;
	/// writes [, name(), and args in []
	void normalize(NormalStream & os) const;
	///
	int latex(Buffer const &, odocstream & os,
			OutputParams const & runparams) const;
	///
	bool setMouseHover(bool mouse_hover);
	///
	bool mouseHovered() const { return mouse_hover_; }

protected:
	///
	InsetMathNest(InsetMathNest const & inset);
	///
	InsetMathNest & operator=(InsetMathNest const &);

	///
	virtual void doDispatch(Cursor & cur, FuncRequest & cmd);
	/// do we want to handle this event?
	bool getStatus(Cursor & cur, FuncRequest const & cmd,
		FuncStatus & status) const;
	///
	void handleFont(Cursor & cur,
		docstring const & arg, docstring const & font);
	void handleFont(Cursor & cur,
		docstring const & arg, char const * const font);
	///
	void handleFont2(Cursor & cur, docstring const & arg);

	/// interpret \p c and insert the result at the current position of
	/// of \p cur. Return whether the cursor should stay in the formula.
	bool interpretChar(Cursor & cur, char_type c);
	///
	bool script(Cursor & cur, bool,
		docstring const & save_selection = docstring());

public:
	/// interpret \p str and insert the result at the current position of
	/// \p cur if it is something known. Return whether \p cur was
	/// inserted.
	bool interpretString(Cursor & cur, docstring const & str);

private:
	/// lfun handler
	void lfunMousePress(Cursor &, FuncRequest &);
	///
	void lfunMouseRelease(Cursor &, FuncRequest &);
	///
	void lfunMouseMotion(Cursor &, FuncRequest &);

protected:
	/// we store the cells in a vector
	typedef std::vector<MathData> cells_type;
	/// thusly:
	cells_type cells_;
	/// if the inset is locked, it can't be entered with the cursor
	bool lock_;
	///
	bool mouse_hover_;
};



} // namespace lyx
#endif