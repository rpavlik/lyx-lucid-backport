/**
 * \file output_latex.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Lars Gullik Bjønnes
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "output_latex.h"

#include "Buffer.h"
#include "BufferParams.h"
#include "Encoding.h"
#include "Font.h"
#include "InsetList.h"
#include "Language.h"
#include "Layout.h"
#include "LyXRC.h"
#include "OutputParams.h"
#include "Paragraph.h"
#include "ParagraphParameters.h"
#include "TextClass.h"
#include "TexRow.h"
#include "VSpace.h"

#include "insets/InsetBibitem.h"
#include "insets/InsetArgument.h"

#include "support/lassert.h"
#include "support/debug.h"
#include "support/lstrings.h"

#include <boost/next_prior.hpp>
#include <list>

using namespace std;
using namespace lyx::support;


namespace lyx {

namespace {

enum OpenEncoding {
		none,
		inputenc,
		CJK
};

static int open_encoding_ = none;
static int cjk_inherited_ = 0;
Language const * prev_env_language_ = 0;


string const getPolyglossiaEnvName(Language const * lang)
{
	string result = lang->polyglossia();
	if (result == "arabic")
		// exceptional spelling; see polyglossia docs.
		result = "Arabic";
	return result;
}


struct TeXEnvironementData
{
	bool cjk_nested;
	Layout const * style;
	Language const * par_language;
	Encoding const * prev_encoding;
	bool leftindent_open;
};


static TeXEnvironementData prepareEnvironement(Buffer const & buf,
					Text const & text,
					ParagraphList::const_iterator pit,
					otexstream & os,
					OutputParams const & runparams)
{
	TeXEnvironementData data;

	BufferParams const & bparams = buf.params();

	// FIXME This test should not be necessary.
	// We should perhaps issue an error if it is.
	Layout const & style = text.inset().forcePlainLayout() ?
		bparams.documentClass().plainLayout() : pit->layout();

	ParagraphList const & paragraphs = text.paragraphs();
	ParagraphList::const_iterator const priorpit =
		pit == paragraphs.begin() ? pit : boost::prior(pit);

	bool const use_prev_env_language = prev_env_language_ != 0
			&& priorpit->layout().isEnvironment()
			&& (priorpit->getDepth() > pit->getDepth()
			    || (priorpit->getDepth() == pit->getDepth()
				&& priorpit->layout() != pit->layout()));

	data.prev_encoding = runparams.encoding;
	data.par_language = pit->getParLanguage(bparams);
	Language const * const doc_language = bparams.language;
	Language const * const prev_par_language =
		(pit != paragraphs.begin())
		? (use_prev_env_language ? prev_env_language_
					 : priorpit->getParLanguage(bparams))
		: doc_language;
	string par_lang = data.par_language->babel();
	string prev_par_lang = prev_par_language->babel();
	string doc_lang = doc_language->babel();
	string lang_begin_command = lyxrc.language_command_begin;
	string lang_end_command = lyxrc.language_command_end;

	if (runparams.use_polyglossia) {
		par_lang = getPolyglossiaEnvName(data.par_language);
		prev_par_lang = getPolyglossiaEnvName(prev_par_language);
		doc_lang = getPolyglossiaEnvName(doc_language);
		lang_begin_command = "\\begin{$$lang}";
		lang_end_command = "\\end{$$lang}";
	}

	if (par_lang != prev_par_lang) {
		if (!lang_end_command.empty() &&
		    prev_par_lang != doc_lang &&
		    !prev_par_lang.empty()) {
			os << from_ascii(subst(
				lang_end_command,
				"$$lang",
				prev_par_lang))
			  // the '%' is necessary to prevent unwanted whitespace
			  << "%\n";
		}

		if ((lang_end_command.empty() ||
		    par_lang != doc_lang) &&
		    !par_lang.empty()) {
			os << from_ascii(subst(
				lang_begin_command,
				"$$lang",
				par_lang));
			if (runparams.use_polyglossia
			    && !data.par_language->polyglossiaOpts().empty())
					os << "["
					   << from_ascii(data.par_language->polyglossiaOpts())
					   << "]";
			  // the '%' is necessary to prevent unwanted whitespace
			os << "%\n";
		}
	}

	data.leftindent_open = false;
	if (!pit->params().leftIndent().zero()) {
		os << "\\begin{LyXParagraphLeftIndent}{"
		   << from_ascii(pit->params().leftIndent().asLatexString())
		   << "}\n";
		data.leftindent_open = true;
	}

	if (style.isEnvironment()) {
		os << "\\begin{" << from_ascii(style.latexname()) << '}';
		if (style.optargs != 0 || style.reqargs != 0)
			latexArgInsets(*pit, os, runparams, style.reqargs, style.optargs);
		if (style.latextype == LATEX_LIST_ENVIRONMENT) {
			os << '{'
			   << pit->params().labelWidthString()
			   << "}\n";
		} else if (style.labeltype == LABEL_BIBLIO) {
			if (pit->params().labelWidthString().empty())
				os << '{' << bibitemWidest(buf, runparams) << "}\n";
			else
				os << '{'
				  << pit->params().labelWidthString()
				  << "}\n";
		} else
			os << from_ascii(style.latexparam()) << '\n';
	}
	data.style = &style;

	// in multilingual environments, the CJK tags have to be nested properly
	data.cjk_nested = false;
	if (data.par_language->encoding()->package() == Encoding::CJK &&
	    open_encoding_ != CJK && pit->isMultiLingual(bparams)) {
		if (prev_par_language->encoding()->package() == Encoding::CJK)
			os << "\\begin{CJK}{" << from_ascii(data.par_language->encoding()->latexName())
			   << "}{" << from_ascii(bparams.fonts_cjk) << "}%\n";
		open_encoding_ = CJK;
		data.cjk_nested = true;
	}
	return data;
}


static void finishEnvironement(otexstream & os, OutputParams const & runparams,
			       TeXEnvironementData const & data)
{
	if (open_encoding_ == CJK && data.cjk_nested) {
		// We need to close the encoding even if it does not change
		// to do correct environment nesting
		os << "\\end{CJK}\n";
		open_encoding_ = none;
	}

	if (data.style->isEnvironment()) {
		os << "\\end{" << from_ascii(data.style->latexname()) << "}\n";
		prev_env_language_ = data.par_language;
		if (runparams.encoding != data.prev_encoding) {
			runparams.encoding = data.prev_encoding;
			if (!runparams.isFullUnicode())
				os << setEncoding(data.prev_encoding->iconvName());
		}
	}

	if (data.leftindent_open) {
		os << "\\end{LyXParagraphLeftIndent}\n";
		prev_env_language_ = data.par_language;
		if (runparams.encoding != data.prev_encoding) {
			runparams.encoding = data.prev_encoding;
			if (!runparams.isFullUnicode())
				os << setEncoding(data.prev_encoding->iconvName());
		}
	}
}


void TeXEnvironment(Buffer const & buf, Text const & text,
		    OutputParams const & runparams,
		    pit_type & pit, otexstream & os)
{
	ParagraphList const & paragraphs = text.paragraphs();
	ParagraphList::const_iterator par = paragraphs.constIterator(pit);
	LYXERR(Debug::LATEX, "TeXEnvironment for paragraph " << pit);

	Layout const & current_layout = par->layout();
	depth_type const current_depth = par->params().depth();
	Length const & current_left_indent = par->params().leftIndent();

	// This is for debugging purpose at the end.
	pit_type const par_begin = pit;
	for (; pit < runparams.par_end; ++pit) {
	
		ParagraphList::const_iterator par = paragraphs.constIterator(pit);

		// check first if this is an higher depth paragraph.
		bool go_out = (par->params().depth() < current_depth);
		if (par->params().depth() == current_depth) {
			// This environment is finished.
			go_out |= (par->layout() != current_layout);
			go_out |= (par->params().leftIndent() != current_left_indent);
		}
		if (go_out) {
			// nothing to do here, restore pit and go out.
			pit--;
			break;
		}

		if (par->layout() == current_layout
			&& par->params().depth() == current_depth
			&& par->params().leftIndent() == current_left_indent) {
			// We are still in the same environment so TeXOnePar and continue;
			TeXOnePar(buf, text, pit, os, runparams);
			continue;
		}

		// We are now in a deeper environment.
		// Either par->layout() != current_layout
		// Or     par->params().depth() > current_depth
		// Or     par->params().leftIndent() != current_left_indent)

		if (par->layout().isParagraph()) {
			// FIXME (Lgb): How to handle this? 
			//&& !suffixIs(os, "\n\n")

			// (ARRae) There should be at least one '\n' already but we need there to
			// be two for Standard paragraphs that are depth-increment'ed to be
			// output correctly. However, tables can also be paragraphs so
			// don't adjust them.
			// 

			// FIXME (Lgb): Will it ever harm to have one '\n' too
			// many? i.e. that we sometimes will have
			// three in a row.
			os << '\n';
		}

		// FIXME This test should not be necessary.
		// We should perhaps issue an error if it is.
		bool const force_plain_layout = text.inset().forcePlainLayout();
		Layout const & style = force_plain_layout
			? buf.params().documentClass().plainLayout()
			: par->layout();

		if (!style.isEnvironment()) {
			// This is a standard paragraph, no need to call TeXEnvironment.
			TeXOnePar(buf, text, pit, os, runparams);
			continue;
		}

		// This is a new environment.
		TeXEnvironementData const data =
			prepareEnvironement(buf, text, par, os, runparams);
		// Recursive call to TeXEnvironment!
		TeXEnvironment(buf, text, runparams, pit, os);
		finishEnvironement(os, runparams, data);
	}

	if (pit != runparams.par_end)
		LYXERR(Debug::LATEX, "TeXEnvironment for paragraph " << par_begin << " done.");
}

} // namespace anon


void latexArgInsets(Paragraph const & par, otexstream & os,
	OutputParams const & runparams, unsigned int reqargs,
	unsigned int optargs)
{
	unsigned int totalargs = reqargs + optargs;
	list<InsetArgument const *> ilist;

	InsetList::const_iterator it = par.insetList().begin();
	InsetList::const_iterator end = par.insetList().end();
	for (; it != end; ++it) {
		if (it->inset->lyxCode() == ARG_CODE) {
			if (ilist.size() >= totalargs) {
				LYXERR0("WARNING: Found extra argument inset.");
				continue;
			}
			InsetArgument const * ins =
				static_cast<InsetArgument const *>(it->inset);
			ilist.push_back(ins);
		}
	}

	if (!reqargs && ilist.size() == 0)
		return;

	bool const have_optional_args = ilist.size() > reqargs;
	if (have_optional_args) {
		unsigned int todo = ilist.size() - reqargs;
		for (unsigned int i = 0; i < todo; ++i) {
			InsetArgument const * ins = ilist.front();
			ilist.pop_front();
			ins->latexArgument(os, runparams, true);
		}
	}

	// we should now have no more insets than there are required
	// arguments.
	LASSERT(ilist.size() <= reqargs, /* */);
	if (!reqargs)
		return;

	for (unsigned int i = 0; i < reqargs; ++i) {
		if (ilist.empty())
			// a required argument wasn't given, so we output {}
			os << "{}";
		else {
			InsetArgument const * ins = ilist.front();
			ilist.pop_front();
			ins->latexArgument(os, runparams, false);
		}
	}
}

