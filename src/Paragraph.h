// -*- C++ -*-
/**
 * \file Paragraph.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Asger Alstrup
 * \author Lars Gullik Bj�nnes
 * \author John Levon
 * \author Andr� P�nitz
 * \author J�rgen Vigna
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef PARAGRAPH_H
#define PARAGRAPH_H

#include "FontEnums.h"
#include "Layout.h"

#include "insets/InsetCode.h"

#include "support/strfwd.h"
#include "support/types.h"

namespace lyx {

class AuthorList;
class Buffer;
class BufferParams;
class Change;
class Counters;
class Cursor;
class CursorSlice;
class DocIterator;
class DocumentClass;
class Inset;
class InsetBibitem;
class LaTeXFeatures;
class Inset_code;
class InsetList;
class Language;
class Font;
class Font_size;
class MetricsInfo;
class OutputParams;
class PainterInfo;
class ParagraphParameters;
class TexRow;
class Toc;

class FontSpan {
public:
	/// Invalid font span containing no character
	FontSpan() : first(0), last(-1) {}
	/// Span including first and last
	FontSpan(pos_type f, pos_type l) : first(f), last(l) {}

public:
	/// Range including first and last.
	pos_type first, last;
};

///
enum TextCase {
	///
	text_lowercase = 0,
	///
	text_capitalization = 1,
	///
	text_uppercase = 2
};


///
enum AsStringParameter
{
	AS_STR_NONE = 0, ///< No option, only printable characters.
	AS_STR_LABEL = 1, ///< Prefix with paragraph label.
	AS_STR_INSETS = 2, ///< Go into insets.
	AS_STR_NEWLINES = 4 ///< Get also newline characters.
};


/// A Paragraph holds all text, attributes and insets in a text paragraph
class Paragraph
{
public:
	///
	Paragraph();
	///
	Paragraph(Paragraph const &);
	/// Partial copy constructor.
	/// Copy the Paragraph contents from \p beg to \p end (without end).
	Paragraph(Paragraph const & par, pos_type beg, pos_type end);
	///
	Paragraph & operator=(Paragraph const &);
	///
	~Paragraph();
	///
	int id() const;

	///
	void addChangesToToc(DocIterator const & cdit, Buffer const & buf) const;
	///
	Language const * getParLanguage(BufferParams const &) const;
	///
	bool isRTL(BufferParams const &) const;
	///
	void changeLanguage(BufferParams const & bparams,
			    Language const * from, Language const * to);
	///
	bool isMultiLingual(BufferParams const &) const;

	/// Convert the paragraph to a string.
	/// \param AsStringParameter options. This can contain any combination of
	/// asStringParameter values. Valid examples:
	///		asString(AS_STR_LABEL)
	///		asString(AS_STR_LABEL | AS_STR_INSETS)
	///		asString(AS_STR_INSETS)
	docstring asString(int options = AS_STR_NONE) const;
	///
	docstring asString(pos_type beg, pos_type end,
		int options = AS_STR_NONE) const;

	///
	void write(std::ostream &, BufferParams const &,
		   depth_type & depth) const;
	///
	void validate(LaTeXFeatures &) const;

	///
	bool latex(BufferParams const &, Font const & outerfont, odocstream &,
		TexRow & texrow, OutputParams const &) const;

	/// Can we drop the standard paragraph wrapper?
	bool emptyTag() const;

	/// Get the id of the paragraph, usefull for docbook
	std::string getID(Buffer const & buf, OutputParams const & runparams) const;

	/// Get the first word of a paragraph, return the position where it left
	pos_type firstWord(odocstream & os, OutputParams const & runparams) const;

	/// Writes to stream the docbook representation
	void simpleDocBookOnePar(Buffer const & buf,
				 odocstream &,
				 OutputParams const & runparams,
				 Font const & outerfont,
				 pos_type initial = 0) const;

	///
	bool hasSameLayout(Paragraph const & par) const;

	///
	void makeSameLayout(Paragraph const & par);

	///
	void setInsetOwner(Inset const * inset);
	///
	Inset const & inInset() const;
	///
	InsetCode ownerCode() const;
	///
	bool forcePlainLayout() const;
	///
	bool allowParagraphCustomization() const;
	///
	bool usePlainLayout() const;
	///
	pos_type size() const;
	///
	bool empty() const;

	///
	Layout const & layout() const;
	/// Do not pass a temporary to this!
	void setLayout(Layout const & layout);
	///
	void setPlainOrDefaultLayout(DocumentClass const & tc);
	///
	void setDefaultLayout(DocumentClass const & tc);
	///
	void setPlainLayout(DocumentClass const & tc);

	/// This is the item depth, only used by enumerate and itemize
	signed char itemdepth;

	/// look up change at given pos
	Change const & lookupChange(pos_type pos) const;

	/// is there a change within the given range ?
	bool isChanged(pos_type start, pos_type end) const;
	/// is there an unchanged char at the given pos ?
	bool isUnchanged(pos_type pos) const;
	/// is there an insertion at the given pos ?
	bool isInserted(pos_type pos) const;
	/// is there a deletion at the given pos ?
	bool isDeleted(pos_type pos) const;

	/// will the paragraph be physically merged with the next
	/// one if the imaginary end-of-par character is logically deleted?
	bool isMergedOnEndOfParDeletion(bool trackChanges) const;

	/// set change for the entire par
	void setChange(Change const & change);

	/// set change at given pos
	void setChange(pos_type pos, Change const & change);

	/// accept changes within the given range
	void acceptChanges(BufferParams const & bparams, pos_type start, pos_type end);

	/// reject changes within the given range
	void rejectChanges(BufferParams const & bparams, pos_type start, pos_type end);

	/// Paragraphs can contain "manual labels", for example, Description
	/// environment. The text for this user-editable label is stored in
	/// the paragraph alongside the text of the rest of the paragraph
	/// (the body). This function returns the starting position of the
	/// body of the text in the paragraph.
	pos_type beginOfBody() const;
	/// recompute this value
	void setBeginOfBody();

	///
	docstring const & labelString() const;

	/// the next two functions are for the manual labels
	docstring const getLabelWidthString() const;
	/// Set label width string.
	void setLabelWidthString(docstring const & s);
	/// translate \p label to the paragraph language if possible.
	docstring const translateIfPossible(docstring const & label,
		BufferParams const & bparams) const;
	/// Expand the counters for the labelstring of \c layout
	docstring expandLabel(Layout const &, BufferParams const &,
		bool process_appendix = true) const;
	/// Actual paragraph alignment used
	char getAlign() const;
	/// The nesting depth of a paragraph
	depth_type getDepth() const;
	/// The maximal possible depth of a paragraph after this one
	depth_type getMaxDepthAfter() const;
	///
	void applyLayout(Layout const & new_layout);

	/// (logically) erase the char at pos; return true if it was actually erased
	bool eraseChar(pos_type pos, bool trackChanges);
	/// (logically) erase the given range; return the number of chars actually erased
	int eraseChars(pos_type start, pos_type end, bool trackChanges);

	///
	void resetFonts(Font const & font);

	/** Get uninstantiated font setting. Returns the difference
	    between the characters font and the layoutfont.
	    This is what is stored in the fonttable
	*/
	Font const &
	getFontSettings(BufferParams const &, pos_type pos) const;
	///
	Font const & getFirstFontSettings(BufferParams const &) const;

	/** Get fully instantiated font. If pos == -1, use the layout
	    font attached to this paragraph.
	    If pos == -2, use the label font of the layout attached here.
	    In all cases, the font is instantiated, i.e. does not have any
	    attributes with values FONT_INHERIT, FONT_IGNORE or
	    FONT_TOGGLE.
	*/
	Font const getFont(BufferParams const &, pos_type pos,
			      Font const & outerfont) const;
	Font const getLayoutFont(BufferParams const &,
				    Font const & outerfont) const;
	Font const getLabelFont(BufferParams const &,
				   Font const & outerfont) const;
	/**
	 * The font returned by the above functions is the same in a
	 * span of characters. This method will return the first and
	 * the last positions in the paragraph for which that font is
	 * the same. This can be used to avoid unnecessary calls to getFont.
	 */
	FontSpan fontSpan(pos_type pos) const;
	///
	char_type getChar(pos_type pos) const;
	/// Get the char, but mirror all bracket characters if it is right-to-left
	char_type getUChar(BufferParams const &, pos_type pos) const;
	/// pos <= size() (there is a dummy font change at the end of each par)
	void setFont(pos_type pos, Font const & font);
	/// Returns the height of the highest font in range
	FontSize highestFontInRange(pos_type startpos,
					pos_type endpos, FontSize def_size) const;
	///
	void insert(pos_type pos, docstring const & str,
		    Font const & font, Change const & change);

	///
	void appendString(docstring const & s, Font const & font,
		Change const & change);
	///
	void appendChar(char_type c, Font const & font, Change const & change);
	///
	void insertChar(pos_type pos, char_type c, bool trackChanges);
	///
	void insertChar(pos_type pos, char_type c,
			Font const &, bool trackChanges);
	///
	void insertChar(pos_type pos, char_type c,
			Font const &, Change const & change);
	/// Insert \p inset at position \p pos with \p change traking status.
	/// \return true if successful.
	bool insertInset(pos_type pos, Inset * inset,
			 Change const & change);
	/// Insert \p inset at position \p pos with \p change traking status and
	/// \p font.
	/// \return true if successful.
	bool insertInset(pos_type pos, Inset * inset,
			 Font const & font, Change const & change);
	///
	Inset * getInset(pos_type pos);
	///
	Inset const * getInset(pos_type pos) const;

	/// Release inset at given position.
	/// \warning does not honour change tracking!
	/// Therefore, it should only be used for breaking and merging
	/// paragraphs
	Inset * releaseInset(pos_type pos);

	///
	InsetList const & insetList() const;
	///
	void setBuffer(Buffer &);

	///
	bool isHfill(pos_type pos) const;

	/// hinted by profiler
	bool isInset(pos_type pos) const;
	///
	bool isNewline(pos_type pos) const;
	/// return true if the char is a word separator
	bool isSeparator(pos_type pos) const;
	///
	bool isLineSeparator(pos_type pos) const;
	/// True if the character/inset at this point can be part of a word.
	/// Note that digits in particular are considered as letters
	bool isLetter(pos_type pos) const;
	/// True if the element at this point is a character that is not a letter.
	bool isChar(pos_type pos) const;
	/// True if the element at this point is a space
	bool isSpace(pos_type pos) const;

	/// returns true if at least one line break or line separator has been deleted
	/// at the beginning of the paragraph (either physically or logically)
	bool stripLeadingSpaces(bool trackChanges);

	/// return true if we allow multiple spaces
	bool isFreeSpacing() const;

	/// return true if we allow this par to stay empty
	bool allowEmpty() const;
	///
	char_type transformChar(char_type c, pos_type pos) const;
	///
	ParagraphParameters & params();
	///
	ParagraphParameters const & params() const;

	/// Check if we are in a Biblio environment and insert or
	/// delete InsetBibitems as necessary.
	/// \retval int 1, if we had to add an inset, in which case
	/// the cursor will need to move cursor forward; -pos, if we deleted
	/// an inset, in which case pos is the position from which the inset
	/// was deleted, and the cursor will need to be moved back one if it
	/// was previously past that position. Return 0 otherwise.
	int checkBiblio(Buffer const & buffer);

	/// For each author, set 'used' to true if there is a change
	/// by this author in the paragraph.
	void checkAuthors(AuthorList const & authorList);

	///
	void changeCase(BufferParams const & bparams, pos_type pos,
		pos_type & right, TextCase action);

	/// find \param str string inside Paragraph.
	/// \return true if the specified string is at the specified position
	/// \param del specifies whether deleted strings in ct mode will be considered
	bool find(
		docstring const & str, ///< string to search
		bool cs, ///<
		bool mw, ///<
		pos_type pos, ///< start from here.
		bool del = true) const;
	
	///
	void updateWords(CursorSlice const & sl);

private:
	///
	void deregisterWords();
	///
	void collectWords(CursorSlice const & sl);
	///
	void registerWords();

	/// Pimpl away stuff
	class Private;
	///
	friend class Paragraph::Private;
	///
	Private * d;
};

} // namespace lyx

#endif // PARAGRAPH_H