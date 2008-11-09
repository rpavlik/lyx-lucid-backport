// -*- C++ -*-
/**
 * \file FileDialog.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author unknown
 * \author John Levon
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef FILEDIALOG_H
#define FILEDIALOG_H

#include "FuncCode.h"

#include <QString>

#include <utility>

namespace lyx {

/**
 * \class FileDialog
 * \brief GUI-I definition of file dialog interface
 */
class FileDialog
{
public:
	/// label, directory path button
	typedef std::pair<QString, QString> Button;

	/// result type
	enum ResultType {
		Later, /**< modeless chooser, no result */
		Chosen /**< string contains filename */
	};

	/// result return
	typedef std::pair<FileDialog::ResultType, QString> Result;

	/**
	 * Constructs a file dialog with title \param title.
	 * If \param a is \const LFUN_SELECT_FILE_SYNC then a value
	 * will be returned immediately upon performing a open(),
	 * otherwise a callback Dispatch() will be invoked with the filename as
	 * argument, of action \param a.
	 *
	 * Up to two optional extra buttons are allowed for specifying
	 * additional directories in the navigation (an empty
	 * directory is interpreted as FileName::getcwd())
	 */
	FileDialog(QString const & title, FuncCode a = LFUN_SELECT_FILE_SYNC);

	~FileDialog();

	void setButton1(QString const & label, QString const & dir);
	void setButton2(QString const & label, QString const & dir);

	/// Choose a file for opening, starting in directory \c path.
	Result open(QString const & path, QStringList const & filters,
			  QString const & suggested = QString());

	/// Choose a directory, starting in directory \c path.
	Result opendir(QString const & path = QString(),
			     QString const & suggested = QString());

	/// Choose a file for saving, starting in directory \c  path.
	Result save(QString const & path, QStringList const & filters,
			  QString const & suggested = QString());

private:
	class Private;
	friend class Private;
	Private * private_;

	/// the dialog title
	QString title_;

	/// success action to perform if not synchronous
	FuncCode success_;
};

} // namespace lyx

#endif // FILEDIALOG_H
