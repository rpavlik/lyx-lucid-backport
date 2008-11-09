/**
 * \file src/Text.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Asger Alstrup
 * \author Lars Gullik Bj�nnes
 * \author Dov Feldstern
 * \author Jean-Marc Lasgouttes
 * \author John Levon
 * \author Andr� P�nitz
 * \author Stefan Schimanski
 * \author Dekel Tsur
 * \author J�rgen Vigna
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "Text.h"

#include "Author.h"
#include "Buffer.h"
#include "buffer_funcs.h"
#include "BufferParams.h"
#include "BufferView.h"
#include "Changes.h"
#include "CompletionList.h"
#include "Cursor.h"
#include "CutAndPaste.h"
#include "DispatchResult.h"
#include "Encoding.h"
#include "ErrorList.h"
#include "FuncRequest.h"
#include "factory.h"
#include "Language.h"
#include "Length.h"
#include "Lexer.h"
#include "LyXRC.h"
#include "Paragraph.h"
#include "paragraph_funcs.h"
#include "ParagraphParameters.h"
#include "ParIterator.h"
#include "TextClass.h"
#include "TextMetrics.h"
#include "VSpace.h"
#include "WordLangTuple.h"
#include "WordList.h"

#include "insets/InsetText.h"
#include "insets/InsetBibitem.h"
#include "insets/InsetCaption.h"
#include "insets/InsetLine.h"
#include "insets/InsetNewline.h"
#include "insets/InsetNewpage.h"
#include "insets/InsetOptArg.h"
#include "insets/InsetSpace.h"
#include "insets/InsetSpecialChar.h"
#include "insets/InsetTabular.h"

#include "support/lassert.h"
#include "support/convert.h"
#include "support/debug.h"
#include "support/docstream.h"
#include "support/gettext.h"
#include "support/lstrings.h"
#include "support/textutils.h"

#include <boost/next_prior.hpp>

#include <sstream>

using namespace std;
using namespace lyx::support;

namespace lyx {

using cap::cutSelection;
using cap::pasteParagraphList;

namespace {

void readParToken(Buffer const & buf, Paragraph & par, Lexer & lex,
	string const & token, Font & font, Change & change, ErrorList & errorList)
{
	BufferParams const & bp = buf.params();

	if (token[0] != '\\') {
		docstring dstr = lex.getDocString();
		par.appendString(dstr, font, change);

	} else if (token == "\\begin_layout") {
		lex.eatLine();
		docstring layoutname = lex.getDocString();

		font = Font(inherit_font, bp.language);
		change = Change(Change::UNCHANGED);

		DocumentClass const & tclass = bp.documentClass();

		if (layoutname.empty())
			layoutname = tclass.defaultLayoutName();

		if (par.forcePlainLayout()) {
			// in this case only the empty layout is allowed
			layoutname = tclass.plainLayoutName();
		} else if (par.usePlainLayout()) {
			// in this case, default layout maps to empty layout 
			if (layoutname == tclass.defaultLayoutName())
				layoutname = tclass.plainLayoutName();
		} else { 
			// otherwise, the empty layout maps to the default
			if (layoutname == tclass.plainLayoutName())
				layoutname = tclass.defaultLayoutName();
		}

		// When we apply an unknown layout to a document, we add this layout to the textclass
		// of this document. For example, when you apply class article to a beamer document,
		// all unknown layouts such as frame will be added to document class article so that
		// these layouts can keep their original names.
		tclass.addLayoutIfNeeded(layoutname);

		par.setLayout(bp.documentClass()[layoutname]);

		// Test whether the layout is obsolete.
		Layout const & layout = par.layout();
		if (!layout.obsoleted_by().empty())
			par.setLayout(bp.documentClass()[layout.obsoleted_by()]);

		par.params().read(lex);

	} else if (token == "\\end_layout") {
		LYXERR0("Solitary \\end_layout in line " << lex.lineNumber() << "\n"
		       << "Missing \\begin_layout ?");
	} else if (token == "\\end_inset") {
		LYXERR0("Solitary \\end_inset in line " << lex.lineNumber() << "\n"
		       << "Missing \\begin_inset ?");
	} else if (token == "\\begin_inset") {
		Inset * inset = readInset(lex, buf);
		if (inset)
			par.insertInset(par.size(), inset, font, change);
		else {
			lex.eatLine();
			docstring line = lex.getDocString();
			errorList.push_back(ErrorItem(_("Unknown Inset"), line,
					    par.id(), 0, par.size()));
		}
	} else if (token == "\\family") {
		lex.next();
		setLyXFamily(lex.getString(), font.fontInfo());
	} else if (token == "\\series") {
		lex.next();
		setLyXSeries(lex.getString(), font.fontInfo());
	} else if (token == "\\shape") {
		lex.next();
		setLyXShape(lex.getString(), font.fontInfo());
	} else if (token == "\\size") {
		lex.next();
		setLyXSize(lex.getString(), font.fontInfo());
	} else if (token == "\\lang") {
		lex.next();
		string const tok = lex.getString();
		Language const * lang = languages.getLanguage(tok);
		if (lang) {
			font.setLanguage(lang);
		} else {
			font.setLanguage(bp.language);
			lex.printError("Unknown language `$$Token'");
		}
	} else if (token == "\\numeric") {
		lex.next();
		font.fontInfo().setNumber(font.setLyXMisc(lex.getString()));
	} else if (token == "\\emph") {
		lex.next();
		font.fontInfo().setEmph(font.setLyXMisc(lex.getString()));
	} else if (token == "\\bar") {
		lex.next();
		string const tok = lex.getString();

		if (tok == "under")
			font.fontInfo().setUnderbar(FONT_ON);
		else if (tok == "no")
			font.fontInfo().setUnderbar(FONT_OFF);
		else if (tok == "default")
			font.fontInfo().setUnderbar(FONT_INHERIT);
		else
			lex.printError("Unknown bar font flag "
				       "`$$Token'");
	} else if (token == "\\noun") {
		lex.next();
		font.fontInfo().setNoun(font.setLyXMisc(lex.getString()));
	} else if (token == "\\color") {
		lex.next();
		setLyXColor(lex.getString(), font.fontInfo());
	} else if (token == "\\SpecialChar") {
			auto_ptr<Inset> inset;
			inset.reset(new InsetSpecialChar);
			inset->read(lex);
			par.insertInset(par.size(), inset.release(),
					font, change);
	} else if (token == "\\backslash") {
		par.appendChar('\\', font, change);
	} else if (token == "\\LyXTable") {
		auto_ptr<Inset> inset(new InsetTabular(const_cast<Buffer &>(buf)));
		inset->read(lex);
		par.insertInset(par.size(), inset.release(), font, change);
	} else if (token == "\\lyxline") {
		par.insertInset(par.size(), new InsetLine, font, change);
	} else if (token == "\\change_unchanged") {
		change = Change(Change::UNCHANGED);
	} else if (token == "\\change_inserted") {
		lex.eatLine();
		istringstream is(lex.getString());
		unsigned int aid;
		time_t ct;
		is >> aid >> ct;
		if (aid >= bp.author_map.size()) {
			errorList.push_back(ErrorItem(_("Change tracking error"),
					    bformat(_("Unknown author index for insertion: %1$d\n"), aid),
					    par.id(), 0, par.size()));
			change = Change(Change::UNCHANGED);
		} else
			change = Change(Change::INSERTED, bp.author_map[aid], ct);
	} else if (token == "\\change_deleted") {
		lex.eatLine();
		istringstream is(lex.getString());
		unsigned int aid;
		time_t ct;
		is >> aid >> ct;
		if (aid >= bp.author_map.size()) {
			errorList.push_back(ErrorItem(_("Change tracking error"),
					    bformat(_("Unknown author index for deletion: %1$d\n"), aid),
					    par.id(), 0, par.size()));
			change = Change(Change::UNCHANGED);
		} else
			change = Change(Change::DELETED, bp.author_map[aid], ct);
	} else {
		lex.eatLine();
		errorList.push_back(ErrorItem(_("Unknown token"),
			bformat(_("Unknown token: %1$s %2$s\n"), from_utf8(token),
			lex.getDocString()),
			par.id(), 0, par.size()));
	}
}


void readParagraph(Buffer const & buf, Paragraph & par, Lexer & lex,
	ErrorList & errorList)
{
	lex.nextToken();
	string token = lex.getString();
	Font font;
	Change change(Change::UNCHANGED);

	while (lex.isOK()) {
		readParToken(buf, par, lex, token, font, change, errorList);

		lex.nextToken();
		token = lex.getString();

		if (token.empty())
			continue;

		if (token == "\\end_layout") {
			//Ok, paragraph finished
			break;
		}

		LYXERR(Debug::PARSER, "Handling paragraph token: `" << token << '\'');
		if (token == "\\begin_layout" || token == "\\end_document"
		    || token == "\\end_inset" || token == "\\begin_deeper"
		    || token == "\\end_deeper") {
			lex.pushToken(token);
			lyxerr << "Paragraph ended in line "
			       << lex.lineNumber() << "\n"
			       << "Missing \\end_layout.\n";
			break;
		}
	}
	// Final change goes to paragraph break:
	par.setChange(par.size(), change);

	// Initialize begin_of_body_ on load; redoParagraph maintains
	par.setBeginOfBody();
}


} // namespace anon

class TextCompletionList : public CompletionList
{
public:
	///
	TextCompletionList(Cursor const & cur)
	: buf_(cur.buffer()), pos_(0) {}
	///
	virtual ~TextCompletionList() {}
	
	///
	virtual bool sorted() const { return true; }
	///
	virtual size_t size() const
	{
		return theWordList().size();
	}
	///
	virtual docstring const & data(size_t idx) const
	{
		return theWordList().word(idx);
	}
	
private:
	///
	Buffer const & buf_;
	///
	size_t pos_;
};


bool Text::empty() const
{
	return pars_.empty() || (pars_.size() == 1 && pars_[0].empty()
		// FIXME: Should we consider the labeled type as empty too? 
		&& pars_[0].layout().labeltype == LABEL_NO_LABEL);
}


double Text::spacing(Buffer const & buffer, Paragraph const & par) const
{
	if (par.params().spacing().isDefault())
		return buffer.params().spacing().getValue();
	return par.params().spacing().getValue();
}


void Text::breakParagraph(Cursor & cur, bool inverse_logic)
{
	LASSERT(this == cur.text(), /**/);

	Paragraph & cpar = cur.paragraph();
	pit_type cpit = cur.pit();

	DocumentClass const & tclass = cur.buffer().params().documentClass();
	Layout const & layout = cpar.layout();

	// this is only allowed, if the current paragraph is not empty
	// or caption and if it has not the keepempty flag active
	if (cur.lastpos() == 0 && !cpar.allowEmpty() &&
	    layout.labeltype != LABEL_SENSITIVE)
		return;

	// a layout change may affect also the following paragraph
	recUndo(cur, cur.pit(), undoSpan(cur.pit()) - 1);

	// Always break behind a space
	// It is better to erase the space (Dekel)
	if (cur.pos() != cur.lastpos() && cpar.isLineSeparator(cur.pos()))
		cpar.eraseChar(cur.pos(), cur.buffer().params().trackChanges);

	// What should the layout for the new paragraph be?
	bool keep_layout = inverse_logic ? 
		!layout.isEnvironment() 
		: layout.isEnvironment();

	// We need to remember this before we break the paragraph, because
	// that invalidates the layout variable
	bool sensitive = layout.labeltype == LABEL_SENSITIVE;

	// we need to set this before we insert the paragraph.
	bool const isempty = cpar.allowEmpty() && cpar.empty();

	lyx::breakParagraph(cur.buffer().params(), paragraphs(), cpit,
			 cur.pos(), keep_layout);

	// After this, neither paragraph contains any rows!

	cpit = cur.pit();
	pit_type next_par = cpit + 1;

	// well this is the caption hack since one caption is really enough
	if (sensitive) {
		if (cur.pos() == 0)
			// set to standard-layout
		//FIXME Check if this should be plainLayout() in some cases
			pars_[cpit].applyLayout(tclass.defaultLayout());
		else
			// set to standard-layout
			//FIXME Check if this should be plainLayout() in some cases
			pars_[next_par].applyLayout(tclass.defaultLayout());
	}

	while (!pars_[next_par].empty() && pars_[next_par].isNewline(0)) {
		if (!pars_[next_par].eraseChar(0, cur.buffer().params().trackChanges))
			break; // the character couldn't be deleted physically due to change tracking
	}

	updateLabels(cur.buffer());

	// A singlePar update is not enough in this case.
	cur.updateFlags(Update::Force);

	// This check is necessary. Otherwise the new empty paragraph will
	// be deleted automatically. And it is more friendly for the user!
	if (cur.pos() != 0 || isempty)
		setCursor(cur, cur.pit() + 1, 0);
	else
		setCursor(cur, cur.pit(), 0);
}


