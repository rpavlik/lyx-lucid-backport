/**
 * \file QLPopupMenu.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author John Levon
 * \author Abdelrazak Younes
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include <boost/current_function.hpp>

// Qt defines a macro 'signals' that clashes with a boost namespace.
// All is well if the namespace is visible first.
#include "GuiView.h"

#include "Action.h"
#include "QLPopupMenu.h"
#include "QLMenubar.h"
#include "qt_helpers.h"
#include "MenuBackend.h"

#include "support/lstrings.h"
#include "debug.h"


using std::make_pair;
using std::string;
using std::pair;
using std::endl;


namespace lyx {
namespace frontend {

QLPopupMenu::QLPopupMenu(QLMenubar * owner,
						 MenuItem const & mi, bool topLevelMenu)
	: owner_(owner)
{
	name_ = mi.submenuname();

	setTitle(toqstr(getLabel(mi)));

	if (topLevelMenu)
		connect(this, SIGNAL(aboutToShow()), this, SLOT(update()));
}


void QLPopupMenu::update()
{
	LYXERR(Debug::GUI) << BOOST_CURRENT_FUNCTION << endl;
	LYXERR(Debug::GUI) << "\tTriggered menu: " << to_utf8(name_) << endl;

	clear();

	if (name_.empty())
		return;

	// Here, We make sure that theLyXFunc points to the correct LyXView.
	theLyXFunc().setLyXView(owner_->view());

	Menu const & fromLyxMenu = owner_->backend().getMenu(name_);
	owner_->backend().expand(fromLyxMenu, topLevelMenu_, owner_->view()->buffer());

	if (!owner_->backend().hasMenu(topLevelMenu_.name())) {
		LYXERR(Debug::GUI) << "\tWARNING: menu seems empty"
			<< to_utf8(topLevelMenu_.name()) << endl;
	}
	populate(this, &topLevelMenu_);
}

void QLPopupMenu::populate(QMenu* qMenu, Menu * menu)
{
	LYXERR(Debug::GUI) << "populating menu " << to_utf8(menu->name()) ;
	if (menu->size() == 0) {
		LYXERR(Debug::GUI) << "\tERROR: empty menu "
			<< to_utf8(menu->name()) << endl;
		return;
	}
	else {
		LYXERR(Debug::GUI) << " *****  menu entries " << menu->size() << endl;
	}

	Menu::const_iterator m = menu->begin();
	Menu::const_iterator end = menu->end();

	for (; m != end; ++m) {

		if (m->kind() == MenuItem::Separator) {

			qMenu->addSeparator();
			LYXERR(Debug::GUI) << "adding Menubar Separator" << endl;

		} else if (m->kind() == MenuItem::Submenu) {

			LYXERR(Debug::GUI) << "** creating New Sub-Menu "
				<< to_utf8(getLabel(*m)) << endl;
			QMenu * subMenu = qMenu->addMenu(toqstr(getLabel(*m)));
			populate(subMenu, m->submenu());

		} else { // we have a MenuItem::Command

			LYXERR(Debug::GUI) << "creating Menu Item "
				<< to_utf8(m->label()) << endl;

			docstring label = getLabel(*m);
			addBinding(label, *m);

			Action * action = new Action(*(owner_->view()),
						     label, m->func());
			qMenu->addAction(action);
		}
	}
}


docstring const QLPopupMenu::getLabel(MenuItem const & mi)
{
	docstring const shortcut = mi.shortcut();
	docstring label = support::subst(mi.label(),
	from_ascii("&"), from_ascii("&&"));

	if (!shortcut.empty()) {
		docstring::size_type pos = label.find(shortcut);
		if (pos != docstring::npos)
			label.insert(pos, 1, char_type('&'));
	}

	return label;
}

void QLPopupMenu::addBinding(docstring & label, MenuItem const & mi)
{
#ifdef Q_WS_MACX
	docstring const binding(mi.binding(false));
#else
	docstring const binding(mi.binding(true));
#endif
	if (!binding.empty()) {
		label += '\t' + binding;
	}
}

} // namespace frontend
} // namespace lyx

#include "QLPopupMenu_moc.cpp"