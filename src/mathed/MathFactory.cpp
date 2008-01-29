/**
 * \file MathFactory.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "MathFactory.h"

#include "InsetMathAMSArray.h"
#include "InsetMathArray.h"
#include "InsetMathBinom.h"
#include "InsetMathBoldSymbol.h"
#include "InsetMathBoxed.h"
#include "InsetMathBox.h"
#include "InsetMathCases.h"
#include "InsetMathColor.h"
#include "InsetMathDecoration.h"
#include "InsetMathDFrac.h"
#include "InsetMathDots.h"
#include "InsetMathFBox.h"
#include "InsetMathFont.h"
#include "InsetMathFontOld.h"
#include "InsetMathFrac.h"
#include "InsetMathFrameBox.h"
#include "InsetMathKern.h"
#include "InsetMathLefteqn.h"
#include "MathMacro.h"
#include "InsetMathMakebox.h"
#include "InsetMathOverset.h"
#include "InsetMathPhantom.h"
#include "InsetMathRef.h"
#include "InsetMathRoot.h"
#include "InsetMathSize.h"
#include "InsetMathSpace.h"
#include "InsetMathSplit.h"
#include "InsetMathSqrt.h"
#include "InsetMathStackrel.h"
#include "InsetMathSubstack.h"
#include "InsetMathSymbol.h"
#include "InsetMathTabular.h"
#include "InsetMathTFrac.h"
#include "InsetMathUnderset.h"
#include "InsetMathUnknown.h"
#include "InsetMathXArrow.h"
#include "InsetMathXYMatrix.h"
#include "MathMacroArgument.h"
#include "MacroTable.h"
#include "MathParser.h"
#include "MathSupport.h"

#include "debug.h"

#include "insets/InsetCommand.h"

#include "support/filetools.h" // LibFileSearch
#include "support/lstrings.h"

#include "frontends/FontLoader.h"


namespace lyx {

using support::libFileSearch;
using support::split;

using std::string;
using std::endl;
using std::istringstream;
using std::vector;

bool has_math_fonts;


namespace {

// file scope
typedef std::map<docstring, latexkeys> WordList;

WordList theWordList;


bool math_font_available(docstring & name)
{
	Font f;
	augmentFont(f, name);

	// Do we have the font proper?
	if (theFontLoader().available(f))
		return true;

	// can we fake it?
	if (name == "eufrak") {
		name = from_ascii("lyxfakefrak");
		return true;
	}

	LYXERR(Debug::MATHED)
		<< "font " << to_utf8(name) << " not available and I can't fake it"
		<< endl;
	return false;
}


void initSymbols()
{
	support::FileName const filename = libFileSearch(string(), "symbols");
	LYXERR(Debug::MATHED) << "read symbols from " << filename << endl;
	if (filename.empty()) {
		lyxerr << "Could not find symbols file" << endl;
		return;
	}

	std::ifstream fs(filename.toFilesystemEncoding().c_str());
	string line;
	bool skip = false;
	while (getline(fs, line)) {
		int charid     = 0;
		int fallbackid = 0;
		if (line.empty() || line[0] == '#')
			continue;

		// special case of iffont/else/endif
		if (line.size() >= 7 && line.substr(0, 6) == "iffont") {
			istringstream is(line);
			string tmp;
			is >> tmp;
			is >> tmp;
			docstring t = from_utf8(tmp);
			skip = !math_font_available(t);
			continue;
		} else if (line.size() >= 4 && line.substr(0, 4) == "else") {
			skip = !skip;
		} else if (line.size() >= 5 && line.substr(0, 5) == "endif") {
			skip = false;
			continue;
		} else if (skip)
			continue;

		// special case of pre-defined macros
		if (line.size() > 8 && line.substr(0, 5) == "\\def\\") {
			//lyxerr << "macro definition: '" << line << '\'' << endl;
			istringstream is(line);
			string macro;
			string requires;
			is >> macro >> requires;
			MacroTable::globalMacros().insert(from_utf8(macro), requires);
			continue;
		}

		idocstringstream is(from_utf8(line));
		latexkeys tmp;
		is >> tmp.name >> tmp.inset;
		if (isFontName(tmp.inset))
			is >> charid >> fallbackid >> tmp.extra >> tmp.xmlname;
		else
			is >> tmp.extra;
		// requires is optional
		if (is)
			is >> tmp.requires;
		else {
			LYXERR(Debug::MATHED) << "skipping line '" << line << '\'' << endl;
			LYXERR(Debug::MATHED)
				<< to_utf8(tmp.name) << ' ' << to_utf8(tmp.inset) << ' ' << to_utf8(tmp.extra) << endl;
			continue;
		}

		if (isFontName(tmp.inset)) {
			// tmp.inset _is_ the fontname here.
			// create fallbacks if necessary

			// store requirements as long as we can
			if (tmp.requires.empty()) {
				if (tmp.inset == "msa" || tmp.inset == "msb")
					tmp.requires = from_ascii("amssymb");
				else if (tmp.inset == "wasy")
					tmp.requires = from_ascii("wasysym");
			}

			// symbol font is not available sometimes
			docstring symbol_font = from_ascii("lyxsymbol");

			if (tmp.extra == "func" || tmp.extra == "funclim" || tmp.extra == "special") {
				LYXERR(Debug::MATHED) << "symbol abuse for " << to_utf8(tmp.name) << endl;
				tmp.draw = tmp.name;
			} else if (math_font_available(tmp.inset)) {
				LYXERR(Debug::MATHED) << "symbol available for " << to_utf8(tmp.name) << endl;
				tmp.draw.push_back(char_type(charid));
			} else if (fallbackid && math_font_available(symbol_font)) {
				if (tmp.inset == "cmex")
					tmp.inset = from_ascii("lyxsymbol");
				else
					tmp.inset = from_ascii("lyxboldsymbol");
				LYXERR(Debug::MATHED) << "symbol fallback for " << to_utf8(tmp.name) << endl;
				tmp.draw.push_back(char_type(fallbackid));
			} else {
				LYXERR(Debug::MATHED) << "faking " << to_utf8(tmp.name) << endl;
				tmp.draw = tmp.name;
				tmp.inset = from_ascii("lyxtex");
			}
		} else {
			// it's a proper inset
			LYXERR(Debug::MATHED) << "inset " << to_utf8(tmp.inset)
					      << " used for " << to_utf8(tmp.name)
					      << endl;
		}

		if (theWordList.find(tmp.name) != theWordList.end())
			LYXERR(Debug::MATHED)
				<< "readSymbols: inset " << to_utf8(tmp.name)
				<< " already exists." << endl;
		else
			theWordList[tmp.name] = tmp;

		LYXERR(Debug::MATHED)
			<< "read symbol '" << to_utf8(tmp.name)
			<< "  inset: " << to_utf8(tmp.inset)
			<< "  draw: " << int(tmp.draw.empty() ? 0 : tmp.draw[0])
			<< "  extra: " << to_utf8(tmp.extra)
			<< "  requires: " << to_utf8(tmp.requires)
			<< '\'' << endl;
	}
	docstring tmp = from_ascii("cmm");
	docstring tmp2 = from_ascii("cmsy");
	has_math_fonts = math_font_available(tmp) && math_font_available(tmp2);
}


} // namespace anon


void initMath()
{
	static bool initialized = false;
	if (!initialized) {
		initialized = true;
		initParser();
		initSymbols();
	}
}


latexkeys const * in_word_set(docstring const & str)
{
	WordList::iterator it = theWordList.find(str);
	return it != theWordList.end() ? &(it->second) : 0;
}


MathAtom createInsetMath(char const * const s)
{
	return createInsetMath(from_utf8(s));
}


MathAtom createInsetMath(docstring const & s)
{
	//lyxerr << "creating inset with name: '" << s << '\'' << endl;
	latexkeys const * l = in_word_set(s);
	if (l) {
		docstring const & inset = l->inset;
		//lyxerr << " found inset: '" << inset << '\'' << endl;
		if (inset == "ref")
			return MathAtom(new InsetMathRef(l->name));
		if (inset == "overset")
			return MathAtom(new InsetMathOverset);
		if (inset == "underset")
			return MathAtom(new InsetMathUnderset);
		if (inset == "decoration")
			return MathAtom(new InsetMathDecoration(l));
		if (inset == "space")
			return MathAtom(new InsetMathSpace(l->name));
		if (inset == "dots")
			return MathAtom(new InsetMathDots(l));
		if (inset == "mbox")
			// return MathAtom(new InsetMathMBox);
			// InsetMathMBox is proposed to replace InsetMathBox,
			// but is not ready yet (it needs a BufferView for
			// construction)
			return MathAtom(new InsetMathBox(l->name));
//		if (inset == "fbox")
//			return MathAtom(new InsetMathFBox(l));
		if (inset == "style")
			return MathAtom(new InsetMathSize(l));
		if (inset == "font")
			return MathAtom(new InsetMathFont(l));
		if (inset == "oldfont")
			return MathAtom(new InsetMathFontOld(l));
		if (inset == "matrix")
			return MathAtom(new InsetMathAMSArray(s));
		if (inset == "split")
			return MathAtom(new InsetMathSplit(s));
		if (inset == "big")
			// we can't create a InsetMathBig, since the argument
			// is missing.
			return MathAtom(new InsetMathUnknown(s));
		return MathAtom(new InsetMathSymbol(l));
	}

	if (s.size() == 2 && s[0] == '#' && s[1] >= '1' && s[1] <= '9')
		return MathAtom(new MathMacroArgument(s[1] - '0'));
	if (s.size() == 3 && s[0] == '\\' && s[1] == '#'
			&& s[2] >= '1' && s[2] <= '9')
		return MathAtom(new MathMacroArgument(s[2] - '0'));
	if (s == "boxed")
		return MathAtom(new InsetMathBoxed());
	if (s == "fbox")
		return MathAtom(new InsetMathFBox());
	if (s == "framebox")
		return MathAtom(new InsetMathFrameBox);
	if (s == "makebox")
		return MathAtom(new InsetMathMakebox);
	if (s == "kern")
		return MathAtom(new InsetMathKern);
	if (s.substr(0, 8) == "xymatrix") {
		char spacing_code = '\0';
		Length spacing;
		size_t const len = s.length();
		size_t i = 8;
		if (i < len && s[i] == '@') {
			++i;
			if (i < len) {
				switch (s[i]) {
				case 'R':
				case 'C':
				case 'M':
				case 'W':
				case 'H':
				case 'L':
					spacing_code = static_cast<char>(s[i]);
					++i;
					break;
				}
			}
			if (i < len && s[i] == '=') {
				++i;
				spacing = Length(to_ascii(s.substr(i)));
			}
		}
		return MathAtom(new InsetMathXYMatrix(spacing, spacing_code));
	}
	if (s == "xrightarrow" || s == "xleftarrow")
		return MathAtom(new InsetMathXArrow(s));
	if (s == "split" || s == "gathered" || s == "aligned" || s == "alignedat")
		return MathAtom(new InsetMathSplit(s));
	if (s == "cases")
		return MathAtom(new InsetMathCases);
	if (s == "substack")
		return MathAtom(new InsetMathSubstack);
	if (s == "subarray" || s == "array")
		return MathAtom(new InsetMathArray(s, 1, 1));
	if (s == "sqrt")
		return MathAtom(new InsetMathSqrt);
	if (s == "root")
		return MathAtom(new InsetMathRoot);
	if (s == "tabular")
		return MathAtom(new InsetMathTabular(s, 1, 1));
	if (s == "stackrel")
		return MathAtom(new InsetMathStackrel);
	if (s == "binom" || s == "choose")
		return MathAtom(new InsetMathBinom(s == "choose"));
	if (s == "frac")
		return MathAtom(new InsetMathFrac);
	if (s == "over")
		return MathAtom(new InsetMathFrac(InsetMathFrac::OVER));
	if (s == "nicefrac")
		return MathAtom(new InsetMathFrac(InsetMathFrac::NICEFRAC));
	//if (s == "infer")
	//	return MathAtom(new MathInferInset);
	if (s == "atop")
		return MathAtom(new InsetMathFrac(InsetMathFrac::ATOP));
	if (s == "lefteqn")
		return MathAtom(new InsetMathLefteqn);
	if (s == "boldsymbol")
		return MathAtom(new InsetMathBoldSymbol);
	if (s == "color" || s == "normalcolor")
		return MathAtom(new InsetMathColor(true));
	if (s == "textcolor")
		return MathAtom(new InsetMathColor(false));
	if (s == "dfrac")
		return MathAtom(new InsetMathDFrac);
	if (s == "tfrac")
		return MathAtom(new InsetMathTFrac);
	if (s == "hphantom")
		return MathAtom(new InsetMathPhantom(InsetMathPhantom::hphantom));
	if (s == "phantom")
		return MathAtom(new InsetMathPhantom(InsetMathPhantom::phantom));
	if (s == "vphantom")
		return MathAtom(new InsetMathPhantom(InsetMathPhantom::vphantom));

	if (MacroTable::globalMacros().has(s))
		return MathAtom(new MathMacro(s,
			MacroTable::globalMacros().get(s).numargs()));
	//if (MacroTable::localMacros().has(s))
	//	return MathAtom(new MathMacro(s,
	//		MacroTable::localMacros().get(s).numargs()));

	//lyxerr << "creating unknown inset '" << s << "'" << endl;
	return MathAtom(new InsetMathUnknown(s));
}


bool createInsetMath_fromDialogStr(docstring const & str, MathData & ar)
{
	// An example str:
	// "ref LatexCommand ref\nreference \"sec:Title\"\n\\end_inset\n\n";
	docstring name;
	docstring body = split(str, name, ' ');

	if (name != "ref" )
		return false;

	InsetCommandParams icp("ref");
	// FIXME UNICODE
	InsetCommandMailer::string2params("ref", to_utf8(str), icp);
	mathed_parse_cell(ar, icp.getCommand());
	if (ar.size() != 1)
		return false;

	return ar[0].nucleus();
}


} // namespace lyx