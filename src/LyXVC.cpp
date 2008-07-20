/**
 * \file LyXVC.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Lars Gullik Bj�nnes
 * \author Jean-Marc Lasgouttes
 * \author Angus Leeming
 * \author John Levon
 * \author Andr� P�nitz
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
	// file is not under any VCS.
	return false;
}


bool LyXVC::file_not_found_hook(FileName const & fn)
{
	// Check if file is under RCS
	if (!RCS::findFile(fn).empty())
		return true;
	if (!CVS::findFile(fn).empty())
		return true;
	return false;
}


void LyXVC::setBuffer(Buffer * buf)
{
	owner_ = buf;
}


void LyXVC::registrer()
{
	FileName const filename = owner_->fileName();

	// there must be a file to save
	if (!filename.isReadableFile()) {
		Alert::error(_("Document not saved"),
			     _("You must save the document "
					    "before it can be registered."));
		return;
	}

	// it is very likely here that the vcs is not created yet...
	if (!vcs) {
		FileName const cvs_entries(makeAbsPath("CVS/Entries"));

		if (cvs_entries.isReadableFile()) {
			LYXERR(Debug::LYXVC, "LyXVC: registering "
				<< to_utf8(filename.displayName()) << " with CVS");
			vcs.reset(new CVS(cvs_entries, filename));

		} else {
			LYXERR(Debug::LYXVC, "LyXVC: registering "
				<< to_utf8(filename.displayName()) << " with RCS");
			vcs.reset(new RCS(filename));
		}

		vcs->owner(owner_);
	}

	LYXERR(Debug::LYXVC, "LyXVC: registrer");
	docstring response;
	bool ok = Alert::askForText(response, _("LyX VC: Initial description"),
			_("(no initial description)"));
	if (!ok || response.empty()) {
		// should we insist on checking response.empty()?
		LYXERR(Debug::LYXVC, "LyXVC: user cancelled");
		return;
	}

	vcs->registrer(to_utf8(response));
}


void LyXVC::checkIn()
{
	LYXERR(Debug::LYXVC, "LyXVC: checkIn");
	docstring response;
	bool ok = Alert::askForText(response, _("LyX VC: Log Message"));
	if (ok) {
		if (response.empty())
			response = _("(no log message)");
		vcs->checkIn(to_utf8(response));
	} else {
		LYXERR(Debug::LYXVC, "LyXVC: user cancelled");
	}
}


void LyXVC::checkOut()
{
	LYXERR(Debug::LYXVC, "LyXVC: checkOut");
	vcs->checkOut();
}


void LyXVC::revert()
{
	LYXERR(Debug::LYXVC, "LyXVC: revert");

	docstring const file = owner_->fileName().displayName(20);
	docstring text = bformat(_("Reverting to the stored version of the "
		"document %1$s will lose all current changes.\n\n"
					     "Do you want to revert to the saved version?"), file);
	int const ret = Alert::prompt(_("Revert to stored version of document?"),
		text, 0, 1, _("&Revert"), _("&Cancel"));

	if (ret == 0)
		vcs->revert();
}


void LyXVC::undoLast()
{
	vcs->undoLast();
}


void LyXVC::toggleReadOnly()
{
	switch (vcs->status()) {
	case VCS::UNLOCKED:
		LYXERR(Debug::LYXVC, "LyXVC: toggle to locked");
		checkOut();
		break;
	case VCS::LOCKED:
		LYXERR(Debug::LYXVC, "LyXVC: toggle to unlocked");
		checkIn();
		break;
	}
}


bool LyXVC::inUse()
{
	if (vcs)
		return true;
	return false;
}


//string const & LyXVC::version() const
//{
//	return vcs->version();
//}


string const LyXVC::versionString() const
{
	return vcs->versionString();
}


string const & LyXVC::locker() const
{
	return vcs->locker();
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
	return tmpf.absFilename();
}


} // namespace lyx
