/**
 * \file Timeout.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Lars Gullik Bj�nnes
 * \author John Levon
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "Timeout.h"


namespace lyx {


Timeout::~Timeout()
{
	pimpl_->stop();
}


bool Timeout::running() const
{
	return pimpl_->running();
}


void Timeout::start()
{
	pimpl_->start();
}


void Timeout::stop()
{
	pimpl_->stop();
}


void Timeout::restart()
{
	pimpl_->stop();
	pimpl_->start();
}


void Timeout::emit()
{
	pimpl_->reset();
	timeout();
	if (type == CONTINUOUS)
		pimpl_->start();
}


Timeout & Timeout::setType(Type t)
{
	type = t;
	return * this;
}


Timeout & Timeout::setTimeout(unsigned int msec)
{
	// Can't have a timeout of zero!
	BOOST_ASSERT(msec);

	timeout_ms = msec;
	return * this;
}


} // namespace lyx
