/**
 * \file InsetMathHull.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author André Pönitz
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "InsetMathHull.h"

#include "InsetMathChar.h"
#include "InsetMathColor.h"
#include "MathExtern.h"
#include "MathFactory.h"
#include "MathStream.h"
#include "MathSupport.h"

#include "Buffer.h"
#include "BufferParams.h"
#include "BufferView.h"
#include "ColorSet.h"
#include "CutAndPaste.h"
#include "Encoding.h"
#include "Exporter.h"
#include "FuncRequest.h"
#include "FuncStatus.h"
#include "Language.h"
#include "LaTeXFeatures.h"
#include "LyXRC.h"
#include "MacroTable.h"
#include "output_xhtml.h"
#include "Paragraph.h"
#include "ParIterator.h"
#include "sgml.h"
#include "TextClass.h"
#include "TextPainter.h"
#include "TocBackend.h"

#include "insets/InsetLabel.h"
#include "insets/InsetRef.h"
#include "insets/RenderPreview.h"

#include "graphics/PreviewImage.h"
#include "graphics/PreviewLoader.h"

#include "frontends/alert.h"
#include "frontends/Painter.h"

#include "support/convert.h"
#include "support/lassert.h"
#include "support/debug.h"
#include "support/filetools.h"
#include "support/gettext.h"
#include "support/lstrings.h"

#include <sstream>

using namespace std;
using namespace lyx::support;

namespace lyx {

using cap::grabAndEraseSelection;

namespace {

	int getCols(HullType type)
	{
		switch (type) {
			case hullEqnArray:
				return 3;
			case hullAlign:
			case hullFlAlign:
			case hullAlignAt:
			case hullXAlignAt:
			case hullXXAlignAt:
				return 2;
			default:
				return 1;
		}
	}


	// returns position of first relation operator in the array
	// used for "intelligent splitting"
	size_t firstRelOp(MathData const & ar)
	{
		for (MathData::const_iterator it = ar.begin(); it != ar.end(); ++it)
			if ((*it)->isRelOp())
				return it - ar.begin();
		return ar.size();
	}


	char const * star(bool numbered)
	{
		return numbered ? "" : "*";
	}


} // end anon namespace


HullType hullType(docstring const & s)
{
	if (s == "none")      return hullNone;
	if (s == "simple")    return hullSimple;
	if (s == "equation")  return hullEquation;
	if (s == "eqnarray")  return hullEqnArray;
	if (s == "align")     return hullAlign;
	if (s == "alignat")   return hullAlignAt;
	if (s == "xalignat")  return hullXAlignAt;
	if (s == "xxalignat") return hullXXAlignAt;
	if (s == "multline")  return hullMultline;
	if (s == "gather")    return hullGather;
	if (s == "flalign")   return hullFlAlign;
	if (s == "regexp")    return hullRegexp;
  	lyxerr << "unknown hull type '" << to_utf8(s) << "'" << endl;
	return HullType(-1);
}


docstring hullName(HullType type)
{
	switch (type) {
		case hullNone:       return from_ascii("none");
		case hullSimple:     return from_ascii("simple");
		case hullEquation:   return from_ascii("equation");
		case hullEqnArray:   return from_ascii("eqnarray");
		case hullAlign:      return from_ascii("align");
		case hullAlignAt:    return from_ascii("alignat");
		case hullXAlignAt:   return from_ascii("xalignat");
		case hullXXAlignAt:  return from_ascii("xxalignat");
		case hullMultline:   return from_ascii("multline");
		case hullGather:     return from_ascii("gather");
		case hullFlAlign:    return from_ascii("flalign");
		case hullRegexp:     return from_ascii("regexp");
 		default:
			lyxerr << "unknown hull type '" << type << "'" << endl;
			return from_ascii("none");
	}
}

static InsetLabel * dummy_pointer = 0;

InsetMathHull::InsetMathHull(Buffer * buf)
	: InsetMathGrid(buf, 1, 1), type_(hullNone), numbered_(1, true),
    numbers_(1, empty_docstring()), label_(1, dummy_pointer),
    preview_(new RenderPreview(this))
{
	//lyxerr << "sizeof InsetMath: " << sizeof(InsetMath) << endl;
	//lyxerr << "sizeof MetricsInfo: " << sizeof(MetricsInfo) << endl;
	//lyxerr << "sizeof InsetMathChar: " << sizeof(InsetMathChar) << endl;
	//lyxerr << "sizeof FontInfo: " << sizeof(FontInfo) << endl;
	buffer_ = buf;
	initMath();
	setDefaults();
}


InsetMathHull::InsetMathHull(Buffer * buf, HullType type)
	: InsetMathGrid(buf, getCols(type), 1), type_(type), numbered_(1, true),
    numbers_(1, empty_docstring()), label_(1, dummy_pointer),
    preview_(new RenderPreview(this))
{
	buffer_ = buf;
	initMath();
	setDefaults();
}


InsetMathHull::InsetMathHull(InsetMathHull const & other) : InsetMathGrid(other)
{
	operator=(other);
}


InsetMathHull::~InsetMathHull()
{
	for (size_t i = 0; i < label_.size(); ++i)
		delete label_[i];
}


Inset * InsetMathHull::clone() const
{
	return new InsetMathHull(*this);
}


InsetMathHull & InsetMathHull::operator=(InsetMathHull const & other)
{
	if (this == &other)
		return *this;
	InsetMathGrid::operator=(other);
	type_  = other.type_;
	numbered_ = other.numbered_;
	numbers_ = other.numbers_;
	buffer_ = other.buffer_;
	for (size_t i = 0; i < label_.size(); ++i)
		delete label_[i];
	label_ = other.label_;
	for (size_t i = 0; i != label_.size(); ++i) {
		if (label_[i])
			label_[i] = new InsetLabel(*label_[i]);
	}
	preview_.reset(new RenderPreview(*other.preview_, this));

	return *this;
}


void InsetMathHull::setBuffer(Buffer & buffer)
{
	InsetMathGrid::setBuffer(buffer);

	for (size_t i = 0; i != label_.size(); ++i) {
		if (label_[i])
			label_[i]->setBuffer(buffer);
	}
}


namespace {
	const char * counters_to_save[] = {"section", "chapter"};
	unsigned int const numcnts = sizeof(counters_to_save)/sizeof(char *);
}


void InsetMathHull::updateBuffer(ParIterator const & it, UpdateType utype)
{
	if (!buffer_) {
		//FIXME: buffer_ should be set at creation for this inset! Problem is
		// This inset is created at too many places (see Parser::parse1() in
		// MathParser.cpp).
		return;
	}

	// if any of the equations are numbered, then we want to save the values
	// of some of the counters.
	if (haveNumbers()) {
		BufferParams const & bp = buffer_->params();
		string const & lang = it->getParLanguage(bp)->code();
		Counters & cnts =
			buffer_->masterBuffer()->params().documentClass().counters();

		// right now, we only need to do this at export time
		if (utype == OutputUpdate) {
			for (size_t i = 0; i < numcnts; ++i) {
				docstring const cnt = from_ascii(counters_to_save[i]);
				if (cnts.hasCounter(cnt))
					counter_map[cnt] = cnts.value(cnt);
			}
		}

		// this has to be done separately
		docstring const eqstr = from_ascii("equation");
		if (cnts.hasCounter(eqstr)) {
			if (utype == OutputUpdate)
				counter_map[eqstr] = cnts.value(eqstr);
			for (size_t i = 0; i != label_.size(); ++i) {
				if (numbered(i)) {
					cnts.step(eqstr, utype);
					numbers_[i] = cnts.theCounter(eqstr, lang);
				} else
					numbers_[i] = empty_docstring();
			}
		}
	}

	// now the labels
	for (size_t i = 0; i != label_.size(); ++i) {
		if (label_[i])
			label_[i]->updateBuffer(it, utype);
	}
	// pass down
	InsetMathGrid::updateBuffer(it, utype);
}


void InsetMathHull::addToToc(DocIterator const & pit) const
{
	if (!buffer_) {
		//FIXME: buffer_ should be set at creation for this inset! Problem is
		// This inset is created at too many places (see Parser::parse1() in
		// MathParser.cpp).
		return;
	}

	Toc & toc = buffer().tocBackend().toc("equation");

	for (row_type row = 0; row != nrows(); ++row) {
		if (!numbered_[row])
			continue;
		if (label_[row])
			label_[row]->addToToc(pit);
		toc.push_back(TocItem(pit, 0, nicelabel(row)));
	}
}


Inset * InsetMathHull::editXY(Cursor & cur, int x, int y)
{
	if (use_preview_) {
		edit(cur, true);
		return this;
	}
	return InsetMathNest::editXY(cur, x, y);
}


InsetMath::mode_type InsetMathHull::currentMode() const
{
	if (type_ == hullNone)
		return UNDECIDED_MODE;
	// definitely math mode ...
	return MATH_MODE;
}


bool InsetMathHull::idxFirst(Cursor & cur) const
{
	cur.idx() = 0;
	cur.pos() = 0;
	return true;
}


bool InsetMathHull::idxLast(Cursor & cur) const
{
	cur.idx() = nargs() - 1;
	cur.pos() = cur.lastpos();
	return true;
}


char InsetMathHull::defaultColAlign(col_type col)
{
	if (type_ == hullEqnArray)
		return "rcl"[col];
	if (type_ == hullGather)
		return 'c';
	if (type_ >= hullAlign)
		return "rl"[col & 1];
	return 'c';
}


int InsetMathHull::defaultColSpace(col_type col)
{
	if (type_ == hullAlign || type_ == hullAlignAt)
		return 0;
	if (type_ == hullXAlignAt)
		return (col & 1) ? 20 : 0;
	if (type_ == hullXXAlignAt || type_ == hullFlAlign)
		return (col & 1) ? 40 : 0;
	return 0;
}


docstring InsetMathHull::standardFont() const
{
	docstring font_name;
	switch (type_) {
	case hullRegexp:
		font_name = from_ascii("texttt");
		break;
	case hullNone:
		font_name = from_ascii("lyxnochange");
		break;
	default:
		font_name = from_ascii("mathnormal");
	}
	return font_name;
}


ColorCode InsetMathHull::standardColor() const
{
	ColorCode color;
	switch (type_) {
	case hullRegexp:
	case hullNone:
		color = Color_foreground;
		break;
	default:
		color = Color_math;
	}
	return color;
}


bool InsetMathHull::previewState(BufferView * bv) const
{
	if (!editing(bv) && RenderPreview::status() == LyXRC::PREVIEW_ON) {
		graphics::PreviewImage const * pimage =
			preview_->getPreviewImage(bv->buffer());
		return pimage && pimage->image();
	}
	return false;
}


void InsetMathHull::metrics(MetricsInfo & mi, Dimension & dim) const
{
	if (previewState(mi.base.bv)) {
		preview_->metrics(mi, dim);
		// insert a one pixel gap in front of the formula
		dim.wid += 1;
		if (display())
			dim.des += displayMargin();
		// Cache the inset dimension.
		setDimCache(mi, dim);
		return;
	}

	FontSetChanger dummy1(mi.base, standardFont());
	StyleChanger dummy2(mi.base, display() ? LM_ST_DISPLAY : LM_ST_TEXT);

	// let the cells adjust themselves
	InsetMathGrid::metrics(mi, dim);

	if (display()) {
		dim.asc += displayMargin();
		dim.des += displayMargin();
	}

	if (numberedType()) {
		FontSetChanger dummy(mi.base, from_ascii("mathbf"));
		int l = 0;
		for (row_type row = 0; row < nrows(); ++row)
			l = max(l, mathed_string_width(mi.base.font, nicelabel(row)));

		if (l)
			dim.wid += 30 + l;
	}

	if (type_ == hullRegexp)
		dim.wid += 2;
	// make it at least as high as the current font
	int asc = 0;
	int des = 0;
	math_font_max_dim(mi.base.font, asc, des);
	dim.asc = max(dim.asc, asc);
	dim.des = max(dim.des, des);
	// Cache the inset dimension.
	// FIXME: This will overwrite InsetMathGrid dimension, is that OK?
	setDimCache(mi, dim);
}


ColorCode InsetMathHull::backgroundColor(PainterInfo const & pi) const
{
	if (previewState(pi.base.bv))
		return graphics::PreviewLoader::backgroundColor();
	return Color_mathbg;
}


void InsetMathHull::drawBackground(PainterInfo & pi, int x, int y) const
{
	Dimension const dim = dimension(*pi.base.bv);
	pi.pain.fillRectangle(x + 1, y - dim.asc + 1, dim.wid - 2,
		dim.asc + dim.des - 1, pi.backgroundColor(this));
}


void InsetMathHull::draw(PainterInfo & pi, int x, int y) const
{
	use_preview_ = previewState(pi.base.bv);

	if (type_ == hullRegexp) {
		Dimension const dim = dimension(*pi.base.bv);
		pi.pain.rectangle(x + 1, y - dim.ascent() + 1,
			dim.width() - 2, dim.height() - 2, Color_regexpframe);
	}
	if (use_preview_) {
		// one pixel gap in front
		preview_->draw(pi, x + 1, y);
		setPosCache(pi, x, y);
		return;
	}

	ColorCode color = pi.selected && lyxrc.use_system_colors
				? Color_selectiontext : standardColor();
	bool const really_change_color = pi.base.font.color() == Color_none;
	ColorChanger dummy0(pi.base.font, color, really_change_color);
	FontSetChanger dummy1(pi.base, standardFont());
	StyleChanger dummy2(pi.base, display() ? LM_ST_DISPLAY : LM_ST_TEXT);

	InsetMathGrid::draw(pi, x + 1, y);

	if (numberedType()) {
		int const xx = x + colinfo_.back().offset_ + colinfo_.back().width_ + 20;
		for (row_type row = 0; row < nrows(); ++row) {
			int const yy = y + rowinfo_[row].offset_;
			FontSetChanger dummy(pi.base, from_ascii("mathrm"));
			docstring const nl = nicelabel(row);
			pi.draw(xx, yy, nl);
		}
	}
	setPosCache(pi, x, y);
}


void InsetMathHull::metricsT(TextMetricsInfo const & mi, Dimension & dim) const
{
	if (display()) {
		InsetMathGrid::metricsT(mi, dim);
	} else {
		odocstringstream os;
		WriteStream wi(os, false, true, WriteStream::wsDefault);
		write(wi);
		dim.wid = os.str().size();
		dim.asc = 1;
		dim.des = 0;
	}
}


void InsetMathHull::drawT(TextPainter & pain, int x, int y) const
{
	if (display()) {
		InsetMathGrid::drawT(pain, x, y);
	} else {
		odocstringstream os;
		WriteStream wi(os, false, true, WriteStream::wsDefault);
		write(wi);
		pain.draw(x, y, os.str().c_str());
	}
}


static docstring latexString(InsetMathHull const & inset)
{
	odocstringstream ls;
	// This has to be static, because a preview snippet or a math
	// macro containing math in text mode (such as $\text{$\phi$}$ or
	// \newcommand{\xxx}{\text{$\phi$}}) gets processed twice. The
	// first time as a whole, and the second time only the inner math.
	// In this last case inset.buffer() would be invalid.
	static Encoding const * encoding = 0;
	if (inset.isBufferValid())
		encoding = &(inset.buffer().params().encoding());
	WriteStream wi(ls, false, true, WriteStream::wsPreview, encoding);
	inset.write(wi);
	return ls.str();
}


void InsetMathHull::initUnicodeMath() const
{
	// Trigger classification of the unicode symbols in this inset
	docstring const dummy = latexString(*this);
}


void InsetMathHull::addPreview(DocIterator const & inset_pos,
	graphics::PreviewLoader & /*ploader*/) const
{
	if (RenderPreview::status() == LyXRC::PREVIEW_ON) {
		preparePreview(inset_pos);
	}
}