// insert a character, moves all the following breaks in the
// same Paragraph one to the right and make a rebreak
void Text::insertChar(Cursor & cur, char_type c)
{
	LASSERT(this == cur.text(), /**/);

	cur.recordUndo(INSERT_UNDO);

	TextMetrics const & tm = cur.bv().textMetrics(this);
	Buffer const & buffer = cur.buffer();
	Paragraph & par = cur.paragraph();
	// try to remove this
	pit_type const pit = cur.pit();

	bool const freeSpacing = par.layout().free_spacing ||
		par.isFreeSpacing();

	if (lyxrc.auto_number) {
		static docstring const number_operators = from_ascii("+-/*");
		static docstring const number_unary_operators = from_ascii("+-");
		static docstring const number_seperators = from_ascii(".,:");

		if (cur.current_font.fontInfo().number() == FONT_ON) {
			if (!isDigit(c) && !contains(number_operators, c) &&
			    !(contains(number_seperators, c) &&
			      cur.pos() != 0 &&
			      cur.pos() != cur.lastpos() &&
			      tm.displayFont(pit, cur.pos()).fontInfo().number() == FONT_ON &&
			      tm.displayFont(pit, cur.pos() - 1).fontInfo().number() == FONT_ON)
			   )
				number(cur); // Set current_font.number to OFF
		} else if (isDigit(c) &&
			   cur.real_current_font.isVisibleRightToLeft()) {
			number(cur); // Set current_font.number to ON

			if (cur.pos() != 0) {
				char_type const c = par.getChar(cur.pos() - 1);
				if (contains(number_unary_operators, c) &&
				    (cur.pos() == 1
				     || par.isSeparator(cur.pos() - 2)
				     || par.isNewline(cur.pos() - 2))
				  ) {
					setCharFont(buffer, pit, cur.pos() - 1, cur.current_font,
						tm.font_);
				} else if (contains(number_seperators, c)
				     && cur.pos() >= 2
				     && tm.displayFont(pit, cur.pos() - 2).fontInfo().number() == FONT_ON) {
					setCharFont(buffer, pit, cur.pos() - 1, cur.current_font,
						tm.font_);
				}
			}
		}
	}

	// In Bidi text, we want spaces to be treated in a special way: spaces
	// which are between words in different languages should get the 
	// paragraph's language; otherwise, spaces should keep the language 
	// they were originally typed in. This is only in effect while typing;
	// after the text is already typed in, the user can always go back and
	// explicitly set the language of a space as desired. But 99.9% of the
	// time, what we're doing here is what the user actually meant.
	// 
	// The following cases are the ones in which the language of the space
	// should be changed to match that of the containing paragraph. In the
	// depictions, lowercase is LTR, uppercase is RTL, underscore (_) 
	// represents a space, pipe (|) represents the cursor position (so the
	// character before it is the one just typed in). The different cases
	// are depicted logically (not visually), from left to right:
	// 
	// 1. A_a|
	// 2. a_A|
	//
	// Theoretically, there are other situations that we should, perhaps, deal
	// with (e.g.: a|_A, A|_a). In practice, though, there really isn't any 
	// point (to understand why, just try to create this situation...).

	if ((cur.pos() >= 2) && (par.isLineSeparator(cur.pos() - 1))) {
		// get font in front and behind the space in question. But do NOT 
		// use getFont(cur.pos()) because the character c is not inserted yet
		Font const pre_space_font  = tm.displayFont(cur.pit(), cur.pos() - 2);
		Font const & post_space_font = cur.real_current_font;
		bool pre_space_rtl  = pre_space_font.isVisibleRightToLeft();
		bool post_space_rtl = post_space_font.isVisibleRightToLeft();
		
		if (pre_space_rtl != post_space_rtl) {
			// Set the space's language to match the language of the 
			// adjacent character whose direction is the paragraph's
			// direction; don't touch other properties of the font
			Language const * lang = 
				(pre_space_rtl == par.isRTL(buffer.params())) ?
				pre_space_font.language() : post_space_font.language();

			Font space_font = tm.displayFont(cur.pit(), cur.pos() - 1);
			space_font.setLanguage(lang);
			par.setFont(cur.pos() - 1, space_font);
		}
	}
	
	// Next check, if there will be two blanks together or a blank at
	// the beginning of a paragraph.
	// I decided to handle blanks like normal characters, the main
	// difference are the special checks when calculating the row.fill
	// (blank does not count at the end of a row) and the check here

	// When the free-spacing option is set for the current layout,
	// disable the double-space checking
	if (!freeSpacing && isLineSeparatorChar(c)) {
		if (cur.pos() == 0) {
			static bool sent_space_message = false;
			if (!sent_space_message) {
				cur.message(_("You cannot insert a space at the "
							   "beginning of a paragraph. Please read the Tutorial."));
				sent_space_message = true;
			}
			return;
		}
		LASSERT(cur.pos() > 0, /**/);
		if ((par.isLineSeparator(cur.pos() - 1) || par.isNewline(cur.pos() - 1))
		    && !par.isDeleted(cur.pos() - 1)) {
			static bool sent_space_message = false;
			if (!sent_space_message) {
				cur.message(_("You cannot type two spaces this way. "
							   "Please read the Tutorial."));
				sent_space_message = true;
			}
			return;
		}
	}

	par.insertChar(cur.pos(), c, cur.current_font, cur.buffer().params().trackChanges);
	cur.checkBufferStructure();

//		cur.updateFlags(Update::Force);
	setCursor(cur.top(), cur.pit(), cur.pos() + 1);
	charInserted(cur);
}


