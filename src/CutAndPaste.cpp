/**
 * \file CutAndPaste.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Jürgen Vigna
 * \author Lars Gullik Bjønnes
 * \author Alfredo Braunstein
 * \author Michael Gerz
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "CutAndPaste.h"

#include "BranchList.h"
#include "Buffer.h"
#include "buffer_funcs.h"
#include "BufferList.h"
#include "BufferParams.h"
#include "BufferView.h"
#include "Changes.h"
#include "Cursor.h"
#include "ErrorList.h"
#include "FuncCode.h"
#include "FuncRequest.h"
#include "InsetIterator.h"
#include "InsetList.h"
#include "Language.h"
#include "LyX.h"
#include "LyXRC.h"
#include "Text.h"
#include "Paragraph.h"
#include "ParagraphParameters.h"
#include "ParIterator.h"
#include "TextClass.h"

#include "insets/InsetBibitem.h"
#include "insets/InsetBranch.h"
#include "insets/InsetCommand.h"
#include "insets/InsetFlex.h"
#include "insets/InsetGraphics.h"
#include "insets/InsetGraphicsParams.h"
#include "insets/InsetInclude.h"
#include "insets/InsetLabel.h"
#include "insets/InsetTabular.h"

#include "mathed/MathData.h"
#include "mathed/InsetMath.h"
#include "mathed/InsetMathHull.h"
#include "mathed/InsetMathRef.h"
#include "mathed/MathSupport.h"

#include "support/debug.h"
#include "support/docstream.h"
#include "support/gettext.h"
#include "support/limited_stack.h"
#include "support/lstrings.h"

#include "frontends/alert.h"
#include "frontends/Clipboard.h"
#include "frontends/Selection.h"

#include <boost/tuple/tuple.hpp>
#include <boost/next_prior.hpp>

#include <string>

using namespace std;
using namespace lyx::support;
using lyx::frontend::Clipboard;

namespace lyx {

namespace {

typedef pair<pit_type, int> PitPosPair;

typedef limited_stack<pair<ParagraphList, DocumentClass const *> > CutStack;

CutStack theCuts(10);
// persistent selection, cleared until the next selection
CutStack selectionBuffer(1);

// store whether the tabular stack is newer than the normal copy stack
// FIXME: this is a workaround for bug 1919. Should be removed for 1.5,
// when we (hopefully) have a one-for-all paste mechanism.
bool dirty_tabular_stack_ = false;


bool checkPastePossible(int index)
{
	return size_t(index) < theCuts.size() && !theCuts[index].first.empty();
}


pair<PitPosPair, pit_type>
pasteSelectionHelper(Cursor const & cur, ParagraphList const & parlist,
		     DocumentClass const * const oldDocClass, ErrorList & errorlist)
{
	Buffer const & buffer = *cur.buffer();
	pit_type pit = cur.pit();
	pos_type pos = cur.pos();
	InsetText * target_inset = cur.inset().asInsetText();
	if (!target_inset) {
		InsetTabular * it = cur.inset().asInsetTabular();
		target_inset = it? it->cell(cur.idx())->asInsetText() : 0;
	}
	LASSERT(target_inset, return make_pair(PitPosPair(pit, pos), pit));
	ParagraphList & pars = target_inset->paragraphs();

	if (parlist.empty())
		return make_pair(PitPosPair(pit, pos), pit);

	BOOST_ASSERT (pos <= pars[pit].size());

	// Make a copy of the CaP paragraphs.
	ParagraphList insertion = parlist;
	DocumentClass const * const newDocClass = 
		buffer.params().documentClassPtr();

	// Now remove all out of the pars which is NOT allowed in the
	// new environment and set also another font if that is required.

	// Convert newline to paragraph break in ParbreakIsNewline
	if (target_inset->getLayout().parbreakIsNewline()
	    || pars[pit].layout().parbreak_is_newline) {
		for (size_t i = 0; i != insertion.size(); ++i) {
			for (pos_type j = 0; j != insertion[i].size(); ++j) {
				if (insertion[i].isNewline(j)) {
					// do not track deletion of newline
					insertion[i].eraseChar(j, false);
					insertion[i].setInsetOwner(target_inset);
					breakParagraphConservative(
							buffer.params(),
							insertion, i, j);
					break;
				}
			}
		}
	}

	// set the paragraphs to plain layout if necessary
	if (cur.inset().usePlainLayout()) {
		bool forcePlainLayout = cur.inset().forcePlainLayout();
		Layout const & plainLayout = newDocClass->plainLayout();
		Layout const & defaultLayout = newDocClass->defaultLayout();
		ParagraphList::iterator const end = insertion.end();
		ParagraphList::iterator par = insertion.begin();
		for (; par != end; ++par) {
			Layout const & parLayout = par->layout();
			if (forcePlainLayout || parLayout == defaultLayout)
				par->setLayout(plainLayout);
		}
	} else { // check if we need to reset from plain layout
		Layout const & defaultLayout = newDocClass->defaultLayout();
		Layout const & plainLayout = newDocClass->plainLayout();
		ParagraphList::iterator const end = insertion.end();
		ParagraphList::iterator par = insertion.begin();
		for (; par != end; ++par) {
			Layout const & parLayout = par->layout();
			if (parLayout == plainLayout)
				par->setLayout(defaultLayout);
		}
	}

	InsetText in(cur.buffer());
	// Make sure there is no class difference.
	in.paragraphs().clear();
	// This works without copying any paragraph data because we have
	// a specialized swap method for ParagraphList. This is important
	// since we store pointers to insets at some places and we don't
	// want to invalidate them.
	insertion.swap(in.paragraphs());
	cap::switchBetweenClasses(oldDocClass, newDocClass, in, errorlist);
	insertion.swap(in.paragraphs());

	ParagraphList::iterator tmpbuf = insertion.begin();
	int depth_delta = pars[pit].params().depth() - tmpbuf->params().depth();

	depth_type max_depth = pars[pit].getMaxDepthAfter();

	for (; tmpbuf != insertion.end(); ++tmpbuf) {
		// If we have a negative jump so that the depth would
		// go below 0 depth then we have to redo the delta to
		// this new max depth level so that subsequent
		// paragraphs are aligned correctly to this paragraph
		// at level 0.
		if (int(tmpbuf->params().depth()) + depth_delta < 0)
			depth_delta = 0;

		// Set the right depth so that we are not too deep or shallow.
		tmpbuf->params().depth(tmpbuf->params().depth() + depth_delta);
		if (tmpbuf->params().depth() > max_depth)
			tmpbuf->params().depth(max_depth);

		// Only set this from the 2nd on as the 2nd depends
		// for maxDepth still on pit.
		if (tmpbuf != insertion.begin())
			max_depth = tmpbuf->getMaxDepthAfter();

		// Set the inset owner of this paragraph.
		tmpbuf->setInsetOwner(target_inset);
		for (pos_type i = 0; i < tmpbuf->size(); ++i) {
			// do not track deletion of invalid insets
			if (Inset * inset = tmpbuf->getInset(i))
				if (!target_inset->insetAllowed(inset->lyxCode()))
					tmpbuf->eraseChar(i--, false);
		}

		tmpbuf->setChange(Change(buffer.params().trackChanges ?
					 Change::INSERTED : Change::UNCHANGED));
	}

	bool const empty = pars[pit].empty();
	if (!empty) {
		// Make the buf exactly the same layout as the cursor
		// paragraph.
		insertion.begin()->makeSameLayout(pars[pit]);
	}

	// Prepare the paragraphs and insets for insertion.
	insertion.swap(in.paragraphs());

	InsetIterator const i_end = inset_iterator_end(in);
	for (InsetIterator it = inset_iterator_begin(in); it != i_end; ++it) {
		// Even though this will also be done later, it has to be done here 
		// since, e.g., InsetLabel::updateCommand() is going to try to access
		// the buffer() member.
		it->setBuffer(const_cast<Buffer &>(buffer));
		switch (it->lyxCode()) {

		case MATH_HULL_CODE: {
			// check for equation labels and resolve duplicates
			InsetMathHull * ins = it->asInsetMath()->asHullInset();
			std::vector<InsetLabel *> labels = ins->getLabels();
			for (size_t i = 0; i != labels.size(); ++i) {
				if (!labels[i])
					continue;
				InsetLabel * lab = labels[i];
				docstring const oldname = lab->getParam("name");
				lab->updateCommand(oldname, false);
				// We need to update the buffer reference cache.
				cur.forceBufferUpdate();
				docstring const newname = lab->getParam("name");
				if (oldname == newname)
					continue;
				// adapt the references
				for (InsetIterator itt = inset_iterator_begin(in);
				      itt != i_end; ++itt) {
					if (itt->lyxCode() == REF_CODE) {
						InsetCommand * ref = itt->asInsetCommand();
						if (ref->getParam("reference") == oldname)
							ref->setParam("reference", newname);
					} else if (itt->lyxCode() == MATH_REF_CODE) {
						InsetMathHull * mi = itt->asInsetMath()->asHullInset();
						// this is necessary to prevent an uninitialized
						// buffer when the RefInset is in a MathBox.
						// FIXME audit setBuffer calls
						mi->setBuffer(const_cast<Buffer &>(buffer));
						if (mi->asRefInset()->getTarget() == oldname)
							mi->asRefInset()->changeTarget(newname);
					}
				}
			}
			break;
		}

		case LABEL_CODE: {
			// check for duplicates
			InsetLabel & lab = static_cast<InsetLabel &>(*it);
			docstring const oldname = lab.getParam("name");
			lab.updateCommand(oldname, false);
			// We need to update the buffer reference cache.
			cur.forceBufferUpdate();
			docstring const newname = lab.getParam("name");
			if (oldname == newname)
				break;
			// adapt the references
			for (InsetIterator itt = inset_iterator_begin(in); itt != i_end; ++itt) {
				if (itt->lyxCode() == REF_CODE) {
					InsetCommand & ref = static_cast<InsetCommand &>(*itt);
					if (ref.getParam("reference") == oldname)
						ref.setParam("reference", newname);
				} else if (itt->lyxCode() == MATH_REF_CODE) {
					InsetMathHull & mi =
						static_cast<InsetMathHull &>(*itt);
					// this is necessary to prevent an uninitialized
					// buffer when the RefInset is in a MathBox.
					// FIXME audit setBuffer calls
					mi.setBuffer(const_cast<Buffer &>(buffer));
					if (mi.asRefInset()->getTarget() == oldname)
						mi.asRefInset()->changeTarget(newname);
				}
			}
			break;
		}

		case INCLUDE_CODE: {
			InsetInclude & inc = static_cast<InsetInclude &>(*it);
			inc.updateCommand();
			// We need to update the list of included files.
			cur.forceBufferUpdate();
			break;
		}

		case BIBITEM_CODE: {
			// check for duplicates
			InsetBibitem & bib = static_cast<InsetBibitem &>(*it);
			docstring const oldkey = bib.getParam("key");
			bib.updateCommand(oldkey, false);
			// We need to update the buffer reference cache.
			cur.forceBufferUpdate();
			docstring const newkey = bib.getParam("key");
			if (oldkey == newkey)
				break;
			// adapt the references
			for (InsetIterator itt = inset_iterator_begin(in);
			     itt != i_end; ++itt) {
				if (itt->lyxCode() == CITE_CODE) {
					InsetCommand * ref = itt->asInsetCommand();
					if (ref->getParam("key") == oldkey)
						ref->setParam("key", newkey);
				}
			}
			break;
		}

		case BRANCH_CODE: {
			// check if branch is known to target buffer
			// or its master
			InsetBranch & br = static_cast<InsetBranch &>(*it);
			docstring const name = br.branch();
			if (name.empty())
				break;
			bool const is_child = (&buffer != buffer.masterBuffer());
			BranchList branchlist = buffer.params().branchlist();
			if ((!is_child && branchlist.find(name))
			    || (is_child && (branchlist.find(name)
			        || buffer.masterBuffer()->params().branchlist().find(name))))
				break;
			// FIXME: add an option to add the branch to the master's BranchList.
			docstring text = bformat(
					_("The pasted branch \"%1$s\" is undefined.\n"
					  "Do you want to add it to the document's branch list?"),
					name);
			if (frontend::Alert::prompt(_("Unknown branch"),
					  text, 0, 1, _("&Add"), _("&Don't Add")) != 0)
				break;
			lyx::dispatch(FuncRequest(LFUN_BRANCH_ADD, name));
			// We need to update the list of branches.
			cur.forceBufferUpdate();
			break;
		}

		default:
			break; // nothing
		}
	}
	insertion.swap(in.paragraphs());

	// Split the paragraph for inserting the buf if necessary.
	if (!empty)
		breakParagraphConservative(buffer.params(), pars, pit, pos);

	// Paste it!
	if (empty) {
		pars.insert(boost::next(pars.begin(), pit),
			    insertion.begin(),
			    insertion.end());

		// merge the empty par with the last par of the insertion
		mergeParagraph(buffer.params(), pars,
			       pit + insertion.size() - 1);
	} else {
		pars.insert(boost::next(pars.begin(), pit + 1),
			    insertion.begin(),
			    insertion.end());

		// merge the first par of the insertion with the current par
		mergeParagraph(buffer.params(), pars, pit);
	}

	// Store the new cursor position.
	pit_type last_paste = pit + insertion.size() - 1;
	pit_type startpit = pit;
	pit = last_paste;
	pos = pars[last_paste].size();

	// FIXME Should we do it here, or should we let updateBuffer() do it?
	// Set paragraph buffers. It's important to do this right away
	// before something calls Inset::buffer() and causes a crash.
	for (pit_type p = startpit; p <= pit; ++p)
		pars[p].setBuffer(const_cast<Buffer &>(buffer));

	// Join (conditionally) last pasted paragraph with next one, i.e.,
	// the tail of the spliced document paragraph
	if (!empty && last_paste + 1 != pit_type(pars.size())) {
		if (pars[last_paste + 1].hasSameLayout(pars[last_paste])) {
			mergeParagraph(buffer.params(), pars, last_paste);
		} else if (pars[last_paste + 1].empty()) {
			pars[last_paste + 1].makeSameLayout(pars[last_paste]);
			mergeParagraph(buffer.params(), pars, last_paste);
		} else if (pars[last_paste].empty()) {
			pars[last_paste].makeSameLayout(pars[last_paste + 1]);
			mergeParagraph(buffer.params(), pars, last_paste);
		} else {
			pars[last_paste + 1].stripLeadingSpaces(buffer.params().trackChanges);
			++last_paste;
		}
	}

	return make_pair(PitPosPair(pit, pos), last_paste + 1);
}


PitPosPair eraseSelectionHelper(BufferParams const & params,
	ParagraphList & pars,
	pit_type startpit, pit_type endpit,
	int startpos, int endpos)
{
	// Start of selection is really invalid.
	if (startpit == pit_type(pars.size()) ||
	    (startpos > pars[startpit].size()))
		return PitPosPair(endpit, endpos);

	// Start and end is inside same paragraph
	if (endpit == pit_type(pars.size()) || startpit == endpit) {
		endpos -= pars[startpit].eraseChars(startpos, endpos, params.trackChanges);
		return PitPosPair(endpit, endpos);
	}

	for (pit_type pit = startpit; pit != endpit + 1;) {
		pos_type const left  = (pit == startpit ? startpos : 0);
		pos_type right = (pit == endpit ? endpos : pars[pit].size() + 1);
		bool const merge = pars[pit].isMergedOnEndOfParDeletion(params.trackChanges);

		// Logically erase only, including the end-of-paragraph character
		pars[pit].eraseChars(left, right, params.trackChanges);

		// Separate handling of paragraph break:
		if (merge && pit != endpit &&
		    (pit + 1 != endpit 
		     || pars[pit].hasSameLayout(pars[endpit])
		     || pars[endpit].size() == endpos)) {
			if (pit + 1 == endpit)
				endpos += pars[pit].size();
			mergeParagraph(params, pars, pit);
			--endpit;
		} else
			++pit;
	}

	// Ensure legal cursor pos:
	endpit = startpit;
	endpos = startpos;
	return PitPosPair(endpit, endpos);
}


void putClipboard(ParagraphList const & paragraphs, 
	DocumentClass const * const docclass, docstring const & plaintext)
{
	// For some strange reason gcc 3.2 and 3.3 do not accept
	// Buffer buffer(string(), false);
	// This needs to be static to avoid a memory leak. When a Buffer is
	// constructed, it constructs a BufferParams, which in turn constructs
	// a DocumentClass, via new, that is never deleted. If we were to go to
	// some kind of garbage collection there, or a shared_ptr, then this
	// would not be needed.
	static Buffer * buffer = theBufferList().newBuffer(
		FileName::tempName().absFileName() + "_clipboard.internal");
	buffer->setUnnamed(true);
	buffer->paragraphs() = paragraphs;
	buffer->inset().setBuffer(*buffer);
	buffer->params().setDocumentClass(docclass);
	ostringstream lyx;
	if (buffer->write(lyx))
		theClipboard().put(lyx.str(), plaintext);
	else
		theClipboard().put(string(), plaintext);
	// Save that memory
	buffer->paragraphs().clear();
}


/// return true if the whole ParagraphList is deleted
static bool isFullyDeleted(ParagraphList const & pars)
{
	pit_type const pars_size = static_cast<pit_type>(pars.size());

	// check all paragraphs
	for (pit_type pit = 0; pit < pars_size; ++pit) {
		if (!pars[pit].empty())   // prevent assertion failure
			if (!pars[pit].isDeleted(0, pars[pit].size()))
				return false;
	}
	return true;
}


void copySelectionHelper(Buffer const & buf, Text const & text,
	pit_type startpit, pit_type endpit,
	int start, int end, DocumentClass const * const dc, CutStack & cutstack)
{
	ParagraphList const & pars = text.paragraphs();

	LASSERT(0 <= start && start <= pars[startpit].size(), /**/);
	LASSERT(0 <= end && end <= pars[endpit].size(), /**/);
	LASSERT(startpit != endpit || start <= end, /**/);

	// Clone the paragraphs within the selection.
	ParagraphList copy_pars(boost::next(pars.begin(), startpit),
				boost::next(pars.begin(), endpit + 1));

	// Remove the end of the last paragraph; afterwards, remove the
	// beginning of the first paragraph. Keep this order - there may only
	// be one paragraph!  Do not track deletions here; this is an internal
	// action not visible to the user

	Paragraph & back = copy_pars.back();
	back.eraseChars(end, back.size(), false);
	Paragraph & front = copy_pars.front();
	front.eraseChars(0, start, false);

	ParagraphList::iterator it = copy_pars.begin();
	ParagraphList::iterator it_end = copy_pars.end();

	for (; it != it_end; it++) {
		// Since we have a copy of the paragraphs, the insets
		// do not have a proper buffer reference. It makes
		// sense to add them temporarily, because the
		// operations below depend on that (acceptChanges included).
		it->setBuffer(const_cast<Buffer &>(buf));
		// PassThru paragraphs have the Language
		// latex_language. This is invalid for others, so we
		// need to change it to the buffer language.
		if (it->isPassThru())
			it->changeLanguage(buf.params(), 
					   latex_language, buf.language());
	}

	// do not copy text (also nested in insets) which is marked as
	// deleted, unless the whole selection was deleted
	if (!isFullyDeleted(copy_pars))
		acceptChanges(copy_pars, buf.params());
	else
		rejectChanges(copy_pars, buf.params());


	// do some final cleanup now, to make sure that the paragraphs
	// are not linked to something else.
	it = copy_pars.begin();
	for (; it != it_end; it++) {
		it->setBuffer(*static_cast<Buffer *>(0));
		it->setInsetOwner(0);
	}

	DocumentClass * d = const_cast<DocumentClass *>(dc);
	cutstack.push(make_pair(copy_pars, d));
}

} // namespace anon




