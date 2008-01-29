/**
 * \file qtTimeout.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author John Levon
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "qtTimeout.h"

#include <QTimerEvent>

#include "debug.h"


namespace lyx {


Timeout::Timeout(unsigned int msec, Type t)
	: pimpl_(new qtTimeout(*this)), type(t), timeout_ms(msec)
{}


qtTimeout::qtTimeout(Timeout & owner)
	: Timeout::Impl(owner), timeout_id(-1)
{}


void qtTimeout::timerEvent(QTimerEvent *)
{
	emit();
}


void qtTimeout::reset()
{
	if (timeout_id != -1)
		killTimer(timeout_id);
	timeout_id = -1;
}


bool qtTimeout::running() const
{
	return timeout_id != -1;
}


void qtTimeout::start()
{
	if (running())
		lyxerr << "Timeout::start: already running!" << std::endl;
	timeout_id = startTimer(timeout_ms());
}


void qtTimeout::stop()
{
	if (running())
		reset();
}


} // namespace lyx