namespace {

// output the proper paragraph start according to latextype.
void parStartCommand(Paragraph const & par, otexstream & os,
		     OutputParams const & runparams, Layout const & style) 
{
	switch (style.latextype) {
	case LATEX_COMMAND:
		os << '\\' << from_ascii(style.latexname());

		// Separate handling of optional argument inset.
		if (style.optargs != 0 || style.reqargs != 0)
			latexArgInsets(par, os, runparams, style.reqargs, style.optargs);
		else
			os << from_ascii(style.latexparam());
		break;
	case LATEX_ITEM_ENVIRONMENT:
	case LATEX_LIST_ENVIRONMENT:
		os << "\\item ";
		break;
	case LATEX_BIB_ENVIRONMENT:
		// ignore this, the inset will write itself
		break;
	default:
		break;
	}
}

} // namespace anon

// FIXME: this should be anonymous
void TeXOnePar(Buffer const & buf,
	       Text const & text,
	       pit_type pit,
	       otexstream & os,
	       OutputParams const & runparams_in,
	       string const & everypar,
	       int start_pos, int end_pos)
{
	BufferParams const & bparams = buf.params();
	ParagraphList const & paragraphs = text.paragraphs();
	Paragraph const & par = paragraphs.at(pit);
	// FIXME This check should not really be needed.
	// Perhaps we should issue an error if it is.
	Layout const style = text.inset().forcePlainLayout() ?
		bparams.documentClass().plainLayout() : par.layout();

	if (style.inpreamble)
		return;

	LYXERR(Debug::LATEX, "TeXOnePar for paragraph " << pit << " ptr " << &par << " '"
		<< everypar << "'");

	OutputParams runparams = runparams_in;
	runparams.isLastPar = (pit == pit_type(paragraphs.size() - 1));
	// We reinitialze par begin and end to be on the safe side
	// with embedded inset as we don't know if they set those
	// value correctly.
	runparams.par_begin = 0;
	runparams.par_end = 0;

	bool const maintext = text.isMainText();
	// we are at the beginning of an inset and CJK is already open;
	// we count inheritation levels to get the inset nesting right.
	if (pit == 0 && !maintext
	    && (cjk_inherited_ > 0 || open_encoding_ == CJK)) {
		cjk_inherited_ += 1;
		open_encoding_ = none;
	}

	if (text.inset().getLayout().isPassThru()) {
		Font const outerfont = text.outerFont(pit);

		// No newline before first paragraph in this lyxtext
		if (pit > 0) {
			os << '\n';
			if (!text.inset().getLayout().parbreakIsNewline())
				os << '\n';
		}

		par.latex(bparams, outerfont, os, runparams, start_pos, end_pos);
		return;
	}

	Paragraph const * nextpar = runparams.isLastPar
		? 0 : &paragraphs.at(pit + 1);

	if (style.pass_thru) {
		Font const outerfont = text.outerFont(pit);
		parStartCommand(par, os, runparams, style);

		par.latex(bparams, outerfont, os, runparams, start_pos, end_pos);

		// I did not create a parEndCommand for this minuscule
		// task because in the other user of parStartCommand
		// the code is different (JMarc)
		if (style.isCommand())
			os << "}\n";
		else
			os << '\n';
		if (!style.parbreak_is_newline) {
			os << '\n';
		} else if (nextpar) {
			Layout const nextstyle = text.inset().forcePlainLayout()
				? bparams.documentClass().plainLayout()
				: nextpar->layout();
			if (nextstyle.name() != style.name())
				os << '\n';
		}

		return;
	}

	// This paragraph's language
	Language const * const par_language = par.getParLanguage(bparams);
	// The document's language
	Language const * const doc_language = bparams.language;
	// The language that was in effect when the environment this paragraph is
	// inside of was opened
	Language const * const outer_language =
		(runparams.local_font != 0) ?
			runparams.local_font->language() : doc_language;

	Paragraph const * priorpar = (pit == 0) ? 0 : &paragraphs.at(pit - 1);

	// The previous language that was in effect is the language of the
	// previous paragraph, unless the previous paragraph is inside an
	// environment with nesting depth greater than (or equal to, but with
	// a different layout) the current one. If there is no previous
	// paragraph, the previous language is the outer language.
	bool const use_prev_env_language = prev_env_language_ != 0
			&& priorpar
			&& priorpar->layout().isEnvironment()
			&& (priorpar->getDepth() > par.getDepth()
			    || (priorpar->getDepth() == par.getDepth()
				    && priorpar->layout() != par.layout()));
	Language const * const prev_language =
		(pit != 0)
		? (use_prev_env_language ? prev_env_language_
					 : priorpar->getParLanguage(bparams))
		: outer_language;

	string par_lang = par_language->babel();
	string prev_lang = prev_language->babel();
	string doc_lang = doc_language->babel();
	string outer_lang = outer_language->babel();
	string lang_begin_command = lyxrc.language_command_begin;
	string lang_end_command = lyxrc.language_command_end;

	if (runparams.use_polyglossia) {
		par_lang = getPolyglossiaEnvName(par_language);
		prev_lang = getPolyglossiaEnvName(prev_language);
		doc_lang = getPolyglossiaEnvName(doc_language);
		outer_lang = getPolyglossiaEnvName(outer_language);
		lang_begin_command = "\\begin{$$lang}";
		lang_end_command = "\\end{$$lang}";
	}
	if (par_lang != prev_lang
		// check if we already put language command in TeXEnvironment()
		&& !(style.isEnvironment()
		     && (pit == 0 || (priorpar->layout() != par.layout()
			                  && priorpar->getDepth() <= par.getDepth())
		                  || priorpar->getDepth() < par.getDepth())))
	{
		if (!lang_end_command.empty() &&
		    prev_lang != outer_lang &&
		    !prev_lang.empty())
		{
			os << from_ascii(subst(lang_end_command,
				"$$lang",
				prev_lang))
			   // the '%' is necessary to prevent unwanted whitespace
			   << "%\n";
		}

		// We need to open a new language if we couldn't close the previous
		// one (because there's no language_command_end); and even if we closed
		// the previous one, if the current language is different than the
		// outer_language (which is currently in effect once the previous one
		// is closed).
		if ((lang_end_command.empty() || par_lang != outer_lang)
			&& !par_lang.empty()) {
			// If we're inside an inset, and that inset is within an \L or \R
			// (or equivalents), then within the inset, too, any opposite
			// language paragraph should appear within an \L or \R (in addition
			// to, outside of, the normal language switch commands).
			// This behavior is not correct for ArabTeX, though.
			if (!runparams.use_polyglossia
			    // not for ArabTeX
				&& par_language->lang() != "arabic_arabtex"
				&& outer_language->lang() != "arabic_arabtex"
			    // are we in an inset?
			    && runparams.local_font != 0
			    // is the inset within an \L or \R?
			    //
			    // FIXME: currently, we don't check this; this means that
			    // we'll have unnnecessary \L and \R commands, but that
			    // doesn't seem to hurt (though latex will complain)
			    //
			    // is this paragraph in the opposite direction?
			    && runparams.local_font->isRightToLeft() != par_language->rightToLeft()) {
				// FIXME: I don't have a working copy of the Arabi package, so
				// I'm not sure if the farsi and arabic_arabi stuff is correct
				// or not...
				if (par_language->lang() == "farsi")
					os << "\\textFR{";
				else if (outer_language->lang() == "farsi")
					os << "\\textLR{";
				else if (par_language->lang() == "arabic_arabi")
					os << "\\textAR{";
				else if (outer_language->lang() == "arabic_arabi")
					os << "\\textLR{";
				// remaining RTL languages currently is hebrew
				else if (par_language->rightToLeft())
					os << "\\R{";
				else
					os << "\\L{";
			}
			// With CJK, the CJK tag has to be closed first (see below)
			if (runparams.encoding->package() != Encoding::CJK) {
				os << from_ascii(subst(
					lang_begin_command,
					"$$lang",
					par_lang));
				if (runparams.use_polyglossia
				    && !par_language->polyglossiaOpts().empty())
						os << "["
						  << from_ascii(par_language->polyglossiaOpts())
						  << "]";
				   // the '%' is necessary to prevent unwanted whitespace
				os << "%\n";
			}
		}
	}

	// Switch file encoding if necessary; no need to do this for "default"
	// encoding, since this only affects the position of the outputted
	// \inputencoding command; the encoding switch will occur when necessary
	if (bparams.inputenc == "auto"
		&& runparams.encoding->package() != Encoding::none) {
		// Look ahead for future encoding changes.
		// We try to output them at the beginning of the paragraph,
		// since the \inputencoding command is not allowed e.g. in
		// sections. For this reason we only set runparams.moving_arg
		// after checking for the encoding change, otherwise the
		// change would be always avoided by switchEncoding().
		for (pos_type i = 0; i < par.size(); ++i) {
			char_type const c = par.getChar(i);
			Encoding const * const encoding =
				par.getFontSettings(bparams, i).language()->encoding();
			if (encoding->package() != Encoding::CJK
				&& runparams.encoding->package() == Encoding::inputenc
				&& c < 0x80)
				continue;
			if (par.isInset(i))
				break;
			// All characters before c are in the ASCII range, and
			// c is non-ASCII (but no inset), so change the
			// encoding to that required by the language of c.
			// With CJK, only add switch if we have CJK content at the beginning
			// of the paragraph
			if (i != 0 && encoding->package() == Encoding::CJK)
				continue;

			pair<bool, int> enc_switch = switchEncoding(os.os(),
						bparams, runparams, *encoding);
			// the following is necessary after a CJK environment in a multilingual
			// context (nesting issue).
			if (par_language->encoding()->package() == Encoding::CJK
				&& open_encoding_ != CJK && cjk_inherited_ == 0) {
				os << "\\begin{CJK}{" << from_ascii(par_language->encoding()->latexName())
				   << "}{" << from_ascii(bparams.fonts_cjk) << "}%\n";
				open_encoding_ = CJK;
			}
			if (encoding->package() != Encoding::none && enc_switch.first) {
				if (enc_switch.second > 0) {
					// the '%' is necessary to prevent unwanted whitespace
					os << "%\n";
				}
				// With CJK, the CJK tag had to be closed first (see above)
				if (runparams.encoding->package() == Encoding::CJK) {
					os << from_ascii(subst(
						lang_begin_command,
						"$$lang",
						par_lang))
					// the '%' is necessary to prevent unwanted whitespace
					<< "%\n";
				}
				runparams.encoding = encoding;
			}
			break;
		}
	}

	runparams.moving_arg |= style.needprotect;
	Encoding const * const prev_encoding = runparams.encoding;

	bool const useSetSpace = bparams.documentClass().provides("SetSpace");
	if (par.allowParagraphCustomization()) {
		if (par.params().startOfAppendix()) {
			os << "\\appendix\n";
		}

		if (!par.params().spacing().isDefault()
			&& (pit == 0 || !priorpar->hasSameLayout(par)))
		{
			os << from_ascii(par.params().spacing().writeEnvirBegin(useSetSpace))
			    << '\n';
		}

		if (style.isCommand()) {
			os << '\n';
		}
	}

	parStartCommand(par, os, runparams, style);

	Font const outerfont = text.outerFont(pit);

	// FIXME UNICODE
	os << from_utf8(everypar);
	par.latex(bparams, outerfont, os, runparams, start_pos, end_pos);

	// Make sure that \\par is done with the font of the last
	// character if this has another size as the default.
	// This is necessary because LaTeX (and LyX on the screen)
	// calculates the space between the baselines according
	// to this font. (Matthias)
	//
	// Is this really needed ? (Dekel)
	// We do not need to use to change the font for the last paragraph
	// or for a command.

	Font const font = par.empty()
		 ? par.getLayoutFont(bparams, outerfont)
		 : par.getFont(bparams, par.size() - 1, outerfont);

	bool const is_command = style.isCommand();

	if (style.resfont.size() != font.fontInfo().size()
	    && nextpar
	    && !is_command) {
		os << '{';
		os << "\\" << from_ascii(font.latexSize()) << " \\par}";
	} else if (is_command) {
		os << '}';
		if (runparams.encoding != prev_encoding) {
			runparams.encoding = prev_encoding;
			if (!runparams.isFullUnicode())
				os << setEncoding(prev_encoding->iconvName());
		}
	}

	bool pending_newline = false;
	switch (style.latextype) {
	case LATEX_ITEM_ENVIRONMENT:
	case LATEX_LIST_ENVIRONMENT:
		if (nextpar && (par.params().depth() < nextpar->params().depth()))
			pending_newline = true;
		break;
	case LATEX_ENVIRONMENT: {
		// if its the last paragraph of the current environment
		// skip it otherwise fall through
		if (nextpar
			&& (nextpar->layout() != par.layout()
		        || nextpar->params().depth() != par.params().depth()))
			break;
	}

	// fall through possible
	default:
		// we don't need it for the last paragraph!!!
		if (nextpar)
			pending_newline = true;
	}

	if (par.allowParagraphCustomization()) {
		if (!par.params().spacing().isDefault()
			&& (runparams.isLastPar || !nextpar->hasSameLayout(par))) {
			if (pending_newline)
				os << '\n';
			os << from_ascii(par.params().spacing().writeEnvirEnd(useSetSpace));
			pending_newline = true;
		}
	}

	// Closing the language is needed for the last paragraph; it is also
	// needed if we're within an \L or \R that we may have opened above (not
	// necessarily in this paragraph) and are about to close.
	bool closing_rtl_ltr_environment = !runparams.use_polyglossia
		// not for ArabTeX
		&& (par_language->lang() != "arabic_arabtex"
		    && outer_language->lang() != "arabic_arabtex")
		     // have we opened and \L or \R environment?
		&& runparams.local_font != 0
		&& runparams.local_font->isRightToLeft() != par_language->rightToLeft()
		// are we about to close the language?
		&&((nextpar && par_language->babel() != (nextpar->getParLanguage(bparams))->babel())
		   || (runparams.isLastPar && par_language->babel() != outer_language->babel()));

	if (closing_rtl_ltr_environment
		|| (runparams.isLastPar && par_language->babel() != outer_language->babel())) {
		// Since \selectlanguage write the language to the aux file,
		// we need to reset the language at the end of footnote or
		// float.

		if (pending_newline)
			os << '\n';

		// when the paragraph uses CJK, the language has to be closed earlier
		if (font.language()->encoding()->package() != Encoding::CJK) {
			if (lang_end_command.empty()) {
				// If this is a child, we should restore the
				// master language after the last paragraph.
				Language const * const current_language =
					(runparams.isLastPar && runparams.master_language)
						? runparams.master_language
						: outer_language;
				string const current_lang = runparams.use_polyglossia ?
					getPolyglossiaEnvName(current_language)
					: current_language->babel();
				if (!current_lang.empty()) {
					os << from_ascii(subst(
						lang_begin_command,
						"$$lang",
						current_lang));
					pending_newline = true;
				}
			} else if (!par_lang.empty()) {
				os << from_ascii(subst(
					lang_end_command,
					"$$lang",
					par_lang));
				pending_newline = true;
			}
		}
	}
	if (closing_rtl_ltr_environment)
		os << "}";

	if (pending_newline)
		os << '\n';

	// if this is a CJK-paragraph and the next isn't, close CJK
	// also if the next paragraph is a multilingual environment (because of nesting)
	if (nextpar
		&& open_encoding_ == CJK
		&& (nextpar->getParLanguage(bparams)->encoding()->package() != Encoding::CJK
		   || (nextpar->layout().isEnvironment() && nextpar->isMultiLingual(bparams)))
		// inbetween environments, CJK has to be closed later (nesting!)
		&& (!style.isEnvironment() || !nextpar->layout().isEnvironment())) {
		os << "\\end{CJK}\n";
		open_encoding_ = none;
	}

	// If this is the last paragraph, close the CJK environment
	// if necessary. If it's an environment, we'll have to \end that first.
	if (runparams.isLastPar && !style.isEnvironment()) {
		switch (open_encoding_) {
			case CJK: {
				// do nothing at the end of child documents
				if (maintext && buf.masterBuffer() != &buf)
					break;
				// end of main text
				if (maintext) {
					os << "\n\\end{CJK}\n";
				// end of an inset
				} else
					os << "\\end{CJK}";
				open_encoding_ = none;
				break;
			}
			case inputenc: {
				os << "\\egroup";
				open_encoding_ = none;
				break;
			}
			case none:
			default:
				// do nothing
				break;
		}
	}

	// If this is the last paragraph, and a local_font was set upon entering
	// the inset, and we're using "auto" or "default" encoding, the encoding
	// should be set back to that local_font's encoding.
	// However, do not change the encoding when a fully unicode aware backend
	// such as XeTeX is used.
	if (runparams.isLastPar && runparams_in.local_font != 0
	    && runparams_in.encoding != runparams_in.local_font->language()->encoding()
	    && (bparams.inputenc == "auto" || bparams.inputenc == "default")
	    && (!runparams.isFullUnicode())) {
		runparams_in.encoding = runparams_in.local_font->language()->encoding();
		os << setEncoding(runparams_in.encoding->iconvName());
	}
	// Otherwise, the current encoding should be set for the next paragraph.
	else
		runparams_in.encoding = runparams.encoding;


	// we don't need a newline for the last paragraph!!!
	// Note from JMarc: we will re-add a \n explicitly in
	// TeXEnvironment, because it is needed in this case
	if (nextpar) {
		Layout const & next_layout = nextpar->layout();
		if (style == next_layout
		    // no blank lines before environments!
		    || !next_layout.isEnvironment()
		    // unless there's a depth change
		    // FIXME What we really want to do here is put every \begin and \end
		    // tag on a new line (which was not the case with nested environments).
		    // But in the present state of play, we don't have access to the
		    // information whether the current TeX row is empty or not.
		    // For some ideas about how to fix this, see this thread:
		    // http://www.mail-archive.com/lyx-devel@lists.lyx.org/msg145787.html
		    || nextpar->params().depth() != par.params().depth()) {
			os << '\n';
		}
	}

	LYXERR(Debug::LATEX, "TeXOnePar for paragraph " << pit << " done; ptr "
		<< &par << " next " << nextpar);

	return;
}