namespace cap {

void region(CursorSlice const & i1, CursorSlice const & i2,
	    Inset::row_type & r1, Inset::row_type & r2,
	    Inset::col_type & c1, Inset::col_type & c2)
{
	Inset & p = i1.inset();
	c1 = p.col(i1.idx());
	c2 = p.col(i2.idx());
	if (c1 > c2)
		swap(c1, c2);
	r1 = p.row(i1.idx());
	r2 = p.row(i2.idx());
	if (r1 > r2)
		swap(r1, r2);
}


docstring grabAndEraseSelection(Cursor & cur)
{
	if (!cur.selection())
		return docstring();
	docstring res = grabSelection(cur);
	eraseSelection(cur);
	return res;
}


bool reduceSelectionToOneCell(Cursor & cur)
{
	if (!cur.selection() || !cur.inMathed())
		return false;

	CursorSlice i1 = cur.selBegin();
	CursorSlice i2 = cur.selEnd();
	if (!i1.inset().asInsetMath())
		return false;

	// the easy case: do nothing if only one cell is selected
	if (i1.idx() == i2.idx())
		return true;
	
	cur.top().pos() = 0;
	cur.resetAnchor();
	cur.top().pos() = cur.top().lastpos();
	
	return true;
}


bool multipleCellsSelected(Cursor const & cur)
{
	if (!cur.selection() || !cur.inMathed())
		return false;
	
	CursorSlice i1 = cur.selBegin();
	CursorSlice i2 = cur.selEnd();
	if (!i1.inset().asInsetMath())
		return false;
	
	if (i1.idx() == i2.idx())
		return false;
	
	return true;
}


void switchBetweenClasses(DocumentClass const * const oldone, 
		DocumentClass const * const newone, InsetText & in, ErrorList & errorlist)
{
	errorlist.clear();

	LASSERT(!in.paragraphs().empty(), /**/);
	if (oldone == newone)
		return;
	
	DocumentClass const & oldtc = *oldone;
	DocumentClass const & newtc = *newone;

	// layouts
	ParIterator it = par_iterator_begin(in);
	ParIterator end = par_iterator_end(in);
	// for remembering which layouts we've had to add
	set<docstring> newlayouts;
	for (; it != end; ++it) {
		docstring const name = it->layout().name();

		// the pasted text will keep their own layout name. If this layout does
		// not exist in the new document, it will behave like a standard layout.
		bool const added_one = newtc.addLayoutIfNeeded(name);
		if (added_one)
			newlayouts.insert(name);

		if (added_one || newlayouts.find(name) != newlayouts.end()) {
			// Warn the user.
			docstring const s = bformat(_("Layout `%1$s' was not found."), name);
			errorlist.push_back(
				ErrorItem(_("Layout Not Found"), s, it->id(), 0, it->size()));
		}

		if (in.usePlainLayout())
			it->setLayout(newtc.plainLayout());
		else
			it->setLayout(newtc[name]);
	}

	// character styles
	InsetIterator const i_end = inset_iterator_end(in);
	for (InsetIterator it = inset_iterator_begin(in); it != i_end; ++it) {
		if (it->lyxCode() != FLEX_CODE)
			// FIXME: Should we verify all InsetCollapsable?
			continue;

		docstring const layoutName = it->layoutName();
		docstring const & n = newone->insetLayout(layoutName).name();
		bool const is_undefined = n.empty() ||
			n == DocumentClass::plainInsetLayout().name();
		if (!is_undefined)
			continue;

		// The flex inset is undefined in newtc
		docstring const oldname = from_utf8(oldtc.name());
		docstring const newname = from_utf8(newtc.name());
		docstring s;
		if (oldname == newname)
			s = bformat(_("Flex inset %1$s is undefined after "
				"reloading `%2$s' layout."), layoutName, oldname);
		else
			s = bformat(_("Flex inset %1$s is undefined because of "
				"conversion from `%2$s' layout to `%3$s'."),
				layoutName, oldname, newname);
		// To warn the user that something had to be done.
		errorlist.push_back(ErrorItem(
				_("Undefined flex inset"),
				s, it.paragraph().id(),	it.pos(), it.pos() + 1));
	}
}


vector<docstring> availableSelections(Buffer const * buf)
{
	vector<docstring> selList;
	if (!buf)
		return selList;

	CutStack::const_iterator cit = theCuts.begin();
	CutStack::const_iterator end = theCuts.end();
	for (; cit != end; ++cit) {
		// we do not use cit-> here because gcc 2.9x does not
		// like it (JMarc)
		ParagraphList const & pars = (*cit).first;
		docstring asciiSel;
		ParagraphList::const_iterator pit = pars.begin();
		ParagraphList::const_iterator pend = pars.end();
		for (; pit != pend; ++pit) {
			Paragraph par(*pit, 0, 26);
			// adapt paragraph to current buffer.
			par.setBuffer(const_cast<Buffer &>(*buf));
			asciiSel += par.asString(AS_STR_INSETS);
			if (asciiSel.size() > 25) {
				asciiSel.replace(22, docstring::npos,
						 from_ascii("..."));
				break;
			}
		}

		selList.push_back(asciiSel);
	}

	return selList;
}


size_type numberOfSelections()
{
	return theCuts.size();
}


void cutSelection(Cursor & cur, bool doclear, bool realcut)
{
	// This doesn't make sense, if there is no selection
	if (!cur.selection())
		return;

	// OK, we have a selection. This is always between cur.selBegin()
	// and cur.selEnd()

	if (cur.inTexted()) {
		Text * text = cur.text();
		LASSERT(text, /**/);

		saveSelection(cur);

		// make sure that the depth behind the selection are restored, too
		cur.recordUndoSelection();
		pit_type begpit = cur.selBegin().pit();
		pit_type endpit = cur.selEnd().pit();

		int endpos = cur.selEnd().pos();

		BufferParams const & bp = cur.buffer()->params();
		if (realcut) {
			copySelectionHelper(*cur.buffer(),
				*text,
				begpit, endpit,
				cur.selBegin().pos(), endpos,
				bp.documentClassPtr(), theCuts);
			// Stuff what we got on the clipboard.
			// Even if there is no selection.
			putClipboard(theCuts[0].first, theCuts[0].second,
				cur.selectionAsString(true));
		}

		if (begpit != endpit)
			cur.screenUpdateFlags(Update::Force | Update::FitCursor);

		boost::tie(endpit, endpos) =
			eraseSelectionHelper(bp,
				text->paragraphs(),
				begpit, endpit,
				cur.selBegin().pos(), endpos);

		// cutSelection can invalidate the cursor so we need to set
		// it anew. (Lgb)
		// we prefer the end for when tracking changes
		cur.pos() = endpos;
		cur.pit() = endpit;

		// sometimes necessary
		if (doclear
			&& text->paragraphs()[begpit].stripLeadingSpaces(bp.trackChanges))
			cur.fixIfBroken();

		// need a valid cursor. (Lgb)
		cur.clearSelection();

		// After a cut operation, we must make sure that the Buffer is updated
		// because some further operation might need updated label information for
		// example. So we cannot just use "cur.forceBufferUpdate()" here.
		// This fixes #7071.
		cur.buffer()->updateBuffer();

		// tell tabular that a recent copy happened
		dirtyTabularStack(false);
	}

	if (cur.inMathed()) {
		if (cur.selBegin().idx() != cur.selEnd().idx()) {
			// The current selection spans more than one cell.
			// Record all cells
			cur.recordUndoInset();
		} else {
			// Record only the current cell to avoid a jumping
			// cursor after undo
			cur.recordUndo();
		}
		if (realcut)
			copySelection(cur);
		eraseSelection(cur);
	}
}


void copySelection(Cursor const & cur)
{
	copySelection(cur, cur.selectionAsString(true));
}


void copyInset(Cursor const & cur, Inset * inset, docstring const & plaintext)
{
	ParagraphList pars;
	Paragraph par;
	BufferParams const & bp = cur.buffer()->params();
	par.setLayout(bp.documentClass().plainLayout());
	par.insertInset(0, inset, Change(Change::UNCHANGED));
	pars.push_back(par);
	theCuts.push(make_pair(pars, bp.documentClassPtr()));

	// stuff the selection onto the X clipboard, from an explicit copy request
	putClipboard(theCuts[0].first, theCuts[0].second, plaintext);
}

namespace {

void copySelectionToStack(Cursor const & cur, CutStack & cutstack)
{
	// this doesn't make sense, if there is no selection
	if (!cur.selection())
		return;

	// copySelection can not yet handle the case of cross idx selection
	if (cur.selBegin().idx() != cur.selEnd().idx())
		return;

	if (cur.inTexted()) {
		Text * text = cur.text();
		LASSERT(text, /**/);
		// ok we have a selection. This is always between cur.selBegin()
		// and sel_end cursor

		// copy behind a space if there is one
		ParagraphList & pars = text->paragraphs();
		pos_type pos = cur.selBegin().pos();
		pit_type par = cur.selBegin().pit();
		while (pos < pars[par].size() &&
		       pars[par].isLineSeparator(pos) &&
		       (par != cur.selEnd().pit() || pos < cur.selEnd().pos()))
			++pos;

		copySelectionHelper(*cur.buffer(), *text, par, cur.selEnd().pit(),
			pos, cur.selEnd().pos(), 
			cur.buffer()->params().documentClassPtr(), cutstack);

		// Reset the dirty_tabular_stack_ flag only when something
		// is copied to the clipboard (not to the selectionBuffer).
		if (&cutstack == &theCuts)
			dirtyTabularStack(false);
	}

	if (cur.inMathed()) {
		//lyxerr << "copySelection in mathed" << endl;
		ParagraphList pars;
		Paragraph par;
		BufferParams const & bp = cur.buffer()->params();
		// FIXME This should be the plain layout...right?
		par.setLayout(bp.documentClass().plainLayout());
		par.insert(0, grabSelection(cur), Font(), Change(Change::UNCHANGED));
		pars.push_back(par);
		cutstack.push(make_pair(pars, bp.documentClassPtr()));
	}
}

}


void copySelectionToStack()
{
	if (!selectionBuffer.empty())
		theCuts.push(selectionBuffer[0]);
}


void copySelection(Cursor const & cur, docstring const & plaintext)
{
	// In tablemode, because copy and paste actually use special table stack
	// we do not attempt to get selected paragraphs under cursor. Instead, a
	// paragraph with the plain text version is generated so that table cells
	// can be pasted as pure text somewhere else.
	if (cur.selBegin().idx() != cur.selEnd().idx()) {
		ParagraphList pars;
		Paragraph par;
		BufferParams const & bp = cur.buffer()->params();
		par.setLayout(bp.documentClass().plainLayout());
		par.insert(0, plaintext, Font(), Change(Change::UNCHANGED));
		pars.push_back(par);
		theCuts.push(make_pair(pars, bp.documentClassPtr()));
	} else {
		copySelectionToStack(cur, theCuts);
	}

	// stuff the selection onto the X clipboard, from an explicit copy request
	putClipboard(theCuts[0].first, theCuts[0].second, plaintext);
}


void saveSelection(Cursor const & cur)
{
	// This function is called, not when a selection is formed, but when
	// a selection is cleared. Therefore, multiple keyboard selection
	// will not repeatively trigger this function (bug 3877).
	if (cur.selection() 
	    && cur.selBegin() == cur.bv().cursor().selBegin()
	    && cur.selEnd() == cur.bv().cursor().selEnd()) {
		LYXERR(Debug::SELECTION, "saveSelection: '" << cur.selectionAsString(true) << "'");
		copySelectionToStack(cur, selectionBuffer);
	}
}


bool selection()
{
	return !selectionBuffer.empty();
}


void clearSelection()
{
	selectionBuffer.clear();
}


void clearCutStack()
{
	theCuts.clear();
}


docstring selection(size_t sel_index)
{
	return sel_index < theCuts.size()
		? theCuts[sel_index].first.back().asString(AS_STR_INSETS | AS_STR_NEWLINES)
		: docstring();
}


void pasteParagraphList(Cursor & cur, ParagraphList const & parlist,
			DocumentClass const * const docclass, ErrorList & errorList)
{
	if (cur.inTexted()) {
		Text * text = cur.text();
		LASSERT(text, /**/);

		pit_type endpit;
		PitPosPair ppp;

		boost::tie(ppp, endpit) =
			pasteSelectionHelper(cur, parlist, docclass, errorList);
		cur.forceBufferUpdate();
		cur.clearSelection();
		text->setCursor(cur, ppp.first, ppp.second);
	}

	// mathed is handled in InsetMathNest/InsetMathGrid
	LASSERT(!cur.inMathed(), /**/);
}


void pasteFromStack(Cursor & cur, ErrorList & errorList, size_t sel_index)
{
	// this does not make sense, if there is nothing to paste
	if (!checkPastePossible(sel_index))
		return;

	cur.recordUndo();
	pasteParagraphList(cur, theCuts[sel_index].first,
			   theCuts[sel_index].second, errorList);
	cur.setSelection();
}


void pasteClipboardText(Cursor & cur, ErrorList & errorList, bool asParagraphs)
{
	// Use internal clipboard if it is the most recent one
	if (theClipboard().isInternal()) {
		pasteFromStack(cur, errorList, 0);
		return;
	}

	// First try LyX format
	if (theClipboard().hasLyXContents()) {
		string lyx = theClipboard().getAsLyX();
		if (!lyx.empty()) {
			// For some strange reason gcc 3.2 and 3.3 do not accept
			// Buffer buffer(string(), false);
			Buffer buffer("", false);
			buffer.setUnnamed(true);
			if (buffer.readString(lyx)) {
				cur.recordUndo();
				pasteParagraphList(cur, buffer.paragraphs(),
					buffer.params().documentClassPtr(), errorList);
				cur.setSelection();
				return;
			}
		}
	}

	// Then try plain text
	docstring const text = theClipboard().getAsText();
	if (text.empty())
		return;
	cur.recordUndo();
	if (asParagraphs)
		cur.text()->insertStringAsParagraphs(cur, text, cur.current_font);
	else
		cur.text()->insertStringAsLines(cur, text, cur.current_font);
}


void pasteClipboardGraphics(Cursor & cur, ErrorList & /* errorList */,
			    Clipboard::GraphicsType preferedType)
{
	LASSERT(theClipboard().hasGraphicsContents(preferedType), /**/);

	// get picture from clipboard
	FileName filename = theClipboard().getAsGraphics(cur, preferedType);
	if (filename.empty())
		return;

	// create inset for graphic
	InsetGraphics * inset = new InsetGraphics(cur.buffer());
	InsetGraphicsParams params;
	params.filename = support::DocFileName(filename.absFileName(), false);
	inset->setParams(params);
	cur.recordUndo();
	cur.insert(inset);
}


void pasteSelection(Cursor & cur, ErrorList & errorList)
{
	if (selectionBuffer.empty())
		return;
	cur.recordUndo();
	pasteParagraphList(cur, selectionBuffer[0].first,
			   selectionBuffer[0].second, errorList);
}


void replaceSelectionWithString(Cursor & cur, docstring const & str, bool backwards)
{
	cur.recordUndo();
	DocIterator selbeg = cur.selectionBegin();

	// Get font setting before we cut, we need a copy here, not a bare reference.
	Font const font =
		selbeg.paragraph().getFontSettings(cur.buffer()->params(), selbeg.pos());

	// Insert the new string
	pos_type pos = cur.selEnd().pos();
	Paragraph & par = cur.selEnd().paragraph();
	docstring::const_iterator cit = str.begin();
	docstring::const_iterator end = str.end();
	for (; cit != end; ++cit, ++pos)
		par.insertChar(pos, *cit, font, cur.buffer()->params().trackChanges);

	// Cut the selection
	cutSelection(cur, true, false);

	// select the replacement
	if (backwards) {
		selbeg.pos() += str.length();
		cur.setSelection(selbeg, -int(str.length()));
	} else
		cur.setSelection(selbeg, str.length());
}


void replaceSelection(Cursor & cur)
{
	if (cur.selection())
		cutSelection(cur, true, false);
}


void eraseSelection(Cursor & cur)
{
	//lyxerr << "cap::eraseSelection begin: " << cur << endl;
	CursorSlice const & i1 = cur.selBegin();
	CursorSlice const & i2 = cur.selEnd();
	if (i1.inset().asInsetMath()) {
		saveSelection(cur);
		cur.top() = i1;
		if (i1.idx() == i2.idx()) {
			i1.cell().erase(i1.pos(), i2.pos());
			// We may have deleted i1.cell(cur.pos()).
			// Make sure that pos is valid.
			if (cur.pos() > cur.lastpos())
				cur.pos() = cur.lastpos();
		} else {
			InsetMath * p = i1.asInsetMath();
			Inset::row_type r1, r2;
			Inset::col_type c1, c2;
			region(i1, i2, r1, r2, c1, c2);
			for (Inset::row_type row = r1; row <= r2; ++row)
				for (Inset::col_type col = c1; col <= c2; ++col)
					p->cell(p->index(row, col)).clear();
			// We've deleted the whole cell. Only pos 0 is valid.
			cur.pos() = 0;
		}
		// need a valid cursor. (Lgb)
		cur.clearSelection();
	} else {
		lyxerr << "can't erase this selection 1" << endl;
	}
	//lyxerr << "cap::eraseSelection end: " << cur << endl;
}


void selDel(Cursor & cur)
{
	//lyxerr << "cap::selDel" << endl;
	if (cur.selection())
		eraseSelection(cur);
}


void selClearOrDel(Cursor & cur)
{
	//lyxerr << "cap::selClearOrDel" << endl;
	if (lyxrc.auto_region_delete)
		selDel(cur);
	else
		cur.setSelection(false);
}


docstring grabSelection(Cursor const & cur)
{
	if (!cur.selection())
		return docstring();

#if 0
	// grab selection by glueing multiple cells together. This is not what
	// we want because selections spanning multiple cells will get "&" and "\\"
	// seperators.
	ostringstream os;
	for (DocIterator dit = cur.selectionBegin();
	     dit != cur.selectionEnd(); dit.forwardPos())
		os << asString(dit.cell());
	return os.str();
#endif

	CursorSlice i1 = cur.selBegin();
	CursorSlice i2 = cur.selEnd();

	if (i1.idx() == i2.idx()) {
		if (i1.inset().asInsetMath()) {
			MathData::const_iterator it = i1.cell().begin();
			Buffer * buf = cur.buffer();
			return asString(MathData(buf, it + i1.pos(), it + i2.pos()));
		} else {
			return from_ascii("unknown selection 1");
		}
	}

	Inset::row_type r1, r2;
	Inset::col_type c1, c2;
	region(i1, i2, r1, r2, c1, c2);

	docstring data;
	if (i1.inset().asInsetMath()) {
		for (Inset::row_type row = r1; row <= r2; ++row) {
			if (row > r1)
				data += "\\\\";
			for (Inset::col_type col = c1; col <= c2; ++col) {
				if (col > c1)
					data += '&';
				data += asString(i1.asInsetMath()->
					cell(i1.asInsetMath()->index(row, col)));
			}
		}
	} else {
		data = from_ascii("unknown selection 2");
	}
	return data;
}


void dirtyTabularStack(bool b)
{
	dirty_tabular_stack_ = b;
}


bool tabularStackDirty()
{
	return dirty_tabular_stack_;
}


} // namespace cap
} // namespace lyx
