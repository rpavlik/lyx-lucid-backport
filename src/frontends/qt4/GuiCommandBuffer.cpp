/**
 * \file GuiCommandBuffer.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Lars
 * \author Asger and Jürgen
 * \author John Levon
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "GuiCommandBuffer.h"

#include "GuiApplication.h"
#include "GuiCommandEdit.h"
#include "GuiView.h"
#include "qt_helpers.h"

#include "BufferView.h"
#include "Cursor.h"
#include "LyXFunc.h"
#include "LyXAction.h"
#include "FuncRequest.h"
#include "Session.h"

#include "support/lyxalgo.h"
#include "support/lstrings.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QListWidget>
#include <QMouseEvent>
#include <QPixmap>
#include <QPushButton>
#include <QToolTip>
#include <QVBoxLayout>

using namespace std;
using namespace lyx::support;

namespace lyx {
namespace frontend {

namespace {

class QTempListBox : public QListWidget {
public:
	QTempListBox() {
		//setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		setWindowModality(Qt::WindowModal);
		setWindowFlags(Qt::Popup);
		setAttribute(Qt::WA_DeleteOnClose);
	}
protected:
	bool event(QEvent * ev) {
		if (ev->type() == QEvent::MouseButtonPress) {
			QMouseEvent * me = static_cast<QMouseEvent *>(ev);
			if (me->x() < 0 || me->y() < 0
			    || me->x() > width() || me->y() > height())
				hide();
			return true;
		}
		return QListWidget::event(ev);
	}

	void keyPressEvent(QKeyEvent * ev) {
		if (ev->key() == Qt::Key_Escape) {
			hide();
			return;
		} else if (ev->key() == Qt::Key_Return || ev->key() == Qt::Key_Space) {
			// emit signal
			itemClicked(currentItem());
		} else
			QListWidget::keyPressEvent(ev);
	}
};

} // end of anon


GuiCommandBuffer::GuiCommandBuffer(GuiView * view)
	: view_(view)
{
	transform(lyxaction.func_begin(), lyxaction.func_end(),
		back_inserter(commands_), firster());

	QPixmap qpup = getPixmap("images/", "up", "png");
	QPixmap qpdown = getPixmap("images/", "down", "png");

	QVBoxLayout * top = new QVBoxLayout(this);
	QHBoxLayout * layout = new QHBoxLayout(0);

	upPB = new QPushButton(qpup, "", this);
	upPB->setToolTip(qt_("List of previous commands"));
	upPB->setMaximumSize(24, 24);
	downPB = new QPushButton(qpdown, "", this);
	downPB->setToolTip(qt_("Next command"));
	downPB->setMaximumSize(24, 24);
	downPB->setEnabled(false);
	connect(downPB, SIGNAL(clicked()), this, SLOT(down()));
	connect(upPB, SIGNAL(pressed()), this, SLOT(listHistoryUp()));

	edit_ = new GuiCommandEdit(this);
	edit_->setMinimumSize(edit_->sizeHint());
	edit_->setFocusPolicy(Qt::ClickFocus);

	connect(edit_, SIGNAL(escapePressed()), this, SLOT(cancel()));
	connect(edit_, SIGNAL(returnPressed()), this, SLOT(dispatch()));
	connect(edit_, SIGNAL(tabPressed()), this, SLOT(complete()));
	connect(edit_, SIGNAL(upPressed()), this, SLOT(up()));
	connect(edit_, SIGNAL(downPressed()), this, SLOT(down()));
	connect(edit_, SIGNAL(hidePressed()), this, SLOT(hideParent()));

	layout->addWidget(upPB, 0);
	layout->addWidget(downPB, 0);
	layout->addWidget(edit_, 10);
	layout->setMargin(0);
	top->addLayout(layout);
	top->setMargin(0);
	setFocusProxy(edit_);

	LastCommandsSection::LastCommands last_commands 
		= theSession().lastCommands().getcommands();
	LastCommandsSection::LastCommands::const_iterator it 
		= last_commands.begin();
	LastCommandsSection::LastCommands::const_iterator end 
		= last_commands.end();

	upPB->setEnabled(it != end);

	for(; it != end; ++it)
		history_.push_back(*it);
	history_pos_ = history_.end();
}


void GuiCommandBuffer::cancel()
{
	view_->setFocus();
	edit_->setText(QString());
}


void GuiCommandBuffer::dispatch()
{
	QString cmd = edit_->text();
	view_->setFocus();
	edit_->setText(QString());
	edit_->clearFocus();
	std::string const cmd_ = fromqstr(cmd);
	theSession().lastCommands().add(cmd_);
	dispatch(cmd_);
}


void GuiCommandBuffer::listHistoryUp()
{
	if (history_.size()==1) {
		edit_->setText(toqstr(history_.back()));
		upPB->setEnabled(false);
		return;
	}
	QPoint const & pos = upPB->mapToGlobal(QPoint(0, 0));
	showList(history_, pos, true);
}


void GuiCommandBuffer::complete()
{
	string const input = fromqstr(edit_->text());
	string new_input;
	vector<string> comp = completions(input, new_input);

	if (comp.empty()) {
		if (new_input != input)
			edit_->setText(toqstr(new_input));
		return;
	}

	edit_->setText(toqstr(new_input));
	QPoint const & pos = edit_->mapToGlobal(QPoint(0, 0));
	showList(comp, pos);
}

void GuiCommandBuffer::showList(vector<string> const & list,
	QPoint const & pos, bool reversed) const
{
	QTempListBox * listBox = new QTempListBox;

	// For some reason the scrollview's contents are larger
	// than the number of actual items...
	vector<string>::const_iterator cit = list.begin();
	vector<string>::const_iterator end = list.end();
	if (!reversed) {
		for (; cit != end; ++cit)
			listBox ->addItem(toqstr(*cit));
	} else {
		for (--end; end != cit; --end)
			listBox ->addItem(toqstr(*end));
	}

	listBox->resize(listBox ->sizeHint());

	int const y = max(0, pos.y() - listBox->height());
	listBox->move(pos.x(), y);

	connect(listBox, SIGNAL(itemClicked(QListWidgetItem *)),
		this, SLOT(item_selected(QListWidgetItem *)));
	connect(listBox, SIGNAL(itemActivated(QListWidgetItem *)),
		this, SLOT(item_selected(QListWidgetItem *)));

	listBox->show();
	listBox->setFocus();
}


void GuiCommandBuffer::item_selected(QListWidgetItem * item)
{
	QWidget const * widget = static_cast<QWidget const *>(sender());
	const_cast<QWidget *>(widget)->hide();
	edit_->setText(item->text()+ ' ');
	edit_->activateWindow();
	edit_->setFocus();
}


void GuiCommandBuffer::up()
{
	string const input = fromqstr(edit_->text());
	string const h = historyUp();

	if (!h.empty())
		edit_->setText(toqstr(h));

	upPB->setEnabled(history_pos_ != history_.begin());
	downPB->setEnabled(history_pos_ != history_.end());
}


void GuiCommandBuffer::down()
{
	string const input = fromqstr(edit_->text());
	string const h = historyDown();

	if (!h.empty())
		edit_->setText(toqstr(h));

	downPB->setEnabled(history_pos_ != history_.end()-1);
	upPB->setEnabled(history_pos_ != history_.begin());
}
	

void GuiCommandBuffer::hideParent()
{
	view_->setFocus();
	edit_->setText(QString());
	edit_->clearFocus();
	hide();
}


namespace {

class prefix_p {
public:
	string p;
	prefix_p(string const & s) : p(s) {}
	bool operator()(string const & s) const { return prefixIs(s, p); }
};

} // end of anon namespace


string const GuiCommandBuffer::historyUp()
{
	if (history_pos_ == history_.begin())
		return string();

	return *(--history_pos_);
}


string const GuiCommandBuffer::historyDown()
{
	if (history_pos_ == history_.end())
		return string();
	if (history_pos_ + 1 == history_.end())
		return string();

	return *(++history_pos_);
}


docstring const GuiCommandBuffer::getCurrentState() const
{
	return view_->view()->cursor().currentState();
}


void GuiCommandBuffer::hide() const
{
	FuncRequest cmd(LFUN_COMMAND_EXECUTE, "off");
	theLyXFunc().setLyXView(view_);
	lyx::dispatch(cmd);
}


vector<string> const
GuiCommandBuffer::completions(string const & prefix, string & new_prefix)
{
	vector<string> comp;

	copy_if(commands_.begin(), commands_.end(),
		back_inserter(comp), prefix_p(prefix));

	if (comp.empty()) {
		new_prefix = prefix;
		return comp;
	}

	if (comp.size() == 1) {
		new_prefix = comp[0];
		return vector<string>();
	}

	// find maximal available prefix
	string const tmp = comp[0];
	string test = prefix;
	if (tmp.length() > test.length())
		test += tmp[test.length()];
	while (test.length() < tmp.length()) {
		vector<string> vtmp;
		copy_if(comp.begin(), comp.end(),
			back_inserter(vtmp), prefix_p(test));
		if (vtmp.size() != comp.size()) {
			test.erase(test.length() - 1);
			break;
		}
		test += tmp[test.length()];
	}

	new_prefix = test;
	return comp;
}


void GuiCommandBuffer::dispatch(string const & str)
{
	if (str.empty())
		return;

	history_.push_back(trim(str));
	history_pos_ = history_.end();
	upPB->setEnabled(history_pos_ != history_.begin());
	downPB->setEnabled(history_pos_ != history_.end());
	FuncRequest func = lyxaction.lookupFunc(str);
	func.origin = FuncRequest::COMMANDBUFFER;
	theLyXFunc().setLyXView(view_);
	lyx::dispatch(func);
}

} // namespace frontend
} // namespace lyx

#include "GuiCommandBuffer_moc.cpp"