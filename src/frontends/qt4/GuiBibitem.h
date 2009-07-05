// -*- C++ -*-
/**
 * \file GuiBibitem.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Angus Leeming
 * \author John Levon
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef GUIBIBITEM_H
#define GUIBIBITEM_H

#include "GuiDialog.h"
#include "ui_BibitemUi.h"

#include "insets/InsetCommandParams.h"

namespace lyx {
namespace frontend {

class GuiBibitem : public GuiDialog, public Ui::BibitemUi
{
	Q_OBJECT

public:
	/// Constructor
	GuiBibitem(GuiView & lv);

private Q_SLOTS:
	void change_adaptor();

private:
	///
	bool isValid();
	/// Apply changes
	void applyView();
	/// update
	void updateContents();
	///
	bool initialiseParams(std::string const & data);
	/// clean-up on hide.
	void clearParams() { params_.clear(); }
	/// clean-up on hide.
	void dispatchParams();
	///
	bool isBufferDependent() const { return true; }

private:
	///
	InsetCommandParams params_;
};

} // namespace frontend
} // namespace lyx

#endif // GUIBIBITEM_H