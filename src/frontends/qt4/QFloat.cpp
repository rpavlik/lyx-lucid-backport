/**
 * \file QFloat.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Edwin Leuven
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "QFloat.h"
#include "QFloatDialog.h"
#include "Qt2BC.h"
#include "FloatPlacement.h"

#include "controllers/ControlFloat.h"

#include "insets/InsetFloat.h"

#include <QPushButton>

namespace lyx {
namespace frontend {

typedef QController<ControlFloat, QView<QFloatDialog> > float_base_class;


QFloat::QFloat(Dialog & parent)
	: float_base_class(parent, _("Float Settings"))
{
}


void QFloat::build_dialog()
{
	dialog_.reset(new QFloatDialog(this));

	bcview().setCancel(dialog_->closePB);
	bcview().setApply(dialog_->applyPB);
	bcview().setOK(dialog_->okPB);
	bcview().setRestore(dialog_->restorePB);

	bcview().addReadOnly(dialog_->floatFP);
}


void QFloat::update_contents()
{
	dialog_->floatFP->set(controller().params());
}


void QFloat::apply()
{
	InsetFloatParams & params = controller().params();

	params.placement = dialog_->floatFP->get(params.wide, params.sideways);
}

} // namespace frontend
} // namespace lyx