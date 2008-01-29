/**
 * \file TextClass.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Lars Gullik Bj�nnes
 * \author Jean-Marc Lasgouttes
 * \author Angus Leeming
 * \author John Levon
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "TextClass.h"
#include "debug.h"
#include "Lexer.h"
#include "Counters.h"
#include "gettext.h"
#include "Floating.h"
#include "FloatList.h"

#include "frontends/alert.h"

#include "support/lstrings.h"
#include "support/lyxlib.h"
#include "support/filetools.h"
#include "support/os.h"

#include <boost/filesystem/operations.hpp>
namespace fs = boost::filesystem;

#include <sstream>


namespace lyx {

using support::FileName;
using support::libFileSearch;
using support::makeDisplayPath;
using support::quoteName;
using support::rtrim;
using support::subst;
using support::addName;

using std::endl;
using std::find_if;
using std::remove_if;
using std::string;
using std::ostream;


namespace {

class LayoutNamesEqual : public std::unary_function<Layout_ptr, bool> {
public:
	LayoutNamesEqual(docstring const & name)
		: name_(name)
	{}
	bool operator()(Layout_ptr const & c) const
	{
		return c->name() == name_;
	}
private:
	docstring name_;
};


int const FORMAT = 4;


bool layout2layout(FileName const & filename, FileName const & tempfile)
{
	FileName const script = libFileSearch("scripts", "layout2layout.py");
	if (script.empty()) {
		lyxerr << "Could not find layout conversion "
			  "script layout2layout.py." << endl;
		return false;
	}

	std::ostringstream command;
	command << support::os::python() << ' ' << quoteName(script.toFilesystemEncoding())
		<< ' ' << quoteName(filename.toFilesystemEncoding())
		<< ' ' << quoteName(tempfile.toFilesystemEncoding());
	string const command_str = command.str();

	LYXERR(Debug::TCLASS) << "Running `" << command_str << '\'' << endl;

	support::cmd_ret const ret =
		support::runCommand(command_str);
	if (ret.first != 0) {
		lyxerr << "Could not run layout conversion "
			  "script layout2layout.py." << endl;
		return false;
	}
	return true;
}

} // namespace anon


TextClass::TextClass(string const & fn, string const & cln,
			   string const & desc, bool texClassAvail )
	: name_(fn), latexname_(cln), description_(desc),
	  floatlist_(new FloatList), ctrs_(new Counters),
	  texClassAvail_(texClassAvail)
{
	outputType_ = LATEX;
	columns_ = 1;
	sides_ = OneSide;
	secnumdepth_ = 3;
	tocdepth_ = 3;
	pagestyle_ = "default";
	defaultfont_ = Font(Font::ALL_SANE);
	opt_fontsize_ = "10|11|12";
	opt_pagestyle_ = "empty|plain|headings|fancy";
	titletype_ = TITLE_COMMAND_AFTER;
	titlename_ = "maketitle";
	loaded_ = false;
}


bool TextClass::isTeXClassAvailable() const
{
	return texClassAvail_;
}


bool TextClass::do_readStyle(Lexer & lexrc, Layout & lay)
{
	LYXERR(Debug::TCLASS) << "Reading style " << to_utf8(lay.name()) << endl;
	if (!lay.read(lexrc, *this)) {
		// Resolve fonts
		lay.resfont = lay.font;
		lay.resfont.realize(defaultfont());
		lay.reslabelfont = lay.labelfont;
		lay.reslabelfont.realize(defaultfont());
		return false; // no errors
	}
	lyxerr << "Error parsing style `" << to_utf8(lay.name()) << '\'' << endl;
	return true;
}


enum TextClassTags {
	TC_OUTPUTTYPE = 1,
	TC_INPUT,
	TC_STYLE,
	TC_DEFAULTSTYLE,
	TC_CHARSTYLE,
	TC_ENVIRONMENT,
	TC_NOSTYLE,
	TC_COLUMNS,
	TC_SIDES,
	TC_PAGESTYLE,
	TC_DEFAULTFONT,
	TC_SECNUMDEPTH,
	TC_TOCDEPTH,
	TC_CLASSOPTIONS,
	TC_PREAMBLE,
	TC_PROVIDES,
	TC_LEFTMARGIN,
	TC_RIGHTMARGIN,
	TC_FLOAT,
	TC_COUNTER,
	TC_NOFLOAT,
	TC_TITLELATEXNAME,
	TC_TITLELATEXTYPE,
	TC_FORMAT
};


// Reads a textclass structure from file.
bool TextClass::read(FileName const & filename, bool merge)
{
	if (!support::isFileReadable(filename)) {
		lyxerr << "Cannot read layout file `" << filename << "'."
		       << endl;
		return true;
	}

	keyword_item textClassTags[] = {
		{ "charstyle",       TC_CHARSTYLE },
		{ "classoptions",    TC_CLASSOPTIONS },
		{ "columns",         TC_COLUMNS },
		{ "counter",         TC_COUNTER },
		{ "defaultfont",     TC_DEFAULTFONT },
		{ "defaultstyle",    TC_DEFAULTSTYLE },
		{ "environment",     TC_ENVIRONMENT },
		{ "float",           TC_FLOAT },
		{ "format",          TC_FORMAT },
		{ "input",           TC_INPUT },
		{ "leftmargin",      TC_LEFTMARGIN },
		{ "nofloat",         TC_NOFLOAT },
		{ "nostyle",         TC_NOSTYLE },
		{ "outputtype",      TC_OUTPUTTYPE },
		{ "pagestyle",       TC_PAGESTYLE },
		{ "preamble",        TC_PREAMBLE },
		{ "provides",        TC_PROVIDES },
		{ "rightmargin",     TC_RIGHTMARGIN },
		{ "secnumdepth",     TC_SECNUMDEPTH },
		{ "sides",           TC_SIDES },
		{ "style",           TC_STYLE },
		{ "titlelatexname",  TC_TITLELATEXNAME },
		{ "titlelatextype",  TC_TITLELATEXTYPE },
		{ "tocdepth",        TC_TOCDEPTH }
	};

	if (!merge)
		LYXERR(Debug::TCLASS) << "Reading textclass "
					<< to_utf8(makeDisplayPath(filename.absFilename()))
					<< endl;
	else
		LYXERR(Debug::TCLASS) << "Reading input file "
				     << to_utf8(makeDisplayPath(filename.absFilename()))
				     << endl;

	Lexer lexrc(textClassTags,
		sizeof(textClassTags) / sizeof(textClassTags[0]));

	lexrc.setFile(filename);
	bool error = !lexrc.isOK();

	// Format of files before the 'Format' tag was introduced
	int format = 1;

	// parsing
	while (lexrc.isOK() && !error) {
		int le = lexrc.lex();

		switch (le) {
		case Lexer::LEX_FEOF:
			continue;

		case Lexer::LEX_UNDEF:
			lexrc.printError("Unknown TextClass tag `$$Token'");
			error = true;
			continue;

		default:
			break;
		}

		switch (static_cast<TextClassTags>(le)) {

		case TC_FORMAT:
			if (lexrc.next())
				format = lexrc.getInteger();
			break;

		case TC_OUTPUTTYPE:   // output type definition
			readOutputType(lexrc);
			break;

		case TC_INPUT: // Include file
			if (lexrc.next()) {
				string const inc = lexrc.getString();
				FileName tmp = libFileSearch("layouts", inc,
							    "layout");

				if (tmp.empty()) {
					lexrc.printError("Could not find input"
							 "file: " + inc);
					error = true;
				} else if (read(tmp, true)) {
					lexrc.printError("Error reading input"
							 "file: " + tmp.absFilename());
					error = true;
				}
			}
			break;

		case TC_DEFAULTSTYLE:
			if (lexrc.next()) {
				docstring const name = from_utf8(subst(lexrc.getString(),
							  '_', ' '));
				defaultlayout_ = name;
			}
			break;

		case TC_ENVIRONMENT:
		case TC_STYLE:
			if (lexrc.next()) {
				docstring const name = from_utf8(subst(lexrc.getString(),
						    '_', ' '));
				if (name.empty()) {
					string s = "Could not read name for style: `$$Token' "
						+ lexrc.getString() + " is probably not valid UTF-8!";
					lexrc.printError(s.c_str());
					Layout lay;
					error = do_readStyle(lexrc, lay);
				} else if (hasLayout(name)) {
					Layout * lay = operator[](name).get();
					error = do_readStyle(lexrc, *lay);
				} else {
					Layout lay;
					lay.setName(name);
					if (le == TC_ENVIRONMENT)
						lay.is_environment = true;
					error = do_readStyle(lexrc, lay);
					if (!error)
						layoutlist_.push_back(
							boost::shared_ptr<Layout>(new Layout(lay))
							);

					if (defaultlayout_.empty()) {
						// We do not have a default
						// layout yet, so we choose
						// the first layout we
						// encounter.
						defaultlayout_ = name;
					}
				}
			}
			else {
				lexrc.printError("No name given for style: `$$Token'.");
				error = true;
			}
			break;

		case TC_NOSTYLE:
			if (lexrc.next()) {
				docstring const style = from_utf8(subst(lexrc.getString(),
						     '_', ' '));
				if (!delete_layout(style))
					lyxerr << "Cannot delete style `"
					       << to_utf8(style) << '\'' << endl;
//					lexrc.printError("Cannot delete style"
//							 " `$$Token'");
			}
			break;

		case TC_COLUMNS:
			if (lexrc.next())
				columns_ = lexrc.getInteger();
			break;

		case TC_SIDES:
			if (lexrc.next()) {
				switch (lexrc.getInteger()) {
				case 1: sides_ = OneSide; break;
				case 2: sides_ = TwoSides; break;
				default:
					lyxerr << "Impossible number of page"
						" sides, setting to one."
					       << endl;
					sides_ = OneSide;
					break;
				}
			}
			break;

		case TC_PAGESTYLE:
			lexrc.next();
			pagestyle_ = rtrim(lexrc.getString());
			break;

		case TC_DEFAULTFONT:
			defaultfont_.lyxRead(lexrc);
			if (!defaultfont_.resolved()) {
				lexrc.printError("Warning: defaultfont should "
						 "be fully instantiated!");
				defaultfont_.realize(Font(Font::ALL_SANE));
			}
			break;

		case TC_SECNUMDEPTH:
			lexrc.next();
			secnumdepth_ = lexrc.getInteger();
			break;

		case TC_TOCDEPTH:
			lexrc.next();
			tocdepth_ = lexrc.getInteger();
			break;

			// First step to support options
		case TC_CLASSOPTIONS:
			readClassOptions(lexrc);
			break;

		case TC_PREAMBLE:
			preamble_ = from_utf8(lexrc.getLongString("EndPreamble"));
			break;

		case TC_PROVIDES: {
			lexrc.next();
			string const feature = lexrc.getString();
			lexrc.next();
			if (lexrc.getInteger())
				provides_.insert(feature);
			else
				provides_.erase(feature);
			break;
		}

		case TC_LEFTMARGIN:	// left margin type
			if (lexrc.next())
				leftmargin_ = lexrc.getString();
			break;

		case TC_RIGHTMARGIN:	// right margin type
			if (lexrc.next())
				rightmargin_ = lexrc.getString();
			break;
		case TC_CHARSTYLE:
			if (lexrc.next()) {
				string const name = subst(lexrc.getString(), '_', ' ');
				readCharStyle(lexrc, name);
			}
			break;
		case TC_FLOAT:
			readFloat(lexrc);
			break;
		case TC_COUNTER:
			readCounter(lexrc);
			break;
		case TC_TITLELATEXTYPE:
			readTitleType(lexrc);
			break;
		case TC_TITLELATEXNAME:
			if (lexrc.next())
				titlename_ = lexrc.getString();
			break;
		case TC_NOFLOAT:
			if (lexrc.next()) {
				string const nofloat = lexrc.getString();
				floatlist_->erase(nofloat);
			}
			break;
		}
		if (format != FORMAT)
			break;
	}

	if (format != FORMAT) {
		LYXERR(Debug::TCLASS) << "Converting layout file from format "
				      << format << " to " << FORMAT << endl;
		FileName const tempfile(support::tempName());
		error = !layout2layout(filename, tempfile);
		if (!error)
			error = read(tempfile, merge);
		support::unlink(tempfile);
		return error;
	}

	if (!merge) { // we are at top level here.
		LYXERR(Debug::TCLASS) << "Finished reading textclass "
				      << to_utf8(makeDisplayPath(filename.absFilename()))
				      << endl;
		if (defaultlayout_.empty()) {
			lyxerr << "Error: Textclass '" << name_
			       << "' is missing a defaultstyle." << endl;
			error = true;
		}

		min_toclevel_ = Layout::NOT_IN_TOC;
		max_toclevel_ = Layout::NOT_IN_TOC;
		const_iterator cit = begin();
		const_iterator the_end = end();
		for ( ; cit != the_end ; ++cit) {
			int const toclevel = (*cit)->toclevel;
			if (toclevel != Layout::NOT_IN_TOC) {
				if (min_toclevel_ == Layout::NOT_IN_TOC)
					min_toclevel_ = toclevel;
				else
					min_toclevel_ = std::min(min_toclevel_,
							 toclevel);
				max_toclevel_ = std::max(max_toclevel_,
							 toclevel);
			}
		}
		LYXERR(Debug::TCLASS)
			<< "Minimum TocLevel is " << min_toclevel_
			<< ", maximum is " << max_toclevel_ <<endl;

	} else
		LYXERR(Debug::TCLASS) << "Finished reading input file "
				      << to_utf8(makeDisplayPath(filename.absFilename()))
				      << endl;

	return error;
}


void TextClass::readTitleType(Lexer & lexrc)
{
	keyword_item titleTypeTags[] = {
		{ "commandafter", TITLE_COMMAND_AFTER },
		{ "environment", TITLE_ENVIRONMENT }
	};

	PushPopHelper pph(lexrc, titleTypeTags, TITLE_ENVIRONMENT);

	int le = lexrc.lex();
	switch (le) {
	case Lexer::LEX_UNDEF:
		lexrc.printError("Unknown output type `$$Token'");
		return;
	case TITLE_COMMAND_AFTER:
	case TITLE_ENVIRONMENT:
		titletype_ = static_cast<LYX_TITLE_LATEX_TYPES>(le);
		break;
	default:
		lyxerr << "Unhandled value " << le
		       << " in TextClass::readTitleType." << endl;

		break;
	}
}


void TextClass::readOutputType(Lexer & lexrc)
{
	keyword_item outputTypeTags[] = {
		{ "docbook", DOCBOOK },
		{ "latex", LATEX },
		{ "literate", LITERATE }
	};

	PushPopHelper pph(lexrc, outputTypeTags, LITERATE);

	int le = lexrc.lex();
	switch (le) {
	case Lexer::LEX_UNDEF:
		lexrc.printError("Unknown output type `$$Token'");
		return;
	case LATEX:
	case DOCBOOK:
	case LITERATE:
		outputType_ = static_cast<OutputType>(le);
		break;
	default:
		lyxerr << "Unhandled value " << le
		       << " in TextClass::readOutputType." << endl;

		break;
	}
}


enum ClassOptionsTags {
	CO_FONTSIZE = 1,
	CO_PAGESTYLE,
	CO_OTHER,
	CO_HEADER,
	CO_END
};


void TextClass::readClassOptions(Lexer & lexrc)
{
	keyword_item classOptionsTags[] = {
		{"end", CO_END },
		{"fontsize", CO_FONTSIZE },
		{"header", CO_HEADER },
		{"other", CO_OTHER },
		{"pagestyle", CO_PAGESTYLE }
	};

	lexrc.pushTable(classOptionsTags, CO_END);
	bool getout = false;
	while (!getout && lexrc.isOK()) {
		int le = lexrc.lex();
		switch (le) {
		case Lexer::LEX_UNDEF:
			lexrc.printError("Unknown ClassOption tag `$$Token'");
			continue;
		default: break;
		}
		switch (static_cast<ClassOptionsTags>(le)) {
		case CO_FONTSIZE:
			lexrc.next();
			opt_fontsize_ = rtrim(lexrc.getString());
			break;
		case CO_PAGESTYLE:
			lexrc.next();
			opt_pagestyle_ = rtrim(lexrc.getString());
			break;
		case CO_OTHER:
			lexrc.next();
			options_ = lexrc.getString();
			break;
		case CO_HEADER:
			lexrc.next();
			class_header_ = subst(lexrc.getString(), "&quot;", "\"");
			break;
		case CO_END:
			getout = true;
			break;
		}
	}
	lexrc.popTable();
}

enum CharStyleTags {
	CS_FONT = 1,
	CS_LABELFONT,
	CS_LATEXTYPE,
	CS_LATEXNAME,
	CS_LATEXPARAM,
	CS_PREAMBLE,
	CS_END
};


void TextClass::readCharStyle(Lexer & lexrc, string const & name)
{
	keyword_item elementTags[] = {
		{ "end", CS_END },
		{ "font", CS_FONT },
		{ "labelfont", CS_LABELFONT },
		{ "latexname", CS_LATEXNAME },
		{ "latexparam", CS_LATEXPARAM },
		{ "latextype", CS_LATEXTYPE },
		{ "preamble", CS_PREAMBLE}
	};

	lexrc.pushTable(elementTags, CS_END);

	string latextype;
	string latexname;
	string latexparam;
	Font font(Font::ALL_INHERIT);
	Font labelfont(Font::ALL_INHERIT);
	string preamble;

	bool getout = false;
	while (!getout && lexrc.isOK()) {
		int le = lexrc.lex();
		switch (le) {
		case Lexer::LEX_UNDEF:
			lexrc.printError("Unknown ClassOption tag `$$Token'");
			continue;
		default: break;
		}
		switch (static_cast<CharStyleTags>(le)) {
		case CS_LATEXTYPE:
			lexrc.next();
			latextype = lexrc.getString();
			break;
		case CS_LATEXNAME:
			lexrc.next();
			latexname = lexrc.getString();
			break;
		case CS_LATEXPARAM:
			lexrc.next();
			latexparam = subst(lexrc.getString(), "&quot;", "\"");
			break;
		case CS_LABELFONT:
			labelfont.lyxRead(lexrc);
			break;
		case CS_FONT:
			font.lyxRead(lexrc);
			labelfont = font;
			break;
		case CS_PREAMBLE:
			preamble = lexrc.getLongString("EndPreamble");
			break;
		case CS_END:
			getout = true;
			break;
		}
	}

	//
	// Here add element to list if getout == true
	if (getout) {
		CharStyle cs;
		cs.name = name;
		cs.latextype = latextype;
		cs.latexname = latexname;
		cs.latexparam = latexparam;
		cs.font = font;
		cs.labelfont = labelfont;
		cs.preamble = from_utf8(preamble);
		charstyles().push_back(cs);
	}

	lexrc.popTable();
}


enum FloatTags {
	FT_TYPE = 1,
	FT_NAME,
	FT_PLACEMENT,
	FT_EXT,
	FT_WITHIN,
	FT_STYLE,
	FT_LISTNAME,
	FT_BUILTIN,
	FT_END
};


void TextClass::readFloat(Lexer & lexrc)
{
	keyword_item floatTags[] = {
		{ "end", FT_END },
		{ "extension", FT_EXT },
		{ "guiname", FT_NAME },
		{ "latexbuiltin", FT_BUILTIN },
		{ "listname", FT_LISTNAME },
		{ "numberwithin", FT_WITHIN },
		{ "placement", FT_PLACEMENT },
		{ "style", FT_STYLE },
		{ "type", FT_TYPE }
	};

	lexrc.pushTable(floatTags, FT_END);

	string type;
	string placement;
	string ext;
	string within;
	string style;
	string name;
	string listname;
	bool builtin = false;

	bool getout = false;
	while (!getout && lexrc.isOK()) {
		int le = lexrc.lex();
		switch (le) {
		case Lexer::LEX_UNDEF:
			lexrc.printError("Unknown ClassOption tag `$$Token'");
			continue;
		default: break;
		}
		switch (static_cast<FloatTags>(le)) {
		case FT_TYPE:
			lexrc.next();
			type = lexrc.getString();
			// Here we could check if this type is already defined
			// and modify it with the rest of the vars instead.
			break;
		case FT_NAME:
			lexrc.next();
			name = lexrc.getString();
			break;
		case FT_PLACEMENT:
			lexrc.next();
			placement = lexrc.getString();
			break;
		case FT_EXT:
			lexrc.next();
			ext = lexrc.getString();
			break;
		case FT_WITHIN:
			lexrc.next();
			within = lexrc.getString();
			if (within == "none")
				within.erase();
			break;
		case FT_STYLE:
			lexrc.next();
			style = lexrc.getString();
			break;
		case FT_LISTNAME:
			lexrc.next();
			listname = lexrc.getString();
			break;
		case FT_BUILTIN:
			lexrc.next();
			builtin = lexrc.getBool();
			break;
		case FT_END:
			getout = true;
			break;
		}
	}

	// Here if have a full float if getout == true
	if (getout) {
		Floating newfloat(type, placement, ext, within,
				  style, name, listname, builtin);
		floatlist_->newFloat(newfloat);
	}

	lexrc.popTable();
}


enum CounterTags {
	CT_NAME = 1,
	CT_WITHIN,
	CT_END
};

void TextClass::readCounter(Lexer & lexrc)
{
	keyword_item counterTags[] = {
		{ "end", CT_END },
		{ "name", CT_NAME },
		{ "within", CT_WITHIN }
	};

	lexrc.pushTable(counterTags, CT_END);

	docstring name;
	docstring within;

	bool getout = false;
	while (!getout && lexrc.isOK()) {
		int le = lexrc.lex();
		switch (le) {
		case Lexer::LEX_UNDEF:
			lexrc.printError("Unknown ClassOption tag `$$Token'");
			continue;
		default: break;
		}
		switch (static_cast<CounterTags>(le)) {
		case CT_NAME:
			lexrc.next();
			name = from_ascii(lexrc.getString());
			break;
		case CT_WITHIN:
			lexrc.next();
			within = from_ascii(lexrc.getString());
			if (within == "none")
				within.erase();
			break;
		case CT_END:
			getout = true;
			break;
		}
	}

	// Here if have a full counter if getout == true
	if (getout) {
		if (within.empty())
			ctrs_->newCounter(name);
		else
			ctrs_->newCounter(name, within);
	}

	lexrc.popTable();
}


Font const & TextClass::defaultfont() const
{
	return defaultfont_;
}


string const & TextClass::leftmargin() const
{
	return leftmargin_;
}


string const & TextClass::rightmargin() const
{
	return rightmargin_;
}


bool TextClass::hasLayout(docstring const & n) const
{
	docstring const name = n.empty() ? defaultLayoutName() : n;

	return find_if(layoutlist_.begin(), layoutlist_.end(),
		       LayoutNamesEqual(name))
		!= layoutlist_.end();
}



Layout_ptr const & TextClass::operator[](docstring const & name) const
{
	BOOST_ASSERT(!name.empty());

	LayoutList::const_iterator cit =
		find_if(layoutlist_.begin(),
			layoutlist_.end(),
			LayoutNamesEqual(name));

	if (cit == layoutlist_.end()) {
		lyxerr << "We failed to find the layout '" << to_utf8(name)
		       << "' in the layout list. You MUST investigate!"
		       << endl;
		for (LayoutList::const_iterator it = layoutlist_.begin();
			 it != layoutlist_.end(); ++it)
			lyxerr  << " " << to_utf8(it->get()->name()) << endl;

		// we require the name to exist
		BOOST_ASSERT(false);
	}

	return (*cit);
}



bool TextClass::delete_layout(docstring const & name)
{
	if (name == defaultLayoutName())
		return false;

	LayoutList::iterator it =
		remove_if(layoutlist_.begin(), layoutlist_.end(),
			  LayoutNamesEqual(name));

	LayoutList::iterator end = layoutlist_.end();
	bool const ret = (it != end);
	layoutlist_.erase(it, end);
	return ret;
}


// Load textclass info if not loaded yet
bool TextClass::load(string const & path) const
{
	if (loaded_)
		return true;

	// Read style-file, provided path is searched before the system ones
	FileName layout_file;
	if (!path.empty())
		layout_file = FileName(addName(path, name_ + ".layout"));
	if (layout_file.empty() || !fs::exists(layout_file.toFilesystemEncoding()))
		layout_file = libFileSearch("layouts", name_, "layout");
	loaded_ = const_cast<TextClass*>(this)->read(layout_file) == 0;

	if (!loaded_) {
		lyxerr << "Error reading `"
		       << to_utf8(makeDisplayPath(layout_file.absFilename()))
		       << "'\n(Check `" << name_
		       << "')\nCheck your installation and "
			"try Options/Reconfigure..." << endl;
	}

	return loaded_;
}


FloatList & TextClass::floats()
{
	return *floatlist_.get();
}


FloatList const & TextClass::floats() const
{
	return *floatlist_.get();
}


Counters & TextClass::counters() const
{
	return *ctrs_.get();
}


CharStyles::iterator TextClass::charstyle(string const & s) const
{
	CharStyles::iterator cs = charstyles().begin();
	CharStyles::iterator csend = charstyles().end();
	for (; cs != csend; ++cs) {
		if (cs->name == s)
			return cs;
	}
	return csend;
}


docstring const & TextClass::defaultLayoutName() const
{
	// This really should come from the actual layout... (Lgb)
	return defaultlayout_;
}


Layout_ptr const & TextClass::defaultLayout() const
{
	return operator[](defaultLayoutName());
}


string const & TextClass::name() const
{
	return name_;
}


string const & TextClass::latexname() const
{
	const_cast<TextClass*>(this)->load();
	return latexname_;
}


string const & TextClass::description() const
{
	return description_;
}


string const & TextClass::opt_fontsize() const
{
	return opt_fontsize_;
}


string const & TextClass::opt_pagestyle() const
{
	return opt_pagestyle_;
}


string const & TextClass::options() const
{
	return options_;
}


string const & TextClass::class_header() const
{
	return class_header_;
}


string const & TextClass::pagestyle() const
{
	return pagestyle_;
}


docstring const & TextClass::preamble() const
{
	return preamble_;
}


TextClass::PageSides TextClass::sides() const
{
	return sides_;
}


int TextClass::secnumdepth() const
{
	return secnumdepth_;
}


int TextClass::tocdepth() const
{
	return tocdepth_;
}


OutputType TextClass::outputType() const
{
	return outputType_;
}


bool TextClass::provides(string const & p) const
{
	return provides_.find(p) != provides_.end();
}


unsigned int TextClass::columns() const
{
	return columns_;
}


LYX_TITLE_LATEX_TYPES TextClass::titletype() const
{
	return titletype_;
}


string const & TextClass::titlename() const
{
	return titlename_;
}


int TextClass::size() const
{
	return layoutlist_.size();
}


int TextClass::min_toclevel() const
{
	return min_toclevel_;
}


int TextClass::max_toclevel() const
{
	return max_toclevel_;
}


bool TextClass::hasTocLevels() const
{
	return min_toclevel_ != Layout::NOT_IN_TOC;
}


ostream & operator<<(ostream & os, TextClass::PageSides p)
{
	switch (p) {
	case TextClass::OneSide:
		os << '1';
		break;
	case TextClass::TwoSides:
		os << '2';
		break;
	}
	return os;
}


} // namespace lyx