void Text::charInserted(Cursor & cur)
{
	Paragraph & par = cur.paragraph();

	// Here we call finishUndo for every 20 characters inserted.
	// This is from my experience how emacs does it. (Lgb)
	static unsigned int counter;
	if (counter < 20) {
		++counter;
	} else {
		cur.finishUndo();
		counter = 0;
	}

	// register word if a non-letter was entered
	if (cur.pos() > 1
	    && par.isLetter(cur.pos() - 2)
	    && !par.isLetter(cur.pos() - 1)) {
		// get the word in front of cursor
		LASSERT(this == cur.text(), /**/);
		cur.paragraph().updateWords(cur.top());
	}
}


// the cursor set functions have a special mechanism. When they
// realize, that you left an empty paragraph, they will delete it.

bool Text::cursorForwardOneWord(Cursor & cur)
{
	LASSERT(this == cur.text(), /**/);

	pos_type const lastpos = cur.lastpos();
	pit_type pit = cur.pit();
	pos_type pos = cur.pos();
	Paragraph const & par = cur.paragraph();

	// Paragraph boundary is a word boundary
	if (pos == lastpos) {
		if (pit != cur.lastpit())
			return setCursor(cur, pit + 1, 0);
		else
			return false;
	}

	if (lyxrc.mac_like_word_movement) {
		// Skip through trailing punctuation and spaces.
		while (pos != lastpos && (par.isChar(pos) || par.isSpace(pos)))
                        ++pos;

		// Skip over either a non-char inset or a full word
		if (pos != lastpos && !par.isLetter(pos))
			++pos;
		else while (pos != lastpos && par.isLetter(pos))
			     ++pos;
	} else {
		LASSERT(pos < lastpos, /**/); // see above
		if (par.isLetter(pos))
			while (pos != lastpos && par.isLetter(pos))
				++pos;
		else if (par.isChar(pos))
			while (pos != lastpos && par.isChar(pos))
				++pos;
		else if (!par.isSpace(pos)) // non-char inset
			++pos;

		// Skip over white space
		while (pos != lastpos && par.isSpace(pos))
			     ++pos;		
	}

	return setCursor(cur, pit, pos);
}