void InsetMathHull::preparePreview(DocIterator const & pos,
                                   bool forexport) const  
{
	// there is no need to do all the macro stuff if we're not
	// actually going to generate the preview.
	if (RenderPreview::status() != LyXRC::PREVIEW_ON && !forexport)
		return;
	
	Buffer const * buffer = pos.buffer();

	// collect macros at this position
	MacroNameSet macros;
	buffer->listMacroNames(macros);
	MacroNameSet::iterator it = macros.begin();
	MacroNameSet::iterator end = macros.end();
	odocstringstream macro_preamble;
	for (; it != end; ++it) {
		MacroData const * data = buffer->getMacro(*it, pos, true);
		if (data) {
			data->write(macro_preamble, true);
			macro_preamble << endl;
		}
	}

	docstring setcnt;
	if (forexport && haveNumbers()) {
		docstring eqstr = from_ascii("equation");
		CounterMap::const_iterator it = counter_map.find(eqstr);
		if (it != counter_map.end()) {
			int num = it->second;
			if (num >= 0)
				setcnt += from_ascii("\\setcounter{") + eqstr + '}' +
				          '{' + convert<docstring>(num) + '}' + '\n';
		}
		for (size_t i = 0; i != numcnts; ++i) {
			docstring cnt = from_ascii(counters_to_save[i]);
			it = counter_map.find(cnt);
			if (it == counter_map.end())
					continue;
			int num = it->second;
			if (num > 0)
				setcnt += from_ascii("\\setcounter{") + cnt + '}' +
				          '{' + convert<docstring>(num) + '}';
		}
	}
	docstring const snippet = macro_preamble.str() +
	    setcnt + latexString(*this);
	LYXERR(Debug::MACROS, "Preview snippet: " << snippet);
	preview_->addPreview(snippet, *buffer, forexport);
}


