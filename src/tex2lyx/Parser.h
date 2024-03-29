// -*- C++ -*-
/**
 * \file Parser.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author André Pönitz
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <utility>
#include <vector>

#include "support/docstream.h"

namespace lyx {


enum mode_type {UNDECIDED_MODE, TEXT_MODE, MATH_MODE, MATHTEXT_MODE, TABLE_MODE};

mode_type asMode(mode_type oldmode, std::string const & str);


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


enum {
	FLAG_BRACE_LAST = 1 << 1,  //  last closing brace ends the parsing
	FLAG_RIGHT      = 1 << 2,  //  next \\right ends the parsing process
	FLAG_END        = 1 << 3,  //  next \\end ends the parsing process
	FLAG_BRACK_LAST = 1 << 4,  //  next closing bracket ends the parsing
	FLAG_TEXTMODE   = 1 << 5,  //  we are in a box
	FLAG_ITEM       = 1 << 6,  //  read a (possibly braced token)
	FLAG_LEAVE      = 1 << 7,  //  leave the loop at the end
	FLAG_SIMPLE     = 1 << 8,  //  next $ leaves the loop
	FLAG_EQUATION   = 1 << 9,  //  next \] leaves the loop
	FLAG_SIMPLE2    = 1 << 10, //  next \) leaves the loop
	FLAG_OPTION     = 1 << 11, //  read [...] style option
	FLAG_BRACED     = 1 << 12, //  read {...} style argument
	FLAG_CELL       = 1 << 13, //  read table cell
	FLAG_TABBING    = 1 << 14  //  We are inside a tabbing environment
};



//
// Helper class for parsing
//

class Token {
public:
	///
	Token() : cs_(), cat_(catIgnore) {}
	///
	Token(docstring const & cs, CatCode cat) : cs_(to_utf8(cs)), cat_(cat) {}

	/// Returns the token as string
	std::string const & cs() const { return cs_; }
	/// Returns the catcode of the token
	CatCode cat() const { return cat_; }
	/** Get the character of tokens that were constructed from a single
	 * character input or a two character input and cat_ == catEscape.
	 * FIXME: The intended usage is not clear. The Token class in
	 *        ../mathed/MathParser.cpp (which is the anchestor of this
	 *        class) uses a separate char member for this method. I
	 *        believe that the intended usage is to not cover tokens with
	 *        catEscape, e.g. \code
	 *        return (cs_.empty() || cat_ == catEscape) ? 0 : cs_[0];
	 *        \endcode
	 *        All usages of this method should be checked. gb 2011-01-05
	 */
	char character() const { return cs_.empty() ? 0 : cs_[0]; }
	/// Returns the token verbatim
	std::string asInput() const;
	/// Is the token an alphanumerical character?
	bool isAlnumASCII() const;

private:
	///
	std::string cs_;
	///
	CatCode cat_;
};

std::ostream & operator<<(std::ostream & os, Token const & t);

#ifdef FILEDEBUG
extern void debugToken(std::ostream & os, Token const & t, unsigned int flags);
#endif


/*!
 * Actual parser class
 *
 * The parser parses every character of the inputstream into a token
 * and classifies the token.
 * The following transformations are done:
 * - Consecutive spaces are combined into one single token with CatCode catSpace
 * - Consecutive newlines are combined into one single token with CatCode catNewline
 * - Comments and %\n combinations are parsed into one token with CatCode catComment
 */

class Parser {
	/// noncopyable
	Parser(Parser const & p);
	Parser & operator=(Parser const & p);
public:
	///
	Parser(idocstream & is);
	///
	Parser(std::string const & s);
	///
	~Parser();

	/// change the latex encoding of the input stream
	void setEncoding(std::string const & encoding);
	/// get the current latex encoding of the input stream
	std::string getEncoding() const { return encoding_latex_; }

	///
	int lineno() const { return lineno_; }
	///
	void putback();
	/// store current position
	void pushPosition();
	/// restore previous position
	void popPosition();
	/// dump contents to screen
	void dump() const;

	/// Does an optional argument follow after the current token?
	bool hasOpt();
	///
	typedef std::pair<bool, std::string> Arg;
	/*!
	 * Get an argument enclosed by \p left and \p right.
	 * \returns wether an argument was found in \p Arg.first and the
	 * argument in \p Arg.second. \see getArg().
	 */
	Arg getFullArg(char left, char right);
	/*!
	 * Get an argument enclosed by \p left and \p right.
	 * \returns the argument (without \p left and \p right) or the empty
	 * string if the next non-space token is not \p left. Use
	 * getFullArg() if you need to know wether there was an empty
	 * argument or no argument at all.
	 */
	std::string getArg(char left, char right);
	/*!
	 * Like getOpt(), but distinguishes between a missing argument ""
	 * and an empty argument "[]".
	 */
	std::string getFullOpt(bool keepws = false);
	/*!
	 * \returns getArg('[', ']') including the brackets or the
	 * empty string if there is no such argument.
	 * No whitespace is eaten if \p keepws is true and no optional
	 * argument exists. This is important if an optional argument is
	 * parsed that would go after a command in ERT: In this case the
	 * whitespace is needed to separate the ERT from the subsequent
	 * word. Without it, the ERT and the next word would be concatenated
	 * during .tex export, thus creating an invalid command.
	 */
	std::string getOpt(bool keepws = false);
	/*!
	 * \returns getFullArg('(', ')') including the parentheses or the
	 * empty string if there is no such argument.
	 */
	std::string getFullParentheseArg();
	/*!
	 * \returns the contents of the environment \p name.
	 * <tt>\begin{name}</tt> must be parsed already, <tt>\end{name}</tt>
	 * is parsed but not returned.
	 */
	std::string const verbatimEnvironment(std::string const & name);
	/*!
	 * Returns the character of the current token and increments
	 * the token position.
	 */
	char getChar();
	///
	void error(std::string const & msg);
	/// Parses one token from \p is 
	void tokenize_one();
	///
	void push_back(Token const & t);
	/// The previous token.
	Token const prev_token() const;
	/// The current token.
	Token const curr_token() const;
	/// The next token.
	Token const next_token();
	/// Make the next token current and return that.
	Token const get_token();
	/// \return whether the current token starts a new paragraph
	bool isParagraph();
	/// skips spaces (and comments if \p skip_comments is true)
	/// \return whether whitespace was skipped (not comments)
	bool skip_spaces(bool skip_comments = false);
	/// puts back spaces (and comments if \p skip_comments is true)
	void unskip_spaces(bool skip_comments = false);
	///
	void lex(std::string const & s);
	///
	bool good();
	///
	std::string verbatim_item();
	///
	std::string verbatimOption();
	/// resets the parser to initial state
	void reset();
	///
	void setCatCode(char c, CatCode cat);
	///
	CatCode getCatCode(char c) const;

private:
	///
	int lineno_;
	///
	std::vector<Token> tokens_;
	///
	unsigned pos_;
	///
	std::vector<unsigned> positions_;
	///
	idocstringstream * iss_;
	///
	idocstream & is_;
	/// latex name of the current encoding
	std::string encoding_latex_;
};



} // namespace lyx

#endif
