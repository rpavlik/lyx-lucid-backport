/**
 * \file CommandInset.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "CommandInset.h"
#include "MathData.h"
#include "MathStream.h"
#include "DispatchResult.h"
#include "FuncRequest.h"

#include <sstream>


namespace lyx {

using std::auto_ptr;
using std::string;

CommandInset::CommandInset(docstring const & name)
	: InsetMathNest(2), name_(name), set_label_(false)
{
	lock_ = true;
}


auto_ptr<Inset> CommandInset::doClone() const
{
	return auto_ptr<Inset>(new CommandInset(*this));
}


bool CommandInset::metrics(MetricsInfo & mi, Dimension & dim) const
{
	if (!set_label_) {
		set_label_ = true;
		button_.update(screenLabel(), true);
	}
	button_.metrics(mi, dim);
	if (dim_ == dim)
		return false;
	dim_ = dim;
	return true;
}


Inset * CommandInset::editXY(Cursor & cur, int /*x*/, int /*y*/)
{
	edit(cur, true);
	return this;
}


void CommandInset::draw(PainterInfo & pi, int x, int y) const
{
	button_.draw(pi, x, y);
	setPosCache(pi, x, y);
}


void CommandInset::write(WriteStream & os) const
{
	os << '\\' << name_.c_str();
	if (cell(1).size())
		os << '[' << cell(1) << ']';
	os << '{' << cell(0) << '}';
}


docstring const CommandInset::screenLabel() const
{
	return name_;
}

} // namespace lyx