void InsetMathHull::reloadPreview(DocIterator const & pos) const
{
	preparePreview(pos);
	preview_->startLoading(*pos.buffer());
}


void InsetMathHull::loadPreview(DocIterator const & pos) const
{
	bool const forexport = true;
	preparePreview(pos, forexport);
	preview_->startLoading(*pos.buffer(), forexport);
}


bool InsetMathHull::notifyCursorLeaves(Cursor const & old, Cursor & cur)
{
	if (RenderPreview::status() == LyXRC::PREVIEW_ON) {
		reloadPreview(old);
		cur.screenUpdateFlags(Update::Force);
	}
	return false;
}


docstring InsetMathHull::label(row_type row) const
{
	LASSERT(row < nrows(), /**/);
	if (InsetLabel * il = label_[row])
		return il->screenLabel();
	return docstring();
}


void InsetMathHull::label(row_type row, docstring const & label)
{
	//lyxerr << "setting label '" << label << "' for row " << row << endl;
	if (label_[row]) {
		if (label.empty()) {
			delete label_[row];
			label_[row] = dummy_pointer;
		} else {
			if (buffer_)
				label_[row]->updateCommand(label);
			else
				label_[row]->setParam("name", label);
		}
		return;
	}
	InsetCommandParams p(LABEL_CODE);
	p["name"] = label;
	label_[row] = new InsetLabel(buffer_, p);
	if (buffer_)
		label_[row]->setBuffer(buffer());
}


void InsetMathHull::numbered(row_type row, bool num)
{
	numbered_[row] = num;
	if (!numbered_[row] && label_[row]) {
		delete label_[row];
		label_[row] = 0;
	}
}


bool InsetMathHull::numbered(row_type row) const
{
	return numbered_[row];
}


bool InsetMathHull::ams() const
{
	return type_ == hullAlign
		|| type_ == hullFlAlign
		|| type_ == hullMultline
		|| type_ == hullGather
		|| type_ == hullAlignAt
		|| type_ == hullXAlignAt
		|| type_ == hullXXAlignAt;
}


Inset::DisplayType InsetMathHull::display() const
{
	if (type_ == hullSimple || type_ == hullNone || type_ == hullRegexp)
		return Inline;
	return AlignCenter;
}

bool InsetMathHull::numberedType() const
{
	if (type_ == hullNone)
		return false;
	if (type_ == hullSimple)
		return false;
	if (type_ == hullXXAlignAt)
		return false;
	if (type_ == hullRegexp)
		return false;
	for (row_type row = 0; row < nrows(); ++row)
		if (numbered_[row])
			return true;
	return false;
}


void InsetMathHull::validate(LaTeXFeatures & features) const
{
	if (features.runparams().isLaTeX()) {
		if (ams())
			features.require("amsmath");
	
		if (type_ == hullRegexp) {
			features.require("color");
			string frcol = lcolor.getLaTeXName(Color_regexpframe);
			string bgcol = "white";
			features.addPreambleSnippet(
				string("\\newcommand{\\regexp}[1]{\\fcolorbox{")
				+ frcol + string("}{")
				+ bgcol + string("}{\\ensuremath{\\mathtt{#1}}}}"));
			features.addPreambleSnippet(
				string("\\newcommand{\\endregexp}{}"));
		}
	
		// Validation is necessary only if not using AMS math.
		// To be safe, we will always run mathedvalidate.
		//if (features.amsstyle)
		//  return;
	
		//features.binom      = true;
	} else if (features.runparams().math_flavor == OutputParams::MathAsHTML) {
		// it would be better to do this elsewhere, but we can't validate in
		// InsetMathMatrix and we have no way, outside MathExtern, to know if
		// we even have any matrices.
				features.addPreambleSnippet("<style type=\"text/css\">\n"
					"table.matrix{display: inline-block; vertical-align: middle; text-align:center;}\n"
					"table.matrix td{padding: 0.25px;}\n"
					"td.ldelim{width: 0.5ex; border: thin solid black; border-right: none;}\n"
					"td.rdelim{width: 0.5ex; border: thin solid black; border-left: none;}\n"
					"</style>");
	}
	InsetMathGrid::validate(features);
}


void InsetMathHull::header_write(WriteStream & os) const
{
	bool n = numberedType();

	switch(type_) {
	case hullNone:
		break;

	case hullSimple:
		os << '$';
		if (cell(0).empty())
			os << ' ';
		break;

	case hullEquation:
		if (n)
			os << "\n\\begin{equation" << star(n) << "}\n";
		else
			os << "\n\\[\n";
		break;

	case hullEqnArray:
	case hullAlign:
	case hullFlAlign:
	case hullGather:
	case hullMultline:
		os << "\n\\begin{" << hullName(type_) << star(n) << "}\n";
		break;

	case hullAlignAt:
	case hullXAlignAt:
		os << "\n\\begin{" << hullName(type_) << star(n) << '}'
		  << '{' << static_cast<unsigned int>((ncols() + 1)/2) << "}\n";
		break;

	case hullXXAlignAt:
		os << "\n\\begin{" << hullName(type_) << '}'
		  << '{' << static_cast<unsigned int>((ncols() + 1)/2) << "}\n";
		break;

	case hullRegexp:
		os << "\\regexp{";
		break;

	default:
		os << "\n\\begin{unknown" << star(n) << "}\n";
		break;
	}
}


void InsetMathHull::footer_write(WriteStream & os) const
{
	bool n = numberedType();

	switch(type_) {
	case hullNone:
		os << "\n";
		break;

	case hullSimple:
		os << '$';
		break;

	case hullEquation:
		if (n)
			os << "\n\\end{equation" << star(n) << "}\n";
		else
			os << "\n\\]\n";
		break;

	case hullEqnArray:
	case hullAlign:
	case hullFlAlign:
	case hullAlignAt:
	case hullXAlignAt:
	case hullGather:
	case hullMultline:
		os << "\n\\end{" << hullName(type_) << star(n) << "}\n";
		break;

	case hullXXAlignAt:
		os << "\n\\end{" << hullName(type_) << "}\n";
		break;

	case hullRegexp:
		// Only used as a heuristic to find the regexp termination, when searching in ignore-format mode
		os << "\\endregexp{}}";
		break;

	default:
		os << "\n\\end{unknown" << star(n) << "}\n";
		break;
	}
}


bool InsetMathHull::rowChangeOK() const
{
	return
		type_ == hullEqnArray || type_ == hullAlign ||
		type_ == hullFlAlign || type_ == hullAlignAt ||
		type_ == hullXAlignAt || type_ == hullXXAlignAt ||
		type_ == hullGather || type_ == hullMultline;
}


