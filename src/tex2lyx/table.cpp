/**
 * \file table.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author André Pönitz
 * \author Jean-Marc Lasgouttes
 * \author Georg Baum
 *
 * Full author contact details are available in file CREDITS.
 */

// {[(

#include <config.h>

#include "tex2lyx.h"

#include "support/lassert.h"
#include "support/convert.h"
#include "support/lstrings.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <map>

using namespace std;

namespace lyx {

// filled in preamble.cpp
map<char, int> special_columns;


namespace {

class ColInfo {
public:
	ColInfo() : align('n'), valign('n'), rightlines(0), leftlines(0) {}
	/// column alignment
	char align;
	/// vertical alignment
	char valign;
	/// column width
	string width;
	/// special column alignment
	string special;
	/// number of lines on the right
	int rightlines;
	/// number of lines on the left
	int leftlines;
};


/// row type for longtables
enum LTRowType
{
	/// normal row
	LT_NORMAL,
	/// part of head
	LT_HEAD,
	/// part of head on first page
	LT_FIRSTHEAD,
	/// part of foot
	LT_FOOT,
	/// part of foot on last page
	LT_LASTFOOT
};


class RowInfo {
public:
	RowInfo() : topline(false), bottomline(false), type(LT_NORMAL),
		    caption(false), newpage(false) {}
	/// horizontal line above
	bool topline;
	/// horizontal line below
	bool bottomline;
	/// These are for longtabulars only
	/// row type (head, foot, firsthead etc.)
	LTRowType type;
	/// row for a caption
	bool caption;
	/// row for a newpage
	bool newpage;
};


/// the numeric values are part of the file format!
enum Multicolumn {
	/// A normal cell
	CELL_NORMAL = 0,
	/// A multicolumn cell. The number of columns is <tt>1 + number
	/// of CELL_PART_OF_MULTICOLUMN cells</tt> that follow directly
	CELL_BEGIN_OF_MULTICOLUMN = 1,
	/// This is a dummy cell (part of a multicolumn cell)
	CELL_PART_OF_MULTICOLUMN = 2
};


class CellInfo {
public:
	CellInfo() : multi(CELL_NORMAL), align('n'), valign('n'),
		     leftlines(0), rightlines(0), topline(false),
		     bottomline(false), rotate(false) {}
	/// cell content
	string content;
	/// multicolumn flag
	Multicolumn multi;
	/// cell alignment
	char align;
	/// vertical cell alignment
	char valign;
	/// number of lines on the left
	int leftlines;
	/// number of lines on the right
	int rightlines;
	/// do we have a line above?
	bool topline;
	/// do we have a line below?
	bool bottomline;
	/// is the cell rotated?
	bool rotate;
	/// width for multicolumn cells
	string width;
	/// special formatting for multicolumn cells
	string special;
};


/// translate a horizontal alignment (as stored in ColInfo and CellInfo) to LyX
inline char const * verbose_align(char c)
{
	switch (c) {
	case 'c':
		return "center";
	case 'r':
		return "right";
	case 'l':
		return "left";
	default:
		return "none";
	}
}


/// translate a vertical alignment (as stored in ColInfo and CellInfo) to LyX
inline char const * verbose_valign(char c)
{
	// The default value for no special alignment is "top".
	switch (c) {
	case 'm':
		return "middle";
	case 'b':
		return "bottom";
	case 'p':
	default:
		return "top";
	}
}


// stripped down from tabluar.C. We use it currently only for bools and
// strings
string const write_attribute(string const & name, bool const & b)
{
	// we write only true attribute values so we remove a bit of the
	// file format bloat for tabulars.
	return b ? ' ' + name + "=\"true\"" : string();
}


string const write_attribute(string const & name, string const & s)
{
	return s.empty() ? string() : ' ' + name + "=\"" + s + '"';
}


/*! rather brutish way to code table structure in a string:

\verbatim
  \begin{tabular}{ccc}
    1 & 2 & 3\\ \hline
    \multicolumn{2}{c}{4} & 5 //
    6 & 7 \\
    8 \endhead
  \end{tabular}
\endverbatim

 gets "translated" to:

\verbatim
	 HLINE 1                     TAB 2 TAB 3 HLINE          HLINE LINE
  \hline HLINE \multicolumn{2}{c}{4} TAB 5       HLINE          HLINE LINE
	 HLINE 6                     TAB 7       HLINE          HLINE LINE
	 HLINE 8                                 HLINE \endhead HLINE LINE
\endverbatim
 */

char const TAB   = '\001';
char const LINE  = '\002';
char const HLINE = '\004';


/*!
 * Move the information in leftlines, rightlines, align and valign to the
 * special field. This is necessary if the special field is not empty,
 * because LyX ignores leftlines > 1, rightlines > 1, align and valign in
 * this case.
 */
void ci2special(ColInfo & ci)
{
	if (ci.width.empty() && ci.align == 'n')
		// The alignment setting is already in special, since
		// handle_colalign() never stores ci with these settings
		// and ensures that leftlines == 0 and rightlines == 0 in
		// this case.
		return;

	if (!ci.width.empty()) {
		switch (ci.align) {
		case 'l':
			ci.special += ">{\\raggedright}";
			break;
		case 'r':
			ci.special += ">{\\raggedleft}";
			break;
		case 'c':
			ci.special += ">{\\centering}";
			break;
		}
		if (ci.valign == 'n')
			ci.special += 'p';
		else
			ci.special += ci.valign;
		ci.special += '{' + ci.width + '}';
		ci.width.erase();
	} else
		ci.special += ci.align;

	// LyX can only have one left and one right line.
	for (int i = 1; i < ci.leftlines; ++i)
		ci.special.insert(0, "|");
	for (int i = 1; i < ci.rightlines; ++i)
		ci.special += '|';
	ci.leftlines = min(ci.leftlines, 1);
	ci.rightlines = min(ci.rightlines, 1);
	ci.align = 'n';
	ci.valign = 'n';
}


/*!
 * Handle column specifications for tabulars and multicolumns.
 * The next token of the parser \p p must be an opening brace, and we read
 * everything until the matching closing brace.
 * The resulting column specifications are filled into \p colinfo. This is
 * in an intermediate form. fix_colalign() makes it suitable for LyX output.
 */
void handle_colalign(Parser & p, vector<ColInfo> & colinfo,
		     ColInfo const & start)
{
	if (p.get_token().cat() != catBegin)
		cerr << "Wrong syntax for table column alignment.\n"
			"Expected '{', got '" << p.curr_token().asInput()
		     << "'.\n";

	ColInfo next = start;
	for (Token t = p.get_token(); p.good() && t.cat() != catEnd;
	     t = p.get_token()) {
#ifdef FILEDEBUG
		cerr << "t: " << t << "  c: '" << t.character() << "'\n";
#endif

		// We cannot handle comments here
		if (t.cat() == catComment) {
			if (t.cs().empty()) {
				// "%\n" combination
				p.skip_spaces();
			} else
				cerr << "Ignoring comment: " << t.asInput();
			continue;
		}

		switch (t.character()) {
			case 'c':
			case 'l':
			case 'r':
				// new column, horizontal aligned
				next.align = t.character();
				if (!next.special.empty())
					ci2special(next);
				colinfo.push_back(next);
				next = ColInfo();
				break;
			case 'p':
			case 'b':
			case 'm':
				// new column, vertical aligned box
				next.valign = t.character();
				next.width = p.verbatim_item();
				if (!next.special.empty())
					ci2special(next);
				colinfo.push_back(next);
				next = ColInfo();
				break;
			case '|':
				// vertical rule
				if (colinfo.empty()) {
					if (next.special.empty())
						++next.leftlines;
					else
						next.special += '|';
				} else if (colinfo.back().special.empty())
					++colinfo.back().rightlines;
				else if (next.special.empty())
					++next.leftlines;
				else
					colinfo.back().special += '|';
				break;
			case '>': {
				// text before the next column
				string const s = trim(p.verbatim_item());
				if (next.special.empty() &&
				    next.align == 'n') {
					// Maybe this can be converted to a
					// horizontal alignment setting for
					// fixed width columns
					if (s == "\\raggedleft")
						next.align = 'r';
					else if (s == "\\raggedright")
						next.align = 'l';
					else if (s == "\\centering")
						next.align = 'c';
					else
						next.special = ">{" + s + '}';
				} else
					next.special += ">{" + s + '}';
				break;
			}
			case '<': {
				// text after the last column
				string const s = trim(p.verbatim_item());
				if (colinfo.empty())
					// This is not possible in LaTeX.
					cerr << "Ignoring separator '<{"
					     << s << "}'." << endl;
				else {
					ColInfo & ci = colinfo.back();
					ci2special(ci);
					ci.special += "<{" + s + '}';
				}
				break;
			}
			case '*': {
				// *{n}{arg} means 'n' columns of type 'arg'
				string const num = p.verbatim_item();
				string const arg = p.verbatim_item();
				size_t const n = convert<unsigned int>(num);
				if (!arg.empty() && n > 0) {
					string s("{");
					for (size_t i = 0; i < n; ++i)
						s += arg;
					s += '}';
					Parser p2(s);
					handle_colalign(p2, colinfo, next);
					next = ColInfo();
				} else {
					cerr << "Ignoring column specification"
						" '*{" << num << "}{"
					     << arg << "}'." << endl;
				}
				break;
			}
			case '@':
				// text instead of the column spacing
			case '!':
				// text in addition to the column spacing
				next.special += t.character();
				next.special += '{' + p.verbatim_item() + '}';
				break;
			default:
				// try user defined column types
				if (special_columns.find(t.character()) !=
				    special_columns.end()) {
					ci2special(next);
					next.special += t.character();
					int const nargs =
						special_columns[t.character()];
					for (int i = 0; i < nargs; ++i)
						next.special += '{' +
							p.verbatim_item() +
							'}';
					colinfo.push_back(next);
					next = ColInfo();
				} else
					cerr << "Ignoring column specification"
						" '" << t << "'." << endl;
				break;
			}
	}

	// Maybe we have some column separators that need to be added to the
	// last column?
	ci2special(next);
	if (!next.special.empty()) {
		ColInfo & ci = colinfo.back();
		ci2special(ci);
		ci.special += next.special;
		next.special.erase();
	}
}


/*!
 * Move the left and right lines and alignment settings of the column \p ci
 * to the special field if necessary.
 */
void fix_colalign(ColInfo & ci)
{
	if (ci.leftlines > 1 || ci.rightlines > 1)
		ci2special(ci);
}


/*!
 * LyX can't handle more than one vertical line at the left or right side
 * of a column.
 * This function moves the left and right lines and alignment settings of all
 * columns in \p colinfo to the special field if necessary.
 */
void fix_colalign(vector<ColInfo> & colinfo)
{
	// Try to move extra leftlines to the previous column.
	// We do this only if both special fields are empty, otherwise we
	// can't tell wether the result will be the same.
	for (size_t col = 0; col < colinfo.size(); ++col) {
		if (colinfo[col].leftlines > 1 &&
		    colinfo[col].special.empty() && col > 0 &&
		    colinfo[col - 1].rightlines == 0 &&
		    colinfo[col - 1].special.empty()) {
			++colinfo[col - 1].rightlines;
			--colinfo[col].leftlines;
		}
	}
	// Try to move extra rightlines to the next column
	for (size_t col = 0; col < colinfo.size(); ++col) {
		if (colinfo[col].rightlines > 1 &&
		    colinfo[col].special.empty() &&
		    col < colinfo.size() - 1 &&
		    colinfo[col + 1].leftlines == 0 &&
		    colinfo[col + 1].special.empty()) {
			++colinfo[col + 1].leftlines;
			--colinfo[col].rightlines;
		}
	}
	// Move the lines and alignment settings to the special field if
	// necessary
	for (size_t col = 0; col < colinfo.size(); ++col)
		fix_colalign(colinfo[col]);
}


/*!
 * Parse hlines and similar stuff.
 * \returns wether the token \p t was parsed
 */
bool parse_hlines(Parser & p, Token const & t, string & hlines,
		  bool is_long_tabular)
{
	LASSERT(t.cat() == catEscape, return false);

	if (t.cs() == "hline")
		hlines += "\\hline";

	else if (t.cs() == "cline")
		hlines += "\\cline{" + p.verbatim_item() + '}';

	else if (is_long_tabular && t.cs() == "newpage")
		hlines += "\\newpage";

	else
		return false;

	return true;
}


/// Position in a row
enum RowPosition {
	/// At the very beginning, before the first token
	ROW_START,
	/// After the first token and before any column token
	IN_HLINES_START,
	/// After the first column token. Comments and whitespace are only
	/// treated as tokens in this position
	IN_COLUMNS,
	/// After the first non-column token at the end
	IN_HLINES_END
};


/*!
 * Parse table structure.
 * We parse tables in a two-pass process: This function extracts the table
 * structure (rows, columns, hlines etc.), but does not change the cell
 * content. The cell content is parsed in a second step in handle_tabular().
 */
void parse_table(Parser & p, ostream & os, bool is_long_tabular,
		 RowPosition & pos, unsigned flags)
{
	// table structure commands such as \hline
	string hlines;

	// comments that occur at places where we can't handle them
	string comments;

	while (p.good()) {
		Token const & t = p.get_token();

#ifdef FILEDEBUG
		debugToken(cerr, t, flags);
#endif

		// comments and whitespace in hlines
		switch (pos) {
		case ROW_START:
		case IN_HLINES_START:
		case IN_HLINES_END:
			if (t.cat() == catComment) {
				if (t.cs().empty())
					// line continuation
					p.skip_spaces();
				else
					// We can't handle comments here,
					// store them for later use
					comments += t.asInput();
				continue;
			} else if (t.cat() == catSpace ||
				   t.cat() == catNewline) {
				// whitespace is irrelevant here, we
				// need to recognize hline stuff
				p.skip_spaces();
				continue;
			}
			break;
		case IN_COLUMNS:
			break;
		}

		// We need to handle structure stuff first in order to
		// determine wether we need to output a HLINE separator
		// before the row or not.
		if (t.cat() == catEscape) {
			if (parse_hlines(p, t, hlines, is_long_tabular)) {
				switch (pos) {
				case ROW_START:
					pos = IN_HLINES_START;
					break;
				case IN_COLUMNS:
					pos = IN_HLINES_END;
					break;
				case IN_HLINES_START:
				case IN_HLINES_END:
					break;
				}
				continue;
			}

			else if (t.cs() == "tabularnewline" ||
				 t.cs() == "\\" ||
				 t.cs() == "cr") {
				if (t.cs() == "cr")
					cerr << "Warning: Converting TeX "
						"'\\cr' to LaTeX '\\\\'."
					     << endl;
				// stuff before the line break
				os << comments << HLINE << hlines << HLINE
				   << LINE;
				//cerr << "hlines: " << hlines << endl;
				hlines.erase();
				comments.erase();
				pos = ROW_START;
				continue;
			}

			else if (is_long_tabular &&
				 (t.cs() == "endhead" ||
				  t.cs() == "endfirsthead" ||
				  t.cs() == "endfoot" ||
				  t.cs() == "endlastfoot")) {
				hlines += t.asInput();
				switch (pos) {
				case IN_COLUMNS:
				case IN_HLINES_END:
					// these commands are implicit line
					// breaks
					os << comments << HLINE << hlines
					   << HLINE << LINE;
					hlines.erase();
					comments.erase();
					pos = ROW_START;
					break;
				case ROW_START:
					pos = IN_HLINES_START;
					break;
				case IN_HLINES_START:
					break;
				}
				continue;
			}

		}

		// We need a HLINE separator if we either have no hline
		// stuff at all and are just starting a row or if we just
		// got the first non-hline token.
		switch (pos) {
		case ROW_START:
			// no hline tokens exist, first token at row start
		case IN_HLINES_START:
			// hline tokens exist, first non-hline token at row
			// start
			os << hlines << HLINE << comments;
			hlines.erase();
			comments.erase();
			pos = IN_COLUMNS;
			break;
		case IN_HLINES_END:
			// Oops, there is still cell content after hline
			// stuff. This does not work in LaTeX, so we ignore
			// the hlines.
			cerr << "Ignoring '" << hlines << "' in a cell"
			     << endl;
			os << comments;
			hlines.erase();
			comments.erase();
			pos = IN_COLUMNS;
			break;
		case IN_COLUMNS:
			break;
		}

		// If we come here we have normal cell content
		//
		// cat codes
		//
		if (t.cat() == catMath) {
			// we are inside some text mode thingy, so opening new math is allowed
			Token const & n = p.get_token();
			if (n.cat() == catMath) {
				// TeX's $$...$$ syntax for displayed math
				os << "\\[";
				// This does only work because parse_math outputs TeX
				parse_math(p, os, FLAG_SIMPLE, MATH_MODE);
				os << "\\]";
				p.get_token(); // skip the second '$' token
			} else {
				// simple $...$  stuff
				p.putback();
				os << '$';
				// This does only work because parse_math outputs TeX
				parse_math(p, os, FLAG_SIMPLE, MATH_MODE);
				os << '$';
			}
		}

		else if (t.cat() == catSpace 
			 || t.cat() == catNewline
			 || t.cat() == catLetter 
			 || t.cat() == catSuper 
			 || t.cat() == catSub 
			 || t.cat() == catOther 
			 || t.cat() == catActive 
			 || t.cat() == catParameter)
			os << t.cs();

		else if (t.cat() == catBegin) {
			os << '{';
			parse_table(p, os, is_long_tabular, pos,
				    FLAG_BRACE_LAST);
			os << '}';
		}

		else if (t.cat() == catEnd) {
			if (flags & FLAG_BRACE_LAST)
				return;
			cerr << "unexpected '}'\n";
		}

		else if (t.cat() == catAlign) {
			os << TAB;
			p.skip_spaces();
		}

		else if (t.cat() == catComment)
			os << t.asInput();

		else if (t.cs() == "(") {
			os << "\\(";
			// This does only work because parse_math outputs TeX
			parse_math(p, os, FLAG_SIMPLE2, MATH_MODE);
			os << "\\)";
		}

		else if (t.cs() == "[") {
			os << "\\[";
			// This does only work because parse_math outputs TeX
			parse_math(p, os, FLAG_EQUATION, MATH_MODE);
			os << "\\]";
		}

		else if (t.cs() == "begin") {
			string const name = p.getArg('{', '}');
			active_environments.push_back(name);
			os << "\\begin{" << name << '}';
			// treat the nested environment as a block, don't
			// parse &, \\ etc, because they don't belong to our
			// table if they appear.
			os << p.verbatimEnvironment(name);
			os << "\\end{" << name << '}';
			active_environments.pop_back();
		}

		else if (t.cs() == "end") {
			if (flags & FLAG_END) {
				// eat environment name
				string const name = p.getArg('{', '}');
				if (name != active_environment())
					p.error("\\end{" + name + "} does not match \\begin{"
						+ active_environment() + "}");
				return;
			}
			p.error("found 'end' unexpectedly");
		}

		else
			os << t.asInput();
	}

	// We can have comments if the last line is incomplete
	os << comments;

	// We can have hline stuff if the last line is incomplete
	if (!hlines.empty()) {
		// this does not work in LaTeX, so we ignore it
		cerr << "Ignoring '" << hlines << "' at end of tabular"
		     << endl;
	}
}


void handle_hline_above(RowInfo & ri, vector<CellInfo> & ci)
{
	ri.topline = true;
	for (size_t col = 0; col < ci.size(); ++col)
		ci[col].topline = true;
}


void handle_hline_below(RowInfo & ri, vector<CellInfo> & ci)
{
	ri.bottomline = true;
	for (size_t col = 0; col < ci.size(); ++col)
		ci[col].bottomline = true;
}


} // anonymous namespace


void handle_tabular(Parser & p, ostream & os, bool is_long_tabular,
		    Context & context)
{
	string posopts = p.getOpt();
	if (!posopts.empty()) {
		// FIXME: Convert this to ERT
		if (is_long_tabular)
			cerr << "horizontal longtable";
		else
			cerr << "vertical tabular";
		cerr << " positioning '" << posopts << "' ignored\n";
	}

	vector<ColInfo> colinfo;

	// handle column formatting
	handle_colalign(p, colinfo, ColInfo());
	fix_colalign(colinfo);

	// first scan of cells
	// use table mode to keep it minimal-invasive
	// not exactly what's TeX doing...
	vector<string> lines;
	ostringstream ss;
	RowPosition rowpos = ROW_START;
	parse_table(p, ss, is_long_tabular, rowpos, FLAG_END);
	split(ss.str(), lines, LINE);

	vector< vector<CellInfo> > cellinfo(lines.size());
	vector<RowInfo> rowinfo(lines.size());

	// split into rows
	//cerr << "// split into rows\n";
	for (size_t row = 0; row < rowinfo.size(); ++row) {

		// init row
		cellinfo[row].resize(colinfo.size());

		// split row
		vector<string> dummy;
		//cerr << "\n########### LINE: " << lines[row] << "########\n";
		split(lines[row], dummy, HLINE);

		// handle horizontal line fragments
		// we do only expect this for a last line without '\\'
		if (dummy.size() != 3) {
			if ((dummy.size() != 1 && dummy.size() != 2) ||
			    row != rowinfo.size() - 1)
				cerr << "unexpected dummy size: " << dummy.size()
					<< " content: " << lines[row] << "\n";
			dummy.resize(3);
		}
		lines[row] = dummy[1];

		//cerr << "line: " << row << " above 0: " << dummy[0] << "\n";
		//cerr << "line: " << row << " below 2: " << dummy[2] <<  "\n";
		//cerr << "line: " << row << " cells 1: " << dummy[1] <<  "\n";

		for (int i = 0; i <= 2; i += 2) {
			//cerr << "   reading from line string '" << dummy[i] << "'\n";
			Parser p1(dummy[i]);
			while (p1.good()) {
				Token t = p1.get_token();
				//cerr << "read token: " << t << "\n";
				if (t.cs() == "hline") {
					if (i == 0) {
						if (rowinfo[row].topline) {
							if (row > 0) // extra bottomline above
								handle_hline_below(rowinfo[row - 1], cellinfo[row - 1]);
							else
								cerr << "dropping extra hline\n";
							//cerr << "below row: " << row-1 << endl;
						} else {
							handle_hline_above(rowinfo[row], cellinfo[row]);
							//cerr << "above row: " << row << endl;
						}
					} else {
						//cerr << "below row: " << row << endl;
						handle_hline_below(rowinfo[row], cellinfo[row]);
					}
				} else if (t.cs() == "cline") {
					string arg = p1.verbatim_item();
					//cerr << "read cline arg: '" << arg << "'\n";
					vector<string> t;
					split(arg, t, '-');
					t.resize(2);
					size_t from = convert<unsigned int>(t[0]);
					if (from == 0)
						cerr << "Could not parse "
							"cline start column."
						     << endl;
					else
						// 1 based index -> 0 based
						--from;
					if (from >= colinfo.size()) {
						cerr << "cline starts at non "
							"existing column "
						     << (from + 1) << endl;
						from = colinfo.size() - 1;
					}
					size_t to = convert<unsigned int>(t[1]);
					if (to == 0)
						cerr << "Could not parse "
							"cline end column."
						     << endl;
					else
						// 1 based index -> 0 based
						--to;
					if (to >= colinfo.size()) {
						cerr << "cline ends at non "
							"existing column "
						     << (to + 1) << endl;
						to = colinfo.size() - 1;
					}
					for (size_t col = from; col <= to; ++col) {
						//cerr << "row: " << row << " col: " << col << " i: " << i << endl;
						if (i == 0) {
							rowinfo[row].topline = true;
							cellinfo[row][col].topline = true;
						} else {
							rowinfo[row].bottomline = true;
							cellinfo[row][col].bottomline = true;
						}
					}
				} else if (t.cs() == "endhead") {
					if (i > 0)
						rowinfo[row].type = LT_HEAD;
					for (int r = row - 1; r >= 0; --r) {
						if (rowinfo[r].type != LT_NORMAL)
							break;
						rowinfo[r].type = LT_HEAD;
					}
				} else if (t.cs() == "endfirsthead") {
					if (i > 0)
						rowinfo[row].type = LT_FIRSTHEAD;
					for (int r = row - 1; r >= 0; --r) {
						if (rowinfo[r].type != LT_NORMAL)
							break;
						rowinfo[r].type = LT_FIRSTHEAD;
					}
				} else if (t.cs() == "endfoot") {
					if (i > 0)
						rowinfo[row].type = LT_FOOT;
					for (int r = row - 1; r >= 0; --r) {
						if (rowinfo[r].type != LT_NORMAL)
							break;
						rowinfo[r].type = LT_FOOT;
					}
				} else if (t.cs() == "endlastfoot") {
					if (i > 0)
						rowinfo[row].type = LT_LASTFOOT;
					for (int r = row - 1; r >= 0; --r) {
						if (rowinfo[r].type != LT_NORMAL)
							break;
						rowinfo[r].type = LT_LASTFOOT;
					}
				} else if (t.cs() == "newpage") {
					if (i == 0) {
						if (row > 0)
							rowinfo[row - 1].newpage = true;
						else
							// This does not work in LaTeX
							cerr << "Ignoring "
								"'\\newpage' "
								"before rows."
							     << endl;
					} else
						rowinfo[row].newpage = true;
				} else {
					cerr << "unexpected line token: " << t << endl;
				}
			}
		}

		// split into cells
		vector<string> cells;
		split(lines[row], cells, TAB);
		for (size_t col = 0, cell = 0; cell < cells.size();
		     ++col, ++cell) {
			//cerr << "cell content: '" << cells[cell] << "'\n";
			if (col >= colinfo.size()) {
				// This does not work in LaTeX
				cerr << "Ignoring extra cell '"
				     << cells[cell] << "'." << endl;
				continue;
			}
			Parser p(cells[cell]);
			p.skip_spaces();
			//cells[cell] << "'\n";
			if (p.next_token().cs() == "multicolumn") {
				// how many cells?
				p.get_token();
				size_t const ncells =
					convert<unsigned int>(p.verbatim_item());

				// special cell properties alignment
				vector<ColInfo> t;
				handle_colalign(p, t, ColInfo());
				p.skip_spaces(true);
				ColInfo & ci = t.front();

				// The logic of LyX for multicolumn vertical
				// lines is too complicated to reproduce it
				// here (see LyXTabular::TeXCellPreamble()).
				// Therefore we simply put everything in the
				// special field.
				ci2special(ci);

				cellinfo[row][col].multi      = CELL_BEGIN_OF_MULTICOLUMN;
				cellinfo[row][col].align      = ci.align;
				cellinfo[row][col].special    = ci.special;
				cellinfo[row][col].leftlines  = ci.leftlines;
				cellinfo[row][col].rightlines = ci.rightlines;
				ostringstream os;
				parse_text_in_inset(p, os, FLAG_ITEM, false, context);
				if (!cellinfo[row][col].content.empty()) {
					// This may or may not work in LaTeX,
					// but it does not work in LyX.
					// FIXME: Handle it correctly!
					cerr << "Moving cell content '"
					     << cells[cell]
					     << "' into a multicolumn cell. "
						"This will probably not work."
					     << endl;
				}
				cellinfo[row][col].content += os.str();

				// add dummy cells for multicol
				for (size_t i = 0; i < ncells - 1 && col < colinfo.size(); ++i) {
					++col;
					cellinfo[row][col].multi = CELL_PART_OF_MULTICOLUMN;
					cellinfo[row][col].align = 'c';
				}

			} else if (col == 0 && is_long_tabular &&
			           p.next_token().cs() == "caption") {
				// longtable caption support in LyX is a hack:
				// Captions require a row of their own with
				// the caption flag set to true, having only
				// one multicolumn cell. The contents of that
				// cell must contain exactly one caption inset
				// and nothing else.
				rowinfo[row].caption = true;
				for (size_t c = 1; c < cells.size(); ++c) {
					if (!cells[c].empty()) {
						cerr << "Moving cell content '"
						     << cells[c]
						     << "' into the caption cell. "
							"This will probably not work."
						     << endl;
						cells[0] += cells[c];
					}
				}
				cells.resize(1);
				cellinfo[row][col].align      = colinfo[col].align;
				cellinfo[row][col].multi      = CELL_BEGIN_OF_MULTICOLUMN;
				ostringstream os;
				parse_text_in_inset(p, os, FLAG_CELL, false, context);
				cellinfo[row][col].content += os.str();
				// add dummy multicolumn cells
				for (size_t c = 1; c < colinfo.size(); ++c)
					cellinfo[row][c].multi = CELL_PART_OF_MULTICOLUMN;

			} else {
				cellinfo[row][col].leftlines  = colinfo[col].leftlines;
				cellinfo[row][col].rightlines = colinfo[col].rightlines;
				cellinfo[row][col].align      = colinfo[col].align;
				ostringstream os;
				parse_text_in_inset(p, os, FLAG_CELL, false, context);
				cellinfo[row][col].content += os.str();
			}
		}

		//cerr << "//  handle almost empty last row what we have\n";
		// handle almost empty last row
		if (row && lines[row].empty() && row + 1 == rowinfo.size()) {
			//cerr << "remove empty last line\n";
			if (rowinfo[row].topline)
				rowinfo[row - 1].bottomline = true;
			for (size_t col = 0; col < colinfo.size(); ++col)
				if (cellinfo[row][col].topline)
					cellinfo[row - 1][col].bottomline = true;
			rowinfo.pop_back();
		}
	}

	// Now we have the table structure and content in rowinfo, colinfo
	// and cellinfo.
	// Unfortunately LyX has some limitations that we need to work around.

	// Convert cells with special content to multicolumn cells
	// (LyX ignores the special field for non-multicolumn cells).
	for (size_t row = 0; row < rowinfo.size(); ++row) {
		for (size_t col = 0; col < cellinfo[row].size(); ++col) {
			if (cellinfo[row][col].multi == CELL_NORMAL &&
			    !cellinfo[row][col].special.empty())
				cellinfo[row][col].multi = CELL_BEGIN_OF_MULTICOLUMN;
		}
	}

	// Distribute lines from rows/columns to cells
	// The code was stolen from convert_tablines() in lyx2lyx/lyx_1_6.py.
	// Each standard cell inherits the settings of the corresponding
	// rowinfo/colinfo. This works because all cells with individual
	// settings were converted to multicolumn cells above.
	// Each multicolumn cell inherits the settings of the rowinfo/colinfo
	// corresponding to the first column of the multicolumn cell (default
	// of the multicol package). This works because the special field
	// overrides the line fields.
	for (size_t row = 0; row < rowinfo.size(); ++row) {
		for (size_t col = 0; col < cellinfo[row].size(); ++col) {
			if (cellinfo[row][col].multi == CELL_NORMAL) {
				cellinfo[row][col].topline = rowinfo[row].topline;
				cellinfo[row][col].bottomline = rowinfo[row].bottomline;
				cellinfo[row][col].leftlines = colinfo[col].leftlines;
				cellinfo[row][col].rightlines = colinfo[col].rightlines;
			} else if (cellinfo[row][col].multi == CELL_BEGIN_OF_MULTICOLUMN) {
				size_t s = col + 1;
				while (s < cellinfo[row].size() &&
				       cellinfo[row][s].multi == CELL_PART_OF_MULTICOLUMN)
					s++;
				if (s < cellinfo[row].size() &&
				    cellinfo[row][s].multi != CELL_BEGIN_OF_MULTICOLUMN)
					cellinfo[row][col].rightlines = colinfo[col].rightlines;
				if (col > 0 && cellinfo[row][col-1].multi == CELL_NORMAL)
					cellinfo[row][col].leftlines = colinfo[col].leftlines;
			}
		}
	}

	//cerr << "// output what we have\n";
	// output what we have
	os << "\n<lyxtabular version=\"3\" rows=\"" << rowinfo.size()
	   << "\" columns=\"" << colinfo.size() << "\">\n";
	os << "<features"
	   << write_attribute("rotate", false)
	   << write_attribute("islongtable", is_long_tabular)
	   << ">\n";

	//cerr << "// after header\n";
	for (size_t col = 0; col < colinfo.size(); ++col) {
		os << "<column alignment=\""
		   << verbose_align(colinfo[col].align) << "\""
		   << " valignment=\""
		   << verbose_valign(colinfo[col].valign) << "\""
		   << write_attribute("width", translate_len(colinfo[col].width))
		   << write_attribute("special", colinfo[col].special)
		   << ">\n";
	}
	//cerr << "// after cols\n";

	for (size_t row = 0; row < rowinfo.size(); ++row) {
		os << "<row"
		   << write_attribute("endhead",
				      rowinfo[row].type == LT_HEAD)
		   << write_attribute("endfirsthead",
				      rowinfo[row].type == LT_FIRSTHEAD)
		   << write_attribute("endfoot",
				      rowinfo[row].type == LT_FOOT)
		   << write_attribute("endlastfoot",
				      rowinfo[row].type == LT_LASTFOOT)
		   << write_attribute("newpage", rowinfo[row].newpage)
		   << write_attribute("caption", rowinfo[row].caption)
		   << ">\n";
		for (size_t col = 0; col < colinfo.size(); ++col) {
			CellInfo const & cell = cellinfo[row][col];
			os << "<cell";
			if (cell.multi != CELL_NORMAL)
				os << " multicolumn=\"" << cell.multi << "\"";
			os << " alignment=\"" << verbose_align(cell.align)
			   << "\""
			   << " valignment=\"" << verbose_valign(cell.valign)
			   << "\""
			   << write_attribute("topline", cell.topline)
			   << write_attribute("bottomline", cell.bottomline)
			   << write_attribute("leftline", cell.leftlines > 0)
			   << write_attribute("rightline", cell.rightlines > 0)
			   << write_attribute("rotate", cell.rotate);
			//cerr << "\nrow: " << row << " col: " << col;
			//if (cell.topline)
			//	cerr << " topline=\"true\"";
			//if (cell.bottomline)
			//	cerr << " bottomline=\"true\"";
			os << " usebox=\"none\""
			   << write_attribute("width", translate_len(cell.width));
			if (cell.multi != CELL_NORMAL)
				os << write_attribute("special", cell.special);
			os << ">"
			   << "\n\\begin_inset Text\n"
			   << cell.content
			   << "\n\\end_inset\n"
			   << "</cell>\n";
		}
		os << "</row>\n";
	}

	os << "</lyxtabular>\n";
}




// }])


} // namespace lyx
