/**
 * \file LyXVC.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Lars Gullik Bjønnes
 * \author Jean-Marc Lasgouttes
 * \author Angus Leeming
 * \author John Levon
 * \author André Pönitz
 * \author Allan Rae
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "LyXVC.h"
#include "VCBackend.h"
#include "Buffer.h"

#include "frontends/alert.h"

#include "support/debug.h"
#include "support/filetools.h"
#include "support/gettext.h"
#include "support/lstrings.h"

using namespace std;
using namespace lyx::support;

namespace lyx {

namespace Alert = frontend::Alert;


LyXVC::LyXVC()
{
	owner_ = 0;
}


// for the sake of boost::scoped_ptr
LyXVC::~LyXVC()
{}


bool LyXVC::file_found_hook(FileName const & fn)
{
	FileName found_file;
	// Check if file is under RCS
	if (!(found_file = RCS::findFile(fn)).empty()) {
		vcs.reset(new RCS(found_file));
		vcs->owner(owner_);
		return true;
	}
	// Check if file is under CVS
	if (!(found_file = CVS::findFile(fn)).empty()) {
		vcs.reset(new CVS(found_file, fn));
		vcs->owner(owner_);
		return true;
	}
	// Check if file is under SVN
	if (!(found_file = SVN::findFile(fn)).empty()) {
		vcs.reset(new SVN(found_file, fn));
		vcs->owner(owner_);
		return true;
	}

	// file is not under any VCS.
	vcs.reset(0);
	return false;
}


bool LyXVC::file_not_found_hook(FileName const & fn)
{
	// Check if file is under RCS.
	// This happens if we are trying to load non existent
	// file on disk, but existent in ,v version.
	// Seems there is no reasonable scenario for adding implementation
	// of retrieve for cvs or svn.
	if (!RCS::findFile(fn).empty()) {	
		docstring const file = makeDisplayPath(fn.absFileName(), 20);
		docstring const text =
			bformat(_("Do you want to retrieve the document"
						   " %1$s from version control?"), file);
		int const ret = Alert::prompt(_("Retrieve from version control?"),
			text, 0, 1, _("&Retrieve"), _("&Cancel"));

		if (ret == 0) {
			// How can we know _how_ to do the checkout?
			// With the current VC support it has to be an RCS
			// file since CVS and SVN do not have special ,v files.
			RCS::retrieve(fn);
			return true;
		}
	}
	return false;
}


void LyXVC::setBuffer(Buffer * buf)
{
	owner_ = buf;
}


bool LyXVC::registrer()
{
	FileName const filename = owner_->fileName();

	// there must be a file to save
	if (!filename.isReadableFile()) {
		Alert::error(_("Document not saved"),
			     _("You must save the document "
					    "before it can be registered."));
		return false;
	}

	// it is very likely here that the vcs is not created yet...
	if (!vcs) {
		//check in the root directory of the document
		FileName const cvs_entries(onlyPath(filename.absFileName()) + "/CVS/Entries");
		FileName const svn_entries(onlyPath(filename.absFileName()) + "/.svn/entries");

		if (svn_entries.isReadableFile()) {
			LYXERR(Debug::LYXVC, "LyXVC: registering "
				<< to_utf8(filename.displayName()) << " with SVN");
			vcs.reset(new SVN(cvs_entries, filename));

		} else if (cvs_entries.isReadableFile()) {
			LYXERR(Debug::LYXVC, "LyXVC: registering "
				<< to_utf8(filename.displayName()) << " with CVS");
			vcs.reset(new CVS(cvs_entries, filename));

		} else {
			LYXERR(Debug::LYXVC, "LyXVC: registering "
				<< to_utf8(filename.displayName()) << " with RCS");
			vcs.reset(new RCS(FileName()));
		}

		vcs->owner(owner_);
	}

	LYXERR(Debug::LYXVC, "LyXVC: registrer");
	docstring response;
	bool ok = Alert::askForText(response, _("LyX VC: Initial description"),
			_("(no initial description)"));
	if (!ok) {
		LYXERR(Debug::LYXVC, "LyXVC: user cancelled");
		vcs.reset(0);
		return false;
	}
	if (response.empty())
		response = _("(no initial description)");
	vcs->registrer(to_utf8(response));
	return true;
}


string LyXVC::checkIn()
{
	LYXERR(Debug::LYXVC, "LyXVC: checkIn");
	docstring empty(_("(no log message)"));
	docstring response;
	string log;
	bool ok = true;
	if (vcs->isCheckInWithConfirmation())
		ok = Alert::askForText(response, _("LyX VC: Log Message"));
	if (ok) {
		if (response.empty())
			response = empty;
		//shell collisions
		response = subst(response, from_utf8("\""), from_utf8("\\\""));
		log = vcs->checkIn(to_utf8(response));

		// Reserve empty string for cancel button
		if (log.empty())
			log = to_utf8(empty);
	} else {
		LYXERR(Debug::LYXVC, "LyXVC: user cancelled");
	}
	return log;
}


string LyXVC::checkOut()
{
	//RCS allows checkOut only in ReadOnly mode
	if (vcs->toggleReadOnlyEnabled() && !owner_->isReadonly())
		return string();

	LYXERR(Debug::LYXVC, "LyXVC: checkOut");
	return vcs->checkOut();
}


string LyXVC::repoUpdate()
{
	LYXERR(Debug::LYXVC, "LyXVC: repoUpdate");
	return vcs->repoUpdate();
}


string LyXVC::lockingToggle()
{
	LYXERR(Debug::LYXVC, "LyXVC: toggle locking property");
	return vcs->lockingToggle();
}


bool LyXVC::revert()
{
	LYXERR(Debug::LYXVC, "LyXVC: revert");

	docstring const file = owner_->fileName().displayName(20);
	docstring text = bformat(_("Reverting to the stored version of the "
				"document %1$s will lose all current changes.\n\n"
				"Do you want to revert to the older version?"), file);
	int ret = 0;
	if (vcs->isRevertWithConfirmation())
		ret = Alert::prompt(_("Revert to stored version of document?"),
			text, 0, 1, _("&Revert"), _("&Cancel"));

	return ret == 0 && vcs->revert();
}


void LyXVC::undoLast()
{
	vcs->undoLast();
}


void LyXVC::toggleReadOnly()
{
	if (!vcs->toggleReadOnlyEnabled())
		return;

	switch (vcs->status()) {
	case VCS::UNLOCKED:
		LYXERR(Debug::LYXVC, "LyXVC: toggle to locked");
		checkOut();
		break;
	case VCS::LOCKED:
		LYXERR(Debug::LYXVC, "LyXVC: toggle to unlocked");
		checkIn();
		break;
	case VCS::NOLOCKING:
		break;
	}
}


bool LyXVC::inUse() const
{
	if (vcs)
		return true;
	return false;
}


string const LyXVC::versionString() const
{
	return vcs->versionString();
}


bool LyXVC::locking() const
{
	return vcs->status() != VCS::NOLOCKING;
}


string const LyXVC::getLogFile() const
{
	if (!vcs)
		return string();

	FileName const tmpf = FileName::tempName("lyxvclog");
	if (tmpf.empty()) {
		LYXERR(Debug::LYXVC, "Could not generate logfile " << tmpf);
		return string();
	}
	LYXERR(Debug::LYXVC, "Generating logfile " << tmpf);
	vcs->getLog(tmpf);
	return tmpf.absFileName();
}


string LyXVC::revisionInfo(RevisionInfo const info) const
{
	if (!vcs)
		return string();

	return vcs->revisionInfo(info);
}


bool LyXVC::checkOutEnabled() const
{
	return vcs && vcs->checkOutEnabled();
}


bool LyXVC::checkInEnabled() const
{
	return vcs && vcs->checkInEnabled();
}


bool LyXVC::lockingToggleEnabled() const
{
	return vcs && vcs->lockingToggleEnabled();
}


bool LyXVC::undoLastEnabled() const
{
	return vcs && vcs->undoLastEnabled();
}


bool LyXVC::repoUpdateEnabled() const
{
	return vcs && vcs->repoUpdateEnabled();
}
	
	
bool LyXVC::prepareFileRevision(string const & rev, std::string & f)
{
	return vcs && vcs->prepareFileRevision(rev, f);
}


bool LyXVC::prepareFileRevisionEnabled()
{
	return vcs && vcs->prepareFileRevisionEnabled();
}

} // namespace lyx
