/**
 * \file QErrorList.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Alfredo Braunstein
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "QErrorList.h"
#include "Qt2BC.h"
#include "qt_helpers.h"

#include "controllers/ControlErrorList.h"

#include <QListWidget>
#include <QTextBrowser>
#include <QPushButton>
#include <QCloseEvent>

namespace lyx {
namespace frontend {

/////////////////////////////////////////////////////////////////////
//
// QErrorListDialog
//
/////////////////////////////////////////////////////////////////////

QErrorListDialog::QErrorListDialog(QErrorList * form)
	: form_(form)
{
	setupUi(this);
	connect(closePB, SIGNAL(clicked()),
		form, SLOT(slotClose()));
	connect( errorsLW, SIGNAL( itemClicked(QListWidgetItem *)),
		this, SLOT(select_adaptor(QListWidgetItem *)));
}


void QErrorListDialog::select_adaptor(QListWidgetItem * item)
{
	form_->select(item);
}


void QErrorListDialog::closeEvent(QCloseEvent * e)
{
	form_->slotWMHide();
	e->accept();
}


void QErrorListDialog::showEvent(QShowEvent *e)
{
	errorsLW->setCurrentRow(0);
	form_->select(errorsLW->item(0));
	e->accept();
}


/////////////////////////////////////////////////////////////////////
//
// QErrorList
//
/////////////////////////////////////////////////////////////////////


typedef QController<ControlErrorList, QView<QErrorListDialog> >
	ErrorListBase;

QErrorList::QErrorList(Dialog & parent)
	: ErrorListBase(parent, docstring())
{}


void QErrorList::build_dialog()
{
	dialog_.reset(new QErrorListDialog(this));
	bcview().setCancel(dialog_->closePB);
}


void QErrorList::select(QListWidgetItem * wi)
{
	int const item = dialog_->errorsLW->row(wi);
	controller().goTo(item);
	dialog_->descriptionTB->setPlainText(toqstr(controller().errorList()[item].description));
}


void QErrorList::update_contents()
{
	setTitle(from_utf8(controller().name()));
	dialog_->errorsLW->clear();
	dialog_->descriptionTB->setPlainText(QString());

	ErrorList::const_iterator it = controller().errorList().begin();
	ErrorList::const_iterator end = controller().errorList().end();
	for(; it != end; ++it) {
		dialog_->errorsLW->addItem(toqstr(it->error));
	}
}

} // namespace frontend
} // namespace lyx


#include "QErrorList_moc.cpp"
