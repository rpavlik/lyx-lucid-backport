/**
 * \file GuiIndices.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Edwin Leuven
 * \author Jürgen Spitzmüller
 * \author Abdelrazak Younes
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "GuiIndices.h"

#include "ColorCache.h"
#include "GuiApplication.h"
#include "Validator.h"
#include "qt_helpers.h"

#include "frontends/alert.h"

#include "BufferParams.h"
#include "LyXRC.h"

#include "support/gettext.h"
#include "support/lstrings.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QPixmap>
#include <QIcon>
#include <QColor>
#include <QColorDialog>


using namespace std;
using namespace lyx::support;


namespace lyx {
namespace frontend {


GuiIndices::GuiIndices(QWidget * parent)
	: QWidget(parent)
{
	setupUi(this);
	indicesTW->setColumnCount(2);
	indicesTW->headerItem()->setText(0, qt_("Name"));
	indicesTW->headerItem()->setText(1, qt_("Label Color"));
	indicesTW->setSortingEnabled(true);

	// NOTE: we do not provide "custom" here for security reasons!
	indexCO->clear();
	indexCO->addItem(qt_("Default"), QString("default"));
	for (set<string>::const_iterator it = lyxrc.index_alternatives.begin();
			     it != lyxrc.index_alternatives.end(); ++it) {
		QString const command = toqstr(*it).left(toqstr(*it).indexOf(" "));
		indexCO->addItem(command, command);
	}
}

void GuiIndices::update(BufferParams const & params)
{
	indiceslist_ = params.indiceslist();
	multipleIndicesCB->setChecked(params.use_indices);
	bool const state = params.use_indices;
	indicesTW->setEnabled(state);
	newIndexLE->setEnabled(state);
	newIndexLA->setEnabled(state);
	addIndexPB->setEnabled(state);
	availableLA->setEnabled(state);
	removePB->setEnabled(state);
	colorPB->setEnabled(state);

	string command;
	string options =
		split(params.index_command, command, ' ');

	int const pos = indexCO->findData(toqstr(command));
	if (pos != -1) {
		indexCO->setCurrentIndex(pos);
		indexOptionsED->setText(toqstr(options).trimmed());
	} else {
		// We reset to default if we do not know the specified compiler
		// This is for security reasons
		indexCO->setCurrentIndex(indexCO->findData(toqstr("default")));
		indexOptionsED->clear();
	}
	indexOptionsED->setEnabled(
		indexCO->currentIndex() != 0);

	updateView();
}


void GuiIndices::updateView()
{
	// store the selected index
	QTreeWidgetItem * item = indicesTW->currentItem();
	QString sel_index;
	if (item != 0)
		sel_index = item->text(0);

	indicesTW->clear();

	IndicesList::const_iterator it = indiceslist_.begin();
	IndicesList::const_iterator const end = indiceslist_.end();
	for (; it != end; ++it) {
		QTreeWidgetItem * newItem = new QTreeWidgetItem(indicesTW);

		QString const iname = toqstr(it->index());
		newItem->setText(0, iname);

		QColor const itemcolor = rgb2qcolor(it->color());
		if (itemcolor.isValid()) {
			QPixmap coloritem(30, 10);
			coloritem.fill(itemcolor);
			newItem->setIcon(1, QIcon(coloritem));
		}
		// restore selected index
		if (iname == sel_index) {
			indicesTW->setCurrentItem(newItem);
			indicesTW->setItemSelected(newItem, true);
		}
	}
	indicesTW->resizeColumnToContents(0);
	bool const have_sel =
		!indicesTW->selectedItems().isEmpty();
	removePB->setEnabled(have_sel);
	renamePB->setEnabled(have_sel);
	colorPB->setEnabled(have_sel);
	// emit signal
	changed();
}


void GuiIndices::apply(BufferParams & params) const
{
	params.use_indices = multipleIndicesCB->isChecked();
	params.indiceslist() = indiceslist_;

	string const index_command =
		fromqstr(indexCO->itemData(
			indexCO->currentIndex()).toString());
	string const index_options = fromqstr(indexOptionsED->text());
	if (index_command == "default" || index_options.empty())
		params.index_command = index_command;
	else
		params.index_command = index_command + " " + index_options;
}


void GuiIndices::on_indexCO_activated(int n)
{
	indexOptionsED->setEnabled(
		indexCO->itemData(n).toString() != "default");
	changed();
}


void GuiIndices::on_indexOptionsED_textChanged(QString)
{
	changed();
}


void GuiIndices::on_addIndexPB_pressed()
{
	QString const new_index = newIndexLE->text();
	if (!new_index.isEmpty()) {
		indiceslist_.add(qstring_to_ucs4(new_index));
		newIndexLE->clear();
		updateView();
	}
}


void GuiIndices::on_removePB_pressed()
{
	QTreeWidgetItem * selItem = indicesTW->currentItem();
	QString sel_index;
	if (selItem != 0)
		sel_index = selItem->text(0);
	if (!sel_index.isEmpty()) {
		if (indiceslist_.find(qstring_to_ucs4(sel_index)) == 
		    indiceslist_.findShortcut(from_ascii("idx"))) {
			Alert::error(_("Cannot remove standard index"), 
			      _("The default index cannot be removed."));
			      return;
		}
		indiceslist_.remove(qstring_to_ucs4(sel_index));
		newIndexLE->clear();
		updateView();
	}
}


void GuiIndices::on_renamePB_clicked()
{
	QTreeWidgetItem * selItem = indicesTW->currentItem();
	QString sel_index;
	if (selItem != 0)
		sel_index = selItem->text(0);
	if (!sel_index.isEmpty()) {
		docstring newname;
		docstring const oldname = qstring_to_ucs4(sel_index);
		bool success = false;
		if (Alert::askForText(newname, _("Enter new index name"), oldname)) {
			if (newname.empty() || oldname == newname)
				return;
			success = indiceslist_.rename(qstring_to_ucs4(sel_index), newname);
			newIndexLE->clear();
			updateView();
			if (!success)
				Alert::error(_("Renaming failed"), 
				      _("The index could not be renamed. "
					"Check if the new name already exists."));
		}
	}
}


void GuiIndices::on_indicesTW_itemDoubleClicked(QTreeWidgetItem * item, int /*col*/)
{
	toggleColor(item);
}