bool Text::cursorBackwardOneWord(Cursor & cur)
{
	LASSERT(this == cur.text(), /**/);

	pit_type pit = cur.pit();
	pos_type pos = cur.pos();
	Paragraph & par = cur.paragraph();

	// Paragraph boundary is a word boundary
	if (pos == 0 && pit != 0)
		return setCursor(cur, pit - 1, getPar(pit - 1).size());

	if (lyxrc.mac_like_word_movement) {
		// Skip through punctuation and spaces.
		while (pos != 0 && (par.isChar(pos - 1) || par.isSpace(pos - 1)))
			--pos;

		// Skip over either a non-char inset or a full word
		if (pos != 0 && !par.isLetter(pos - 1) && !par.isChar(pos - 1))
			--pos;
		else while (pos != 0 && par.isLetter(pos - 1))
			     --pos;
	} else {
		// Skip over white space
		while (pos != 0 && par.isSpace(pos - 1))
			     --pos;

		if (pos != 0 && par.isLetter(pos - 1))
			while (pos != 0 && par.isLetter(pos - 1))
				--pos;
		else if (pos != 0 && par.isChar(pos - 1))
			while (pos != 0 && par.isChar(pos - 1))
				--pos;
		else if (pos != 0 && !par.isSpace(pos - 1)) // non-char inset
			--pos;
	}

	return setCursor(cur, pit, pos);
}


bool Text::cursorVisLeftOneWord(Cursor & cur)
{
	LASSERT(this == cur.text(), /**/);

	pos_type left_pos, right_pos;
	bool left_is_letter, right_is_letter;

	Cursor temp_cur = cur;

	// always try to move at least once...
	while (temp_cur.posVisLeft(true /* skip_inset */)) {

		// collect some information about current cursor position
		temp_cur.getSurroundingPos(left_pos, right_pos);
		left_is_letter = 
			(left_pos > -1 ? temp_cur.paragraph().isLetter(left_pos) : false);
		right_is_letter = 
			(right_pos > -1 ? temp_cur.paragraph().isLetter(right_pos) : false);

		// if we're not at a letter/non-letter boundary, continue moving
		if (left_is_letter == right_is_letter)
			continue;

		// we should stop when we have an LTR word on our right or an RTL word
		// on our left
		if ((left_is_letter && temp_cur.paragraph().getFontSettings(
				temp_cur.bv().buffer().params(), 
				left_pos).isRightToLeft())
			|| (right_is_letter && !temp_cur.paragraph().getFontSettings(
				temp_cur.bv().buffer().params(), 
				right_pos).isRightToLeft()))
			break;
	}

	return setCursor(cur, temp_cur.pit(), temp_cur.pos(), 
					 true, temp_cur.boundary());
}


bool Text::cursorVisRightOneWord(Cursor & cur)
{
	LASSERT(this == cur.text(), /**/);

	pos_type left_pos, right_pos;
	bool left_is_letter, right_is_letter;

	Cursor temp_cur = cur;

	// always try to move at least once...
	while (temp_cur.posVisRight(true /* skip_inset */)) {

		// collect some information about current cursor position
		temp_cur.getSurroundingPos(left_pos, right_pos);
		left_is_letter = 
			(left_pos > -1 ? temp_cur.paragraph().isLetter(left_pos) : false);
		right_is_letter = 
			(right_pos > -1 ? temp_cur.paragraph().isLetter(right_pos) : false);

		// if we're not at a letter/non-letter boundary, continue moving
		if (left_is_letter == right_is_letter)
			continue;

		// we should stop when we have an LTR word on our right or an RTL word
		// on our left
		if ((left_is_letter && temp_cur.paragraph().getFontSettings(
				temp_cur.bv().buffer().params(), 
				left_pos).isRightToLeft())
			|| (right_is_letter && !temp_cur.paragraph().getFontSettings(
				temp_cur.bv().buffer().params(), 
				right_pos).isRightToLeft()))
			break;
	}

	return setCursor(cur, temp_cur.pit(), temp_cur.pos(), 
					 true, temp_cur.boundary());
}


void Text::selectWord(Cursor & cur, word_location loc)
{
	LASSERT(this == cur.text(), /**/);
	CursorSlice from = cur.top();
	CursorSlice to = cur.top();
	getWord(from, to, loc);
	if (cur.top() != from)
		setCursor(cur, from.pit(), from.pos());
	if (to == from)
		return;
	cur.resetAnchor();
	setCursor(cur, to.pit(), to.pos());
	cur.setSelection();
}


// Select the word currently under the cursor when no
// selection is currently set
bool Text::selectWordWhenUnderCursor(Cursor & cur, word_location loc)
{
	LASSERT(this == cur.text(), /**/);
	if (cur.selection())
		return false;
	selectWord(cur, loc);
	return cur.selection();
}


