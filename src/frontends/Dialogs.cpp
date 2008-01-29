/**
 * \file frontends/Dialogs.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Angus Leeming
 *
 * Full author contact details are available in file CREDITS.
 *
 * Common to all frontends' Dialogs
 */

#include <config.h>

#include "Dialogs.h"

#include "callback.h"

#include "controllers/Dialog.h"

#include <boost/signal.hpp>
#include <boost/bind.hpp>

using std::string;

namespace lyx {

using lyx::frontend::Dialog;


Dialogs::Dialogs(LyXView & lyxview)
	: lyxview_(lyxview), in_show_(false)
{
}


Dialog * Dialogs::find_or_build(string const & name)
{
	if (!isValidName(name))
		return 0;

	std::map<string, DialogPtr>::iterator it =
		dialogs_.find(name);

	if (it != dialogs_.end())
		return it->second.get();

	dialogs_[name] = build(name);
	return dialogs_[name].get();
}


void Dialogs::show(string const & name, string const & data)
{
	if (in_show_) {
		return;
	}
	in_show_ = true;
	Dialog * dialog = find_or_build(name);
	if (dialog) {
		// FIXME! Should check that the dialog is NOT an inset dialog.
		dialog->show(data);
	}
	in_show_ = false;
}


void Dialogs::show(string const & name, string const & data, Inset * inset)
{
	if (in_show_) {
		return;
	}
	in_show_ = true;
	Dialog * dialog = find_or_build(name);
	if (dialog) {
		// FIXME! Should check that the dialog IS an inset dialog.
		dialog->show(data);
		open_insets_[name] = inset;
	}
	in_show_ = false;
}


bool Dialogs::visible(string const & name) const
{
	std::map<string, DialogPtr>::const_iterator it =
		dialogs_.find(name);
	if (it == dialogs_.end())
		return false;
	return it->second.get()->isVisible();
}


void Dialogs::update(string const & name, string const & data)
{
	std::map<string, DialogPtr>::const_iterator it =
		dialogs_.find(name);
	if (it == dialogs_.end())
		return;

	Dialog * const dialog = it->second.get();
	if (dialog->isVisible())
		dialog->update(data);
}


void Dialogs::hide(string const & name, Inset* inset)
{
	// Don't send the signal if we are quitting, because on MSVC it is
	// destructed before the cut stack in CutAndPaste.cpp, and this method
	// is called from some inset destructor if the cut stack is not empty
	// on exit.
	if (quitting)
		return;

	std::map<string, DialogPtr>::const_iterator it =
		dialogs_.find(name);
	if (it == dialogs_.end())
		return;

	if (inset && inset != getOpenInset(name))
		return;

	Dialog * const dialog = it->second.get();
	if (dialog->isVisible())
		dialog->hide();
	open_insets_[name] = 0;
}


void Dialogs::disconnect(string const & name)
{
	if (!isValidName(name))
		return;

	if (open_insets_.find(name) != open_insets_.end())
		open_insets_[name] = 0;
}


Inset * Dialogs::getOpenInset(string const & name) const
{
	if (!isValidName(name))
		return 0;

	std::map<string, Inset *>::const_iterator it =
		open_insets_.find(name);
	return it == open_insets_.end() ? 0 : it->second;
}


void Dialogs::hideAll() const
{
	std::map<string, DialogPtr>::const_iterator it  = dialogs_.begin();
	std::map<string, DialogPtr>::const_iterator end = dialogs_.end();

	for(; it != end; ++it) {
		it->second->hide();
	}
}


void Dialogs::hideBufferDependent() const
{
	std::map<string, DialogPtr>::const_iterator it  = dialogs_.begin();
	std::map<string, DialogPtr>::const_iterator end = dialogs_.end();

	for(; it != end; ++it) {
		Dialog * dialog =  it->second.get();
		if (dialog->controller().isBufferDependent())
			dialog->hide();
	}
}


void Dialogs::updateBufferDependent(bool switched) const
{
	std::map<string, DialogPtr>::const_iterator it  = dialogs_.begin();
	std::map<string, DialogPtr>::const_iterator end = dialogs_.end();

	for(; it != end; ++it) {
		Dialog * dialog =  it->second.get();
		if (switched && dialog->controller().isBufferDependent()) {
			if (dialog->isVisible() && dialog->controller().initialiseParams(""))
				dialog->view().update();
			else
				dialog->hide();
		} else {
			// A bit clunky, but the dialog will request
			// that the kernel provides it with the necessary
			// data.
			dialog->RestoreButton();
		}
	}
}


void Dialogs::redraw() const
{
	std::map<string, DialogPtr>::const_iterator it  = dialogs_.begin();
	std::map<string, DialogPtr>::const_iterator end = dialogs_.end();

	for(; it != end; ++it) {
		it->second->redraw();
	}
}


void Dialogs::checkStatus()
{
	std::map<string, DialogPtr>::const_iterator it  = dialogs_.begin();
	std::map<string, DialogPtr>::const_iterator end = dialogs_.end();

	for(; it != end; ++it) {
		Dialog * const dialog = it->second.get();
		if (dialog->isVisible())
			dialog->checkStatus();
	}
}


} // namespace lyx