bool InsetMathHull::colChangeOK() const
{
	return
		type_ == hullAlign || type_ == hullFlAlign ||type_ == hullAlignAt ||
		type_ == hullXAlignAt || type_ == hullXXAlignAt;
}


void InsetMathHull::addRow(row_type row)
{
	if (!rowChangeOK())
		return;

	bool numbered = numberedType();
	// Move the number and raw pointer, do not call label() (bug 7511)
	InsetLabel * label = dummy_pointer;
	docstring number = empty_docstring();
	if (type_ == hullMultline) {
		if (row + 1 == nrows())  {
			numbered_[row] = false;
			swap(label, label_[row]);
			swap(number, numbers_[row]);
		} else
			numbered = false;
	}

	numbered_.insert(numbered_.begin() + row + 1, numbered);
	numbers_.insert(numbers_.begin() + row + 1, number);
	label_.insert(label_.begin() + row + 1, label);
	InsetMathGrid::addRow(row);
}


void InsetMathHull::swapRow(row_type row)
{
	if (nrows() <= 1)
		return;
	if (row + 1 == nrows())
		--row;
	// gcc implements the standard std::vector<bool> which is *not* a container:
	//   http://www.gotw.ca/publications/N1185.pdf
	// As a results, it doesn't like this:
	//	swap(numbered_[row], numbered_[row + 1]);
	// so we do it manually:
	bool const b = numbered_[row];
	numbered_[row] = numbered_[row + 1];
	numbered_[row + 1] = b;
	swap(numbers_[row], numbers_[row + 1]);
	swap(label_[row], label_[row + 1]);
	InsetMathGrid::swapRow(row);
}


void InsetMathHull::delRow(row_type row)
{
	if (nrows() <= 1 || !rowChangeOK())
		return;
	if (row + 1 == nrows() && type_ == hullMultline) {
		bool const b = numbered_[row - 1];
		numbered_[row - 1] = numbered_[row];
		numbered_[row] = b;
		swap(numbers_[row - 1], numbers_[row]);
		swap(label_[row - 1], label_[row]);
		InsetMathGrid::delRow(row);
		return;
	}
	InsetMathGrid::delRow(row);
	// The last dummy row has no number info nor a label.
	// Test nrows() + 1 because we have already erased the row.
	if (row == nrows() + 1)
		row--;
	numbered_.erase(numbered_.begin() + row);
	numbers_.erase(numbers_.begin() + row);
	delete label_[row];
	label_.erase(label_.begin() + row);
}


void InsetMathHull::addCol(col_type col)
{
	if (!colChangeOK())
		return;
	InsetMathGrid::addCol(col);
}


void InsetMathHull::delCol(col_type col)
{
	if (ncols() <= 1 || !colChangeOK())
		return;
	InsetMathGrid::delCol(col);
}


docstring InsetMathHull::nicelabel(row_type row) const
{
	if (!numbered_[row])
		return docstring();
	docstring const & val = numbers_[row];
	if (!label_[row])
		return '(' + val + ')';
	return '(' + val + ',' + label_[row]->screenLabel() + ')';
}


void InsetMathHull::glueall(HullType type)
{
	MathData ar;
	for (idx_type i = 0; i < nargs(); ++i)
		ar.append(cell(i));
	InsetLabel * label = 0;
	if (type == hullEquation) {
		// preserve first non-empty label
		for (row_type row = 0; row < nrows(); ++row) {
			if (label_[row]) {
				label = label_[row];
				label_[row] = 0;
				break;
			}
		}
	}
	*this = InsetMathHull(buffer_, hullSimple);
	label_[0] = label;
	cell(0) = ar;
	setDefaults();
}


void InsetMathHull::splitTo2Cols()
{
	LASSERT(ncols() == 1, /**/);
	InsetMathGrid::addCol(1);
	for (row_type row = 0; row < nrows(); ++row) {
		idx_type const i = 2 * row;
		pos_type pos = firstRelOp(cell(i));
		cell(i + 1) = MathData(buffer_, cell(i).begin() + pos, cell(i).end());
		cell(i).erase(pos, cell(i).size());
	}
}


void InsetMathHull::splitTo3Cols()
{
	LASSERT(ncols() < 3, /**/);
	if (ncols() < 2)
		splitTo2Cols();
	InsetMathGrid::addCol(2);
	for (row_type row = 0; row < nrows(); ++row) {
		idx_type const i = 3 * row + 1;
		if (cell(i).size()) {
			cell(i + 1) = MathData(buffer_, cell(i).begin() + 1, cell(i).end());
			cell(i).erase(1, cell(i).size());
		}
	}
}


void InsetMathHull::changeCols(col_type cols)
{
	if (ncols() == cols)
		return;
	else if (ncols() < cols) {
		// split columns
		if (cols < 3)
			splitTo2Cols();
		else {
			splitTo3Cols();
			while (ncols() < cols)
				InsetMathGrid::addCol(ncols());
		}
		return;
	}

	// combine columns
	for (row_type row = 0; row < nrows(); ++row) {
		idx_type const i = row * ncols();
		for (col_type col = cols; col < ncols(); ++col) {
			cell(i + cols - 1).append(cell(i + col));
		}
	}
	// delete columns
	while (ncols() > cols) {
		InsetMathGrid::delCol(ncols() - 1);
	}
}


HullType InsetMathHull::getType() const
{
	return type_;
}


void InsetMathHull::setType(HullType type)
{
	type_ = type;
	setDefaults();
}


void InsetMathHull::mutate(HullType newtype)
{
	//lyxerr << "mutating from '" << type_ << "' to '" << newtype << "'" << endl;

	// we try to move along the chain
	// none <-> simple <-> equation <-> eqnarray -> *align* -> multline, gather -+
	//                                     ^                                     |
	//                                     +-------------------------------------+
	// we use eqnarray as intermediate type for mutations that are not
	// directly supported because it handles labels and numbering for
	// "down mutation".

	if (newtype == type_) {
		// done
	}

	else if (newtype < hullNone) {
		// unknown type
		dump();
	}

	else if (type_ == hullNone) {
		setType(hullSimple);
		numbered(0, false);
		mutate(newtype);
	}

	else if (type_ == hullSimple) {
		if (newtype == hullNone) {
			setType(hullNone);
			numbered(0, false);
		} else {
			setType(hullEquation);
			numbered(0, label_[0] ? true : false);
			mutate(newtype);
		}
	}

	else if (type_ == hullEquation) {
		if (newtype < type_) {
			setType(hullSimple);
			numbered(0, false);
			mutate(newtype);
		} else if (newtype == hullEqnArray) {
			// split it "nicely" on the first relop
			splitTo3Cols();
			setType(hullEqnArray);
		} else if (newtype == hullMultline || newtype == hullGather) {
			setType(newtype);
		} else {
			// split it "nicely"
			splitTo2Cols();
			setType(hullAlign);
			mutate(newtype);
		}
	}

	else if (type_ == hullEqnArray) {
		if (newtype < type_) {
			glueall(newtype);
			mutate(newtype);
		} else { // align & Co.
			changeCols(2);
			setType(hullAlign);
			mutate(newtype);
		}
	}

	else if (type_ ==  hullAlign || type_ == hullAlignAt ||
		 type_ == hullXAlignAt || type_ == hullFlAlign) {
		if (newtype < hullAlign) {
			changeCols(3);
			setType(hullEqnArray);
			mutate(newtype);
		} else if (newtype == hullGather || newtype == hullMultline) {
			changeCols(1);
			setType(newtype);
		} else if (newtype ==   hullXXAlignAt) {
			for (row_type row = 0; row < nrows(); ++row)
				numbered(row, false);
			setType(newtype);
		} else {
			setType(newtype);
		}
	}

	else if (type_ == hullXXAlignAt) {
		for (row_type row = 0; row < nrows(); ++row)
			numbered(row, false);
		if (newtype < hullAlign) {
			changeCols(3);
			setType(hullEqnArray);
			mutate(newtype);
		} else if (newtype == hullGather || newtype == hullMultline) {
			changeCols(1);
			setType(newtype);
		} else {
			setType(newtype);
		}
	}

	else if (type_ == hullMultline || type_ == hullGather) {
		if (newtype == hullGather || newtype == hullMultline)
			setType(newtype);
		else if (newtype == hullAlign || newtype == hullFlAlign  ||
			 newtype == hullAlignAt || newtype == hullXAlignAt) {
			splitTo2Cols();
			setType(newtype);
		} else if (newtype ==   hullXXAlignAt) {
			splitTo2Cols();
			for (row_type row = 0; row < nrows(); ++row)
				numbered(row, false);
			setType(newtype);
		} else {
			splitTo3Cols();
			setType(hullEqnArray);
			mutate(newtype);
		}
	}

	else {
		lyxerr << "mutation from '" << to_utf8(hullName(type_))
		       << "' to '" << to_utf8(hullName(newtype))
		       << "' not implemented" << endl;
	}
}


