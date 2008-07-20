/**
 * \file MathParser.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

/*

If someone desperately needs partial "structures" (such as a few
cells of an array inset or similar) (s)he could uses the
following hack as starting point to write some macros:

  \newif\ifcomment
  \commentfalse
  \ifcomment
	  \def\makeamptab{\catcode`\&=4\relax}
	  \def\makeampletter{\catcode`\&=11\relax}
    \def\b{\makeampletter\expandafter\makeamptab\bi}
    \long\def\bi#1\e{}
  \else
    \def\b{}\def\e{}
  \fi
  ...

  \[\begin{array}{ccc}
1
&

  \end{array}\]

*/


#include <config.h>

#include "MathParser.h"

#include "InsetMathArray.h"
#include "InsetMathBig.h"
#include "InsetMathBrace.h"
#include "InsetMathChar.h"
#include "InsetMathColor.h"
#include "InsetMathComment.h"
#include "InsetMathDelim.h"
#include "InsetMathEnv.h"
#include "InsetMathFrac.h"
#include "InsetMathKern.h"
#include "MathMacro.h"
#include "InsetMathPar.h"
#include "InsetMathRef.h"
#include "InsetMathRoot.h"
#include "InsetMathScript.h"
#include "InsetMathSplit.h"
#include "InsetMathSqrt.h"
#include "InsetMathTabular.h"
#include "MathMacroTemplate.h"
#include "MathFactory.h"
#include "MathMacroArgument.h"
#include "MathSupport.h"

#include "Encoding.h"
#include "Lexer.h"

#include "support/debug.h"
#include "support/convert.h"
#include "support/docstream.h"

#include <sstream>

//#define FILEDEBUG

using namespace std;

namespace lyx {

namespace {

InsetMath::mode_type asMode(InsetMath::mode_type oldmode, docstring const & str)
{
	//lyxerr << "handling mode: '" << str << "'" << endl;
	if (str == "mathmode")
		return InsetMath::MATH_MODE;
	if (str == "textmode" || str == "forcetext")
		return InsetMath::TEXT_MODE;
	return oldmode;
}


bool stared(docstring const & s)
{
	size_t const n = s.size();
	return n && s[n - 1] == '*';
}


/*!
 * Add the row \p cellrow to \p grid.
 * \returns wether the row could be added. Adding a row can fail for
 * environments like "equation" that have a fixed number of rows.
 */
bool addRow(InsetMathGrid & grid, InsetMathGrid::row_type & cellrow,
	    docstring const & vskip, bool allow_newpage_ = true)
{
	++cellrow;
	if (cellrow == grid.nrows()) {
		//lyxerr << "adding row " << cellrow << endl;
		grid.addRow(cellrow - 1);
		if (cellrow == grid.nrows()) {
			// We can't add a row to this grid, so let's
			// append the content of this cell to the previous
			// one.
			// This does not happen in well formed .lyx files,
			// but LyX versions 1.3.x and older could create
			// such files and tex2lyx can still do that.
			--cellrow;
			lyxerr << "ignoring extra row";
			if (!vskip.empty())
				lyxerr << " with extra space " << to_utf8(vskip);
			if (!allow_newpage_)
				lyxerr << " with no page break allowed";
			lyxerr << '.' << endl;
			return false;
		}
	}
	grid.vcrskip(Length(to_utf8(vskip)), cellrow - 1);
	grid.rowinfo(cellrow - 1).allow_newpage_ = allow_newpage_;
	return true;
}


/*!
 * Add the column \p cellcol to \p grid.
 * \returns wether the column could be added. Adding a column can fail for
 * environments like "eqnarray" that have a fixed number of columns.
 */
bool addCol(InsetMathGrid & grid, InsetMathGrid::col_type & cellcol)
{
	++cellcol;
	if (cellcol == grid.ncols()) {
		//lyxerr << "adding column " << cellcol << endl;
		grid.addCol(cellcol);
		if (cellcol == grid.ncols()) {
			// We can't add a column to this grid, so let's
			// append the content of this cell to the previous
			// one.
			// This does not happen in well formed .lyx files,
			// but LyX versions 1.3.x and older could create
			// such files and tex2lyx can still do that.
			--cellcol;
			lyxerr << "ignoring extra column." << endl;
			return false;
		}
	}
	return true;
}


/*!
 * Check wether the last row is empty and remove it if yes.
 * Otherwise the following code
 * \verbatim
\begin{array}{|c|c|}
\hline
1 & 2 \\ \hline
3 & 4 \\ \hline
\end{array}
 * \endverbatim
 * will result in a grid with 3 rows (+ the dummy row that is always present),
 * because the last '\\' opens a new row.
 */
void delEmptyLastRow(InsetMathGrid & grid)
{
	InsetMathGrid::row_type const row = grid.nrows() - 1;
	for (InsetMathGrid::col_type col = 0; col < grid.ncols(); ++col) {
		if (!grid.cell(grid.index(row, col)).empty())
			return;
	}
	// Copy the row information of the empty row (which would contain the
	// last hline in the example above) to the dummy row and delete the
	// empty row.
	grid.rowinfo(row + 1) = grid.rowinfo(row);
	grid.delRow(row);
}


// These are TeX's catcodes
enum CatCode {
	catEscape,     // 0    backslash
	catBegin,      // 1    {
	catEnd,        // 2    }
	catMath,       // 3    $
	catAlign,      // 4    &
	catNewline,    // 5    ^^M
	catParameter,  // 6    #
	catSuper,      // 7    ^
	catSub,        // 8    _
	catIgnore,     // 9
	catSpace,      // 10   space
	catLetter,     // 11   a-zA-Z
	catOther,      // 12   none of the above
	catActive,     // 13   ~
	catComment,    // 14   %
	catInvalid     // 15   <delete>
};

CatCode theCatcode[128];


inline CatCode catcode(char_type c)
{
	/* The only characters that are not catOther lie in the pure ASCII
	 * range. Therefore theCatcode has only 128 entries.
	 * TeX itself deals with 8bit characters, so if needed this table
	 * could be enlarged to 256 entries.
	 * Any larger value does not make sense, since the fact that we use
	 * unicode internally does not change Knuth's TeX engine.
	 * Apart from that a table for the full 21bit UCS4 range would waste
	 * too much memory. */
	if (c >= 128)
		return catOther;

	return theCatcode[c];
}


enum {
	FLAG_ALIGN      = 1 << 0,  //  next & or \\ ends the parsing process
	FLAG_BRACE_LAST = 1 << 1,  //  next closing brace ends the parsing
	FLAG_RIGHT      = 1 << 2,  //  next \\right ends the parsing process
	FLAG_END        = 1 << 3,  //  next \\end ends the parsing process
	FLAG_BRACK_LAST = 1 << 4,  //  next closing bracket ends the parsing
	FLAG_TEXTMODE   = 1 << 5,  //  we are in a box
	FLAG_ITEM       = 1 << 6,  //  read a (possibly braced) token
	FLAG_LEAVE      = 1 << 7,  //  leave the loop at the end
	FLAG_SIMPLE     = 1 << 8,  //  next $ leaves the loop
	FLAG_EQUATION   = 1 << 9,  //  next \] leaves the loop
	FLAG_SIMPLE2    = 1 << 10, //  next \) leaves the loop
	FLAG_OPTION     = 1 << 11, //  read [...] style option
	FLAG_BRACED     = 1 << 12  //  read {...} style argument
};


//
// Helper class for parsing
//

class Token {
public:
	///
	Token() : cs_(), char_(0), cat_(catIgnore) {}
	///
	Token(char_type c, CatCode cat) : cs_(), char_(c), cat_(cat) {}
	///
	explicit Token(docstring const & cs) : cs_(cs), char_(0), cat_(catIgnore) {}