void Text::acceptOrRejectChanges(Cursor & cur, ChangeOp op)
{
	LASSERT(this == cur.text(), /**/);

	if (!cur.selection())
		return;

	cur.recordUndoSelection();

	pit_type begPit = cur.selectionBegin().pit();
	pit_type endPit = cur.selectionEnd().pit();

	pos_type begPos = cur.selectionBegin().pos();
	pos_type endPos = cur.selectionEnd().pos();

	// keep selection info, because endPos becomes invalid after the first loop
	bool endsBeforeEndOfPar = (endPos < pars_[endPit].size());

	// first, accept/reject changes within each individual paragraph (do not consider end-of-par)

	for (pit_type pit = begPit; pit <= endPit; ++pit) {
		pos_type parSize = pars_[pit].size();

		// ignore empty paragraphs; otherwise, an assertion will fail for
		// acceptChanges(bparams, 0, 0) or rejectChanges(bparams, 0, 0)
		if (parSize == 0)
			continue;

		// do not consider first paragraph if the cursor starts at pos size()
		if (pit == begPit && begPos == parSize)
			continue;

		// do not consider last paragraph if the cursor ends at pos 0
		if (pit == endPit && endPos == 0)
			break; // last iteration anyway

		pos_type left  = (pit == begPit ? begPos : 0);
		pos_type right = (pit == endPit ? endPos : parSize);

		if (op == ACCEPT) {
			pars_[pit].acceptChanges(cur.buffer().params(), left, right);
		} else {
			pars_[pit].rejectChanges(cur.buffer().params(), left, right);
		}
	}

	// next, accept/reject imaginary end-of-par characters

	for (pit_type pit = begPit; pit <= endPit; ++pit) {
		pos_type pos = pars_[pit].size();

		// skip if the selection ends before the end-of-par
		if (pit == endPit && endsBeforeEndOfPar)
			break; // last iteration anyway

		// skip if this is not the last paragraph of the document
		// note: the user should be able to accept/reject the par break of the last par!
		if (pit == endPit && pit + 1 != int(pars_.size()))
			break; // last iteration anway

		if (op == ACCEPT) {
			if (pars_[pit].isInserted(pos)) {
				pars_[pit].setChange(pos, Change(Change::UNCHANGED));
			} else if (pars_[pit].isDeleted(pos)) {
				if (pit + 1 == int(pars_.size())) {
					// we cannot remove a par break at the end of the last paragraph;
					// instead, we mark it unchanged
					pars_[pit].setChange(pos, Change(Change::UNCHANGED));
				} else {
					mergeParagraph(cur.buffer().params(), pars_, pit);
					--endPit;
					--pit;
				}
			}
		} else {
			if (pars_[pit].isDeleted(pos)) {
				pars_[pit].setChange(pos, Change(Change::UNCHANGED));
			} else if (pars_[pit].isInserted(pos)) {
				if (pit + 1 == int(pars_.size())) {
					// we mark the par break at the end of the last paragraph unchanged
					pars_[pit].setChange(pos, Change(Change::UNCHANGED));
				} else {
					mergeParagraph(cur.buffer().params(), pars_, pit);
					--endPit;
					--pit;
				}
			}
		}
	}

	// finally, invoke the DEPM

	deleteEmptyParagraphMechanism(begPit, endPit, cur.buffer().params().trackChanges);

	//

	cur.finishUndo();
	cur.clearSelection();
	setCursorIntern(cur, begPit, begPos);
	cur.updateFlags(Update::Force);
	updateLabels(cur.buffer());
}


void Text::acceptChanges(BufferParams const & bparams)
{
	lyx::acceptChanges(pars_, bparams);
	deleteEmptyParagraphMechanism(0, pars_.size() - 1, bparams.trackChanges);
}


void Text::rejectChanges(BufferParams const & bparams)
{
	pit_type pars_size = static_cast<pit_type>(pars_.size());

	// first, reject changes within each individual paragraph
	// (do not consider end-of-par)
	for (pit_type pit = 0; pit < pars_size; ++pit) {
		if (!pars_[pit].empty())   // prevent assertion failure
			pars_[pit].rejectChanges(bparams, 0, pars_[pit].size());
	}

	// next, reject imaginary end-of-par characters
	for (pit_type pit = 0; pit < pars_size; ++pit) {
		pos_type pos = pars_[pit].size();

		if (pars_[pit].isDeleted(pos)) {
			pars_[pit].setChange(pos, Change(Change::UNCHANGED));
		} else if (pars_[pit].isInserted(pos)) {
			if (pit == pars_size - 1) {
				// we mark the par break at the end of the last
				// paragraph unchanged
				pars_[pit].setChange(pos, Change(Change::UNCHANGED));
			} else {
				mergeParagraph(bparams, pars_, pit);
				--pit;
				--pars_size;
			}
		}
	}

	// finally, invoke the DEPM
	deleteEmptyParagraphMechanism(0, pars_size - 1, bparams.trackChanges);
}


void Text::deleteWordForward(Cursor & cur)
{
	LASSERT(this == cur.text(), /**/);
	if (cur.lastpos() == 0)
		cursorForward(cur);
	else {
		cur.resetAnchor();
		cur.setSelection(true);
		cursorForwardOneWord(cur);
		cur.setSelection();
		cutSelection(cur, true, false);
		cur.checkBufferStructure();
	}
}


void Text::deleteWordBackward(Cursor & cur)
{
	LASSERT(this == cur.text(), /**/);
	if (cur.lastpos() == 0)
		cursorBackward(cur);
	else {
		cur.resetAnchor();
		cur.setSelection(true);
		cursorBackwardOneWord(cur);
		cur.setSelection();
		cutSelection(cur, true, false);
		cur.checkBufferStructure();
	}
}


// Kill to end of line.
void Text::changeCase(Cursor & cur, TextCase action)
{
	LASSERT(this == cur.text(), /**/);
	CursorSlice from;
	CursorSlice to;

	bool gotsel = false;
	if (cur.selection()) {
		from = cur.selBegin();
		to = cur.selEnd();
		gotsel = true;
	} else {
		from = cur.top();
		getWord(from, to, PARTIAL_WORD);
		cursorForwardOneWord(cur);
	}

	cur.recordUndoSelection();

	pit_type begPit = from.pit();
	pit_type endPit = to.pit();

	pos_type begPos = from.pos();
	pos_type endPos = to.pos();

	pos_type right = 0; // needed after the for loop

	for (pit_type pit = begPit; pit <= endPit; ++pit) {
		Paragraph & par = pars_[pit];
		pos_type const pos = (pit == begPit ? begPos : 0);
		right = (pit == endPit ? endPos : par.size());
		par.changeCase(cur.buffer().params(), pos, right, action);
	}

	// the selection may have changed due to logically-only deleted chars
	if (gotsel) {
		setCursor(cur, begPit, begPos);
		cur.resetAnchor();
		setCursor(cur, endPit, right);
		cur.setSelection();
	} else
		setCursor(cur, endPit, right);

	cur.checkBufferStructure();
}