docstring InsetMathHull::eolString(row_type row, bool fragile, bool latex,
		bool last_eoln) const
{
	docstring res;
	if (numberedType()) {
		if (label_[row] && numbered_[row]) {
			docstring const name =
				latex ? escape(label_[row]->getParam("name"))
				      : label_[row]->getParam("name");
			res += "\\label{" + name + '}';
		}
		if (!numbered_[row] && (type_ != hullMultline))
			res += "\\nonumber ";
	}
	// Never add \\ on the last empty line of eqnarray and friends
	last_eoln = false;
	return res + InsetMathGrid::eolString(row, fragile, latex, last_eoln);
}


void InsetMathHull::write(WriteStream & os) const
{
	ModeSpecifier specifier(os, MATH_MODE);
	header_write(os);
	InsetMathGrid::write(os);
	footer_write(os);
}


void InsetMathHull::normalize(NormalStream & os) const
{
	os << "[formula " << hullName(type_) << ' ';
	InsetMathGrid::normalize(os);
	os << "] ";
}


void InsetMathHull::infoize(odocstream & os) const
{
	os << "Type: " << hullName(type_);
}


void InsetMathHull::check() const
{
	LASSERT(numbered_.size() == nrows(), /**/);
	LASSERT(numbers_.size() == nrows(), /**/);
	LASSERT(label_.size() == nrows(), /**/);
}


void InsetMathHull::doExtern(Cursor & cur, FuncRequest & func)
{
	docstring dlang;
	docstring extra;
	idocstringstream iss(func.argument());
	iss >> dlang >> extra;
	if (extra.empty())
		extra = from_ascii("noextra");
	string const lang = to_ascii(dlang);

	// FIXME: temporarily disabled
	//if (cur.selection()) {
	//	MathData ar;
	//	selGet(cur.ar);
	//	lyxerr << "use selection: " << ar << endl;
	//	insert(pipeThroughExtern(lang, extra, ar));
	//	return;
	//}

	// only inline, display or eqnarray math is allowed
	if (getType() > hullEqnArray) {
		frontend::Alert::warning(_("Bad math environment"),
				_("Computation cannot be performed for AMS "
				  "math environments.\nChange the math "
				  "formula type and try again."));
		return;
	}

	MathData eq;
	eq.push_back(MathAtom(new InsetMathChar('=')));

	// go to first item in line
	cur.idx() -= cur.idx() % ncols();
	cur.pos() = 0;

	if (getType() == hullSimple) {
		size_type pos = cur.cell().find_last(eq);
		MathData ar;
		if (cur.inMathed() && cur.selection()) {
			asArray(grabAndEraseSelection(cur), ar);
		} else if (pos == cur.cell().size()) {
			ar = cur.cell();
			lyxerr << "use whole cell: " << ar << endl;
		} else {
			ar = MathData(buffer_, cur.cell().begin() + pos + 1, cur.cell().end());
			lyxerr << "use partial cell form pos: " << pos << endl;
		}
		cur.cell().append(eq);
		cur.cell().append(pipeThroughExtern(lang, extra, ar));
		cur.pos() = cur.lastpos();
		return;
	}

	if (getType() == hullEquation) {
		lyxerr << "use equation inset" << endl;
		mutate(hullEqnArray);
		MathData & ar = cur.cell();
		lyxerr << "use cell: " << ar << endl;
		++cur.idx();
		cur.cell() = eq;
		++cur.idx();
		cur.cell() = pipeThroughExtern(lang, extra, ar);
		// move to end of line
		cur.pos() = cur.lastpos();
		return;
	}

	{
		lyxerr << "use eqnarray" << endl;
		cur.idx() += 2 - cur.idx() % ncols();
		cur.pos() = 0;
		MathData ar = cur.cell();
		lyxerr << "use cell: " << ar << endl;
		// FIXME: temporarily disabled
		addRow(cur.row());
		++cur.idx();
		++cur.idx();
		cur.cell() = eq;
		++cur.idx();
		cur.cell() = pipeThroughExtern(lang, extra, ar);
		cur.pos() = cur.lastpos();
	}
}


void InsetMathHull::doDispatch(Cursor & cur, FuncRequest & cmd)
{
	//lyxerr << "action: " << cmd.action() << endl;
	switch (cmd.action()) {

	case LFUN_FINISHED_BACKWARD:
	case LFUN_FINISHED_FORWARD:
	case LFUN_FINISHED_RIGHT:
	case LFUN_FINISHED_LEFT:
		//lyxerr << "action: " << cmd.action() << endl;
		InsetMathGrid::doDispatch(cur, cmd);
		cur.undispatched();
		break;

	case LFUN_BREAK_PARAGRAPH:
		// just swallow this
		break;

	case LFUN_NEWLINE_INSERT:
		// some magic for the common case
		if (type_ == hullSimple || type_ == hullEquation) {
			cur.recordUndoInset();
			bool const align =
				cur.bv().buffer().params().use_amsmath == BufferParams::package_on;
			mutate(align ? hullAlign : hullEqnArray);
			// mutate() may change labels and such.
			cur.forceBufferUpdate();
			cur.idx() = nrows() * ncols() - 1;
			cur.pos() = cur.lastpos();
		}
		InsetMathGrid::doDispatch(cur, cmd);
		break;

	case LFUN_MATH_NUMBER_TOGGLE: {
		//lyxerr << "toggling all numbers" << endl;
		cur.recordUndoInset();
		bool old = numberedType();
		if (type_ == hullMultline)
			numbered(nrows() - 1, !old);
		else
			for (row_type row = 0; row < nrows(); ++row)
				numbered(row, !old);

		cur.message(old ? _("No number") : _("Number"));
		cur.forceBufferUpdate();
		break;
	}

	case LFUN_MATH_NUMBER_LINE_TOGGLE: {
		cur.recordUndoInset();
		row_type r = (type_ == hullMultline) ? nrows() - 1 : cur.row();
		bool old = numbered(r);
		cur.message(old ? _("No number") : _("Number"));
		numbered(r, !old);
		cur.forceBufferUpdate();
		break;
	}

	case LFUN_LABEL_INSERT: {
		row_type r = (type_ == hullMultline) ? nrows() - 1 : cur.row();
		docstring old_label = label(r);
		docstring const default_label = from_ascii("eq:");
		if (old_label.empty())
			old_label = default_label;

		InsetCommandParams p(LABEL_CODE);
		p["name"] = cmd.argument().empty() ? old_label : cmd.argument();
		string const data = InsetCommand::params2string(p);

		if (cmd.argument().empty())
			cur.bv().showDialog("label", data);
		else {
			FuncRequest fr(LFUN_INSET_INSERT, data);
			dispatch(cur, fr);
		}
		break;
	}

	case LFUN_LABEL_COPY_AS_REF: {
		row_type row;
		if (cmd.argument().empty() && &cur.inset() == this)
			// if there is no argument and we're inside math, we retrieve
			// the row number from the cursor position.
			row = (type_ == hullMultline) ? nrows() - 1 : cur.row();
		else {
			// if there is an argument, find the corresponding label, else
			// check whether there is at least one label.
			for (row = 0; row != nrows(); ++row)
				if (numbered_[row] && label_[row]
					  && (cmd.argument().empty() || label(row) == cmd.argument()))
					break;
		}

		if (row == nrows())
			break;

		InsetCommandParams p(REF_CODE, "ref");
		p["reference"] = label(row);
		cap::clearSelection();
		cap::copyInset(cur, new InsetRef(buffer_, p), label(row));
		break;
	}

	case LFUN_WORD_DELETE_FORWARD:
	case LFUN_CHAR_DELETE_FORWARD:
		if (col(cur.idx()) + 1 == ncols()
		    && cur.pos() == cur.lastpos()
		    && !cur.selection()) {
			if (!label(row(cur.idx())).empty()) {
				cur.recordUndoInset();
				label(row(cur.idx()), docstring());
			} else if (numbered(row(cur.idx()))) {
				cur.recordUndoInset();
				numbered(row(cur.idx()), false);
				cur.forceBufferUpdate();
			} else {
				InsetMathGrid::doDispatch(cur, cmd);
				return;
			}
		} else {
			InsetMathGrid::doDispatch(cur, cmd);
			return;
		}
		break;

	case LFUN_INSET_INSERT: {
		//lyxerr << "arg: " << to_utf8(cmd.argument()) << endl;
		// FIXME: this should be cleaned up to use InsetLabel methods directly.
		string const name = cmd.getArg(0);
		if (name == "label") {
			InsetCommandParams p(LABEL_CODE);
			InsetCommand::string2params(to_utf8(cmd.argument()), p);
			docstring str = p["name"];
			cur.recordUndoInset();
			row_type const r = (type_ == hullMultline) ? nrows() - 1 : cur.row();
			str = trim(str);
			if (!str.empty())
				numbered(r, true);
			docstring old = label(r);
			if (str != old) {
				if (label_[r])
					// The label will take care of the reference update.
					label(r, str);
				else {
					label(r, str);
					// Newly created inset so initialize it.
					label_[r]->initView();
				}
			}
			cur.forceBufferUpdate();
			break;
		}
		InsetMathGrid::doDispatch(cur, cmd);
		return;
	}

	case LFUN_MATH_EXTERN:
		cur.recordUndoInset();
		doExtern(cur, cmd);
		break;

	case LFUN_MATH_MUTATE: {
		cur.recordUndoInset();
		row_type row = cur.row();
		col_type col = cur.col();
		mutate(hullType(cmd.argument()));
		cur.idx() = row * ncols() + col;
		if (cur.idx() > cur.lastidx()) {
			cur.idx() = cur.lastidx();
			cur.pos() = cur.lastpos();
		}
		if (cur.pos() > cur.lastpos())
			cur.pos() = cur.lastpos();

		cur.forceBufferUpdate();
		// FIXME: find some more clever handling of the selection,
		// i.e. preserve it.
		cur.clearSelection();
		//cur.dispatched(FINISHED);
		break;
	}

	case LFUN_MATH_DISPLAY: {
		cur.recordUndoInset();
		mutate(type_ == hullSimple ? hullEquation : hullSimple);
		cur.idx() = 0;
		cur.pos() = cur.lastpos();
		//cur.dispatched(FINISHED);
		break;
	}

	default:
		InsetMathGrid::doDispatch(cur, cmd);
		break;
	}
}


