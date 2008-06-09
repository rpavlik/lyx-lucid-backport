/**
 * \file FileMonitor.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Angus Leeming
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "support/FileMonitor.h"
#include "support/filetools.h"
#include "support/FileName.h"
#include "support/lyxlib.h"

// FIXME Interface violation
#include "frontends/Timeout.h"

#include <boost/bind.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/signals/trackable.hpp>


using std::string;

namespace fs = boost::filesystem;

namespace lyx {
namespace support {

class FileMonitor::Impl : public boost::signals::trackable {
public:

	///
	Impl(FileName const & file_with_path, int interval);

	///
	void monitorFile();

	///
	FileName filename_;

	///
	Timeout timer_;

	/// This signal is emitted if the file is modified (has a new checksum).
	FileMonitor::FileChangedSig fileChanged_;

	/** We use these to ascertain whether a file (once loaded successfully)
	 *  has changed.
	 */
	time_t timestamp_;
	///
	unsigned long checksum_;
};


FileMonitor::FileMonitor(FileName const & file_with_path, int interval)
	: pimpl_(new Impl(file_with_path, interval))
{}


FileMonitor::~FileMonitor()
{}


void FileMonitor::reset(FileName const & file_with_path) const
{
	if (pimpl_->filename_ == file_with_path)
		return;

	bool const monitor = pimpl_->timer_.running();
	if (monitor)
		stop();

	pimpl_->filename_ = file_with_path;

	if (monitor)
		start();
}


FileName const & FileMonitor::filename() const
{
	return pimpl_->filename_;
}


void FileMonitor::start() const
{
	if (monitoring())
		return;

	if (!doesFileExist(pimpl_->filename_))
		return;

	pimpl_->timestamp_ = fs::last_write_time(pimpl_->filename_.toFilesystemEncoding());
	pimpl_->checksum_ = sum(pimpl_->filename_);

	if (pimpl_->timestamp_ && pimpl_->checksum_) {
		pimpl_->timer_.start();
	} else {
		pimpl_->timestamp_ = 0;
		pimpl_->checksum_ = 0;
	}
}


void FileMonitor::stop() const
{
	pimpl_->timestamp_ = 0;
	pimpl_->checksum_ = 0;
	pimpl_->timer_.stop();
}


bool FileMonitor::monitoring() const
{
	return pimpl_->timer_.running();
}


unsigned long FileMonitor::checksum() const
{
	// If we aren't actively monitoring the file, then recompute the
	// checksum explicitly.
	if (!pimpl_->timer_.running() && !pimpl_->filename_.empty())
		return sum(pimpl_->filename_);

	return pimpl_->checksum_;
}


boost::signals::connection FileMonitor::connect(slot_type const & slot) const
{
	return pimpl_->fileChanged_.connect(slot);
}


//------------------------------
// Implementation details follow
//------------------------------


FileMonitor::Impl::Impl(FileName const & file_with_path, int interval)
	: filename_(file_with_path),
	  timer_(interval, Timeout::ONETIME),
	  timestamp_(0),
	  checksum_(0)
{
	timer_.timeout.connect(boost::bind(&Impl::monitorFile, this));
}


void FileMonitor::Impl::monitorFile()
{
	bool changed = false;

	if (!doesFileExist(filename_)) {
		changed = timestamp_ || checksum_;
		timestamp_ = 0;
		checksum_ = 0;

	} else {
		time_t const new_timestamp = fs::last_write_time(filename_.toFilesystemEncoding());

		if (new_timestamp != timestamp_) {
			timestamp_ = new_timestamp;

			unsigned long const new_checksum = sum(filename_);
			if (new_checksum != checksum_) {
				checksum_ = new_checksum;
				changed = true;
			}
		}
	}

	timer_.start();
	if (changed)
		fileChanged_();
}

} // namespace support
} // namespace lyx