bool Text::handleBibitems(Cursor & cur)
{
	if (cur.paragraph().layout().labeltype != LABEL_BIBLIO)
		return false;

	if (cur.pos() != 0)
		return false;

	BufferParams const & bufparams = cur.buffer().params();
	Paragraph const & par = cur.paragraph();
	Cursor prevcur = cur;
	if (cur.pit() > 0) {
		--prevcur.pit();
		prevcur.pos() = prevcur.lastpos();
	}
	Paragraph const & prevpar = prevcur.paragraph();

	// if a bibitem is deleted, merge with previous paragraph
	// if this is a bibliography item as well
	if (cur.pit() > 0 && par.layout() == prevpar.layout()) {
		cur.recordUndo(ATOMIC_UNDO, prevcur.pit());
		mergeParagraph(bufparams, cur.text()->paragraphs(),
							prevcur.pit());
		updateLabels(cur.buffer());
		setCursorIntern(cur, prevcur.pit(), prevcur.pos());
		cur.updateFlags(Update::Force);
		return true;
	} 

	// otherwise reset to default
	cur.paragraph().setPlainOrDefaultLayout(bufparams.documentClass());
	return true;
}


bool Text::erase(Cursor & cur)
{
	LASSERT(this == cur.text(), /**/);
	bool needsUpdate = false;
	Paragraph & par = cur.paragraph();

	if (cur.pos() != cur.lastpos()) {
		// this is the code for a normal delete, not pasting
		// any paragraphs
		cur.recordUndo(DELETE_UNDO);
		bool const was_inset = cur.paragraph().isInset(cur.pos());
		if(!par.eraseChar(cur.pos(), cur.buffer().params().trackChanges))
			// the character has been logically deleted only => skip it
			cur.top().forwardPos();

		if (was_inset)
			updateLabels(cur.buffer());
		else
			cur.checkBufferStructure();
		needsUpdate = true;
	} else {
		if (cur.pit() == cur.lastpit())
			return dissolveInset(cur);

		if (!par.isMergedOnEndOfParDeletion(cur.buffer().params().trackChanges)) {
			par.setChange(cur.pos(), Change(Change::DELETED));
			cur.forwardPos();
			needsUpdate = true;
		} else {
			setCursorIntern(cur, cur.pit() + 1, 0);
			needsUpdate = backspacePos0(cur);
		}
	}

	needsUpdate |= handleBibitems(cur);

	if (needsUpdate) {
		// Make sure the cursor is correct. Is this really needed?
		// No, not really... at least not here!
		cur.text()->setCursor(cur.top(), cur.pit(), cur.pos());
		cur.checkBufferStructure();
	}

	return needsUpdate;
}


bool Text::backspacePos0(Cursor & cur)
{
	LASSERT(this == cur.text(), /**/);
	if (cur.pit() == 0)
		return false;

	bool needsUpdate = false;

	BufferParams const & bufparams = cur.buffer().params();
	DocumentClass const & tclass = bufparams.documentClass();
	ParagraphList & plist = cur.text()->paragraphs();
	Paragraph const & par = cur.paragraph();
	Cursor prevcur = cur;
	--prevcur.pit();
	prevcur.pos() = prevcur.lastpos();
	Paragraph const & prevpar = prevcur.paragraph();

	// is it an empty paragraph?
	if (cur.lastpos() == 0
	    || (cur.lastpos() == 1 && par.isSeparator(0))) {
		cur.recordUndo(ATOMIC_UNDO, prevcur.pit(), cur.pit());
		plist.erase(boost::next(plist.begin(), cur.pit()));
		needsUpdate = true;
	}
	// is previous par empty?
	else if (prevcur.lastpos() == 0
		 || (prevcur.lastpos() == 1 && prevpar.isSeparator(0))) {
		cur.recordUndo(ATOMIC_UNDO, prevcur.pit(), cur.pit());
		plist.erase(boost::next(plist.begin(), prevcur.pit()));
		needsUpdate = true;
	}
	// Pasting is not allowed, if the paragraphs have different
	// layouts. I think it is a real bug of all other
	// word processors to allow it. It confuses the user.
	// Correction: Pasting is always allowed with standard-layout
	// or the empty layout.
	else if (par.layout() == prevpar.layout()
		 || tclass.isDefaultLayout(par.layout())
		 || tclass.isPlainLayout(par.layout())) {
		cur.recordUndo(ATOMIC_UNDO, prevcur.pit());
		mergeParagraph(bufparams, plist, prevcur.pit());
		needsUpdate = true;
	}

	if (needsUpdate) {
		updateLabels(cur.buffer());
		setCursorIntern(cur, prevcur.pit(), prevcur.pos());
	}

	return needsUpdate;
}


bool Text::backspace(Cursor & cur)
{
	LASSERT(this == cur.text(), /**/);
	bool needsUpdate = false;
	if (cur.pos() == 0) {
		if (cur.pit() == 0)
			return dissolveInset(cur);

		Paragraph & prev_par = pars_[cur.pit() - 1];

		if (!prev_par.isMergedOnEndOfParDeletion(cur.buffer().params().trackChanges)) {
			prev_par.setChange(prev_par.size(), Change(Change::DELETED));
			setCursorIntern(cur, cur.pit() - 1, prev_par.size());
			return true;
		}
		// The cursor is at the beginning of a paragraph, so
		// the backspace will collapse two paragraphs into one.
		needsUpdate = backspacePos0(cur);

	} else {
		// this is the code for a normal backspace, not pasting
		// any paragraphs
		cur.recordUndo(DELETE_UNDO);
		// We used to do cursorBackwardIntern() here, but it is
		// not a good idea since it triggers the auto-delete
		// mechanism. So we do a cursorBackwardIntern()-lite,
		// without the dreaded mechanism. (JMarc)
		setCursorIntern(cur, cur.pit(), cur.pos() - 1,
				false, cur.boundary());
		bool const was_inset = cur.paragraph().isInset(cur.pos());
		cur.paragraph().eraseChar(cur.pos(), cur.buffer().params().trackChanges);
		if (was_inset)
			updateLabels(cur.buffer());
		else
			cur.checkBufferStructure();
	}

	if (cur.pos() == cur.lastpos())
		cur.setCurrentFont();

	needsUpdate |= handleBibitems(cur);

	// A singlePar update is not enough in this case.
//		cur.updateFlags(Update::Force);
	setCursor(cur.top(), cur.pit(), cur.pos());

	return needsUpdate;
}