void GuiIndices::on_indicesTW_itemSelectionChanged()
{
	bool const have_sel =
		!indicesTW->selectedItems().isEmpty();
	removePB->setEnabled(have_sel);
	renamePB->setEnabled(have_sel);
	colorPB->setEnabled(have_sel);
}


void GuiIndices::on_colorPB_clicked()
{
	toggleColor(indicesTW->currentItem());
}


void GuiIndices::on_multipleIndicesCB_toggled(bool const state)
{
	bool const have_sel =
		!indicesTW->selectedItems().isEmpty();
	indicesTW->setEnabled(state);
	newIndexLE->setEnabled(state);
	newIndexLA->setEnabled(state);
	addIndexPB->setEnabled(state);
	availableLA->setEnabled(state);
	removePB->setEnabled(state && have_sel);
	colorPB->setEnabled(state && have_sel);
	renamePB->setEnabled(state && have_sel);
	// emit signal
	changed();
}


void GuiIndices::toggleColor(QTreeWidgetItem * item)
{
	if (item == 0)
		return;

	QString sel_index = item->text(0);
	if (sel_index.isEmpty())
		return;

	docstring current_index = qstring_to_ucs4(sel_index);
	Index * index = indiceslist_.find(current_index);
	if (!index)
		return;

	QColor const initial = rgb2qcolor(index->color());
	QColor ncol = QColorDialog::getColor(initial, qApp->focusWidget());
	if (!ncol.isValid())
		return;

	// add the color to the indiceslist
	index->setColor(fromqstr(ncol.name()));
	newIndexLE->clear();
	updateView();
}

} // namespace frontend
} // namespace lyx

#include "moc_GuiIndices.cpp"