bool InsetMathHull::getStatus(Cursor & cur, FuncRequest const & cmd,
		FuncStatus & status) const
{
	switch (cmd.action()) {
	case LFUN_FINISHED_BACKWARD:
	case LFUN_FINISHED_FORWARD:
	case LFUN_FINISHED_RIGHT:
	case LFUN_FINISHED_LEFT:
	case LFUN_UP:
	case LFUN_DOWN:
	case LFUN_NEWLINE_INSERT:
	case LFUN_MATH_EXTERN:
		// we handle these
		status.setEnabled(true);
		return true;

	// we never allow this in math, and we want to bind enter
	// to another actions in command-alternatives
	case LFUN_BREAK_PARAGRAPH:
		status.setEnabled(false);
		return true;
	case LFUN_MATH_MUTATE: {
		HullType const ht = hullType(cmd.argument());
		status.setOnOff(type_ == ht);
		status.setEnabled(true);

		if (ht != hullSimple) {
			Cursor tmpcur = cur;
			while (!tmpcur.empty()) {
				InsetCode code = tmpcur.inset().lyxCode();
				if (code == BOX_CODE) {
					return true;
				} else if (code == TABULAR_CODE) {
					FuncRequest tmpcmd(LFUN_MATH_DISPLAY);
					if (tmpcur.getStatus(tmpcmd, status) && !status.enabled())
						return true;
				}
				tmpcur.pop_back();
			}
		}
		return true;
	}
	case LFUN_MATH_DISPLAY: {
		bool enable = true;
		if (cur.depth() > 1) {
			Inset const & in = cur[cur.depth()-2].inset();
			if (in.lyxCode() == SCRIPT_CODE)
				enable = display() != Inline;
		}
		status.setEnabled(enable);
		return true;
	}

	case LFUN_MATH_NUMBER_TOGGLE:
		// FIXME: what is the right test, this or the one of
		// LABEL_INSERT?
		status.setEnabled(display() != Inline);
		status.setOnOff(numberedType());
		return true;

	case LFUN_MATH_NUMBER_LINE_TOGGLE: {
		// FIXME: what is the right test, this or the one of
		// LABEL_INSERT?
		bool const enable = (type_ == hullMultline)
			? (nrows() - 1 == cur.row())
			: display() != Inline;
		row_type const r = (type_ == hullMultline) ? nrows() - 1 : cur.row();
		status.setEnabled(enable);
		status.setOnOff(enable && numbered(r));
		return true;
	}

	case LFUN_LABEL_INSERT:
		status.setEnabled(type_ != hullSimple);
		return true;

	case LFUN_LABEL_COPY_AS_REF: {
		bool enabled = false;
		row_type row;
		if (cmd.argument().empty() && &cur.inset() == this) {
			// if there is no argument and we're inside math, we retrieve
			// the row number from the cursor position.
			row = (type_ == hullMultline) ? nrows() - 1 : cur.row();
			enabled = numberedType() && label_[row] && numbered_[row];
		} else {
			// if there is an argument, find the corresponding label, else
			// check whether there is at least one label.
			for (row_type row = 0; row != nrows(); ++row) {
				if (numbered_[row] && label_[row] && 
					(cmd.argument().empty() || label(row) == cmd.argument())) {
						enabled = true;
						break;
				}
			}
		}
		status.setEnabled(enabled);
		return true;
	}

	case LFUN_INSET_INSERT:
		if (cmd.getArg(0) == "label") {
			status.setEnabled(type_ != hullSimple);
			return true;
		}
		return InsetMathGrid::getStatus(cur, cmd, status);

	case LFUN_INSET_MODIFY: {
		istringstream is(to_utf8(cmd.argument()));
		string s;
		is >> s;
		if (s != "tabular")
			return InsetMathGrid::getStatus(cur, cmd, status);
		is >> s;
		if (!rowChangeOK()
		    && (s == "append-row"
			|| s == "delete-row"
			|| s == "copy-row")) {
			status.message(bformat(
				from_utf8(N_("Can't change number of rows in '%1$s'")),
				hullName(type_)));
			status.setEnabled(false);
			return true;
		}
		if (!colChangeOK()
		    && (s == "append-column"
			|| s == "delete-column"
			|| s == "copy-column")) {
			status.message(bformat(
				from_utf8(N_("Can't change number of columns in '%1$s'")),
				hullName(type_)));
			status.setEnabled(false);
			return true;
		}
		if ((type_ == hullSimple
		  || type_ == hullEquation
		  || type_ == hullNone) &&
		    (s == "add-hline-above" || s == "add-hline-below")) {
			status.message(bformat(
				from_utf8(N_("Can't add horizontal grid lines in '%1$s'")),
				hullName(type_)));
			status.setEnabled(false);
			return true;
		}
		if (s == "add-vline-left" || s == "add-vline-right") {
			status.message(bformat(
				from_utf8(N_("Can't add vertical grid lines in '%1$s'")),
				hullName(type_)));
			status.setEnabled(false);
			return true;
		}
		if (s == "valign-top" || s == "valign-middle"
		 || s == "valign-bottom" || s == "align-left"
		 || s == "align-center" || s == "align-right") {
			status.setEnabled(false);
			return true;
		}
		return InsetMathGrid::getStatus(cur, cmd, status);
	}

	default:
		return InsetMathGrid::getStatus(cur, cmd, status);
	}

	// This cannot really happen, but inserted to shut-up gcc
	return InsetMathGrid::getStatus(cur, cmd, status);
}