	///
	docstring const & cs() const { return cs_; }
	///
	CatCode cat() const { return cat_; }
	///
	char_type character() const { return char_; }
	///
	docstring asString() const { return cs_.size() ? cs_ : docstring(1, char_); }
	///
	docstring asInput() const { return cs_.size() ? '\\' + cs_ : docstring(1, char_); }

private:
	///
	docstring cs_;
	///
	char_type char_;
	///
	CatCode cat_;
};


ostream & operator<<(ostream & os, Token const & t)
{
	if (t.cs().size()) {
		docstring const & cs = t.cs();
		// FIXME: For some strange reason, the stream operator instanciate
		// a new Token before outputting the contents of t.cs().
		// Because of this the line
		//     os << '\\' << cs;
		// below becomes recursive.
		// In order to avoid that we return early:
		if (cs == "\\")
			return os;
		os << '\\' << to_utf8(cs);
	}
	else if (t.cat() == catLetter)
		os << t.character();
	else
		os << '[' << t.character() << ',' << t.cat() << ']';
	return os;
}


class Parser {
public:
	///
	typedef  InsetMath::mode_type mode_type;

	///
	Parser(Lexer & lex);
	/// Only use this for reading from .lyx file format, for the reason
	/// see Parser::tokenize(istream &).
	Parser(istream & is);
	///
	Parser(docstring const & str);

	///
	bool parse(MathAtom & at);
	///
	void parse(MathData & array, unsigned flags, mode_type mode);
	///
	void parse1(InsetMathGrid & grid, unsigned flags, mode_type mode,
		bool numbered);
	///
	MathData parse(unsigned flags, mode_type mode);
	///
	int lineno() const { return lineno_; }
	///
	void putback();

private:
	///
	void parse2(MathAtom & at, unsigned flags, mode_type mode, bool numbered);
	/// get arg delimited by 'left' and 'right'
	docstring getArg(char_type left, char_type right);
	///
	char_type getChar();
	///
	void error(string const & msg);
	void error(docstring const & msg) { error(to_utf8(msg)); }
	/// dump contents to screen
	void dump() const;
	/// Only use this for reading from .lyx file format (see
	/// implementation for reason)
	void tokenize(istream & is);
	///
	void tokenize(docstring const & s);
	///
	void skipSpaceTokens(idocstream & is, char_type c);
	///
	void push_back(Token const & t);
	///
	void pop_back();
	///
	Token const & prevToken() const;
	///
	Token const & nextToken() const;
	///
	Token const & getToken();
	/// skips spaces if any
	void skipSpaces();
	///
	void lex(docstring const & s);
	///
	bool good() const;
	///
	docstring parse_verbatim_item();
	///
	docstring parse_verbatim_option();

