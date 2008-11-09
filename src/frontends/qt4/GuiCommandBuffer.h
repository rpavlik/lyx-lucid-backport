// -*- C++ -*-
/**
 * \file GuiCommandBuffer.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Lars
 * \author Asger and Jürgen
 * \author John Levon
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef GUICOMMANDBUFFER_H
#define GUICOMMANDBUFFER_H

#include "support/docstring.h"

#include <QWidget>

#include <vector>

class QListWidgetItem;

namespace lyx {
namespace frontend {

class GuiView;
class GuiCommandEdit;

class GuiCommandBuffer : public QWidget
{
	Q_OBJECT

public:
	GuiCommandBuffer(GuiView * view);

public Q_SLOTS:
	/// cancel command compose
	void cancel();
	/// dispatch a command
	void dispatch();
	/// tab-complete
	void complete();
	/// select-complete
	void complete_selected(QListWidgetItem *);
	/// up
	void up();
	/// down
	void down();
	/// leave and hide the command buffer
	void hideParent();
private:
	/// owning view
	GuiView * view_;
	/// command widget
	GuiCommandEdit * edit_;

	/// return the previous history entry if any
	std::string const historyUp();
	/// return the next history entry if any
	std::string const historyDown();

	/// return the font and depth in the active BufferView as a message.
	docstring const getCurrentState() const;

	/// hide the command buffer.
	void hide() const;

	/// return the possible completions
	std::vector<std::string> const completions(std::string const & prefix,
					      std::string & new_prefix);

	/// dispatch a command
	void dispatch(std::string const & str);

	/// available command names
	std::vector<std::string> commands_;

	/// command history
	std::vector<std::string> history_;

	/// current position in command history
	std::vector<std::string>::const_iterator history_pos_;
};

} // namespace frontend
} // namespace lyx

#endif // GUICOMMANDBUFFER_H