// LaTeX all paragraphs
void latexParagraphs(Buffer const & buf,
		     Text const & text,
		     otexstream & os,
		     OutputParams const & runparams,
		     string const & everypar)
{
	BufferParams const & bparams = buf.params();

	bool const maintext = text.isMainText();
	bool const is_child = buf.masterBuffer() != &buf;

	// Open a CJK environment at the beginning of the main buffer
	// if the document's language is a CJK language
	// (but not in child documents)
	if (maintext && !is_child
	    && bparams.encoding().package() == Encoding::CJK) {
		os << "\\begin{CJK}{" << from_ascii(bparams.encoding().latexName())
		<< "}{" << from_ascii(bparams.fonts_cjk) << "}%\n";
		open_encoding_ = CJK;
	}
	// if "auto begin" is switched off, explicitly switch the
	// language on at start
	string const mainlang = runparams.use_polyglossia ?
		getPolyglossiaEnvName(bparams.language)
		: bparams.language->babel();
	string const lang_begin_command = runparams.use_polyglossia ?
		"\\begin{$$lang}" : lyxrc.language_command_begin;

	if (maintext && !lyxrc.language_auto_begin &&
	    !mainlang.empty()) {
		// FIXME UNICODE
		os << from_utf8(subst(lang_begin_command,
					"$$lang",
					mainlang));
		if (runparams.use_polyglossia
		    && !bparams.language->polyglossiaOpts().empty())
			os << "["
			    << from_ascii(bparams.language->polyglossiaOpts())
			    << "]";
		os << '\n';
	}

	ParagraphList const & paragraphs = text.paragraphs();
	LASSERT(runparams.par_begin <= runparams.par_end, /**/);

	if (runparams.par_begin == runparams.par_end) {
		// The full doc will be exported but it is easier to just rely on
		// runparams range parameters that will be passed TeXEnvironment.
		runparams.par_begin = 0;
		runparams.par_end = paragraphs.size();
	}

	pit_type pit = runparams.par_begin;
	// lastpit is for the language check after the loop.
	pit_type lastpit = pit;
	// variables used in the loop:
	bool was_title = false;
	bool already_title = false;
	DocumentClass const & tclass = bparams.documentClass();

	for (; pit < runparams.par_end; ++pit) {
		lastpit = pit;
		ParagraphList::const_iterator par = paragraphs.constIterator(pit);

		// FIXME This check should not be needed. We should
		// perhaps issue an error if it is.
		Layout const & layout = text.inset().forcePlainLayout() ?
				tclass.plainLayout() : par->layout();

		if (layout.intitle) {
			if (already_title) {
				LYXERR0("Error in latexParagraphs: You"
					" should not mix title layouts"
					" with normal ones.");
			} else if (!was_title) {
				was_title = true;
				if (tclass.titletype() == TITLE_ENVIRONMENT) {
					os << "\\begin{"
							<< from_ascii(tclass.titlename())
							<< "}\n";
				}
			}
		} else if (was_title && !already_title) {
			if (tclass.titletype() == TITLE_ENVIRONMENT) {
				os << "\\end{" << from_ascii(tclass.titlename())
						<< "}\n";
			}
			else {
				os << "\\" << from_ascii(tclass.titlename())
						<< "\n";
			}
			already_title = true;
			was_title = false;
		}


		if (!layout.isEnvironment() && par->params().leftIndent().zero()) {
			// This is a standard top level paragraph, TeX it and continue.
			TeXOnePar(buf, text, pit, os, runparams, everypar);
			continue;
		}
		
		TeXEnvironementData const data =
			prepareEnvironement(buf, text, par, os, runparams);
		// pit can be changed in TeXEnvironment.
		TeXEnvironment(buf, text, runparams, pit, os);
		finishEnvironement(os, runparams, data);
	}

	if (pit == runparams.par_end) {
			// Make sure that the last paragraph is
			// correctly terminated (because TeXOnePar does
			// not add a \n in this case)
			//os << '\n';
	}

	// It might be that we only have a title in this document
	if (was_title && !already_title) {
		if (tclass.titletype() == TITLE_ENVIRONMENT) {
			os << "\\end{" << from_ascii(tclass.titlename())
			   << "}\n";
		} else {
			os << "\\" << from_ascii(tclass.titlename())
			   << "\n";
		}
	}

	// if "auto end" is switched off, explicitly close the language at the end
	// but only if the last par is in a babel language
	string const lang_end_command = runparams.use_polyglossia ?
		"\\end{$$lang}" : lyxrc.language_command_end;
	if (maintext && !lyxrc.language_auto_end && !mainlang.empty() &&
		paragraphs.at(lastpit).getParLanguage(bparams)->encoding()->package() != Encoding::CJK) {
		os << from_utf8(subst(lang_end_command,
					"$$lang",
					mainlang))
			<< '\n';
	}

	// If the last paragraph is an environment, we'll have to close
	// CJK at the very end to do proper nesting.
	if (maintext && !is_child && open_encoding_ == CJK) {
		os << "\\end{CJK}\n";
		open_encoding_ = none;
	}

	// reset inherited encoding
	if (cjk_inherited_ > 0) {
		cjk_inherited_ -= 1;
		if (cjk_inherited_ == 0)
			open_encoding_ = CJK;
	}
}