	///
	int lineno_;
	///
	vector<Token> tokens_;
	///
	unsigned pos_;
	/// Stack of active environments
	vector<docstring> environments_;
};


Parser::Parser(Lexer & lexer)
	: lineno_(lexer.lineNumber()), pos_(0)
{
	tokenize(lexer.getStream());
	lexer.eatLine();
}


Parser::Parser(istream & is)
	: lineno_(0), pos_(0)
{
	tokenize(is);
}


Parser::Parser(docstring const & str)
	: lineno_(0), pos_(0)
{
	tokenize(str);
}


void Parser::push_back(Token const & t)
{
	tokens_.push_back(t);
}


void Parser::pop_back()
{
	tokens_.pop_back();
}


Token const & Parser::prevToken() const
{
	static const Token dummy;
	return pos_ > 0 ? tokens_[pos_ - 1] : dummy;
}


Token const & Parser::nextToken() const
{
	static const Token dummy;
	return good() ? tokens_[pos_] : dummy;
}


Token const & Parser::getToken()
{
	static const Token dummy;
	//lyxerr << "looking at token " << tokens_[pos_] << " pos: " << pos_ << endl;
	return good() ? tokens_[pos_++] : dummy;
}


void Parser::skipSpaces()
{
	while (nextToken().cat() == catSpace || nextToken().cat() == catNewline)
		getToken();
}


void Parser::putback()
{
	--pos_;
}


bool Parser::good() const
{
	return pos_ < tokens_.size();
}


char_type Parser::getChar()
{
	if (!good()) {
		error("The input stream is not well...");
		putback();
		return 0;
	}
	return tokens_[pos_++].character();
}


docstring Parser::getArg(char_type left, char_type right)
{
	skipSpaces();

	docstring result;
	char_type c = getChar();

	if (c != left)
		putback();
	else
		while ((c = getChar()) != right && good())
			result += c;

	return result;
}


void Parser::skipSpaceTokens(idocstream & is, char_type c)
{
	// skip trailing spaces
	while (catcode(c) == catSpace || catcode(c) == catNewline)
		if (!is.get(c))
			break;
	//lyxerr << "putting back: " << c << endl;
	is.putback(c);
}


void Parser::tokenize(istream & is)
{
	// eat everything up to the next \end_inset or end of stream
	// and store it in s for further tokenization
	string s;
	char c;
	while (is.get(c)) {
		s += c;
		if (s.size() >= 10 && s.substr(s.size() - 10) == "\\end_inset") {
			s = s.substr(0, s.size() - 10);
			break;
		}
	}
	// Remove the space after \end_inset
	if (is.get(c) && c != ' ')
		is.unget();

	// tokenize buffer
	tokenize(from_utf8(s));
}


void Parser::tokenize(docstring const & buffer)
{
	idocstringstream is(buffer, ios::in | ios::binary);

	char_type c;
	while (is.get(c)) {
		//lyxerr << "reading c: " << c << endl;

		switch (catcode(c)) {
			case catNewline: {
				++lineno_;
				is.get(c);
				if (catcode(c) == catNewline)
					; //push_back(Token("par"));
				else {
					push_back(Token('\n', catNewline));
					is.putback(c);
				}
				break;
			}

/*
			case catComment: {
				while (is.get(c) && catcode(c) != catNewline)
					;
				++lineno_;
				break;
			}
*/

			case catEscape: {
				is.get(c);
				if (!is) {
					error("unexpected end of input");
				} else {
					docstring s(1, c);
					if (catcode(c) == catLetter) {
						// collect letters
						while (is.get(c) && catcode(c) == catLetter)
							s += c;
						skipSpaceTokens(is, c);
					}
					push_back(Token(s));
				}
				break;
			}

			case catSuper:
			case catSub: {
				push_back(Token(c, catcode(c)));
				is.get(c);
				skipSpaceTokens(is, c);
				break;
			}

			case catIgnore: {
				lyxerr << "ignoring a char: " << int(c) << endl;
				break;
			}

			default:
				push_back(Token(c, catcode(c)));
		}
	}

#ifdef FILEDEBUG
	dump();
#endif
}


void Parser::dump() const
{
	lyxerr << "\nTokens: ";
	for (unsigned i = 0; i < tokens_.size(); ++i) {
		if (i == pos_)
			lyxerr << " <#> ";
		lyxerr << tokens_[i];
	}
	lyxerr << " pos: " << pos_ << endl;
}


void Parser::error(string const & msg)
{
	lyxerr << "Line ~" << lineno_ << ": Math parse error: " << msg << endl;
	dump();
	//exit(1);
}


bool Parser::parse(MathAtom & at)
{
	skipSpaces();
	MathData ar;
	parse(ar, false, InsetMath::UNDECIDED_MODE);
	if (ar.size() != 1 || ar.front()->getType() == hullNone) {
		lyxerr << "unusual contents found: " << ar << endl;
		at = MathAtom(new InsetMathPar(ar));
		//if (at->nargs() > 0)
		//	at.nucleus()->cell(0) = ar;
		//else
		//	lyxerr << "unusual contents found: " << ar << endl;
		return true;
	}
	at = ar[0];
	return true;
}


docstring Parser::parse_verbatim_option()
{
	skipSpaces();
	docstring res;
	if (nextToken().character() == '[') {
		Token t = getToken();
		for (Token t = getToken(); t.character() != ']' && good(); t = getToken()) {
			if (t.cat() == catBegin) {
				putback();
				res += '{' + parse_verbatim_item() + '}';
			} else
				res += t.asString();
		}
	}
	return res;
}


docstring Parser::parse_verbatim_item()
{
	skipSpaces();
	docstring res;
	if (nextToken().cat() == catBegin) {
		Token t = getToken();
		for (Token t = getToken(); t.cat() != catEnd && good(); t = getToken()) {
			if (t.cat() == catBegin) {
				putback();
				res += '{' + parse_verbatim_item() + '}';
			}
			else
				res += t.asString();
		}
	}
	return res;
}


MathData Parser::parse(unsigned flags, mode_type mode)
{
	MathData ar;
	parse(ar, flags, mode);
	return ar;
}


void Parser::parse(MathData & array, unsigned flags, mode_type mode)
{
	InsetMathGrid grid(1, 1);
	parse1(grid, flags, mode, false);
	array = grid.cell(0);
}


void Parser::parse2(MathAtom & at, const unsigned flags, const mode_type mode,
	const bool numbered)
{
	parse1(*(at.nucleus()->asGridInset()), flags, mode, numbered);
}


void Parser::parse1(InsetMathGrid & grid, unsigned flags,
	const mode_type mode, const bool numbered)
{
	int limits = 0;
	InsetMathGrid::row_type cellrow = 0;
	InsetMathGrid::col_type cellcol = 0;
	MathData * cell = &grid.cell(grid.index(cellrow, cellcol));

	if (grid.asHullInset())
		grid.asHullInset()->numbered(cellrow, numbered);

	//dump();
	//lyxerr << " flags: " << flags << endl;
	//lyxerr << " mode: " << mode  << endl;
	//lyxerr << "grid: " << grid << endl;

	while (good()) {
		Token const & t = getToken();

#ifdef FILEDEBUG
		lyxerr << "t: " << t << " flags: " << flags << endl;
		lyxerr << "mode: " << mode  << endl;
		cell->dump();
		lyxerr << endl;
#endif

		if (flags & FLAG_ITEM) {

			if (t.cat() == catBegin) {
				// skip the brace and collect everything to the next matching
				// closing brace
				parse1(grid, FLAG_BRACE_LAST, mode, numbered);
				return;
			}

			// handle only this single token, leave the loop if done
			flags = FLAG_LEAVE;
		}


		if (flags & FLAG_BRACED) {
			if (t.cat() == catSpace)
				continue;

			if (t.cat() != catBegin) {
				error("opening brace expected");
				return;
			}

			// skip the brace and collect everything to the next matching
			// closing brace
			flags = FLAG_BRACE_LAST;
		}


		if (flags & FLAG_OPTION) {
			if (t.cat() == catOther && t.character() == '[') {
				MathData ar;
				parse(ar, FLAG_BRACK_LAST, mode);
				cell->append(ar);
			} else {
				// no option found, put back token and we are done
				putback();
			}
			return;
		}

		//
		// cat codes
		//
		if (t.cat() == catMath) {
			if (mode != InsetMath::MATH_MODE) {
				// we are inside some text mode thingy, so opening new math is allowed
				Token const & n = getToken();
				if (n.cat() == catMath) {
					// TeX's $$...$$ syntax for displayed math
					cell->push_back(MathAtom(new InsetMathHull(hullEquation)));
					parse2(cell->back(), FLAG_SIMPLE, InsetMath::MATH_MODE, false);
					getToken(); // skip the second '$' token
				} else {
					// simple $...$  stuff
					putback();
					cell->push_back(MathAtom(new InsetMathHull(hullSimple)));
					parse2(cell->back(), FLAG_SIMPLE, InsetMath::MATH_MODE, false);
				}
			}

			else if (flags & FLAG_SIMPLE) {
				// this is the end of the formula
				return;
			}

			else {
				error("something strange in the parser");
				break;
			}
		}

		else if (t.cat() == catLetter)
			cell->push_back(MathAtom(new InsetMathChar(t.character())));

		else if (t.cat() == catSpace && mode != InsetMath::MATH_MODE) {
			if (cell->empty() || cell->back()->getChar() != ' ')
				cell->push_back(MathAtom(new InsetMathChar(t.character())));
		}

		else if (t.cat() == catNewline && mode != InsetMath::MATH_MODE) {
			if (cell->empty() || cell->back()->getChar() != ' ')
				cell->push_back(MathAtom(new InsetMathChar(' ')));
		}

		else if (t.cat() == catParameter) {
			Token const & n	= getToken();
			cell->push_back(MathAtom(new MathMacroArgument(n.character()-'0')));
		}

		else if (t.cat() == catActive)
			cell->push_back(MathAtom(new InsetMathChar(t.character())));

		else if (t.cat() == catBegin) {
			MathData ar;
			parse(ar, FLAG_BRACE_LAST, mode);
			// do not create a BraceInset if they were written by LyX
			// this helps to keep the annoyance of  "a choose b"  to a minimum
			if (ar.size() == 1 && ar[0]->extraBraces())
				cell->append(ar);
			else
				cell->push_back(MathAtom(new InsetMathBrace(ar)));
		}

		else if (t.cat() == catEnd) {
			if (flags & FLAG_BRACE_LAST)
				return;
			error("found '}' unexpectedly");
			//LASSERT(false, /**/);
			//add(cell, '}', LM_TC_TEX);
		}

		else if (t.cat() == catAlign) {
			//lyxerr << " column now " << (cellcol + 1)
			//       << " max: " << grid.ncols() << endl;
			if (flags & FLAG_ALIGN)
				return;
			if (addCol(grid, cellcol))
				cell = &grid.cell(grid.index(cellrow, cellcol));
		}

		else if (t.cat() == catSuper || t.cat() == catSub) {
			bool up = (t.cat() == catSuper);
			// we need no new script inset if the last thing was a scriptinset,
			// which has that script already not the same script already
			if (!cell->size())
				cell->push_back(MathAtom(new InsetMathScript(up)));
			else if (cell->back()->asScriptInset() &&
					!cell->back()->asScriptInset()->has(up))
				cell->back().nucleus()->asScriptInset()->ensure(up);
			else if (cell->back()->asScriptInset())
				cell->push_back(MathAtom(new InsetMathScript(up)));
			else
				cell->back() = MathAtom(new InsetMathScript(cell->back(), up));
			InsetMathScript * p = cell->back().nucleus()->asScriptInset();
			// special handling of {}-bases
			// Here we could remove the brace inset for things
			// like {a'}^2 and add the braces back in
			// InsetMathScript::write().
			// We do not do it, since it is not possible to detect
			// reliably whether the braces are needed because the
			// nucleus contains more than one symbol, or whether
			// they are needed for unknown commands like \xx{a}_0
			// or \yy{a}{b}_0. This was done in revision 14819
			// in an unreliable way. See this thread
			// http://www.mail-archive.com/lyx-devel%40lists.lyx.org/msg104917.html
			// for more details.
			parse(p->cell(p->idxOfScript(up)), FLAG_ITEM, mode);
			if (limits) {
				p->limits(limits);
				limits = 0;
			}
		}

		else if (t.character() == ']' && (flags & FLAG_BRACK_LAST)) {
			//lyxerr << "finished reading option" << endl;
			return;
		}

		else if (t.cat() == catOther)
			cell->push_back(MathAtom(new InsetMathChar(t.character())));

		else if (t.cat() == catComment) {
			docstring s;
			while (good()) {
				Token const & t = getToken();
				if (t.cat() == catNewline)
					break;
				s += t.asString();
			}
			cell->push_back(MathAtom(new InsetMathComment(s)));
			skipSpaces();
		}

		//
		// control sequences
		//

		else if (t.cs() == "lyxlock") {
			if (cell->size())
				cell->back().nucleus()->lock(true);
		}

		else if ((t.cs() == "global" && nextToken().cs() == "def") ||
			 t.cs() == "def") {
			if (t.cs() == "global")
				getToken();
			
			// get name
			docstring name = getToken().cs();
			
			// read parameters
			int nargs = 0;
			docstring pars;
			while (good() && nextToken().cat() != catBegin) {
				pars += getToken().cs();
				++nargs;
			}
			nargs /= 2;
			
			// read definition
			MathData def;
			parse(def, FLAG_ITEM, InsetMath::UNDECIDED_MODE);
			
			// is a version for display attached?
			skipSpaces();
			MathData display;
			if (nextToken().cat() == catBegin)
				parse(display, FLAG_ITEM, InsetMath::MATH_MODE);
			
			cell->push_back(MathAtom(new MathMacroTemplate(name, nargs,
			       0, MacroTypeDef, vector<MathData>(), def, display)));
		}
		
		else if (t.cs() == "newcommand" ||
			 t.cs() == "renewcommand" ||
			 t.cs() == "newlyxcommand") {
			// get name
			if (getToken().cat() != catBegin) {
				error("'{' in \\newcommand expected (1) ");
				return;
			}
			docstring name = getToken().cs();
			if (getToken().cat() != catEnd) {
				error("'}' in \\newcommand expected");
				return;
			}
				
			// get arity
			docstring const arg = getArg('[', ']');
			int nargs = 0;
			if (!arg.empty())
				nargs = convert<int>(arg);
				
			// optional argument given?
			skipSpaces();
			int optionals = 0;
			vector<MathData> optionalValues;
			while (nextToken().character() == '[') {
				getToken();
				optionalValues.push_back(MathData());
				parse(optionalValues[optionals], FLAG_BRACK_LAST, mode);
				++optionals;
			}
			
			MathData def;
			parse(def, FLAG_ITEM, InsetMath::UNDECIDED_MODE);
			
			// is a version for display attached?
			skipSpaces();
			MathData display;
			if (nextToken().cat() == catBegin)
				parse(display, FLAG_ITEM, InsetMath::MATH_MODE);
			
			cell->push_back(MathAtom(new MathMacroTemplate(name, nargs,
				optionals, MacroTypeNewcommand, optionalValues, def, display)));
			
		}
		
		else if (t.cs() == "newcommandx" ||
			 t.cs() == "renewcommandx") {
			// \newcommandx{\foo}[2][usedefault, addprefix=\global,1=default]{#1,#2}
			// get name
			docstring name;
			if (nextToken().cat() == catBegin) {
				getToken();
				name = getToken().cs();
				if (getToken().cat() != catEnd) {
					error("'}' in \\newcommandx expected");
					return;
				}
			} else
				name = getToken().cs();
				
			// get arity
			docstring const arg = getArg('[', ']');
			if (arg.empty()) {
				error("[num] in \\newcommandx expected");
				return;
			}
			int nargs = convert<int>(arg);
			
			// get options
			int optionals = 0;
			vector<MathData> optionalValues;
			if (nextToken().character() == '[') {
				// skip '['
				getToken();
					
				// handle 'opt=value' options, separated by ','.
				skipSpaces();
				while (nextToken().character() != ']' && good()) {
					if (nextToken().character() >= '1'
					    && nextToken().character() <= '9') {
						// optional value -> get parameter number
						int n = getChar() - '0';
						if (n > nargs) {
							error("Arity of \\newcommandx too low "
							      "for given optional parameter.");
							return;
						}
						
						// skip '='
						if (getToken().character() != '=') {
							error("'=' and optional parameter value "
							      "expected for \\newcommandx");
							return;
						}
						
						// get value
						int optNum = max(size_t(n), optionalValues.size());
						optionalValues.resize(optNum);
						optionalValues[n - 1].clear();
						while (nextToken().character() != ']'
						       && nextToken().character() != ',') {
							MathData data;
							parse(data, FLAG_ITEM, InsetMath::UNDECIDED_MODE);
							optionalValues[n - 1].append(data);
						}
						optionals = max(n, optionals);
					} else if (nextToken().cat() == catLetter) {
						// we in fact ignore every non-optional
						// parameter
						
						// get option name
						docstring opt;
						while (nextToken().cat() == catLetter)
							opt += getChar();
					
						// value?
						skipSpaces();
						MathData value;
						if (nextToken().character() == '=') {
							getToken();
							while (nextToken().character() != ']'
								&& nextToken().character() != ',')
								parse(value, FLAG_ITEM, 
								      InsetMath::UNDECIDED_MODE);
						}
					} else {
						error("option for \\newcommandx expected");
						return;
					}
					
					// skip komma
					skipSpaces();
					if (nextToken().character() == ',') {
						getChar();
						skipSpaces();
					} else if (nextToken().character() != ']') {
						error("Expecting ',' or ']' in options "
						      "of \\newcommandx");
						return;
					}
				}
				
				// skip ']'
				if (!good())
					return;
				getToken();
			}

			// get definition
			MathData def;
			parse(def, FLAG_ITEM, InsetMath::UNDECIDED_MODE);

			// is a version for display attached?
			skipSpaces();
			MathData display;
			if (nextToken().cat() == catBegin)
				parse(display, FLAG_ITEM, InsetMath::MATH_MODE);

			cell->push_back(MathAtom(new MathMacroTemplate(name, nargs,
				optionals, MacroTypeNewcommandx, optionalValues, def, 
				display)));
		}

		else if (t.cs() == "(") {
			cell->push_back(MathAtom(new InsetMathHull(hullSimple)));
			parse2(cell->back(), FLAG_SIMPLE2, InsetMath::MATH_MODE, false);
		}

		else if (t.cs() == "[") {
			cell->push_back(MathAtom(new InsetMathHull(hullEquation)));
			parse2(cell->back(), FLAG_EQUATION, InsetMath::MATH_MODE, false);
		}

		else if (t.cs() == "protect")
			// ignore \\protect, will hopefully be re-added during output
			;

		else if (t.cs() == "end") {
			if (flags & FLAG_END) {
				// eat environment name
				docstring const name = getArg('{', '}');
				if (environments_.empty())
					error("'found \\end{" + name +
					      "}' without matching '\\begin{" +
					      name + "}'");
				else if (name != environments_.back())
					error("'\\end{" + name +
					      "}' does not match '\\begin{" +
					      environments_.back() + "}'");
				else {
					environments_.pop_back();
					// Delete empty last row in matrix
					// like insets.
					// If you abuse InsetMathGrid for
					// non-matrix like structures you
					// probably need to refine this test.
					// Right now we only have to test for
					// single line hull insets.
					if (grid.nrows() > 1)
						delEmptyLastRow(grid);
					return;
				}
			} else
				error("found 'end' unexpectedly");
		}

		else if (t.cs() == ")") {
			if (flags & FLAG_SIMPLE2)
				return;
			error("found '\\)' unexpectedly");
		}

		else if (t.cs() == "]") {
			if (flags & FLAG_EQUATION)
				return;
			error("found '\\]' unexpectedly");
		}

		else if (t.cs() == "\\") {
			if (flags & FLAG_ALIGN)
				return;
			bool added = false;
			if (nextToken().asInput() == "*") {
				getToken();
				added = addRow(grid, cellrow, docstring(), false);
			} else if (good())
				added = addRow(grid, cellrow, getArg('[', ']'));
			else
				error("missing token after \\\\");
			if (added) {
				cellcol = 0;
				if (grid.asHullInset())
					grid.asHullInset()->numbered(
							cellrow, numbered);
				cell = &grid.cell(grid.index(cellrow,
							     cellcol));
			}
		}

#if 0
		else if (t.cs() == "multicolumn") {
			// extract column count and insert dummy cells
			MathData count;
			parse(count, FLAG_ITEM, mode);
			int cols = 1;
			if (!extractNumber(count, cols)) {
				lyxerr << " can't extract number of cells from " << count << endl;
			}
			// resize the table if necessary
			for (int i = 0; i < cols; ++i) {
				if (addCol(grid, cellcol)) {
					cell = &grid.cell(grid.index(
							cellrow, cellcol));
					// mark this as dummy
					grid.cellinfo(grid.index(
						cellrow, cellcol)).dummy_ = true;
				}
			}
			// the last cell is the real thing, not a dummy
			grid.cellinfo(grid.index(cellrow, cellcol)).dummy_ = false;

			// read special alignment
			MathData align;
			parse(align, FLAG_ITEM, mode);
			//grid.cellinfo(grid.index(cellrow, cellcol)).align_ = extractString(align);

			// parse the remaining contents into the "real" cell
			parse(*cell, FLAG_ITEM, mode);
		}
#endif

		else if (t.cs() == "limits")
			limits = 1;

		else if (t.cs() == "nolimits")
			limits = -1;

		else if (t.cs() == "nonumber") {
			if (grid.asHullInset())
				grid.asHullInset()->numbered(cellrow, false);
		}

		else if (t.cs() == "number") {
			if (grid.asHullInset())
				grid.asHullInset()->numbered(cellrow, true);
		}

		else if (t.cs() == "hline") {
			grid.rowinfo(cellrow).lines_ ++;
		}

		else if (t.cs() == "sqrt") {
			MathData ar;
			parse(ar, FLAG_OPTION, mode);
			if (ar.size()) {
				cell->push_back(MathAtom(new InsetMathRoot));
				cell->back().nucleus()->cell(0) = ar;
				parse(cell->back().nucleus()->cell(1), FLAG_ITEM, mode);
			} else {
				cell->push_back(MathAtom(new InsetMathSqrt));
				parse(cell->back().nucleus()->cell(0), FLAG_ITEM, mode);
			}
		}

		else if (t.cs() == "unit") {
			// Allowed formats \unit[val]{unit}
			MathData ar;
			parse(ar, FLAG_OPTION, mode);
			if (ar.size()) {
				cell->push_back(MathAtom(new InsetMathFrac(InsetMathFrac::UNIT)));
				cell->back().nucleus()->cell(0) = ar;
				parse(cell->back().nucleus()->cell(1), FLAG_ITEM, mode);
			} else {
				cell->push_back(MathAtom(new InsetMathFrac(InsetMathFrac::UNIT, 1)));
				parse(cell->back().nucleus()->cell(0), FLAG_ITEM, mode);
			}
		}
		else if (t.cs() == "unitfrac") {
			// Here allowed formats are \unitfrac[val]{num}{denom}
			MathData ar;
			parse(ar, FLAG_OPTION, mode);
			if (ar.size()) {
				cell->push_back(MathAtom(new InsetMathFrac(InsetMathFrac::UNITFRAC, 3)));
				cell->back().nucleus()->cell(2) = ar;
			} else {
				cell->push_back(MathAtom(new InsetMathFrac(InsetMathFrac::UNITFRAC)));
			}
			parse(cell->back().nucleus()->cell(0), FLAG_ITEM, mode);
			parse(cell->back().nucleus()->cell(1), FLAG_ITEM, mode);
		}

		else if (t.cs() == "xrightarrow" || t.cs() == "xleftarrow") {
			cell->push_back(createInsetMath(t.cs()));
			parse(cell->back().nucleus()->cell(1), FLAG_OPTION, mode);
			parse(cell->back().nucleus()->cell(0), FLAG_ITEM, mode);
		}

		else if (t.cs() == "ref" || t.cs() == "eqref" || t.cs() == "prettyref"
			  || t.cs() == "pageref" || t.cs() == "vpageref" || t.cs() == "vref") {
			cell->push_back(MathAtom(new InsetMathRef(t.cs())));
			parse(cell->back().nucleus()->cell(1), FLAG_OPTION, mode);
			parse(cell->back().nucleus()->cell(0), FLAG_ITEM, mode);
		}

		else if (t.cs() == "left") {
			skipSpaces();
			Token const & tl = getToken();
			// \| and \Vert are equivalent, and InsetMathDelim
			// can't handle \|
			// FIXME: fix this in InsetMathDelim itself!
			docstring const l = tl.cs() == "|" ? from_ascii("Vert") : tl.asString();
			MathData ar;
			parse(ar, FLAG_RIGHT, mode);
			if (!good())
				break;
			skipSpaces();
			Token const & tr = getToken();
			docstring const r = tr.cs() == "|" ? from_ascii("Vert") : tr.asString();
			cell->push_back(MathAtom(new InsetMathDelim(l, r, ar)));
		}

		else if (t.cs() == "right") {
			if (flags & FLAG_RIGHT)
				return;
			//lyxerr << "got so far: '" << cell << "'" << endl;
			error("Unmatched right delimiter");
			return;
		}

		else if (t.cs() == "begin") {
			docstring const name = getArg('{', '}');
			environments_.push_back(name);

			if (name == "array" || name == "subarray") {
				docstring const valign = parse_verbatim_option() + 'c';
				docstring const halign = parse_verbatim_item();
				cell->push_back(MathAtom(new InsetMathArray(name,
					InsetMathGrid::guessColumns(halign), 1, (char)valign[0], halign)));
				parse2(cell->back(), FLAG_END, mode, false);
			}

			else if (name == "tabular") {
				docstring const valign = parse_verbatim_option() + 'c';
				docstring const halign = parse_verbatim_item();
				cell->push_back(MathAtom(new InsetMathTabular(name,
					InsetMathGrid::guessColumns(halign), 1, (char)valign[0], halign)));
				parse2(cell->back(), FLAG_END, InsetMath::TEXT_MODE, false);
			}

			else if (name == "split" || name == "cases") {
				cell->push_back(createInsetMath(name));
				parse2(cell->back(), FLAG_END, mode, false);
			}

			else if (name == "alignedat") {
				docstring const valign = parse_verbatim_option() + 'c';
				// ignore this for a while
				getArg('{', '}');
				cell->push_back(MathAtom(new InsetMathSplit(name, (char)valign[0])));
				parse2(cell->back(), FLAG_END, mode, false);
			}

			else if (name == "math") {
				cell->push_back(MathAtom(new InsetMathHull(hullSimple)));
				parse2(cell->back(), FLAG_END, InsetMath::MATH_MODE, true);
			}

			else if (name == "equation" || name == "equation*"
					|| name == "displaymath") {
				cell->push_back(MathAtom(new InsetMathHull(hullEquation)));
				parse2(cell->back(), FLAG_END, InsetMath::MATH_MODE, (name == "equation"));
			}

			else if (name == "eqnarray" || name == "eqnarray*") {
				cell->push_back(MathAtom(new InsetMathHull(hullEqnArray)));
				parse2(cell->back(), FLAG_END, InsetMath::MATH_MODE, !stared(name));
			}

			else if (name == "align" || name == "align*") {
				cell->push_back(MathAtom(new InsetMathHull(hullAlign)));
				parse2(cell->back(), FLAG_END, InsetMath::MATH_MODE, !stared(name));
			}

			else if (name == "flalign" || name == "flalign*") {
				cell->push_back(MathAtom(new InsetMathHull(hullFlAlign)));
				parse2(cell->back(), FLAG_END, InsetMath::MATH_MODE, !stared(name));
			}

			else if (name == "alignat" || name == "alignat*") {
				// ignore this for a while
				getArg('{', '}');
				cell->push_back(MathAtom(new InsetMathHull(hullAlignAt)));
				parse2(cell->back(), FLAG_END, InsetMath::MATH_MODE, !stared(name));
			}

			else if (name == "xalignat" || name == "xalignat*") {
				// ignore this for a while
				getArg('{', '}');
				cell->push_back(MathAtom(new InsetMathHull(hullXAlignAt)));
				parse2(cell->back(), FLAG_END, InsetMath::MATH_MODE, !stared(name));
			}

			else if (name == "xxalignat") {
				// ignore this for a while
				getArg('{', '}');
				cell->push_back(MathAtom(new InsetMathHull(hullXXAlignAt)));
				parse2(cell->back(), FLAG_END, InsetMath::MATH_MODE, !stared(name));
			}

			else if (name == "multline" || name == "multline*") {
				cell->push_back(MathAtom(new InsetMathHull(hullMultline)));
				parse2(cell->back(), FLAG_END, InsetMath::MATH_MODE, !stared(name));
			}

			else if (name == "gather" || name == "gather*") {
				cell->push_back(MathAtom(new InsetMathHull(hullGather)));
				parse2(cell->back(), FLAG_END, InsetMath::MATH_MODE, !stared(name));
			}

			else if (latexkeys const * l = in_word_set(name)) {
				if (l->inset == "matrix") {
					cell->push_back(createInsetMath(name));
					parse2(cell->back(), FLAG_END, mode, false);
				} else if (l->inset == "split") {
					docstring const valign = parse_verbatim_option() + 'c';
					cell->push_back(MathAtom(new InsetMathSplit(name, (char)valign[0])));
					parse2(cell->back(), FLAG_END, mode, false);
				} else {
					dump();
					lyxerr << "found math environment `" << to_utf8(name)
					       << "' in symbols file with unsupported inset `"
					       << to_utf8(l->inset) << "'." << endl;
					// create generic environment inset
					cell->push_back(MathAtom(new InsetMathEnv(name)));
					parse(cell->back().nucleus()->cell(0), FLAG_END, mode);
				}
			}

			else {
				dump();
				lyxerr << "found unknown math environment '" << to_utf8(name)
					<< "'" << endl;
				// create generic environment inset
				cell->push_back(MathAtom(new InsetMathEnv(name)));
				parse(cell->back().nucleus()->cell(0), FLAG_END, mode);
			}
		}

		else if (t.cs() == "kern") {
			// FIXME: A hack...
			docstring s;
			while (true) {
				Token const & t = getToken();
				if (!good()) {
					putback();
					break;
				}
				s += t.character();
				if (isValidLength(to_utf8(s)))
					break;
			}
			cell->push_back(MathAtom(new InsetMathKern(s)));
		}

		else if (t.cs() == "label") {
			// FIXME: This is swallowed in inline formulas
			docstring label = parse_verbatim_item();
			MathData ar;
			asArray(label, ar);
			if (grid.asHullInset()) {
				grid.asHullInset()->label(cellrow, label);
			} else {
				cell->push_back(createInsetMath(t.cs()));
				cell->push_back(MathAtom(new InsetMathBrace(ar)));
			}
		}

		else if (t.cs() == "choose" || t.cs() == "over"
				|| t.cs() == "atop" || t.cs() == "brace"
				|| t.cs() == "brack") {
			MathAtom at = createInsetMath(t.cs());
			at.nucleus()->cell(0) = *cell;
			cell->clear();
			parse(at.nucleus()->cell(1), flags, mode);
			cell->push_back(at);
			return;
		}

		else if (t.cs() == "color") {
			docstring const color = parse_verbatim_item();
			cell->push_back(MathAtom(new InsetMathColor(true, color)));
			parse(cell->back().nucleus()->cell(0), flags, mode);
			return;
		}

		else if (t.cs() == "textcolor") {
			docstring const color = parse_verbatim_item();
			cell->push_back(MathAtom(new InsetMathColor(false, color)));
			parse(cell->back().nucleus()->cell(0), FLAG_ITEM, InsetMath::TEXT_MODE);
		}

		else if (t.cs() == "normalcolor") {
			cell->push_back(createInsetMath(t.cs()));
			parse(cell->back().nucleus()->cell(0), flags, mode);
			return;
		}

		else if (t.cs() == "substack") {
			cell->push_back(createInsetMath(t.cs()));
			parse2(cell->back(), FLAG_ITEM, mode, false);
		}

		else if (t.cs() == "xymatrix") {
			odocstringstream os;
			while (good() && nextToken().cat() != catBegin)
				os << getToken().asInput();
			cell->push_back(createInsetMath(t.cs() + os.str()));
			parse2(cell->back(), FLAG_ITEM, mode, false);
		}

		else if (t.cs() == "framebox" || t.cs() == "makebox") {
			cell->push_back(createInsetMath(t.cs()));
			parse(cell->back().nucleus()->cell(0), FLAG_OPTION, InsetMath::TEXT_MODE);
			parse(cell->back().nucleus()->cell(1), FLAG_OPTION, InsetMath::TEXT_MODE);
			parse(cell->back().nucleus()->cell(2), FLAG_ITEM, InsetMath::TEXT_MODE);
		}

		else if (t.cs() == "tag") {
			if (nextToken().character() == '*') {
				getToken();
				cell->push_back(createInsetMath(t.cs() + '*'));
			} else
				cell->push_back(createInsetMath(t.cs()));
			parse(cell->back().nucleus()->cell(0), FLAG_ITEM, InsetMath::TEXT_MODE);
		}

#if 0
		else if (t.cs() == "infer") {
			MathData ar;
			parse(ar, FLAG_OPTION, mode);
			cell->push_back(createInsetMath(t.cs()));
			parse2(cell->back(), FLAG_ITEM, mode, false);
		}

		// Disabled
		else if (1 && t.cs() == "ar") {
			auto_ptr<InsetMathXYArrow> p(new InsetMathXYArrow);
			// try to read target
			parse(p->cell(0), FLAG_OTPTION, mode);
			// try to read label
			if (nextToken().cat() == catSuper || nextToken().cat() == catSub) {
				p->up_ = nextToken().cat() == catSuper;
				getToken();
				parse(p->cell(1), FLAG_ITEM, mode);
				//lyxerr << "read label: " << p->cell(1) << endl;
			}

			cell->push_back(MathAtom(p.release()));
			//lyxerr << "read cell: " << cell << endl;
		}
#endif

		else if (t.cs() == "lyxmathsym" || t.cs() == "ensuremath") {
			skipSpaces();
			if (getToken().cat() != catBegin) {
				error("'{' expected in \\" + t.cs());
				return;
			}
			int count = 0;
			docstring cmd;
			CatCode cat = nextToken().cat();
			while (good() && (count || cat != catEnd)) {
				if (cat == catBegin)
					++count;
				else if (cat == catEnd)
					--count;
				cmd += getToken().asInput();
				cat = nextToken().cat();
			}
			if (getToken().cat() != catEnd) {
				error("'}' expected in \\" + t.cs());
				return;
			}
			if (t.cs() == "ensuremath") {
				MathData ar;
				mathed_parse_cell(ar, cmd);
				cell->append(ar);
			} else {
				docstring rem;
				cmd = Encodings::fromLaTeXCommand(cmd, rem);
				for (size_t i = 0; i < cmd.size(); ++i)
					cell->push_back(MathAtom(new InsetMathChar(cmd[i])));
				if (rem.size()) {
					MathAtom at = createInsetMath(t.cs());
					cell->push_back(at);
					MathData ar;
					mathed_parse_cell(ar, '{' + rem + '}');
					cell->append(ar);
				}
			}
		}

		else if (t.cs().size()) {
			latexkeys const * l = in_word_set(t.cs());
			if (l) {
				if (l->inset == "big") {
					skipSpaces();
					docstring const delim = getToken().asInput();
					if (InsetMathBig::isBigInsetDelim(delim))
						cell->push_back(MathAtom(
							new InsetMathBig(t.cs(), delim)));
					else {
						cell->push_back(createInsetMath(t.cs()));
						putback();
					}
				}

				else if (l->inset == "font") {
					cell->push_back(createInsetMath(t.cs()));
					parse(cell->back().nucleus()->cell(0),
						FLAG_ITEM, asMode(mode, l->extra));
				}

				else if (l->inset == "oldfont") {
					cell->push_back(createInsetMath(t.cs()));
					parse(cell->back().nucleus()->cell(0),
						flags | FLAG_ALIGN, asMode(mode, l->extra));
					if (prevToken().cat() != catAlign &&
					    prevToken().cs() != "\\")
						return;
					putback();
				}

				else if (l->inset == "style") {
					cell->push_back(createInsetMath(t.cs()));
					parse(cell->back().nucleus()->cell(0),
						flags | FLAG_ALIGN, mode);
					if (prevToken().cat() != catAlign &&
					    prevToken().cs() != "\\")
						return;
					putback();
				}

				else {
					MathAtom at = createInsetMath(t.cs());
					for (InsetMath::idx_type i = 0; i < at->nargs(); ++i)
						parse(at.nucleus()->cell(i),
							FLAG_ITEM, asMode(mode, l->extra));
					cell->push_back(at);
				}
			}

			else {
				bool is_unicode_symbol = false;
				if (mode == InsetMath::TEXT_MODE) {
					int num_tokens = 0;
					docstring cmd = prevToken().asInput();
					skipSpaces();
					CatCode cat = nextToken().cat();
					if (cat == catBegin) {
						int count = 0;
						while (good() && (count || cat != catEnd)) {
							cat = nextToken().cat();
							cmd += getToken().asInput();
							++num_tokens;
							if (cat == catBegin)
								++count;
							else if (cat == catEnd)
								--count;
						}
					}
					bool is_combining;
					char_type c =
						Encodings::fromLaTeXCommand(cmd, is_combining);
					if (is_combining) {
						if (cat == catLetter)
							cmd += '{';
						cmd += getToken().asInput();
						++num_tokens;
						if (cat == catLetter)
							cmd += '}';
						c = Encodings::fromLaTeXCommand(cmd, is_combining);
					}
					if (c) {
						is_unicode_symbol = true;
						cell->push_back(MathAtom(new InsetMathChar(c)));
					} else {
						while (num_tokens--)
							putback();
					}
				}
				if (!is_unicode_symbol) {
					MathAtom at = createInsetMath(t.cs());
					InsetMath::mode_type m = mode;
					//if (m == InsetMath::UNDECIDED_MODE)
					//lyxerr << "default creation: m1: " << m << endl;
					if (at->currentMode() != InsetMath::UNDECIDED_MODE)
						m = at->currentMode();
					//lyxerr << "default creation: m2: " << m << endl;
					InsetMath::idx_type start = 0;
					// this fails on \bigg[...\bigg]
					//MathData opt;
					//parse(opt, FLAG_OPTION, InsetMath::VERBATIM_MODE);
					//if (opt.size()) {
					//	start = 1;
					//	at.nucleus()->cell(0) = opt;
					//}
					for (InsetMath::idx_type i = start; i < at->nargs(); ++i) {
						parse(at.nucleus()->cell(i), FLAG_ITEM, m);
						skipSpaces();
					}
					cell->push_back(at);
				}
			}
		}


		if (flags & FLAG_LEAVE) {
			flags &= ~FLAG_LEAVE;
			break;
		}
	}
}



} // anonymous namespace


void mathed_parse_cell(MathData & ar, docstring const & str)
{
	Parser(str).parse(ar, 0, InsetMath::MATH_MODE);
}


void mathed_parse_cell(MathData & ar, istream & is)
{
	Parser(is).parse(ar, 0, InsetMath::MATH_MODE);
}


bool mathed_parse_normal(MathAtom & t, docstring const & str)
{
	return Parser(str).parse(t);
}


bool mathed_parse_normal(MathAtom & t, Lexer & lex)
{
	return Parser(lex).parse(t);
}


void mathed_parse_normal(InsetMathGrid & grid, docstring const & str)
{
	Parser(str).parse1(grid, 0, InsetMath::MATH_MODE, false);
}


void initParser()
{
	fill(theCatcode, theCatcode + 128, catOther);
	fill(theCatcode + 'a', theCatcode + 'z' + 1, catLetter);
	fill(theCatcode + 'A', theCatcode + 'Z' + 1, catLetter);

	theCatcode[int('\\')] = catEscape;
	theCatcode[int('{')]  = catBegin;
	theCatcode[int('}')]  = catEnd;
	theCatcode[int('$')]  = catMath;
	theCatcode[int('&')]  = catAlign;
	theCatcode[int('\n')] = catNewline;
	theCatcode[int('#')]  = catParameter;
	theCatcode[int('^')]  = catSuper;
	theCatcode[int('_')]  = catSub;
	theCatcode[int(0x7f)] = catIgnore;
	theCatcode[int(' ')]  = catSpace;
	theCatcode[int('\t')] = catSpace;
	theCatcode[int('\r')] = catNewline;
	theCatcode[int('~')]  = catActive;
	theCatcode[int('%')]  = catComment;
}


} // namespace lyx
