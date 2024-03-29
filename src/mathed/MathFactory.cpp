/**
 * \file MathFactory.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author André Pönitz
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "MathFactory.h"

#include "InsetMathAMSArray.h"
#include "InsetMathArray.h"
#include "InsetMathBoldSymbol.h"
#include "InsetMathBox.h"
#include "InsetMathCases.h"
#include "InsetMathColor.h"
#include "InsetMathDecoration.h"
#include "InsetMathDots.h"
#include "InsetMathEnsureMath.h"
#include "InsetMathFont.h"
#include "InsetMathFontOld.h"
#include "InsetMathFrac.h"
#include "InsetMathKern.h"
#include "InsetMathLefteqn.h"
#include "InsetMathOverset.h"
#include "InsetMathPhantom.h"
#include "InsetMathRef.h"
#include "InsetMathRoot.h"
#include "InsetMathSize.h"
#include "InsetMathSpace.h"
#include "InsetMathSpecialChar.h"
#include "InsetMathSplit.h"
#include "InsetMathSqrt.h"
#include "InsetMathStackrel.h"
#include "InsetMathSubstack.h"
#include "InsetMathSymbol.h"
#include "InsetMathTabular.h"
#include "InsetMathUnderset.h"
#include "InsetMathUnknown.h"
#include "InsetMathHull.h"
#include "InsetMathXArrow.h"
#include "InsetMathXYMatrix.h"
#include "InsetMathDiagram.h"
#include "MacroTable.h"
#include "MathMacro.h"
#include "MathMacroArgument.h"
#include "MathParser.h"
#include "MathStream.h"
#include "MathSupport.h"

#include "insets/InsetCommand.h"
#include "insets/InsetSpace.h"

#include "support/debug.h"
#include "support/docstream.h"
#include "support/FileName.h"
#include "support/filetools.h" // LibFileSearch
#include "support/lstrings.h"

#include "frontends/FontLoader.h"

#include "Buffer.h"
#include "BufferParams.h"
#include "Encoding.h"
#include "LyX.h" // use_gui
#include "OutputParams.h"

using namespace std;
using namespace lyx::support;

namespace lyx {

bool has_math_fonts;


namespace {

MathWordList theMathWordList;


bool isMathFontAvailable(docstring & name)
{
	if (!use_gui)
		return false;

	FontInfo f;
	augmentFont(f, name);

	// Do we have the font proper?
	if (theFontLoader().available(f))
		return true;

	// can we fake it?
	if (name == "eufrak") {
		name = from_ascii("lyxfakefrak");
		return true;
	}

	LYXERR(Debug::MATHED,
		"font " << to_utf8(name) << " not available and I can't fake it");
	return false;
}


void initSymbols()
{
	FileName const filename = libFileSearch(string(), "symbols");
	LYXERR(Debug::MATHED, "read symbols from " << filename);
	if (filename.empty()) {
		lyxerr << "Could not find symbols file" << endl;
		return;
	}

	ifstream fs(filename.toFilesystemEncoding().c_str());
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
			skip = !isMathFontAvailable(t);
			continue;
		} else if (line.size() >= 4 && line.substr(0, 4) == "else") {
			skip = !skip;
			continue;
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
			MacroTable::globalMacros().insert(0, from_utf8(macro), requires);
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
			LYXERR(Debug::MATHED, "skipping line '" << line << "'\n"
				<< to_utf8(tmp.name) << ' ' << to_utf8(tmp.inset) << ' '
				<< to_utf8(tmp.extra));
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
				else if (tmp.inset == "mathscr")
					tmp.requires = from_ascii("mathrsfs");
			}

			// symbol font is not available sometimes
			docstring symbol_font = from_ascii("lyxsymbol");

			if (tmp.extra == "func" || tmp.extra == "funclim" || tmp.extra == "special") {
				LYXERR(Debug::MATHED, "symbol abuse for " << to_utf8(tmp.name));
				tmp.draw = tmp.name;
			} else if (isMathFontAvailable(tmp.inset)) {
				LYXERR(Debug::MATHED, "symbol available for " << to_utf8(tmp.name));
				tmp.draw.push_back(char_type(charid));
			} else if (fallbackid && isMathFontAvailable(symbol_font)) {
				if (tmp.inset == "cmex")
					tmp.inset = from_ascii("lyxsymbol");
				else
					tmp.inset = from_ascii("lyxboldsymbol");
				LYXERR(Debug::MATHED, "symbol fallback for " << to_utf8(tmp.name));
				tmp.draw.push_back(char_type(fallbackid));
			} else {
				LYXERR(Debug::MATHED, "faking " << to_utf8(tmp.name));
				tmp.draw = tmp.name;
				tmp.inset = from_ascii("lyxtex");
			}
		} else {
			// it's a proper inset
			LYXERR(Debug::MATHED, "inset " << to_utf8(tmp.inset)
					      << " used for " << to_utf8(tmp.name));
		}

		if (theMathWordList.find(tmp.name) != theMathWordList.end())
			LYXERR(Debug::MATHED, "readSymbols: inset " << to_utf8(tmp.name)
				<< " already exists.");
		else
			theMathWordList[tmp.name] = tmp;

		LYXERR(Debug::MATHED, "read symbol '" << to_utf8(tmp.name)
			<< "  inset: " << to_utf8(tmp.inset)
			<< "  draw: " << int(tmp.draw.empty() ? 0 : tmp.draw[0])
			<< "  extra: " << to_utf8(tmp.extra)
			<< "  requires: " << to_utf8(tmp.requires) << '\'');
	}
	docstring tmp = from_ascii("cmm");
	docstring tmp2 = from_ascii("cmsy");
	has_math_fonts = isMathFontAvailable(tmp) && isMathFontAvailable(tmp2);
}


bool isSpecialChar(docstring const & name)
{
	if (name.size() != 1)
		return  name == "textasciicircum" || name == "mathcircumflex" ||
			name == "textasciitilde"  || name == "textbackslash";

	char_type const c = name.at(0);
	return  c == '{' || c == '}' || c == '&' || c == '$' ||
		c == '#' || c == '%' || c == '_' || c == ' ';
}


} // namespace anon

MathWordList const & mathedWordList()
{
	return theMathWordList;
}


void initMath()
{
	static bool initialized = false;
	if (!initialized) {
		initialized = true;
		initParser();
		initSymbols();
	}
}


bool ensureMath(WriteStream & os, bool needs_math_mode, bool macro)
{
	bool brace = os.pendingBrace();
	os.pendingBrace(false);
	if (!os.latex())
		return brace;
	if (os.textMode() && needs_math_mode) {
		os << "\\ensuremath{";
		os.textMode(false);
		brace = true;
	} else if (macro && brace && !needs_math_mode) {
		// This is a user defined macro, but not a MathMacro, so we
		// cannot be sure what mode is needed. As it was entered in
		// a text box, we restore the text mode.
		os << '}';
		os.textMode(true);
		brace = false;
	}
	return brace;
}


int ensureMode(WriteStream & os, InsetMath::mode_type mode,
		bool locked, bool ascii)
{
	bool textmode = mode == InsetMath::TEXT_MODE;
	if (os.latex() && textmode && os.pendingBrace()) {
		os.os() << '}';
		os.pendingBrace(false);
		os.pendingSpace(false);
		os.textMode(true);
	}
	int oldmodes = os.textMode() ? 0x01 : 0;
	os.textMode(textmode);
	oldmodes |= os.lockedMode() ? 0x02 : 0;
	os.lockedMode(locked);
	oldmodes |= os.asciiOnly() ? 0x04 : 0;
	os.asciiOnly(ascii);
	return oldmodes;
}


latexkeys const * in_word_set(docstring const & str)
{
	MathWordList::iterator it = theMathWordList.find(str);
	return it != theMathWordList.end() ? &(it->second) : 0;
}


MathAtom createInsetMath(char const * const s, Buffer * buf)
{
	return createInsetMath(from_utf8(s), buf);
}


MathAtom createInsetMath(docstring const & s, Buffer * buf)
{
	//lyxerr << "creating inset with name: '" << to_utf8(s) << '\'' << endl;
	if ((s == "ce" || s == "cf") && buf
	    && buf->params().use_mhchem == BufferParams::package_off)
		return MathAtom(new MathMacro(buf, s));

	latexkeys const * l = in_word_set(s);
	if (l) {
		docstring const & inset = l->inset;
		//lyxerr << " found inset: '" << inset << '\'' << endl;
		if (inset == "ref")
			return MathAtom(new InsetMathRef(buf, l->name));
		if (inset == "overset")
			return MathAtom(new InsetMathOverset(buf));
		if (inset == "underset")
			return MathAtom(new InsetMathUnderset(buf));
		if (inset == "decoration")
			return MathAtom(new InsetMathDecoration(buf, l));
		if (inset == "space")
			return MathAtom(new InsetMathSpace(to_ascii(l->name), ""));
		if (inset == "dots")
			return MathAtom(new InsetMathDots(l));
		if (inset == "mbox")
			// return MathAtom(new InsetMathMBox);
			// InsetMathMBox is proposed to replace InsetMathBox,
			// but is not ready yet (it needs a BufferView for
			// construction)
			return MathAtom(new InsetMathBox(buf, l->name));
//		if (inset == "fbox")
//			return MathAtom(new InsetMathFBox(l));
		if (inset == "style")
			return MathAtom(new InsetMathSize(buf, l));
		if (inset == "font")
			return MathAtom(new InsetMathFont(buf, l));
		if (inset == "oldfont")
			return MathAtom(new InsetMathFontOld(buf, l));
		if (inset == "matrix")
			return MathAtom(new InsetMathAMSArray(buf, s));
		if (inset == "split")
			return MathAtom(new InsetMathSplit(buf, s));
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
		return MathAtom(new InsetMathBoxed(buf));
	if (s == "fbox")
		return MathAtom(new InsetMathFBox(buf));
	if (s == "framebox")
		return MathAtom(new InsetMathMakebox(buf, true));
	if (s == "makebox")
		return MathAtom(new InsetMathMakebox(buf, false));
	if (s == "kern")
		return MathAtom(new InsetMathKern);
	if (s.substr(0, 8) == "xymatrix") {
		char spacing_code = '\0';
		Length spacing;
		bool equal_spacing = false;
		size_t const len = s.length();
		size_t i = 8;
		if (i < len && s[i] == '@') {
			++i;
			if (i < len && s[i] == '!') {
				equal_spacing = true;
				++i;
				if (i < len) {
					switch (s[i]) {
					case '0':
					case 'R':
					case 'C':
						spacing_code = static_cast<char>(s[i]);
					}
				}
			} else if (i < len) {
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
				if (i < len && s[i] == '=') {
					++i;
					spacing = Length(to_ascii(s.substr(i)));
				}
			}
		}
		return MathAtom(new InsetMathXYMatrix(buf, spacing, spacing_code,
			equal_spacing));
	}

	if (s == "Diagram")
		return MathAtom(new InsetMathDiagram(buf));
	if (s == "xrightarrow" || s == "xleftarrow")
		return MathAtom(new InsetMathXArrow(buf, s));
	if (s == "split" || s == "alignedat")
		return MathAtom(new InsetMathSplit(buf, s));
	if (s == "cases")
		return MathAtom(new InsetMathCases(buf));
	if (s == "substack")
		return MathAtom(new InsetMathSubstack(buf));
	if (s == "subarray" || s == "array")
		return MathAtom(new InsetMathArray(buf, s, 1, 1));
	if (s == "sqrt")
		return MathAtom(new InsetMathSqrt(buf));
	if (s == "root")
		return MathAtom(new InsetMathRoot(buf));
	if (s == "tabular")
		return MathAtom(new InsetMathTabular(buf, s, 1, 1));
	if (s == "stackrel")
		return MathAtom(new InsetMathStackrel(buf));
	if (s == "binom")
		return MathAtom(new InsetMathBinom(buf, InsetMathBinom::BINOM));
	if (s == "dbinom")
		return MathAtom(new InsetMathBinom(buf, InsetMathBinom::DBINOM));
	if (s == "tbinom")
		return MathAtom(new InsetMathBinom(buf, InsetMathBinom::TBINOM));
	if (s == "choose")
		return MathAtom(new InsetMathBinom(buf, InsetMathBinom::CHOOSE));
	if (s == "brace")
		return MathAtom(new InsetMathBinom(buf, InsetMathBinom::BRACE));
	if (s == "brack")
		return MathAtom(new InsetMathBinom(buf, InsetMathBinom::BRACK));
	if (s == "frac")
		return MathAtom(new InsetMathFrac(buf));
	if (s == "cfrac")
		return MathAtom(new InsetMathFrac(buf, InsetMathFrac::CFRAC));
	if (s == "dfrac")
		return MathAtom(new InsetMathFrac(buf, InsetMathFrac::DFRAC));
	if (s == "tfrac")
		return MathAtom(new InsetMathFrac(buf, InsetMathFrac::TFRAC));
	if (s == "over")
		return MathAtom(new InsetMathFrac(buf, InsetMathFrac::OVER));
	if (s == "nicefrac")
		return MathAtom(new InsetMathFrac(buf, InsetMathFrac::NICEFRAC));
	if (s == "unitfrac")
		return MathAtom(new InsetMathFrac(buf, InsetMathFrac::UNITFRAC));
	// These string values are only for math toolbar use, no LaTeX names
	if (s == "unitfracthree")
		return MathAtom(new InsetMathFrac(buf, InsetMathFrac::UNITFRAC, 3));
	if (s == "unitone")
		return MathAtom(new InsetMathFrac(buf, InsetMathFrac::UNIT, 1));
	if (s == "unittwo")
		return MathAtom(new InsetMathFrac(buf, InsetMathFrac::UNIT));
	if (s == "cfracleft")
		return MathAtom(new InsetMathFrac(buf, InsetMathFrac::CFRACLEFT));
	if (s == "cfracright")
		return MathAtom(new InsetMathFrac(buf, InsetMathFrac::CFRACRIGHT));
	//if (s == "infer")
	//	return MathAtom(new MathInferInset);
	if (s == "atop")
		return MathAtom(new InsetMathFrac(buf, InsetMathFrac::ATOP));
	if (s == "lefteqn")
		return MathAtom(new InsetMathLefteqn(buf));
	if (s == "boldsymbol")
		return MathAtom(new InsetMathBoldSymbol(buf, InsetMathBoldSymbol::AMS_BOLD));
	if (s == "bm")
		return MathAtom(new InsetMathBoldSymbol(buf, InsetMathBoldSymbol::BM_BOLD));
	if (s == "heavysymbol" || s == "hm")
		return MathAtom(new InsetMathBoldSymbol(buf, InsetMathBoldSymbol::BM_HEAVY));
	if (s == "color" || s == "normalcolor")
		return MathAtom(new InsetMathColor(buf, true));
	if (s == "textcolor")
		return MathAtom(new InsetMathColor(buf, false));
	if (s == "hphantom")
		return MathAtom(new InsetMathPhantom(buf, InsetMathPhantom::hphantom));
	if (s == "phantom")
		return MathAtom(new InsetMathPhantom(buf, InsetMathPhantom::phantom));
	if (s == "vphantom")
		return MathAtom(new InsetMathPhantom(buf, InsetMathPhantom::vphantom));
	if (s == "ensuremath")
		return MathAtom(new InsetMathEnsureMath(buf));
	if (isSpecialChar(s))
		return MathAtom(new InsetMathSpecialChar(s));

	if (s == "regexp")
		return MathAtom(new InsetMathHull(buf, hullRegexp));

	return MathAtom(new MathMacro(buf, s));
}


bool createInsetMath_fromDialogStr(docstring const & str, MathData & ar)
{
	// An example str:
	// "ref LatexCommand ref\nreference \"sec:Title\"\n\\end_inset\n\n";
	docstring name;
	docstring body = split(str, name, ' ');

	if (name == "ref") {
		InsetCommandParams icp(REF_CODE);
		// FIXME UNICODE
		InsetCommand::string2params(to_utf8(str), icp);
		Encoding const * const utf8 = encodings.fromLyXName("utf8");
		OutputParams op(utf8);
		mathed_parse_cell(ar, icp.getCommand(op));
	} else if (name == "mathspace") {
		InsetSpaceParams isp(true);
		InsetSpace::string2params(to_utf8(str), isp);
		InsetSpace is(isp);
		TexRow texrow;
		odocstringstream ods;
		otexstream os(ods, texrow);
		Encoding const * const ascii = encodings.fromLyXName("ascii");
		OutputParams op(ascii);
		is.latex(os, op);
		mathed_parse_cell(ar, ods.str());
		if (ar.size() == 2) {
			// remove "{}"
			if (ar[1].nucleus()->asBraceInset())
				ar.pop_back();
		}
	} else
		return false;

	if (ar.size() != 1)
		return false;

	return ar[0].nucleus();
}


bool isAsciiOrMathAlpha(char_type c)
{
	return c < 0x80 || Encodings::isMathAlpha(c);
}


} // namespace lyx
