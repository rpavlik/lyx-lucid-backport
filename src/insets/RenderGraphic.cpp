/**
 * \file RenderGraphic.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Angus Leeming
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "RenderGraphic.h"

#include "insets/Inset.h"

#include "gettext.h"
#include "Color.h"
#include "LyX.h"
#include "LyXRC.h"
#include "MetricsInfo.h"

#include "frontends/FontMetrics.h"
#include "frontends/Painter.h"

#include "graphics/GraphicsImage.h"

#include "support/filetools.h"

#include <boost/bind.hpp>


namespace lyx {

using support::onlyFilename;

using std::string;
using std::auto_ptr;


RenderGraphic::RenderGraphic(Inset const * inset)
{
	loader_.connect(boost::bind(&LyX::updateInset,
				    boost::cref(LyX::cref()), inset));
}


RenderGraphic::RenderGraphic(RenderGraphic const & other,
			     Inset const * inset)
	: RenderBase(other),
	  loader_(other.loader_),
	  params_(other.params_)
{
	loader_.connect(boost::bind(&LyX::updateInset,
				    boost::cref(LyX::cref()), inset));
}


auto_ptr<RenderBase> RenderGraphic::clone(Inset const * inset) const
{
	return auto_ptr<RenderBase>(new RenderGraphic(*this, inset));
}


void RenderGraphic::update(graphics::Params const & params)
{
	params_ = params;

	if (!params_.filename.empty()) {
		loader_.reset(params_.filename, params_);
	}
}


namespace {

bool displayGraphic(graphics::Params const & params)
{
	return params.display != graphics::NoDisplay &&
		lyxrc.display_graphics != graphics::NoDisplay;
}


docstring const statusMessage(graphics::Params const & params,
			   graphics::ImageStatus status)
{
	docstring ret;

	if (!displayGraphic(params))
		ret = _("Not shown.");
	else {
		switch (status) {
		case graphics::WaitingToLoad:
			ret = _("Not shown.");
			break;
		case graphics::Loading:
			ret = _("Loading...");
			break;
		case graphics::Converting:
			ret = _("Converting to loadable format...");
			break;
		case graphics::Loaded:
			ret = _("Loaded into memory. Generating pixmap...");
			break;
		case graphics::ScalingEtc:
			ret = _("Scaling etc...");
			break;
		case graphics::Ready:
			ret = _("Ready to display");
			break;
		case graphics::ErrorNoFile:
			ret = _("No file found!");
			break;
		case graphics::ErrorConverting:
			ret = _("Error converting to loadable format");
			break;
		case graphics::ErrorLoading:
			ret = _("Error loading file into memory");
			break;
		case graphics::ErrorGeneratingPixmap:
			ret = _("Error generating the pixmap");
			break;
		case graphics::ErrorUnknown:
			ret = _("No image");
			break;
		}
	}

	return ret;
}


bool readyToDisplay(graphics::Loader const & loader)
{
	if (!loader.image() || loader.status() != graphics::Ready)
		return false;
	return loader.image()->isDrawable();
}

} // namespace anon


bool RenderGraphic::metrics(MetricsInfo & mi, Dimension & dim) const
{
	bool image_ready = displayGraphic(params_) && readyToDisplay(loader_);

	dim.asc = image_ready ? loader_.image()->getHeight() : 50;
	dim.des = 0;

	if (image_ready) {
		dim.wid = loader_.image()->getWidth() +
			2 * Inset::TEXT_TO_INSET_OFFSET;
	} else {
		int font_width = 0;

		Font msgFont(mi.base.font);
		msgFont.setFamily(Font::SANS_FAMILY);

		// FIXME UNICODE
		docstring const justname =
			from_utf8(onlyFilename(params_.filename.absFilename()));
		if (!justname.empty()) {
			msgFont.setSize(Font::SIZE_FOOTNOTE);
			font_width = theFontMetrics(msgFont)
				.width(justname);
		}

		docstring const msg = statusMessage(params_, loader_.status());
		if (!msg.empty()) {
			msgFont.setSize(Font::SIZE_TINY);
			font_width = std::max(font_width,
				theFontMetrics(msgFont).width(msg));
		}

		dim.wid = std::max(50, font_width + 15);
	}

	bool const changed = dim_ != dim;
	dim_ = dim;
	return changed;
}


void RenderGraphic::draw(PainterInfo & pi, int x, int y) const
{
	if (displayGraphic(params_)) {
		if (loader_.status() == graphics::WaitingToLoad)
			loader_.startLoading();
		if (!loader_.monitoring())
			loader_.startMonitoring();
	}

	// This will draw the graphics. If the graphics has not been
	// loaded yet, we draw just a rectangle.

	if (displayGraphic(params_) && readyToDisplay(loader_)) {
		pi.pain.image(x + Inset::TEXT_TO_INSET_OFFSET,
			      y - dim_.asc,
			      dim_.wid - 2 * Inset::TEXT_TO_INSET_OFFSET,
			      dim_.asc + dim_.des,
			      *loader_.image());

	} else {
		pi.pain.rectangle(x + Inset::TEXT_TO_INSET_OFFSET,
				  y - dim_.asc,
				  dim_.wid - 2 * Inset::TEXT_TO_INSET_OFFSET,
				  dim_.asc + dim_.des,
				  Color::foreground);

		// Print the file name.
		Font msgFont = pi.base.font;
		msgFont.setFamily(Font::SANS_FAMILY);
		string const justname = onlyFilename(params_.filename.absFilename());

		if (!justname.empty()) {
			msgFont.setSize(Font::SIZE_FOOTNOTE);
			pi.pain.text(x + Inset::TEXT_TO_INSET_OFFSET + 6,
				   y - theFontMetrics(msgFont).maxAscent() - 4,
				   from_utf8(justname), msgFont);
		}

		// Print the message.
		docstring const msg = statusMessage(params_, loader_.status());
		if (!msg.empty()) {
			msgFont.setSize(Font::SIZE_TINY);
			pi.pain.text(x + Inset::TEXT_TO_INSET_OFFSET + 6,
				     y - 4, msg, msgFont);
		}
	}
}


} // namespace lyx