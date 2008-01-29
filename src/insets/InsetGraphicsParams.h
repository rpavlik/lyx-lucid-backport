// -*- C++ -*-
/**
 * \file InsetGraphicsParams.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Baruch Even
 * \author Herbert Vo�
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef INSETGRAPHICSPARAMS_H
#define INSETGRAPHICSPARAMS_H


#include "graphics/GraphicsTypes.h"
#include "Length.h"
#include "support/FileName.h"

namespace lyx {

namespace graphics { class Params; }

class Lexer;


/// This class holds all the parameters needed by insetGraphics.
class InsetGraphicsParams
{
public:
	/// Image filename.
	support::DocFileName filename;
	/// Scaling the Screen inside Lyx
	unsigned int lyxscale;
	/// How to display the image inside LyX
	graphics::DisplayType display;
	/// Scaling for output (LaTeX)
	std::string scale;
	/// sizes for output (LaTeX)
	Length width;
	///
	Length height;
	/// Keep the ratio between height and width when resizing.
	bool keepAspectRatio;
	/// draft mode
	bool draft;
	/// what to do with zipped files
	bool noUnzip;
	/// scale image before rotating
	bool scaleBeforeRotation;

	/// The bounding box with "xLB yLB yRT yRT ", divided by a space!
	std::string bb;
	/// clip image
	bool clip;

	/// Rotation angle.
	std::string rotateAngle;
	/// Origin point of rotation
	std::string rotateOrigin;
	/// Do we have a subcaption?
	bool subcaption;
	/// The text of the subcaption.
	std::string subcaptionText;
	/// any userdefined special command
	std::string special;

	///
	InsetGraphicsParams();
	///
	InsetGraphicsParams(InsetGraphicsParams const &);
	///
	InsetGraphicsParams & operator=(InsetGraphicsParams const &);
	/// Save the parameters in the LyX format stream.
	void Write(std::ostream & os, std::string const & bufpath) const;
	/// If the token belongs to our parameters, read it.
	bool Read(Lexer & lex, std::string const & token, std::string const & bufpath);
	/// convert
  // Only a subset of InsetGraphicsParams is needed for display purposes.
  // This function also interrogates lyxrc to ascertain whether
  // to display or not.
	graphics::Params as_grfxParams() const;

private:
	/// Initialize the object to a default status.
	void init();
	/// Copy the other objects content to us, used in copy c-tor and assignment
	void copy(InsetGraphicsParams const & params);
};

///
bool operator==(InsetGraphicsParams const &, InsetGraphicsParams const &);
///
bool operator!=(InsetGraphicsParams const &, InsetGraphicsParams const &);

} // namespace lyx

#endif