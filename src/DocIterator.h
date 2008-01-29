// -*- C++ -*-
/**
 * \file DocIterator.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef DOCITERATOR_H
#define DOCITERATOR_H

#include "CursorSlice.h"

#include <vector>
#include <iosfwd>


namespace lyx {

class Text;
class MathAtom;
class Paragraph;


// only needed for gcc 2.95, remove when support terminated
template <typename A, typename B>
bool ptr_cmp(A const * a, B const * b)
{
	return a == b;
}


// The public inheritance should go in favour of a suitable data member
// (or maybe private inheritance) at some point of time.
class DocIterator // : public std::vector<CursorSlice>
{
public:
	/// type for cell number in inset
	typedef CursorSlice::idx_type idx_type;
	/// type for row indices
	typedef CursorSlice::row_type row_type;
	/// type for col indices
	typedef CursorSlice::col_type col_type;

public:
	///
	DocIterator();
	///
	explicit DocIterator(Inset & inset);

	/// access slice at position \p i
	CursorSlice const & operator[](size_t i) const { return slices_[i]; }
	/// access slice at position \p i
	CursorSlice & operator[](size_t i) { return slices_[i]; }
	/// chop a few slices from the iterator
	void resize(size_t i) { slices_.resize(i); }

	/// is the iterator valid?
	operator const void*() const { return empty() ? 0 : this; }
	/// is this iterator invalid?
	bool operator!() const { return empty(); }

	/// does this iterator have any content?
	bool empty() const { return slices_.empty(); }

	//
	// access to slice at tip
	//
	/// access to tip
	CursorSlice & top() { return slices_.back(); }
	/// access to tip
	CursorSlice const & top() const { return slices_.back(); }
	/// access to outermost slice
	CursorSlice & bottom() { return slices_.front(); }
	/// access to outermost slice
	CursorSlice const & bottom() const { return slices_.front(); }
	/// how many nested insets do we have?
	size_t depth() const { return slices_.size(); }
	/// the containing inset
	Inset & inset() const { return top().inset(); }
	/// return the cell of the inset this cursor is in
	idx_type idx() const { return top().idx(); }
	/// return the cell of the inset this cursor is in
	idx_type & idx() { return top().idx(); }
	/// return the last possible cell in this inset
	idx_type lastidx() const;
	/// return the paragraph this cursor is in
	pit_type pit() const { return top().pit(); }
	/// return the paragraph this cursor is in
	pit_type & pit() { return top().pit(); }
	/// return the last possible paragraph in this inset
	pit_type lastpit() const;
	/// return the position within the paragraph
	pos_type pos() const { return top().pos(); }
	/// return the position within the paragraph
	pos_type & pos() { return top().pos(); }
	/// return the last position within the paragraph
	pos_type lastpos() const;

	/// return the number of embedded cells
	size_t nargs() const;
	/// return the number of embedded cells
	size_t ncols() const;
	/// return the number of embedded cells
	size_t nrows() const;
	/// return the grid row of the top cell
	row_type row() const;
	/// return the last row of the top grid
	row_type lastrow() const { return nrows() - 1; }
	/// return the grid column of the top cell
	col_type col() const;
	/// return the last column of the top grid
	col_type lastcol() const { return ncols() - 1; }
	/// the inset just behind the cursor
	Inset * nextInset();
	/// the inset just in front of the cursor
	Inset * prevInset();
	/// the inset just in front of the cursor
	Inset const * prevInset() const;
	///
	bool boundary() const { return boundary_; }
	///
	void boundary(bool b) { boundary_ = b; }

	// the two methods below have been inlined out because of
	// profiling results under linux when opening a document.
	/// are we in mathed?.
	bool inMathed() const
	{ return !empty() && inset().inMathed(); }
	/// are we in texted?.
	bool inTexted() const
	{ return !empty() && !inset().inMathed(); }

	//
	// math-specific part
	//
	/// return the mathed cell this cursor is in
	MathData const & cell() const;
	/// return the mathed cell this cursor is in
	MathData & cell();
	/// the mathatom left of the cursor
	MathAtom const & prevAtom() const;
	/// the mathatom left of the cursor
	MathAtom & prevAtom();
	/// the mathatom right of the cursor
	MathAtom const & nextAtom() const;
	/// the mathatom right of the cursor
	MathAtom & nextAtom();

	//
	// text-specific part
	//
	/// the paragraph we're in
	Paragraph & paragraph();
	/// the paragraph we're in in text mode.
	/// \warning only works within text!
	Paragraph const & paragraph() const;
	/// the paragraph we're in in any case.
	/// This method will give the containing paragraph if
	/// in not in text mode (ex: in mathed).
	Paragraph const & innerParagraph() const;
	///
	Text * text();
	///
	Text const * text() const;
	/// the containing inset or the cell, respectively
	Inset * realInset() const;
	///
	Inset * innerInsetOfType(int code) const;
	///
	Text * innerText();
	///
	Text const * innerText() const;

	//
	// elementary moving
	//
	/// move on one logical position, do not descend into nested insets
	void forwardPosNoDescend();
	/**
	 * move on one logical position, descend into nested insets
	 * skip collapsed insets if \p ignorecollapsed is true
	 */
	void forwardPos(bool ignorecollapsed = false);
	/// move on one physical character or inset
	void forwardChar();
	/// move on one paragraph
	void forwardPar();
	/// move on one cell
	void forwardIdx();
	/// move on one inset
	void forwardInset();
	/// move backward one logical position
	void backwardPos();
	/// move backward one physical character or inset
	void backwardChar();
	/// move backward one paragraph
	void backwardPar();
	/// move backward one cell
	void backwardIdx();
	/// move backward one inset
	/// FIXME: This is not implemented!
	//void backwardInset();

	/// are we some 'extension' (i.e. deeper nested) of the given iterator
	bool hasPart(DocIterator const & it) const;

	/// output
	friend std::ostream &
	operator<<(std::ostream & os, DocIterator const & cur);
	///
	friend bool operator==(DocIterator const &, DocIterator const &);
	friend bool operator<(DocIterator const &, DocIterator const &);
	friend bool operator>(DocIterator const &, DocIterator const &);
	friend bool operator<=(DocIterator const &, DocIterator const &);
	///
	friend class StableDocIterator;
