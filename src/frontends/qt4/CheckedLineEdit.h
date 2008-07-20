// -*- C++ -*-
/**
 * \file qt4/CheckedLineEdit.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Angus Leeming
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef CHECKEDLINEEDIT_H
#define CHECKEDLINEEDIT_H

#include "BCView.h"

class QWidget;
class QLineEdit;

namespace lyx {
namespace frontend {

void addCheckedLineEdit(BCView & bcview,
	QLineEdit * input, QWidget * label = 0);

class CheckedLineEdit : public CheckedWidget {
public:
	CheckedLineEdit(QLineEdit * input, QWidget * label = 0);

private:
	///
	virtual bool check() const;

	///
	QLineEdit * input_;
	QWidget * label_;
};

} // namespace frontend
} // namespace lyx

#endif // CHECKEDLINEEDIT_H