/////////////////////////////////////////////////////////////////////



// simply scrap this function if you want
void InsetMathHull::mutateToText()
{
#if 0
	// translate to latex
	ostringstream os;
	latex(os, false, false);
	string str = os.str();

	// insert this text
	Text * lt = view_->cursor().innerText();
	string::const_iterator cit = str.begin();
	string::const_iterator end = str.end();
	for (; cit != end; ++cit)
		view_->getIntl()->getTransManager().TranslateAndInsert(*cit, lt);

	// remove ourselves
	//dispatch(LFUN_ESCAPE);
#endif
}


void InsetMathHull::handleFont(Cursor & cur, docstring const & arg,
	docstring const & font)
{
	// this whole function is a hack and won't work for incremental font
	// changes...
	cur.recordUndo();
	if (cur.inset().asInsetMath()->name() == font)
		cur.handleFont(to_utf8(font));
	else {
		cur.handleNest(createInsetMath(font, cur.buffer()));
		cur.insert(arg);
	}
}


void InsetMathHull::handleFont2(Cursor & cur, docstring const & arg)
{
	cur.recordUndo();
	Font font;
	bool b;
	font.fromString(to_utf8(arg), b);
	if (font.fontInfo().color() != Color_inherit) {
		MathAtom at = MathAtom(new InsetMathColor(buffer_, true, font.fontInfo().color()));
		cur.handleNest(at, 0);
	}
}


void InsetMathHull::edit(Cursor & cur, bool front, EntryDirection entry_from)
{
	cur.push(*this);
	bool enter_front = (entry_from == Inset::ENTRY_DIRECTION_LEFT ||
		(entry_from == Inset::ENTRY_DIRECTION_IGNORE && front));
	enter_front ? idxFirst(cur) : idxLast(cur);
	// The inset formula dimension is not necessarily the same as the
	// one of the instant preview image, so we have to indicate to the
	// BufferView that a metrics update is needed.
	cur.screenUpdateFlags(Update::Force);
}


void InsetMathHull::revealCodes(Cursor & cur) const
{
	if (!cur.inMathed())
		return;
	odocstringstream os;
	cur.info(os);
	cur.message(os.str());
/*
	// write something to the minibuffer
	// translate to latex
	cur.markInsert(bv);
	ostringstream os;
	write(os);
	string str = os.str();
	cur.markErase(bv);
	string::size_type pos = 0;
	string res;
	for (string::iterator it = str.begin(); it != str.end(); ++it) {
		if (*it == '\n')
			res += ' ';
		else if (*it == '\0') {
			res += "  -X-  ";
			pos = it - str.begin();
		}
		else
			res += *it;
	}
	if (pos > 30)
		res = res.substr(pos - 30);
	if (res.size() > 60)
		res = res.substr(0, 60);
	cur.message(res);
*/
}


/////////////////////////////////////////////////////////////////////


#if 0
bool InsetMathHull::searchForward(BufferView * bv, string const & str,
				     bool, bool)
{
	// FIXME: completely broken
	static InsetMathHull * lastformula = 0;
	static CursorBase current = DocIterator(ibegin(nucleus()));
	static MathData ar;
	static string laststr;

	if (lastformula != this || laststr != str) {
		//lyxerr << "reset lastformula to " << this << endl;
		lastformula = this;
		laststr = str;
		current	= ibegin(nucleus());
		ar.clear();
		mathed_parse_cell(ar, str, Parse::NORMAL, &buffer());
	} else {
		increment(current);
	}
	//lyxerr << "searching '" << str << "' in " << this << ar << endl;

	for (DocIterator it = current; it != iend(nucleus()); increment(it)) {
		CursorSlice & top = it.back();
		MathData const & a = top.asInsetMath()->cell(top.idx_);
		if (a.matchpart(ar, top.pos_)) {
			bv->cursor().setSelection(it, ar.size());
			current = it;
			top.pos_ += ar.size();
			bv->update();
			return true;
		}
	}

	//lyxerr << "not found!" << endl;
	lastformula = 0;
	return false;
}
#endif


void InsetMathHull::write(ostream & os) const
{
	odocstringstream oss;
	WriteStream wi(oss, false, false, WriteStream::wsDefault);
	oss << "Formula ";
	write(wi);
	os << to_utf8(oss.str());
}


void InsetMathHull::read(Lexer & lex)
{
	MathAtom at;
	mathed_parse_normal(buffer_, at, lex, Parse::TRACKMACRO);
	operator=(*at->asHullInset());
}


bool InsetMathHull::readQuiet(Lexer & lex)
{
	MathAtom at;
	bool success = mathed_parse_normal(buffer_, at, lex, Parse::QUIET);
	if (success)
		operator=(*at->asHullInset());
	return success;
}


int InsetMathHull::plaintext(odocstream & os, OutputParams const &) const
{
	// disables ASCII-art for export of equations. See #2275.
	if (0 && display()) {
		Dimension dim;
		TextMetricsInfo mi;
		metricsT(mi, dim);
		TextPainter tpain(dim.width(), dim.height());
		drawT(tpain, 0, dim.ascent());
		tpain.show(os, 3);
		// reset metrics cache to "real" values
		//metrics();
		return tpain.textheight();
	} else {
		odocstringstream oss;
		Encoding const * const enc = encodings.fromLyXName("utf8");
		WriteStream wi(oss, false, true, WriteStream::wsDefault, enc);
		// Fix Bug #6139
		if (type_ == hullRegexp)
			write(wi);
		else {
			for (row_type r = 0; r < nrows(); ++r) {
				for (col_type c = 0; c < ncols(); ++c)
					wi << (c == 0 ? "" : "\t") << cell(index(r, c));
				wi << "\n";
			}
		}
		docstring const str = oss.str();
		os << str;
		return str.size();
	}
}


int InsetMathHull::docbook(odocstream & os, OutputParams const & runparams) const
{
	MathStream ms(os);
	int res = 0;
	docstring name;
	if (getType() == hullSimple)
		name = from_ascii("inlineequation");
	else
		name = from_ascii("informalequation");

	docstring bname = name;
	if (!label(0).empty())
		bname += " id='" + sgml::cleanID(buffer(), runparams, label(0)) + "'";

	++ms.tab(); ms.cr(); ms.os() << '<' << bname << '>';

	odocstringstream ls;
	if (runparams.flavor == OutputParams::XML) {
		ms << MTag("alt role='tex' ");
		// Workaround for db2latex: db2latex always includes equations with
		// \ensuremath{} or \begin{display}\end{display}
		// so we strip LyX' math environment
		WriteStream wi(ls, false, false, WriteStream::wsDefault, runparams.encoding);
		InsetMathGrid::write(wi);
		ms << from_utf8(subst(subst(to_utf8(ls.str()), "&", "&amp;"), "<", "&lt;"));
		ms << ETag("alt");
		ms << MTag("math");
		ms << ETag("alt");
		ms << MTag("math");
		InsetMathGrid::mathmlize(ms);
		ms << ETag("math");
	} else {
		TexRow texrow;
		texrow.reset();
		otexstream ols(ls, texrow);
		ms << MTag("alt role='tex'");
		latex(ols, runparams);
		res = texrow.rows();
		ms << from_utf8(subst(subst(to_utf8(ls.str()), "&", "&amp;"), "<", "&lt;"));
		ms << ETag("alt");
	}

	ms << from_ascii("<graphic fileref=\"eqn/");
	if (!label(0).empty())
		ms << sgml::cleanID(buffer(), runparams, label(0));
	else
		ms << sgml::uniqueID(from_ascii("anon"));

	if (runparams.flavor == OutputParams::XML)
		ms << from_ascii("\"/>");
	else
		ms << from_ascii("\">");

	ms.cr(); --ms.tab(); ms.os() << "</" << name << '>';

	return ms.line() + res;
}