//protected:
	///
	void clear() { slices_.clear(); }
	///
	void push_back(CursorSlice const & sl) { slices_.push_back(sl); }
	///
	void pop_back() { slices_.pop_back(); }
	/// recompute the inset parts of the cursor from the document data
	void updateInsets(Inset * inset);
	/// fix DocIterator in circumstances that should never happen.
	/// \return true if the DocIterator was fixed.
	bool fixIfBroken();

private:
	/**
	 * When the cursor position is i, is the cursor after the i-th char
	 * or before the i+1-th char ? Normally, these two interpretations are
	 * equivalent, except when the fonts of the i-th and i+1-th char
	 * differ.
	 * We use boundary_ to distinguish between the two options:
	 * If boundary_=true, then the cursor is after the i-th char
	 * and if boundary_=false, then the cursor is before the i+1-th char.
	 *
	 * We currently use the boundary only when the language direction of
	 * the i-th char is different than the one of the i+1-th char.
	 * In this case it is important to distinguish between the two
	 * cursor interpretations, in order to give a reasonable behavior to
	 * the user.
	 */
	bool boundary_;
	///
	std::vector<CursorSlice> const & internalData() const {
		return slices_;
	}
	///
	std::vector<CursorSlice> slices_;
	///
	Inset * inset_;
};


DocIterator doc_iterator_begin(Inset & inset);
DocIterator doc_iterator_end(Inset & inset);


inline
bool operator==(DocIterator const & di1, DocIterator const & di2)
{
	return di1.slices_ == di2.slices_;
}


inline
bool operator!=(DocIterator const & di1, DocIterator const & di2)
{
	return !(di1 == di2);
}


// The difference to a ('non stable') DocIterator is the removed
// (overwritten by 0...) part of the CursorSlice data items. So this thing
// is suitable for external storage, but not for iteration as such.

class StableDocIterator {
public:
	///
	StableDocIterator() {}
	/// non-explicit intended
	StableDocIterator(const DocIterator & it);
	///
	DocIterator asDocIterator(Inset * start) const;
	///
	size_t size() const { return data_.size(); }
	///  return the position within the paragraph
	pos_type pos() const { return data_.back().pos(); }
	///  return the position within the paragraph
	pos_type & pos() { return data_.back().pos(); }
	///
	friend std::ostream &
	operator<<(std::ostream & os, StableDocIterator const & cur);
	///
	friend std::istream &
	operator>>(std::istream & is, StableDocIterator & cur);
	///
	friend bool
	operator==(StableDocIterator const &, StableDocIterator const &);
private:
	std::vector<CursorSlice> data_;
};


} // namespace lyx

#endif