bool Text::dissolveInset(Cursor & cur)
{
	LASSERT(this == cur.text(), return false);

	if (isMainText(cur.bv().buffer()) || cur.inset().nargs() != 1)
		return false;

	cur.recordUndoInset();
	cur.setMark(false);
	cur.selHandle(false);
	// save position
	pos_type spos = cur.pos();
	pit_type spit = cur.pit();
	ParagraphList plist;
	if (cur.lastpit() != 0 || cur.lastpos() != 0)
		plist = paragraphs();
	cur.popBackward();
	// store cursor offset
	if (spit == 0)
		spos += cur.pos();
	spit += cur.pit();
	Buffer & b = cur.buffer();
	cur.paragraph().eraseChar(cur.pos(), b.params().trackChanges);
	if (!plist.empty()) {
		// ERT paragraphs have the Language latex_language.
		// This is invalid outside of ERT, so we need to
		// change it to the buffer language.
		ParagraphList::iterator it = plist.begin();
		ParagraphList::iterator it_end = plist.end();
		for (; it != it_end; it++)
			it->changeLanguage(b.params(), latex_language, b.language());

		pasteParagraphList(cur, plist, b.params().documentClassPtr(),
				   b.errorList("Paste"));
		// restore position
		cur.pit() = min(cur.lastpit(), spit);
		cur.pos() = min(cur.lastpos(), spos);
	}
	cur.clearSelection();
	cur.resetAnchor();
	return true;
}


void Text::getWord(CursorSlice & from, CursorSlice & to,
	word_location const loc) const
{
	Paragraph const & from_par = pars_[from.pit()];
	switch (loc) {
	case WHOLE_WORD_STRICT:
		if (from.pos() == 0 || from.pos() == from_par.size()
		    || !from_par.isLetter(from.pos())
		    || !from_par.isLetter(from.pos() - 1)) {
			to = from;
			return;
		}
		// no break here, we go to the next

	case WHOLE_WORD:
		// If we are already at the beginning of a word, do nothing
		if (!from.pos() || !from_par.isLetter(from.pos() - 1))
			break;
		// no break here, we go to the next

	case PREVIOUS_WORD:
		// always move the cursor to the beginning of previous word
		while (from.pos() && from_par.isLetter(from.pos() - 1))
			--from.pos();
		break;
	case NEXT_WORD:
		LYXERR0("Text::getWord: NEXT_WORD not implemented yet");
		break;
	case PARTIAL_WORD:
		// no need to move the 'from' cursor
		break;
	}
	to = from;
	Paragraph const & to_par = pars_[to.pit()];
	while (to.pos() < to_par.size() && to_par.isLetter(to.pos()))
		++to.pos();
}


void Text::write(Buffer const & buf, ostream & os) const
{
	ParagraphList::const_iterator pit = paragraphs().begin();
	ParagraphList::const_iterator end = paragraphs().end();
	depth_type dth = 0;
	for (; pit != end; ++pit)
		pit->write(os, buf.params(), dth);

	// Close begin_deeper
	for(; dth > 0; --dth)
		os << "\n\\end_deeper";
}


bool Text::read(Buffer const & buf, Lexer & lex, 
		ErrorList & errorList, InsetText * insetPtr)
{
	depth_type depth = 0;

	while (lex.isOK()) {
		lex.nextToken();
		string const token = lex.getString();

		if (token.empty())
			continue;

		if (token == "\\end_inset")
			break;

		if (token == "\\end_body")
			continue;

		if (token == "\\begin_body")
			continue;

		if (token == "\\end_document")
			return false;

		if (token == "\\begin_layout") {
			lex.pushToken(token);

			Paragraph par;
			par.setInsetOwner(insetPtr);
			par.params().depth(depth);
			par.setFont(0, Font(inherit_font, buf.params().language));
			pars_.push_back(par);

			// FIXME: goddamn InsetTabular makes us pass a Buffer
			// not BufferParams
			lyx::readParagraph(buf, pars_.back(), lex, errorList);

			// register the words in the global word list
			CursorSlice sl = CursorSlice(*insetPtr);
			sl.pit() = pars_.size() - 1;
			pars_.back().updateWords(sl);
		} else if (token == "\\begin_deeper") {
			++depth;
		} else if (token == "\\end_deeper") {
			if (!depth)
				lex.printError("\\end_deeper: " "depth is already null");
			else
				--depth;
		} else {
			LYXERR0("Handling unknown body token: `" << token << '\'');
		}
	}
	return true;
}

// Returns the current font and depth as a message.
docstring Text::currentState(Cursor const & cur) const
{
	LASSERT(this == cur.text(), /**/);
	Buffer & buf = cur.buffer();
	Paragraph const & par = cur.paragraph();
	odocstringstream os;

	if (buf.params().trackChanges)
		os << _("[Change Tracking] ");

	Change change = par.lookupChange(cur.pos());

	if (change.type != Change::UNCHANGED) {
		Author const & a = buf.params().authors().get(change.author);
		os << _("Change: ") << a.name();
		if (!a.email().empty())
			os << " (" << a.email() << ")";
		// FIXME ctime is english, we should translate that
		os << _(" at ") << ctime(&change.changetime);
		os << " : ";
	}

	// I think we should only show changes from the default
	// font. (Asger)
	// No, from the document font (MV)
	Font font = cur.real_current_font;
	font.fontInfo().reduce(buf.params().getFont().fontInfo());

	os << bformat(_("Font: %1$s"), font.stateText(&buf.params()));

	// The paragraph depth
	int depth = cur.paragraph().getDepth();
	if (depth > 0)
		os << bformat(_(", Depth: %1$d"), depth);

	// The paragraph spacing, but only if different from
	// buffer spacing.
	Spacing const & spacing = par.params().spacing();
	if (!spacing.isDefault()) {
		os << _(", Spacing: ");
		switch (spacing.getSpace()) {
		case Spacing::Single:
			os << _("Single");
			break;
		case Spacing::Onehalf:
			os << _("OneHalf");
			break;
		case Spacing::Double:
			os << _("Double");
			break;
		case Spacing::Other:
			os << _("Other (") << from_ascii(spacing.getValueAsString()) << ')';
			break;
		case Spacing::Default:
			// should never happen, do nothing
			break;
		}
	}

#ifdef DEVEL_VERSION
	os << _(", Inset: ") << &cur.inset();
	os << _(", Paragraph: ") << cur.pit();
	os << _(", Id: ") << par.id();
	os << _(", Position: ") << cur.pos();
	// FIXME: Why is the check for par.size() needed?
	// We are called with cur.pos() == par.size() quite often.
	if (!par.empty() && cur.pos() < par.size()) {
		// Force output of code point, not character
		size_t const c = par.getChar(cur.pos());
		os << _(", Char: 0x") << hex << c;
	}
	os << _(", Boundary: ") << cur.boundary();
//	Row & row = cur.textRow();
//	os << bformat(_(", Row b:%1$d e:%2$d"), row.pos(), row.endpos());
#endif
	return os.str();
}