pair<bool, int> switchEncoding(odocstream & os, BufferParams const & bparams,
		   OutputParams const & runparams, Encoding const & newEnc,
		   bool force)
{
	Encoding const & oldEnc = *runparams.encoding;
	bool moving_arg = runparams.moving_arg;
	if (!force && ((bparams.inputenc != "auto" && bparams.inputenc != "default")
		|| moving_arg))
		return make_pair(false, 0);

	// Do nothing if the encoding is unchanged.
	if (oldEnc.name() == newEnc.name())
		return make_pair(false, 0);

	// FIXME We ignore encoding switches from/to encodings that do
	// neither support the inputenc package nor the CJK package here.
	// This does of course only work in special cases (e.g. switch from
	// tis620-0 to latin1, but the text in latin1 contains ASCII only),
	// but it is the best we can do
	if (oldEnc.package() == Encoding::none
		|| newEnc.package() == Encoding::none)
		return make_pair(false, 0);

	LYXERR(Debug::LATEX, "Changing LaTeX encoding from "
		<< oldEnc.name() << " to " << newEnc.name());
	os << setEncoding(newEnc.iconvName());
	if (bparams.inputenc == "default")
		return make_pair(true, 0);

	docstring const inputenc_arg(from_ascii(newEnc.latexName()));
	switch (newEnc.package()) {
		case Encoding::none:
		case Encoding::japanese:
			// shouldn't ever reach here, see above
			return make_pair(true, 0);
		case Encoding::inputenc: {
			int count = inputenc_arg.length();
			if (oldEnc.package() == Encoding::CJK &&
			    open_encoding_ == CJK) {
				os << "\\end{CJK}";
				open_encoding_ = none;
				count += 9;
			}
			else if (oldEnc.package() == Encoding::inputenc &&
				 open_encoding_ == inputenc) {
				os << "\\egroup";
				open_encoding_ = none;
				count += 7;
			}
			if (runparams.local_font != 0
			    && 	oldEnc.package() == Encoding::CJK) {
				// within insets, \inputenc switches need
				// to be embraced within \bgroup...\egroup;
				// else CJK fails.
				os << "\\bgroup";
				count += 7;
				open_encoding_ = inputenc;
			}
			// with the japanese option, inputenc is omitted.
			if (runparams.use_japanese)
				return make_pair(true, count);
			os << "\\inputencoding{" << inputenc_arg << '}';
			return make_pair(true, count + 16);
		}
		case Encoding::CJK: {
			int count = inputenc_arg.length();
			if (oldEnc.package() == Encoding::CJK &&
			    open_encoding_ == CJK) {
				os << "\\end{CJK}";
				count += 9;
			}
			if (oldEnc.package() == Encoding::inputenc &&
			    open_encoding_ == inputenc) {
				os << "\\egroup";
				count += 7;
			}
			os << "\\begin{CJK}{" << inputenc_arg << "}{"
			   << from_ascii(bparams.fonts_cjk) << "}";
			open_encoding_ = CJK;
			return make_pair(true, count + 15);
		}
	}
	// Dead code to avoid a warning:
	return make_pair(true, 0);

}

} // namespace lyx
