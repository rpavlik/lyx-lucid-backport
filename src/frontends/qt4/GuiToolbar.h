// -*- C++ -*-
/**
 * \file GuiToolbar.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Lars Gullik Bjønnes
 * \author John Levon
 * \author Jean-Marc Lasgouttes
 * \author Angus Leeming
 * \author Abdelrazak Younes
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef GUITOOLBAR_H
#define GUITOOLBAR_H

#include "Session.h"

#include <QList>
#include <QToolBar>
#include <QToolButton>

namespace lyx {

class DocumentClass;
class Inset;

namespace frontend {

class Action;
class GuiCommandBuffer;
class GuiLayoutFilterModel;
class GuiToolbar;
class GuiView;
class LayoutBox;
class ToolbarInfo;
class ToolbarItem;

class MenuButton : public QToolButton
{
	Q_OBJECT
public:
	///
	MenuButton(GuiToolbar * bar, ToolbarItem const & item,
		bool const sticky = false);
	///
	void mousePressEvent(QMouseEvent * e);

private:
	///
	GuiToolbar * bar_;
	///
	ToolbarItem const & tbitem_;
	///
	bool initialized_;

private Q_SLOTS:
	///
	void actionTriggered(QAction * action);
};



class GuiToolbar : public QToolBar
{
	Q_OBJECT
public:
	///
	GuiToolbar(ToolbarInfo const &, GuiView &);

	///
	void setVisibility(int visibility);

	/// Add a button to the bar.
	void add(ToolbarItem const & item);

	/// Session key.
	/**
	 * This key must be used for any session setting.
	 **/
	QString sessionKey() const;
	/// Save session settings.
	void saveSession() const;
	/// Restore session settings.
	void restoreSession();

	/// Refresh the contents of the bar.
	void update(bool in_math, bool in_table, bool review,
		bool in_mathmacrotemplate);

	///
	void toggle();

	///
	GuiCommandBuffer * commandBuffer() { return command_buffer_; }

	///
	Action * addItem(ToolbarItem const & item);

Q_SIGNALS:
	///
	void updated();

private:
	// load flags with saved values
	void initFlags();
	///
	void fill();
	///
	void showEvent(QShowEvent *);

	///
	QList<Action *> actions_;
	/// initial visibility flags
	int visibility_;
	///
	GuiView & owner_;
	///
	GuiCommandBuffer * command_buffer_;
	///
	ToolbarInfo const & tbinfo_;
	///
	bool filled_;
};

} // namespace frontend
} // namespace lyx

#endif // GUITOOLBAR_H
