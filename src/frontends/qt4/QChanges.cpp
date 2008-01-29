/**
 * \file QChanges.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author John Levon
 * \author Michael Gerz
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "QChanges.h"
#include "Qt2BC.h"
#include "qt_helpers.h"

#include "support/lstrings.h"

#include "controllers/ControlChanges.h"

#include <QPushButton>
#include <QCloseEvent>
#include <QTextBrowser>

using lyx::support::bformat;


namespace lyx {
namespace frontend {

/////////////////////////////////////////////////////////////////////
//
// QChangesDialog
//
/////////////////////////////////////////////////////////////////////

QChangesDialog::QChangesDialog(QChanges * form)
	: form_(form)
{
	setupUi(this);
	connect(closePB, SIGNAL(clicked()), form, SLOT(slotClose()));
	connect(nextPB, SIGNAL(clicked()), this, SLOT(nextPressed()));
	connect(rejectPB, SIGNAL(clicked()), this, SLOT(rejectPressed()));
	connect(acceptPB, SIGNAL(clicked()), this, SLOT(acceptPressed()));
}


void QChangesDialog::nextPressed()
{
	form_->next();
}


void QChangesDialog::acceptPressed()
{
	form_->accept();
}


void QChangesDialog::rejectPressed()
{
	form_->reject();
}


void QChangesDialog::closeEvent(QCloseEvent *e)
{
	form_->slotWMHide();
	e->accept();
}



/////////////////////////////////////////////////////////////////////
//
// QChanges
//
/////////////////////////////////////////////////////////////////////

typedef QController<ControlChanges, QView<QChangesDialog> > ChangesBase;


QChanges::QChanges(Dialog & parent)
	: ChangesBase(parent, _("Merge Changes"))
{
}


void QChanges::build_dialog()
{
	dialog_.reset(new QChangesDialog(this));

	bcview().setCancel(dialog_->closePB);
	bcview().addReadOnly(dialog_->acceptPB);
	bcview().addReadOnly(dialog_->rejectPB);
}


void QChanges::update_contents()
{
	docstring text;
	docstring author = controller().getChangeAuthor();
	docstring date = controller().getChangeDate();

	if (!author.empty())
		text += bformat(_("Change by %1$s\n\n"), author);
	if (!date.empty())
		text += bformat(_("Change made at %1$s\n"), date);

	dialog_->changeTB->setPlainText(toqstr(text));
}


void QChanges::next()
{
	controller().next();
}


void QChanges::accept()
{
	controller().accept();
}


void QChanges::reject()
{
	controller().reject();
}

} // namespace frontend
} // namespace lyx

#include "QChanges_moc.cpp"