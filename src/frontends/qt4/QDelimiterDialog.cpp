/**
 * \file QDelimiterDialog.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author John Levon
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "QDelimiterDialog.h"

#include "GuiApplication.h"
#include "GuiView.h"


#include "qt_helpers.h"
#include "controllers/ControlMath.h"

#include "gettext.h"

#include <QPixmap>
#include <QCheckBox>
#include <QListWidgetItem>

#include <sstream>

// Set to zero if unicode symbols are preferred.
#define USE_PIXMAP 1

using std::string;

namespace lyx {
namespace frontend {

namespace {

QString const bigleft[]  = {"", "bigl", "Bigl", "biggl", "Biggl"};


QString const bigright[] = {"", "bigr", "Bigr", "biggr", "Biggr"};


char const * const biggui[]   = {N_("big[[delimiter size]]"), N_("Big[[delimiter size]]"),
	N_("bigg[[delimiter size]]"), N_("Bigg[[delimiter size]]"), ""};


// FIXME: It might be better to fix the big delim LFUN to not require
// additional '\' prefix.
QString fix_name(QString const & str, bool big)
{
	if (str.isEmpty())
		return ".";
	if (!big || str == "(" || str == ")" || str == "[" || str == "]"
	    || str == "|" || str == "/")
		return str;

	return "\\" + str;
}

} // namespace anon


typedef QController<ControlMath, QView<QDelimiterDialog> > delimiter_base;

QMathDelimiter::QMathDelimiter(Dialog & parent)
	: delimiter_base(parent, _("Math Delimiter"))
{}


void QMathDelimiter::build_dialog()
{
	dialog_.reset(new QDelimiterDialog(this,
		static_cast<GuiView *>(controller().view())));
}


char_type QDelimiterDialog::doMatch(char_type const symbol) const
{
	string const & str = form_->controller().texName(symbol);
	string match;
	if (str == "(") match = ")";
	else if (str == ")") match = "(";
	else if (str == "[") match = "]";
	else if (str == "]") match = "[";
	else if (str == "{") match = "}";
	else if (str == "}") match = "{";
	else if (str == "l") match = "r";
	else if (str == "rceil") match = "lceil";
	else if (str == "lceil") match = "rceil";
	else if (str == "rfloor") match = "lfloor";
	else if (str == "lfloor") match = "rfloor";
	else if (str == "rangle") match = "langle";
	else if (str == "langle") match = "rangle";
	else if (str == "backslash") match = "/";
	else if (str == "/") match = "backslash";
	else return symbol;

	return form_->controller().mathSymbol(match).unicode;
}


QDelimiterDialog::QDelimiterDialog(QMathDelimiter * form, QWidget * parent)
	: QDialog(parent), form_(form)
{
	setupUi(this);

	connect(closePB, SIGNAL(clicked()), this, SLOT(accept()));

	setWindowTitle(qt_("LyX: Delimiters"));
	setFocusProxy(leftLW);

	leftLW->setViewMode(QListView::IconMode);
	rightLW->setViewMode(QListView::IconMode);

	typedef std::map<char_type, QListWidgetItem *> ListItems;
	ListItems list_items;
	// The last element is the empty one.
	int const end = nr_latex_delimiters - 1;
	for (int i = 0; i < end; ++i) {
		string const delim = latex_delimiters[i];
		MathSymbol const & ms =	form_->controller().mathSymbol(delim);
		QString symbol(ms.fontcode?
			QChar(ms.fontcode) : toqstr(docstring(1, ms.unicode)));
		QListWidgetItem * lwi = new QListWidgetItem(symbol);
		lwi->setToolTip(toqstr(delim));
		Font lyxfont;
		lyxfont.setFamily(ms.fontfamily);
		QFont const & symbol_font = guiApp->guiFontLoader().get(lyxfont);
		lwi->setFont(symbol_font);
		list_items[ms.unicode] = lwi;
		leftLW->addItem(lwi);
	}

	for (int i = 0; i != leftLW->count(); ++i) {
		MathSymbol const & ms =	form_->controller().mathSymbol(
			fromqstr(leftLW->item(i)->toolTip()));
		rightLW->addItem(list_items[doMatch(ms.unicode)]->clone());
	}

	// The last element is the empty one.
	leftLW->addItem(qt_("(None)"));
	rightLW->addItem(qt_("(None)"));

	sizeCO->addItem(qt_("Variable"));

	for (int i = 0; *biggui[i]; ++i)
		sizeCO->addItem(qt_(biggui[i]));

	on_leftLW_currentRowChanged(0);
}


void QDelimiterDialog::updateTeXCode(int size)
{
	bool const bigsize = size != 0;

	QString left_str = fix_name(leftLW->currentItem()->toolTip(), bigsize);
	QString right_str = fix_name(rightLW->currentItem()->toolTip(), bigsize);

	if (!bigsize)
		tex_code_ = left_str + ' ' + right_str;
	else {
		tex_code_ = bigleft[size] + ' '
			+ left_str + ' '
			+ bigright[size] + ' '
			+ right_str;
	}

	// Generate TeX-code for GUI display.
	// FIXME: Instead of reconstructing the TeX code it would be nice to
	// FIXME: retrieve the LateX code directly from mathed.
	// In all cases, we want the '\' prefix if needed, so we pass 'true'
	// to fix_name.
	left_str = fix_name(leftLW->currentItem()->toolTip(), true);
	right_str = fix_name(rightLW->currentItem()->toolTip(), true);
	QString code_str;
	if (!bigsize)
		code_str = "\\left" + left_str + " \\right" + right_str;
	else {
		// There should be nothing in the TeX-code when the delimiter is "None".
		if (left_str != ".")
			code_str = "\\" + bigleft[size] + left_str + ' ';
		if (right_str != ".")
			code_str += "\\" + bigright[size] + right_str;
	}

	texCodeL->setText(qt_("TeX Code: ") + code_str);
}


void QDelimiterDialog::on_insertPB_clicked()
{
	if (sizeCO->currentIndex() == 0)
		form_->controller().dispatchDelim(fromqstr(tex_code_));
	else {
		QString command = '"' + tex_code_ + '"';
		command.replace(' ', "\" \"");
		form_->controller().dispatchBigDelim(fromqstr(command));
	}
 }


void QDelimiterDialog::on_sizeCO_activated(int index)
{
	updateTeXCode(index);
}


void QDelimiterDialog::on_leftLW_itemActivated(QListWidgetItem *)
{
	on_insertPB_clicked();
	accept();
}


void QDelimiterDialog::on_rightLW_itemActivated(QListWidgetItem *)
{
	on_insertPB_clicked();
	accept();
}


void QDelimiterDialog::on_leftLW_currentRowChanged(int item)
{
	if (matchCB->isChecked())
		rightLW->setCurrentRow(item);

	updateTeXCode(sizeCO->currentIndex());
}


void QDelimiterDialog::on_rightLW_currentRowChanged(int item)
{
	if (matchCB->isChecked())
		leftLW->setCurrentRow(item);

	updateTeXCode(sizeCO->currentIndex());
}


void QDelimiterDialog::on_matchCB_stateChanged(int state)
{
	if (state == Qt::Checked)
		on_leftLW_currentRowChanged(leftLW->currentRow());

	updateTeXCode(sizeCO->currentIndex());
}


} // namespace frontend
} // namespace lyx

#include "QDelimiterDialog_moc.cpp"
