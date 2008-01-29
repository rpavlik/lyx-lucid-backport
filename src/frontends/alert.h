// -*- C++ -*-
/**
 * \file alert.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author John Levon
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef LYX_ALERT_H
#define LYX_ALERT_H

#include "support/lstrings.h"


namespace lyx {
namespace frontend {
namespace Alert {

/**
 * Prompt for a question. Returns 0-2 for the chosen button.
 * Set default_button and cancel_button to reasonable values. b1-b3
 * should have accelerators marked with an '&'. title should be
 * a short summary. Strings should be gettextised.
 * Please think about the poor user.
 *
 * Remember to use boost::format. If you make any of these buttons
 * "Yes" or "No", I will personally come around to your house and
 * slap you with fish, and not in an enjoyable way either.
 */
int prompt(docstring const & title, docstring const & question,
	   int default_button, int cancel_button,
	   docstring const & b1, docstring const & b2, docstring const & b3 = docstring());

/**
 * Display a warning to the user. Title should be a short (general) summary.
 * Only use this if the user cannot perform some remedial action.
 */
void warning(docstring const & title, docstring const & message);

/**
 * Display a warning to the user. Title should be a short (general) summary.
 * Only use this if the user cannot perform some remedial action.
 */
void error(docstring const & title, docstring const & message);

/**
 * Informational message. Use very very sparingly. That is, you must
 * apply to me, in triplicate, under the sea, breathing in petrol
 * and reciting the Nicene Creed, whilst running uphill and also
 * eating.
 */
void information(docstring const & title, docstring const & message);

/// Asks for a text. DO NOT USE !!
std::pair<bool, docstring> const
askForText(docstring const & msg, docstring const & dflt = docstring());

} // namespace Alert
} // namespace frontend
} // namespace lyx

#endif // LYX_ALERT_H