bool InsetMathHull::haveNumbers() const
{
	bool havenumbers = false;
	// inline formulas are never numbered (bug 7351 part 3)
	if (getType() == hullSimple)
		return havenumbers;
	for (size_t i = 0; i != numbered_.size(); ++i) {
		if (numbered_[i]) {
			havenumbers = true;
			break;
		}
	}
	return havenumbers;
}


// FIXME XHTML
// We need to do something about alignment here.
//
// This duplicates code from InsetMathGrid, but
// we need access here to number information,
// and we simply do not have that in InsetMathGrid.
void InsetMathHull::htmlize(HtmlStream & os) const
{
	bool const havenumbers = haveNumbers();
	bool const havetable = havenumbers || nrows() > 1 || ncols() > 1;

	if (!havetable) {
		os << cell(index(0, 0));
		return;
	}

	os << MTag("table", "class='mathtable'");
	for (row_type row = 0; row < nrows(); ++row) {
		os << MTag("tr");
		for (col_type col = 0; col < ncols(); ++col) {
			os << MTag("td");
			os << cell(index(row, col));
			os << ETag("td");
		}
		if (havenumbers) {
			os << MTag("td");
			docstring const & num = numbers_[row];
			if (!num.empty())
				os << '(' << num << ')';
		  os << ETag("td");
		}
		os << ETag("tr");
	}
	os << ETag("table");
}


// this duplicates code from InsetMathGrid, but
// we need access here to number information,
// and we simply do not have that in InsetMathGrid.
void InsetMathHull::mathmlize(MathStream & os) const
{
	bool const havenumbers = haveNumbers();
	bool const havetable = havenumbers || nrows() > 1 || ncols() > 1;

	if (havetable)
		os << MTag("mtable");
	char const * const celltag = havetable ? "mtd" : "mrow";
	// FIXME There does not seem to be wide support at the moment
	// for mlabeledtr, so we have to use just mtr for now.
	// char const * const rowtag = havenumbers ? "mlabeledtr" : "mtr";
	char const * const rowtag = "mtr";
	for (row_type row = 0; row < nrows(); ++row) {
		if (havetable)
			os << MTag(rowtag);
		for (col_type col = 0; col < ncols(); ++col) {
			os << MTag(celltag)
			   << cell(index(row, col))
			   << ETag(celltag);
		}
		// fleqn?
		if (havenumbers) {
			os << MTag("mtd");
			docstring const & num = numbers_[row];
			if (!num.empty())
				os << '(' << num << ')';
		  os << ETag("mtd");
		}
		if (havetable)
			os << ETag(rowtag);
	}
	if (havetable)
		os << ETag("mtable");
}


void InsetMathHull::mathAsLatex(WriteStream & os) const
{
	MathEnsurer ensurer(os, false);
	bool havenumbers = haveNumbers();
	bool const havetable = havenumbers || nrows() > 1 || ncols() > 1;

	if (!havetable) {
		os << cell(index(0, 0));
		return;
	}

	os << "<table class='mathtable'>";
	for (row_type row = 0; row < nrows(); ++row) {
		os << "<tr>";
		for (col_type col = 0; col < ncols(); ++col) {
			os << "<td class='math'>";
			os << cell(index(row, col));
			os << "</td>";
		}
		if (havenumbers) {
			os << "<td>";
			docstring const & num = numbers_[row];
			if (!num.empty())
				os << '(' << num << ')';
		  os << "</td>";
		}
		os << "</tr>";
	}
	os << "</table>";
}


docstring InsetMathHull::xhtml(XHTMLStream & xs, OutputParams const & op) const
{
	BufferParams::MathOutput const mathtype = 
		buffer().params().html_math_output;

	bool success = false;

	// we output all the labels just at the beginning of the equation.
	// this should be fine.
	for (size_t i = 0; i != label_.size(); ++i) {
		InsetLabel const * const il = label_[i];
		if (!il)
			continue;
		il->xhtml(xs, op);
	}

	// FIXME Eventually we would like to do this inset by inset.
	if (mathtype == BufferParams::MathML) {
		odocstringstream os;
		MathStream ms(os);
		try {
			mathmlize(ms);
			success = true;
		} catch (MathExportException const &) {}
		if (success) {
			if (getType() == hullSimple)
				xs << html::StartTag("math", 
							"xmlns=\"http://www.w3.org/1998/Math/MathML\"", true);
			else 
				xs << html::StartTag("math", 
				      "display=\"block\" xmlns=\"http://www.w3.org/1998/Math/MathML\"", true);
			xs << XHTMLStream::ESCAPE_NONE
				 << os.str()
				 << html::EndTag("math");
		}
	} else if (mathtype == BufferParams::HTML) {
		odocstringstream os;
		HtmlStream ms(os);
		try {
			htmlize(ms);
			success = true;
		} catch (MathExportException const &) {}
		if (success) {
			string const tag = (getType() == hullSimple) ? "span" : "div";
			xs << html::StartTag(tag, "class='formula'", true)
			   << XHTMLStream::ESCAPE_NONE
			   << os.str()
			   << html::EndTag(tag);
		}
	}
	
	// what we actually want is this:
	// if (
	//     ((mathtype == BufferParams::MathML || mathtype == BufferParams::HTML) 
	//       && !success)
	//     || mathtype == BufferParams::Images
	//    )
	// but what follows is equivalent, since we'll enter only if either (a) we 
	// tried and failed with MathML or HTML or (b) didn't try yet at all but
	// aren't doing LaTeX, in which case we are doing Images.
	if (!success && mathtype != BufferParams::LaTeX) {
		graphics::PreviewImage const * pimage = 0;
		if (!op.dryrun) {
			loadPreview(docit_);
			pimage = preview_->getPreviewImage(buffer());
			// FIXME Do we always have png?
		}

		if (pimage || op.dryrun) {
			string const filename = pimage ? pimage->filename().onlyFileName()
			                               : "previewimage.png";
			if (pimage) {
				// if we are not in the master buffer, then we need to see that the
				// generated image is copied there; otherwise, preview fails.
				Buffer const * mbuf = buffer().masterBuffer();
				if (mbuf != &buffer()) {
					string mbtmp = mbuf->temppath();
					FileName const mbufimg(support::addName(mbtmp, filename));
					pimage->filename().copyTo(mbufimg);
				}
				// add the file to the list of files to be exported
				op.exportdata->addExternalFile("xhtml", pimage->filename());
			}

			string const tag = (getType() == hullSimple) ? "span" : "div";
			xs << html::CR()
			   << html::StartTag(tag)
				 << html::CompTag("img", "src=\"" + filename + "\"")
				 << html::EndTag(tag)
				 << html::CR();
			success = true;
		}
	}
	
	// so we'll pass this test if we've failed everything else, or
	// if mathtype was LaTeX, since we won't have entered any of the
	// earlier branches
	if (!success /* || mathtype != BufferParams::LaTeX */) {
		// Unfortunately, we cannot use latexString() because we do not want
		// $...$ or whatever.
		odocstringstream ls;
		WriteStream wi(ls, false, true, WriteStream::wsPreview);
		ModeSpecifier specifier(wi, MATH_MODE);
		mathAsLatex(wi);
		docstring const latex = ls.str();
		
		// class='math' allows for use of jsMath
		// http://www.math.union.edu/~dpvc/jsMath/
		// FIXME XHTML
		// probably should allow for some kind of customization here
		string const tag = (getType() == hullSimple) ? "span" : "div";
		xs << html::StartTag(tag, "class='math'")
		   << latex 
		   << html::EndTag(tag)
		   << html::CR();
	}
	return docstring();
}


void InsetMathHull::toString(odocstream & os) const
{
	plaintext(os, OutputParams(0));
}


void InsetMathHull::forToc(docstring & os, size_t) const
{
	odocstringstream ods;
	plaintext(ods, OutputParams(0));
	os += ods.str();
}


docstring InsetMathHull::contextMenuName() const
{
	return from_ascii("context-math");
}


void InsetMathHull::recordLocation(DocIterator const & di)
{
	docit_ = di;
}

} // namespace lyx