docstring Text::getPossibleLabel(Cursor const & cur) const
{
	pit_type pit = cur.pit();

	Layout const * layout = &(pars_[pit].layout());

	docstring text;
	docstring par_text = pars_[pit].asString();
	string piece;
	// the return string of math matrices might contain linebreaks
	par_text = subst(par_text, '\n', '-');
	for (int i = 0; i < lyxrc.label_init_length; ++i) {
		if (par_text.empty())
			break;
		docstring head;
		par_text = split(par_text, head, ' ');
		// Is it legal to use spaces in labels ?
		if (i > 0)
			text += '-';
		text += head;
	}

	// No need for a prefix if the user said so.
	if (lyxrc.label_init_length <= 0)
		return text;

	// Will contain the label type.
	docstring name;

	// For section, subsection, etc...
	if (layout->latextype == LATEX_PARAGRAPH && pit != 0) {
		Layout const * layout2 = &(pars_[pit - 1].layout());
		if (layout2->latextype != LATEX_PARAGRAPH) {
			--pit;
			layout = layout2;
		}
	}
	if (layout->latextype != LATEX_PARAGRAPH)
		name = from_ascii(layout->latexname());

	// for captions, we just take the caption type
	Inset * caption_inset = cur.innerInsetOfType(CAPTION_CODE);
	if (caption_inset)
		name = from_ascii(static_cast<InsetCaption *>(caption_inset)->type());

	// If none of the above worked, we'll see if we're inside various
	// types of insets and take our abbreviation from them.
	if (name.empty()) {
		InsetCode const codes[] = {
			FLOAT_CODE,
			WRAP_CODE,
			FOOT_CODE
		};
		for (unsigned int i = 0; i < (sizeof codes / sizeof codes[0]); ++i) {
			Inset * float_inset = cur.innerInsetOfType(codes[i]);
			if (float_inset) {
				name = float_inset->name();
				break;
			}
		}
	}

	// Create a correct prefix for prettyref
	if (name == "theorem")
		name = from_ascii("thm");
	else if (name == "Foot")
		name = from_ascii("fn");
	else if (name == "listing")
		name = from_ascii("lst");

	if (!name.empty())
		text = name.substr(0, 3) + ':' + text;

	return text;
}


docstring Text::asString(int options) const
{
	return asString(0, pars_.size(), options);
}


docstring Text::asString(pit_type beg, pit_type end, int options) const
{
	size_t i = size_t(beg);
	docstring str = pars_[i].asString(options);
	for (++i; i != size_t(end); ++i) {
		str += '\n';
		str += pars_[i].asString(options);
	}
	return str;
}



void Text::charsTranspose(Cursor & cur)
{
	LASSERT(this == cur.text(), /**/);

	pos_type pos = cur.pos();

	// If cursor is at beginning or end of paragraph, do nothing.
	if (pos == cur.lastpos() || pos == 0)
		return;

	Paragraph & par = cur.paragraph();

	// Get the positions of the characters to be transposed.
	pos_type pos1 = pos - 1;
	pos_type pos2 = pos;

	// In change tracking mode, ignore deleted characters.
	while (pos2 < cur.lastpos() && par.isDeleted(pos2))
		++pos2;
	if (pos2 == cur.lastpos())
		return;

	while (pos1 >= 0 && par.isDeleted(pos1))
		--pos1;
	if (pos1 < 0)
		return;

	// Don't do anything if one of the "characters" is not regular text.
	if (par.isInset(pos1) || par.isInset(pos2))
		return;

	// Store the characters to be transposed (including font information).
	char_type const char1 = par.getChar(pos1);
	Font const font1 =
		par.getFontSettings(cur.buffer().params(), pos1);

	char_type const char2 = par.getChar(pos2);
	Font const font2 =
		par.getFontSettings(cur.buffer().params(), pos2);

	// And finally, we are ready to perform the transposition.
	// Track the changes if Change Tracking is enabled.
	bool const trackChanges = cur.buffer().params().trackChanges;

	cur.recordUndo();

	par.eraseChar(pos2, trackChanges);
	par.eraseChar(pos1, trackChanges);
	par.insertChar(pos1, char2, font2, trackChanges);
	par.insertChar(pos2, char1, font1, trackChanges);

	cur.checkBufferStructure();

	// After the transposition, move cursor to after the transposition.
	setCursor(cur, cur.pit(), pos2);
	cur.forwardPos();
}


DocIterator Text::macrocontextPosition() const
{
	return macrocontext_position_;
}


void Text::setMacrocontextPosition(DocIterator const & pos)
{
	macrocontext_position_ = pos;
}


docstring Text::previousWord(CursorSlice const & sl) const
{
	CursorSlice from = sl;
	CursorSlice to = sl;
	getWord(from, to, PREVIOUS_WORD);
	if (sl == from || to == from)
		return docstring();
	
	Paragraph const & par = sl.paragraph();
	return par.asString(from.pos(), to.pos());
}


bool Text::completionSupported(Cursor const & cur) const
{
	Paragraph const & par = cur.paragraph();
	return cur.pos() > 0
		&& (cur.pos() >= par.size() || !par.isLetter(cur.pos()))
		&& par.isLetter(cur.pos() - 1);
}


CompletionList const * Text::createCompletionList(Cursor const & cur) const
{
	return new TextCompletionList(cur);
}


bool Text::insertCompletion(Cursor & cur, docstring const & s, bool /*finished*/)
{	
	LASSERT(cur.bv().cursor() == cur, /**/);
	cur.insert(s);
	cur.bv().cursor() = cur;
	if (!(cur.disp_.update() & Update::Force))
		cur.updateFlags(cur.disp_.update() | Update::SinglePar);
	return true;
}
	
	
docstring Text::completionPrefix(Cursor const & cur) const
{
	return previousWord(cur.top());
}

} // namespace lyx
